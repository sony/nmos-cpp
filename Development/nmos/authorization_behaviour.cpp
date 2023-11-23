#include "nmos/authorization_behaviour.h"

#include "cpprest/response_type.h"
#include "mdns/service_discovery.h"
#include "nmos/api_utils.h"
#include "nmos/authorization.h"
#include "nmos/authorization_operation.h"
#include "nmos/authorization_scopes.h"
#include "nmos/authorization_state.h"
#include "nmos/authorization_utils.h"
#include "nmos/is10_versions.h"
#include "nmos/model.h"
#include "nmos/random.h"
#include "nmos/slog.h"

namespace nmos
{
    namespace experimental
    {
        namespace fields
        {
            const web::json::field_as_string_or ver{ U("ver"),{} };
            //const web::json::field_as_integer_or pri{ U("pri"), nmos::service_priorities::no_priority }; already defined in settings.h
            const web::json::field_as_string_or uri{ U("uri"),{} };
        }

        namespace details
        {
            // thread to fetch token and public keys from service
            void authorization_behaviour_thread(nmos::base_model& model, nmos::experimental::authorization_state& authorization_state, nmos::load_ca_certificates_handler load_ca_certificates, nmos::load_rsa_private_keys_handler load_rsa_private_keys, load_authorization_clients_handler load_authorization_clients, save_authorization_client_handler save_authorization_client, request_authorization_code_handler request_authorization_code, mdns::service_discovery& discovery, slog::base_gate& gate);
            // thread to fetch public keys from token issuer
            void authorization_token_issuer_thread(nmos::base_model& model, nmos::experimental::authorization_state& authorization_state, nmos::load_ca_certificates_handler load_ca_certificates, slog::base_gate& gate);

            // background service discovery
            void authorization_services_background_discovery(nmos::base_model& model, mdns::service_discovery& discovery, slog::base_gate& gate);

            // service discovery
            bool discover_authorization_services(nmos::base_model& model, mdns::service_discovery& discovery, slog::base_gate& gate, const pplx::cancellation_token& token = pplx::cancellation_token::none());
            bool has_discovered_authorization_services(const nmos::base_model& model);
        }

        // uses the default DNS-SD implementation
        // callbacks from this function are called with the model locked, and may read or write directly to the model
        void authorization_behaviour_thread(nmos::base_model& model, authorization_state& authorization_state, nmos::load_ca_certificates_handler load_ca_certificates, nmos::load_rsa_private_keys_handler load_rsa_private_keys, load_authorization_clients_handler load_authorization_clients, save_authorization_client_handler save_authorization_client, request_authorization_code_handler request_authorization_code, slog::base_gate& gate_)
        {
            nmos::details::omanip_gate gate(gate_, nmos::stash_category(nmos::categories::authorization_behaviour));

            mdns::service_discovery discovery(gate);

            details::authorization_behaviour_thread(model, authorization_state, std::move(load_ca_certificates), std::move(load_rsa_private_keys), std::move(load_authorization_clients), std::move(save_authorization_client), std::move(request_authorization_code), discovery, gate);
        }

        // uses the specified DNS-SD implementation
        // callbacks from this function are called with the model locked, and may read or write directly to the model
        void authorization_behaviour_thread(nmos::base_model& model, authorization_state& authorization_state, nmos::load_ca_certificates_handler load_ca_certificates, nmos::load_rsa_private_keys_handler load_rsa_private_keys, load_authorization_clients_handler load_authorization_clients, save_authorization_client_handler save_authorization_client, request_authorization_code_handler request_authorization_code, mdns::service_discovery& discovery, slog::base_gate& gate_)
        {
            nmos::details::omanip_gate gate(gate_, nmos::stash_category(nmos::categories::authorization_behaviour));

            details::authorization_behaviour_thread(model, authorization_state, std::move(load_ca_certificates), std::move(load_rsa_private_keys), std::move(load_authorization_clients), std::move(save_authorization_client), std::move(request_authorization_code), discovery, gate);
        }

