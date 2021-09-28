#include "nmos/node_system_behaviour.h"

#include "pplx/pplx_utils.h" // for pplx::complete_at
#include "cpprest/http_client.h"
#include "cpprest/json_validator.h"
#include "mdns/service_discovery.h"
#include "nmos/api_utils.h"
#include "nmos/client_utils.h"
#include "nmos/is09_versions.h"
#include "nmos/json_schema.h"
#include "nmos/mdns.h"
#include "nmos/model.h"
#include "nmos/random.h"
#include "nmos/slog.h"
#include "nmos/thread_utils.h" // for wait_until, reverse_lock_guard

namespace nmos
{
    namespace details
    {
        void node_system_behaviour_thread(nmos::model& model, load_ca_certificates_handler load_ca_certificates, system_global_handler system_changed, mdns::service_discovery& discovery, slog::base_gate& gate);

        void node_system_behaviour(nmos::model& model, load_ca_certificates_handler load_ca_certificates, system_global_handler system_changed, slog::base_gate& gate);

        // background service discovery
        void system_services_background_discovery(nmos::model& model, mdns::service_discovery& discovery, slog::base_gate& gate);

        // service discovery
        bool discover_system_services(nmos::base_model& model, mdns::service_discovery& discovery, slog::base_gate& gate, const pplx::cancellation_token& token = pplx::cancellation_token::none());
        bool has_discovered_system_services(const nmos::model& model);
    }

    // uses the default DNS-SD implementation
    // callbacks from this function are called with the model locked, and may read or write directly to the model
    void node_system_behaviour_thread(nmos::model& model, load_ca_certificates_handler load_ca_certificates, system_global_handler system_changed, slog::base_gate& gate_)
    {
        nmos::details::omanip_gate gate(gate_, nmos::stash_category(nmos::categories::node_system_behaviour));

        mdns::service_discovery discovery(gate);

        details::node_system_behaviour_thread(model, std::move(load_ca_certificates), std::move(system_changed), discovery, gate);
    }

    // uses the specified DNS-SD implementation
    // callbacks from this function are called with the model locked, and may read or write directly to the model
    void node_system_behaviour_thread(nmos::model& model, load_ca_certificates_handler load_ca_certificates, system_global_handler system_changed, mdns::service_discovery& discovery, slog::base_gate& gate_)
    {
        nmos::details::omanip_gate gate(gate_, nmos::stash_category(nmos::categories::node_system_behaviour));

        details::node_system_behaviour_thread(model, std::move(load_ca_certificates), std::move(system_changed), discovery, gate);
    }

    void details::node_system_behaviour_thread(nmos::model& model, load_ca_certificates_handler load_ca_certificates, system_global_handler system_changed, mdns::service_discovery& discovery, slog::base_gate& gate)
    {
        enum
        {
            initial_discovery,
            node_system_behaviour,
            rediscovery,
            background_discovery
        } mode = initial_discovery;

        // If the chosen System API does not respond correctly at any time, another System API should be selected from the discovered list.

        with_write_lock(model.mutex, [&model] { model.settings[nmos::fields::system_services] = web::json::value::array(); });

        nmos::details::seed_generator discovery_backoff_seeder;
        std::default_random_engine discovery_backoff_engine(discovery_backoff_seeder);
        double discovery_backoff = 0;

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
                    slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "Waiting to retry System API discovery for about " << std::fixed << std::setprecision(3) << random_backoff << " seconds (current backoff limit: " << discovery_backoff << " seconds)";
                    model.wait_for(lock, std::chrono::milliseconds(std::chrono::milliseconds::rep(1000 * random_backoff)), [&] { return model.shutdown; });
                    if (model.shutdown) break;
                }

                // The Node performs a DNS-SD browse for services of type '_nmos-system._tcp' as specified.
                if (details::discover_system_services(model, discovery, gate))
                {
                    mode = node_system_behaviour;

                    // If the Node is unable to contact the System API, the Node implements an exponential backoff algorithm
                    // to avoid overloading the System API in the event of a system restart.
                    auto lock = model.read_lock();
                    discovery_backoff = (std::min)((std::max)((double)nmos::fields::discovery_backoff_min(model.settings), discovery_backoff * nmos::fields::discovery_backoff_factor(model.settings)), (double)nmos::fields::discovery_backoff_max(model.settings));
                }
                else
                {
                    mode = background_discovery;
                }
                break;

            case node_system_behaviour:
                details::node_system_behaviour(model, load_ca_certificates, system_changed, gate);

                // Should no further System APIs be available or TTLs on advertised services expired, a re-query may be performed.
                mode = rediscovery;
                break;