        void details::authorization_behaviour_thread(nmos::base_model& model, nmos::experimental::authorization_state& authorization_state, nmos::load_ca_certificates_handler load_ca_certificates, nmos::load_rsa_private_keys_handler load_rsa_private_keys, load_authorization_clients_handler load_authorization_clients, save_authorization_client_handler save_authorization_client, request_authorization_code_handler request_authorization_code, mdns::service_discovery& discovery, slog::base_gate& gate)
        {
            enum
            {
                initial_discovery,
                request_authorization_server_metadata,
                client_registration,
                authorization_code_flow,
                authorization_operation,
                authorization_operation_with_immediate_token_fetch,
                rediscovery,
                background_discovery
            } mode = initial_discovery;

            // If the chosen Authorization API does not respond correctly at any time, another Authorization API should be selected from the discovered list.
            with_write_lock(model.mutex, [&model] { model.settings[nmos::experimental::fields::authorization_services] = web::json::value::array(); });

            nmos::details::seed_generator discovery_backoff_seeder;
            std::default_random_engine discovery_backoff_engine(discovery_backoff_seeder);
            double discovery_backoff = 0;

            // load authorization clients metadata to cache
            if (load_authorization_clients)
            {
                const auto auth_clients = load_authorization_clients();

                if (!auth_clients.is_null() && auth_clients.is_array())
                {
                    slog::log<slog::severities::too_much_info>(gate, SLOG_FLF) << "Retrieved authorization clients: " << utility::us2s(auth_clients.serialize()) << " from non-volatile memory";

                    for (const auto& auth_client : auth_clients.as_array())
                    {
                        nmos::experimental::update_client_metadata(authorization_state, auth_client.at(nmos::experimental::fields::authorization_server_uri).as_string(), nmos::experimental::fields::client_metadata(auth_client));
                    }
                }
            }

            bool authorization_service_error{ false };

            // continue until the server is being shut down
            for (;;)
            {
                if (with_read_lock(model.mutex, [&] { return model.shutdown; })) break;

                switch (mode)
                {
                case initial_discovery:
                case rediscovery:
                    if (0 != discovery_backoff)
                    {
                        auto lock = model.read_lock();
                        const auto random_backoff = std::uniform_real_distribution<>(0, discovery_backoff)(discovery_backoff_engine);
                        slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "Waiting to retry Authorization API discovery for about " << std::fixed << std::setprecision(3) << random_backoff << " seconds (current backoff limit: " << discovery_backoff << " seconds)";
                        model.wait_for(lock, std::chrono::milliseconds(std::chrono::milliseconds::rep(1000 * random_backoff)), [&] { return model.shutdown; });
                        if (model.shutdown) break;
                    }

                    // The Node performs a DNS-SD browse for services of type '_nmos-auth._tcp' as specified.
                    if (details::discover_authorization_services(model, discovery, gate))
                    {
                        mode = request_authorization_server_metadata;

                        // If unable to contact the Authorization server, we MUST implement a
                        // random back-off mechanism to avoid overloading the Authorization server in the event of a system restart.
                        auto lock = model.read_lock();
                        discovery_backoff = (std::min)((std::max)((double)nmos::fields::discovery_backoff_min(model.settings), discovery_backoff * nmos::fields::discovery_backoff_factor(model.settings)), (double)nmos::fields::discovery_backoff_max(model.settings));
                    }
                    else
                    {
                        mode = background_discovery;
                    }
                    break;

                case request_authorization_server_metadata:
                    if (details::request_authorization_server_metadata(model, authorization_state, authorization_service_error, load_ca_certificates, gate))
                    {
                        // reterive client metadata from cache
                        const auto client_metadata = nmos::experimental::get_client_metadata(authorization_state);

                        // is it not a scopeless client (where scopeless client doesn't access any protected APIs, i.e. doesn't require to register to Authorization server)
                        if (with_read_lock(model.mutex, [&] { return details::scopes(client_metadata, nmos::experimental::authorization_scopes::from_settings(model.settings)).size(); }))
                        {
                            // is the client already registered to Authorization server, i.e. found it in cache
                            if (!client_metadata.is_null())
                            {
                                // no token or token expired
                                auto is_access_token_bad = [&]
                                {
                                    auto lock = authorization_state.read_lock();

                                    const auto& bearer_token = authorization_state.bearer_token;
                                    return (!bearer_token.is_valid_access_token() || is_access_token_expired(bearer_token.access_token(), authorization_state.issuers, authorization_state.authorization_server_uri, gate));
                                };

                                auto is_client_expired = [&]
                                {
                                    // Time at which the client_secret will expire. If time is 0, it will never expire
                                    // The time is represented as the number of seconds from 1970-01-01T0:0:0Z as measured in UTC
                                    const auto expires_at = nmos::experimental::fields::client_secret_expires_at(client_metadata);
                                    if (expires_at == 0)
                                    {
                                        return false;
                                    }
                                    auto now = std::chrono::system_clock::now();
                                    auto exp = std::chrono::system_clock::from_time_t(expires_at);
                                    return (now > exp);
                                };

                                utility::string_t authorization_flow;
                                auto validate_openid_client = false;
                                with_read_lock(model.mutex, [&]
                                {
                                    authorization_flow = nmos::experimental::fields::authorization_flow(model.settings);
                                    validate_openid_client = nmos::experimental::fields::validate_openid_client(model.settings);
                                });

                                // if using OpenID Connect Authorization server, update the cache client metadata, in case it has been changed (e.g. changed by the system admin)
                                if (validate_openid_client)
                                {
                                    // if OpenID Connect Authorization server is used, client status can be obtained via the Client Configuration Endpoint
                                    // "The Client Configuration Endpoint is an OAuth 2.0 Protected Resource that MAY be provisioned by the server for a
                                    //  specific Client to be able to view and update its registered information."
                                    // see 3.2 of https://openid.net/specs/openid-connect-registration-1_0.html#ClientConfigurationEndpoint
                                    // registration_access_token
                                    //     OPTIONAL. Registration Access Token that can be used at the Client Configuration Endpoint to perform subsequent operations upon the
                                    //               Client registration.
                                    // registration_client_uri
                                    //     OPTIONAL. Location of the Client Configuration Endpoint where the Registration Access Token can be used to perform subsequent operations
                                    //               upon the resulting Client registration.
                                    //               Implementations MUST either return both a Client Configuration Endpoint and a Registration Access Token or neither of them.
                                    if (client_metadata.has_string_field(nmos::experimental::fields::registration_access_token) && client_metadata.has_string_field(nmos::experimental::fields::registration_client_uri))
                                    {
                                        // fetch client metadata from Authorization server in case it has been changed (e.g. changed by the system admin)
                                        if (details::request_client_metadata_from_openid_connect(model, authorization_state, load_ca_certificates, save_authorization_client, gate))
                                        {
                                            mode = (web::http::oauth2::experimental::grant_types::client_credentials.name == authorization_flow) ? authorization_operation // client credentials flow
                                                : (is_access_token_bad() ? authorization_code_flow : authorization_operation_with_immediate_token_fetch); // bad access token must start from authorization code flow, otherise do token refresh
                                        }
                                        else
                                        {
                                            // remove client metadata from cache
                                            nmos::experimental::erase_client_metadata(authorization_state);

                                            // client not known by the Authorization server, trigger client registration process
                                            mode = client_registration;
                                        }
                                    }
                                    else
                                    {
                                        // no registration_access_token and registration_client_uri found, treat it has connected with a non-OpenID Connect server
                                        // start grant flow based on what been defined in the settings
                                        // hmm, maybe use of the OpenID API to extend the client lifespan instead of re-registration
                                        mode = is_client_expired() ? client_registration // client registration
                                            : ((web::http::oauth2::experimental::grant_types::client_credentials.name == authorization_flow) ? authorization_operation // client credentials flow
                                                : (is_access_token_bad() ? authorization_code_flow : authorization_operation_with_immediate_token_fetch)); // bad access token must start from authorization code flow, otherise do token refresh
                                    }
                                }
                                else
                                {
                                    // start grant flow based on what been defined in the settings
                                    // hmm, maybe use of the OpenID API to extend the client lifespan instead of re-registration
                                    mode = is_client_expired() ? client_registration // client registration
                                        : ((web::http::oauth2::experimental::grant_types::client_credentials.name == authorization_flow) ? authorization_operation // client credentials flow
                                            : (is_access_token_bad() ? authorization_code_flow : authorization_operation_with_immediate_token_fetch));  // bad access token must start from authorization code flow, otherise do token refresh
                                }
                            }
                            else
                            {
                                // client has not been registered to the Authorization server yet
                                mode = client_registration;
                            }
                        }
                        else
                        {
                            // scope-less client, not require to obtain access token
                            mode = authorization_operation;
                        }
                    }
                    else
                    {
                        // Should no further Authorization APIs be available or TTLs on advertised services expired, a re-query may be performed.
                        mode = rediscovery;
                    }
                    break;

                case client_registration:
                    // register to the Authorization server to obtain client_id and client_secret (they can be found inside the client metadata)
                    if (details::client_registration(model, authorization_state, load_ca_certificates, save_authorization_client, gate))
                    {
                        // client registered
                        mode = with_read_lock(model.mutex, [&]
                        {
                            const auto& authorization_flow = nmos::experimental::fields::authorization_flow(model.settings);
                            return (web::http::oauth2::experimental::grant_types::client_credentials.name == authorization_flow) ? authorization_operation : authorization_code_flow;
                        });
                    }
                    else
                    {
                        // client registration failure, start authorization sequence again on next available Authorization server
                        authorization_service_error = true;
                        mode = request_authorization_server_metadata;
                    }
                    break;

                case authorization_code_flow:
                    if (details::authorization_code_flow(model, authorization_state, request_authorization_code, gate))
                    {
                        mode = authorization_operation;
                    }
                    else
                    {
                        // authorization code flow failure, start authorization sequence again on next available Authorization server
                        authorization_service_error = true;
                        mode = request_authorization_server_metadata;
                    }
                    break;

                case authorization_operation:
                    // fetch public keys
                    // fetch access token in 1/2 token life time interval
                    details::authorization_operation(model, authorization_state, load_ca_certificates, load_rsa_private_keys, false, gate);

                    // reaching here, there must be failure within the authorization operation,
                    // start the authorization sequence again on next available Authorization server
                    authorization_service_error = true;
                    mode = request_authorization_server_metadata;
                    break;

                case authorization_operation_with_immediate_token_fetch:
                    // fetch public keys
                    // immediately fetch access token
                    details::authorization_operation(model, authorization_state, load_ca_certificates, load_rsa_private_keys, true, gate);

                    // reaching here, there must be failure within the authorization operation,
                    // start the authorization sequence again on next available Authorization server
                    authorization_service_error = true;
                    mode = request_authorization_server_metadata;
                    break;

                case background_discovery:
                    details::authorization_services_background_discovery(model, discovery, gate);

                    if (details::has_discovered_authorization_services(model))
                    {
                        mode = request_authorization_server_metadata;
                    }
                }
            }
        }