            case background_discovery:
                details::system_services_background_discovery(model, discovery, gate);

                if (details::has_discovered_system_services(model))
                {
                    mode = node_system_behaviour;
                }

                break;
            }
        }
    }

    // service discovery
    namespace details
    {
        // query DNS Service Discovery for any System API in the specified browse domain, having priority in the specified range
        // otherwise, after timeout or cancellation, returning the fallback system service
        // see https://github.com/AMWA-TV/nmos-system/blob/v1.0/docs/3.0.%20Discovery.md
        web::json::value discover_system_services(mdns::service_discovery& discovery, const std::string& browse_domain, const std::set<nmos::api_version>& versions, const std::pair<nmos::service_priority, nmos::service_priority>& priorities, const std::set<nmos::service_protocol>& protocols, const web::uri& fallback_service, slog::base_gate& gate, const std::chrono::steady_clock::duration& timeout, const pplx::cancellation_token& token = pplx::cancellation_token::none())
        {
            std::list<web::uri> system_services;

            if (nmos::service_priorities::no_priority != priorities.first)
            {
                slog::log<slog::severities::info>(gate, SLOG_FLF) << "Attempting discovery of a System API in domain: " << browse_domain;

                system_services = nmos::experimental::resolve_service(discovery, nmos::service_types::system, browse_domain, versions, priorities, protocols, { false }, true, timeout, token).get();

                if (!system_services.empty())
                {
                    slog::log<slog::severities::info>(gate, SLOG_FLF) << "Discovered " << system_services.size() << " System API(s)";
                }
                else
                {
                    slog::log<slog::severities::warning>(gate, SLOG_FLF) << "Did not discover a suitable System API via DNS-SD";
                }
            }

            if (system_services.empty())
            {
                if (!fallback_service.is_empty())
                {
                    system_services.push_back(fallback_service);
                }
            }

            return web::json::value_from_elements(system_services | boost::adaptors::transformed([](const web::uri& u) { return u.to_string(); }));
        }

        // get the fallback system service from settings (if present)
        web::uri get_system_service(const nmos::settings& settings)
        {
            return settings.has_field(nmos::fields::system_address)
                ? web::uri_builder()
                .set_scheme(nmos::http_scheme(settings))
                .set_host(nmos::fields::system_address(settings))
                .set_port(nmos::fields::system_port(settings))
                .set_path(U("/x-nmos/system/") + nmos::fields::system_version(settings))
                .to_uri()
                : web::uri();
        }

        // query DNS Service Discovery for any System API based on settings
        bool discover_system_services(nmos::base_model& model, mdns::service_discovery& discovery, slog::base_gate& gate, const pplx::cancellation_token& token)
        {
            std::string browse_domain;
            std::set<nmos::api_version> versions;
            std::pair<nmos::service_priority, nmos::service_priority> priorities;
            std::set<nmos::service_protocol> protocols;
            web::uri fallback_system_service;
            int timeout;
            with_read_lock(model.mutex, [&]
            {
                auto& settings = model.settings;
                browse_domain = utility::us2s(nmos::get_domain(settings));
                versions = nmos::is09_versions::from_settings(settings);
                priorities = { nmos::fields::highest_pri(settings), nmos::fields::lowest_pri(settings) };
                protocols = { nmos::get_service_protocol(settings) };
                fallback_system_service = get_system_service(settings);

                // use a short timeout that's long enough to ensure the daemon's cache is exhausted
                // when no cancellation token is specified
                timeout = token.is_cancelable() ? nmos::fields::discovery_backoff_max(settings) : 1;
            });

            slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "Trying System API discovery for about " << std::fixed << std::setprecision(3) << (double)timeout << " seconds";
            auto services = discover_system_services(discovery, browse_domain, versions, priorities, protocols, fallback_system_service, gate, std::chrono::seconds(timeout), token);
            with_write_lock(model.mutex, [&] { model.settings[nmos::fields::system_services] = services; });
            model.notify();

            return !web::json::empty(services);
        }

        bool empty_system_services(const nmos::settings& settings)
        {
            return web::json::empty(nmos::fields::system_services(settings));
        }

        bool has_discovered_system_services(const nmos::model& model)
        {
            return with_read_lock(model.mutex, [&] { return !empty_system_services(model.settings); });
        }

        // "The Node selects a System API to use based on the priority"
        web::uri top_system_service(const nmos::settings& settings)
        {
            return web::uri(web::json::front(nmos::fields::system_services(settings)).as_string());
        }

        // If the chosen System API does not respond correctly at any time,
        // another System API should be selected from the discovered list.
        void pop_system_service(nmos::settings& settings)
        {
            web::json::pop_front(nmos::fields::system_services(settings));
        }
    }

    namespace details
    {
        web::http::client::http_client_config make_system_client_config(const nmos::settings& settings, load_ca_certificates_handler load_ca_certificates, slog::base_gate& gate)
        {
            auto config = nmos::make_http_client_config(settings, std::move(load_ca_certificates), gate);
            config.set_timeout(std::chrono::seconds(nmos::fields::system_request_max(settings)));
            return config;
        }

        struct system_service_exception {};

        // make an asynchronous GET request on the System API to fetch the global configuration resource
        pplx::task<web::json::value> request_system_global(web::http::client::http_client client, slog::base_gate& gate, const pplx::cancellation_token& token = pplx::cancellation_token::none())
        {
            slog::log<slog::severities::too_much_info>(gate, SLOG_FLF) << "Requesting system global configuration resource";

            return client.request(web::http::methods::GET, U("/global"), token).then([=, &gate](pplx::task<web::http::http_response> response_task)
            {
                auto response = response_task.get(); // may throw http_exception

                if (web::http::status_codes::OK == response.status_code())
                {
                    return response.extract_json().then([&gate](web::json::value body)
                    {
                        slog::log<slog::severities::too_much_info>(gate, SLOG_FLF) << "Received system global configuration resource: " << body.serialize();
                        return body;
                    }, token);
                }
                else
                {
                    slog::log<slog::severities::error>(gate, SLOG_FLF) << "System global configuration resource error: " << response.status_code() << " " << response.reason_phrase();
                    throw system_service_exception();
                }
            }, token);
        }

        static const web::json::experimental::json_validator& system_validator()
        {
            // hmm, could be based on supported API versions from settings, like other APIs' validators?
            static const web::json::experimental::json_validator validator
            {
                nmos::experimental::load_json_schema,
                boost::copy_range<std::vector<web::uri>>(is09_versions::all | boost::adaptors::transformed(experimental::make_systemapi_global_schema_uri))
            };
            return validator;
        }

        struct node_system_shared_state
        {
            system_global_handler handler;
            web::uri base_uri;
            web::json::value data;

            bool system_service_error;

            nmos::details::seed_generator seeder;
            std::default_random_engine engine;
            std::unique_ptr<web::http::client::http_client> client;

            explicit node_system_shared_state(system_global_handler handler) : handler(std::move(handler)), system_service_error(false), engine(seeder) {}
        };

        // task to continuously fetch the system global configuration resource on a time interval until failure or cancellation
        pplx::task<void> do_system_global_requests(nmos::model& model, node_system_shared_state& state, slog::base_gate& gate, const pplx::cancellation_token& token = pplx::cancellation_token::none())
        {
            const auto& system_interval_min(nmos::experimental::fields::system_interval_min(model.settings));
            const auto& system_interval_max(nmos::experimental::fields::system_interval_max(model.settings));

            // start a background task to continously fetch the system global configuration resource on a given interval
            return pplx::do_while([=, &model, &state, &gate]
            {
                auto fetch_interval = std::chrono::seconds(0);
                if (state.base_uri == state.client->base_uri())
                {
                    auto interval = std::uniform_int_distribution<>(system_interval_min, system_interval_max)(state.engine);
                    fetch_interval = std::chrono::seconds(interval);

                    slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "Waiting to request system global configuration resource for about " << interval << " seconds";
                }

                auto fetch_time = std::chrono::steady_clock::now();
                return pplx::complete_at(fetch_time + fetch_interval, token).then([=, &state, &gate]() mutable
                {
                    return request_system_global(*state.client, gate, token);
                }).then([&model, &state, &gate](web::json::value data)
                {
                    // changes in the system global configuration resource?
                    if (state.base_uri != state.client->base_uri() || state.data != data)
                    {
                        auto lock = model.write_lock(); // in order to update local state

                        state.base_uri = state.client->base_uri();
                        state.data = data;

                        // base uri should be like http://api.example.com/x-nmos/system/{version}
                        const auto system_api_version = parse_api_version(web::uri::split_path(state.base_uri.path()).back());

                        system_validator().validate(data, experimental::make_systemapi_global_schema_uri(system_api_version));

                        // Synchronous notification of the system global configuration resource changes

                        if (state.handler)
                        {
                            // this callback should not throw exceptions
                            state.handler(state.base_uri, state.data);
                        }

                        model.notify();
                    }

                    return true;
                });
            }).then([&](pplx::task<void> finally)
            {
                auto lock = model.write_lock(); // in order to update local state

                try
                {
                    finally.get();
                }
                catch (const web::json::json_exception& e)
                {
                    slog::log<slog::severities::error>(gate, SLOG_FLF) << "System API request JSON error: " << e.what();
                }
                catch (const web::http::http_exception& e)
                {
                    slog::log<slog::severities::error>(gate, SLOG_FLF) << "System API request HTTP error: " << e.what() << " [" << e.error_code() << "]";
                }
                catch (const system_service_exception&)
                {
                    slog::log<slog::severities::error>(gate, SLOG_FLF) << "System API request error";
                }

                // reaching here, there must be something has gone wrong with the System API
                // let's select the next available System API
                state.system_service_error = true;

                model.notify();
            });
        }

        void node_system_behaviour(nmos::model& model, load_ca_certificates_handler load_ca_certificates, system_global_handler system_changed, slog::base_gate& gate)
        {
            slog::log<slog::severities::info>(gate, SLOG_FLF) << "Attempting System API node behaviour";

            auto lock = model.write_lock();
            auto& condition = model.condition;
            auto& shutdown = model.shutdown;

            // background task may also read/write the local state by reference
            node_system_shared_state state(std::move(system_changed));
            pplx::cancellation_token_source cancellation_source;
            auto requests = pplx::task_from_result();

            for (;;)
            {
                // wait for the thread to be interrupted because an error has been encountered with the selected system service
                // or because the server is being shut down
                // (or this is the first time through)
                condition.wait(lock, [&] { return shutdown || state.system_service_error || state.base_uri.is_empty(); });
                if (state.system_service_error)
                {
                    pop_system_service(model.settings);
                    model.notify();
                    state.system_service_error = false;

                    cancellation_source.cancel();
                    // wait without the lock since it is also used by the background tasks
                    details::reverse_lock_guard<nmos::write_lock> unlock{ lock };
                    requests.wait();

                    state.client.reset();
                    cancellation_source = pplx::cancellation_token_source();
                }
                if (shutdown || empty_system_services(model.settings)) break;

                // selects a System API to use based on the priority
                if (!state.client)
                {
                    const auto base_uri = top_system_service(model.settings);
                    state.client.reset(new web::http::client::http_client(base_uri, make_system_client_config(model.settings, load_ca_certificates, gate)));
                }

                auto token = cancellation_source.get_token();

                // start a background task to intermittently fetch the system global configuration resource
                requests = do_system_global_requests(model, state, gate, token);

                condition.wait(lock, [&] { return shutdown || state.system_service_error; });
            }

            // Synchronous notification that errors have been encountered with all discoverable System API instances
            // hmm, perhaps only if (!shutdown)?

            if (state.handler)
            {
                // this callback should not throw exceptions
                state.handler({}, {});
            }

            cancellation_source.cancel();
            details::reverse_lock_guard<nmos::write_lock> unlock{ lock };
            requests.wait();
        }
    }

    // service discovery operation
    namespace details
    {
        void system_services_background_discovery(nmos::model& model, mdns::service_discovery& discovery, slog::base_gate& gate)
        {
            slog::log<slog::severities::info>(gate, SLOG_FLF) << "Adopting background discovery of a System API";

            auto lock = model.write_lock();
            auto& condition = model.condition;
            auto& shutdown = model.shutdown;

            bool system_services_discovered(false);

            // background tasks may read/write the above local state by reference
            pplx::cancellation_token_source cancellation_source;
            auto token = cancellation_source.get_token();
            pplx::task<void> background_discovery = pplx::do_while([&]
            {
                // add a short delay since initial discovery or rediscovery must have only just failed
                // (this also prevents a tight loop in the case that the underlying DNS-SD implementation is just refusing to co-operate
                // though that would be better indicated by an exception from discover_system_services)
                return pplx::complete_after(std::chrono::seconds(1), token).then([&]
                {
                    return !discover_system_services(model, discovery, gate, token);
                });
            }, token).then([&]
            {
                auto lock = model.write_lock(); // in order to update local state

                system_services_discovered = true; // since discovery must have succeeded

                model.notify();
            });

            for (;;)
            {
                // wait for the thread to be interrupted because a System API has been discovered
                // or because the server is being shut down
                condition.wait(lock, [&] { return shutdown || system_services_discovered; });
                if (shutdown || system_services_discovered) break;
            }

            cancellation_source.cancel();
            // wait without the lock since it is also used by the background tasks
            nmos::details::reverse_lock_guard<nmos::write_lock> unlock{ lock };
            background_discovery.wait();
        }
    }
}