        void authorization_token_issuer_thread(nmos::base_model& model, authorization_state& authorization_state, nmos::load_ca_certificates_handler load_ca_certificates, slog::base_gate& gate_)
        {
            nmos::details::omanip_gate gate(gate_, nmos::stash_category(nmos::categories::authorization_behaviour));

            details::authorization_token_issuer_thread(model, authorization_state, load_ca_certificates, gate);
        }

        void details::authorization_token_issuer_thread(nmos::base_model& model, nmos::experimental::authorization_state& authorization_state, nmos::load_ca_certificates_handler load_ca_certificates, slog::base_gate& gate)
        {
            enum
            {
                fetch_issuer_metadata,
                fetch_issuer_pubkeys,
            } mode = fetch_issuer_metadata;

            // continue until the server is being shut down
            for (;;)
            {
                if (with_read_lock(model.mutex, [&] { return model.shutdown; })) break;

                switch (mode)
                {
                case fetch_issuer_metadata:
                    // fetch token issuer metadata
                    if (details::request_token_issuer_metadata(model, authorization_state, load_ca_certificates, gate))
                    {
                        mode = fetch_issuer_pubkeys;
                    }
                    break;

                case fetch_issuer_pubkeys:
                    // fetch token issuer public keys
                    details::request_token_issuer_public_keys(model, authorization_state, load_ca_certificates, gate);
                    mode = fetch_issuer_metadata;
                    break;
                }
            }
        }

        // service discovery
        namespace details
        {
            static web::json::value make_service(const resolved_service& service)
            {
                using  web::json::value;

                return web::json::value_of({
                    { nmos::experimental::fields::ver, value::string(make_api_version(service.first.first)) },
                    { nmos::fields::pri, service.first.second },
                    { nmos::experimental::fields::uri, value::string(service.second.to_string()) }
                });
            }

            static resolved_service parse_service(const web::json::value& data)
            {

                return {
                    {parse_api_version(nmos::experimental::fields::ver(data)), nmos::fields::pri(data)},
                    web::uri(nmos::experimental::fields::uri(data))
                };
            }

            // get the fallback authorization service from settings (if present)
            resolved_service get_authorization_service(const nmos::settings& settings)
            {
                if (settings.has_field(nmos::experimental::fields::authorization_address))
                {
                    const auto api_selector = nmos::experimental::fields::authorization_selector(settings);

                    return { { parse_api_version(nmos::experimental::fields::authorization_version(settings)), 0 },
                        web::uri_builder()
                        .set_scheme(nmos::http_scheme(settings))
                        .set_host(nmos::experimental::fields::authorization_address(settings))
                        .set_port(nmos::experimental::fields::authorization_port(settings))
                        .set_path(U("/.well-known/oauth-authorization-server")).append_path(!api_selector.empty() ? U("/") + api_selector : U(""))
                        .to_uri() };
                }
                return {};
            }

            // query DNS Service Discovery for any Authorization API based on settings
            bool discover_authorization_services(nmos::base_model& model, mdns::service_discovery& discovery, slog::base_gate& gate, const pplx::cancellation_token& token)
            {
                slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "Trying Authorization API discovery";

                // lock to read settings, then unlock to wait for the discovery task to complete
                auto authorization_services = with_read_lock(model.mutex, [&]
                {
                    auto& settings = model.settings;

                    if (nmos::service_priorities::no_priority != nmos::fields::authorization_highest_pri(settings))
                    {
                        slog::log<slog::severities::info>(gate, SLOG_FLF) << "Attempting discovery of a Authorization API in domain: " << nmos::get_domain(settings);

                        return nmos::experimental::resolve_service_(discovery, nmos::service_types::authorization, settings, token);
                    }
                    else
                    {
                        return pplx::task_from_result(std::list<resolved_service>{});
                    }
                }).get();

                with_write_lock(model.mutex, [&]
                {
                    if (!authorization_services.empty())
                    {
                        slog::log<slog::severities::info>(gate, SLOG_FLF) << "Discovered " << authorization_services.size() << " Authorization API(s)";
                    }
                    else
                    {
                        slog::log<slog::severities::warning>(gate, SLOG_FLF) << "Did not discover a suitable Authorization API via DNS-SD";

                        auto fallback_authorization_service = get_authorization_service(model.settings);
                        if (!fallback_authorization_service.second.is_empty())
                        {
                            authorization_services.push_back(fallback_authorization_service);
                        }
                    }

                    if (!authorization_services.empty()) slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "Using the Authorization API(s):" << slog::log_manip([&](slog::log_statement& s)
                    {
                        for (auto& authorization_service : authorization_services)
                        {
                            s << '\n' << authorization_service.second.to_string();
                        }
                    });

                    model.settings[nmos::experimental::fields::authorization_services] = web::json::value_from_elements(authorization_services | boost::adaptors::transformed([](const resolved_service& authorization_service) { return make_service(authorization_service); }));

                    model.notify();
                });

                return !authorization_services.empty();
            }

            bool empty_authorization_services(const nmos::settings& settings)
            {
                return web::json::empty(nmos::experimental::fields::authorization_services(settings));
            }

            bool has_discovered_authorization_services(const nmos::base_model& model)
            {
                return with_read_lock(model.mutex, [&] { return !empty_authorization_services(model.settings); });
            }

            // "The Node selects an Authorization API to use based on the priority"
            resolved_service top_authorization_service(const nmos::settings& settings)
            {
                const auto value = web::json::front(nmos::experimental::fields::authorization_services(settings));
                return parse_service(value);
            }

            // "If the chosen Authorization API does not respond correctly at any time,
            // another Authorization API should be selected from the discovered list."
            void pop_authorization_service(nmos::settings& settings)
            {
                web::json::pop_front(nmos::experimental::fields::authorization_services(settings));
                // "TTLs on advertised services" may have expired too, so should cache time-to-live values
                // using DNSServiceQueryRecord instead of DNSServiceResolve?
            }
        }

        // service discovery operation
        namespace details
        {
            void authorization_services_background_discovery(nmos::base_model& model, mdns::service_discovery& discovery, slog::base_gate& gate)
            {
                slog::log<slog::severities::info>(gate, SLOG_FLF) << "Adopting background discovery of an Authorization API";

                auto lock = model.write_lock();
                auto& condition = model.condition;
                auto& shutdown = model.shutdown;

                bool authorization_services_discovered(false);

                // background tasks may read/write the above local state by reference
                pplx::cancellation_token_source cancellation_source;
                auto token = cancellation_source.get_token();
                pplx::task<void> background_discovery = pplx::do_while([&]
                {
                    // add a short delay since initial discovery or rediscovery must have only just failed
                    // (this also prevents a tight loop in the case that the underlying DNS-SD implementation is just refusing to co-operate
                    // though that would be better indicated by an exception from discover_authorization_services)
                    return pplx::complete_after(std::chrono::seconds(1), token).then([&]
                    {
                        return !discover_authorization_services(model, discovery, gate, token);
                    });
                }, token).then([&]
                {
                    auto lock = model.write_lock(); // in order to update local state

                    authorization_services_discovered = true; // since discovery must have succeeded

                    model.notify();
                });

                for (;;)
                {
                    // wait for the thread to be interrupted because an Authorization API has been discovered
                    // or because the server is being shut down
                    condition.wait(lock, [&] { return shutdown || authorization_services_discovered; });
                    if (shutdown || authorization_services_discovered) break;
                }

                cancellation_source.cancel();
                // wait without the lock since it is also used by the background tasks
                nmos::details::reverse_lock_guard<nmos::write_lock> unlock{ lock };
                background_discovery.wait();
            }
        }
    }
}
