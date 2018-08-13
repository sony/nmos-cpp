#include "nmos/node_behaviour.h"

#include "pplx/pplx_utils.h" // for pplx::complete_at
#include "cpprest/http_client.h"
#include "mdns/service_advertiser.h"
#include "mdns/service_discovery.h"
#include "nmos/api_downgrade.h"
#include "nmos/api_utils.h" // for nmos::type_from_resourceType
#include "nmos/log_manip.h"
#include "nmos/mdns.h"
#include "nmos/model.h"
#include "nmos/query_utils.h"
#include "nmos/rational.h"
#include "nmos/slog.h"
#include "nmos/thread_utils.h" // for wait_until, reverse_lock_guard
#include "nmos/version.h"

namespace nmos
{
    namespace details
    {
        // registered operation
        void initial_registration(nmos::id& self_id, nmos::model& model, const nmos::id& grain_id, const bool& shutdown, nmos::mutex& mutex, nmos::condition_variable& condition, std::multimap<service_priority, web::uri>& registration_services, slog::base_gate& gate);
        void registered_operation(const nmos::id& self_id, nmos::model& model, const nmos::id& grain_id, const bool& shutdown, nmos::mutex& mutex, nmos::condition_variable& condition, std::multimap<service_priority, web::uri>& registration_services, slog::base_gate& gate);

        // peer to peer operation
        void peer_to_peer_operation(nmos::model& model, const nmos::id& grain_id, const bool& shutdown, nmos::mutex& mutex, nmos::condition_variable& condition, std::multimap<service_priority, web::uri>& registration_services, mdns::service_discovery& discovery, mdns::service_advertiser& advertiser, slog::base_gate& gate);

        // service advertisement/discovery
        void advertise_node_service(const nmos::settings& settings, nmos::mutex& mutex, mdns::service_advertiser& advertiser);
        void discover_registration_services(const nmos::settings& settings, nmos::mutex& mutex, std::multimap<service_priority, web::uri>& registration_services, mdns::service_discovery& discovery, slog::base_gate& gate);

        // a (fake) subscription to keep track of all resource events
        nmos::resource make_node_behaviour_subscription(const nmos::id& id);
        nmos::resource make_node_behaviour_grain(const nmos::id& id, const nmos::id& subscription_id);
    }

    void node_behaviour_thread(nmos::model& model, const bool& shutdown, nmos::mutex& mutex, nmos::condition_variable& condition, slog::base_gate& gate)
    {
        // The possible states of node behaviour represent the two primary modes (registered operation and peer-to-peer operation)
        // and a few hopefully ephemeral states as the node works through the "Standard Registration Sequences".
        // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/docs/4.1.%20Behaviour%20-%20Registration.md
        enum
        {
            initial_discovery,
            initial_registration,
            registered_operation,
            rediscovery,
            peer_to_peer_operation
        } mode = initial_discovery;

        // "1. A Node is connected to the network"
        // "2. The Node runs an HTTP accessible Node API."
        // These should have happened by now...

        // "3. The Node produces an mDNS advertisement of type '_nmos-node._tcp' in the '.local' domain as specified in Node API."
        std::unique_ptr<mdns::service_advertiser> advertiser = mdns::make_advertiser(gate);
        mdns::service_advertiser_guard advertiser_guard(*advertiser);
        details::advertise_node_service(model.settings, mutex, *advertiser);

        // "If the chosen Registration API does not respond correctly at any time, another Registration API should be selected from the discovered list."
        // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/docs/3.1.%20Discovery%20-%20Registered%20Operation.md
        std::unique_ptr<mdns::service_discovery> discovery = mdns::make_discovery(gate);
        std::multimap<service_priority, web::uri> registration_services;

        double discovery_backoff = 0;

        // a (fake) subscription to keep track of all resource events
        auto grain_id = nmos::make_id();
        {
            nmos::write_lock lock(mutex);
            auto subscription_id = nmos::make_id();

            insert_resource(model.resources, details::make_node_behaviour_subscription(subscription_id));
            insert_resource(model.resources, details::make_node_behaviour_grain(grain_id, subscription_id));
        }

        // there should be exactly one node resource, but it may not have been added yet
        // and during a controlled shutdown it may be removed; it is therefore identified
        // during initial registration for use in registered operation
        nmos::id self_id;

        // continue until the server is being shut down
        for (;;)
        {
            if (with_read_lock(mutex, [&] { return shutdown; })) break;

            switch (mode)
            {
            case initial_discovery:
            case rediscovery:
                if (0 != discovery_backoff)
                {
                    nmos::read_lock lock(mutex);
                    // using wait_until rather than wait_for as a workaround for an awful bug in VS2015, resolved in VS2017
                    condition.wait_until(lock, std::chrono::steady_clock::now() + std::chrono::milliseconds(std::chrono::milliseconds::rep(1000 * discovery_backoff)), [&] { return shutdown; });
                    if (shutdown) break;
                }

                // "4. The Node performs a DNS-SD browse for services of type '_nmos-registration._tcp' as specified."
                details::discover_registration_services(model.settings, mutex, registration_services, *discovery, gate);

                if (!registration_services.empty())
                {
                    mode = initial_discovery == mode ? initial_registration : registered_operation;

                    // "Should a 5xx error be encountered when interacting with all discoverable Registration APIs it is recommended that clients
                    // implement an exponential backoff algorithm in their next attempts until a non-5xx response code is received."
                    // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/docs/4.1.%20Behaviour%20-%20Registration.md#node-encounters-http-500-or-other-5xx-inability-to-connect-or-a-timeout-on-heartbeat
                    nmos::read_lock lock(mutex);
                    discovery_backoff = (std::min)((std::max)((double)nmos::fields::discovery_backoff_min(model.settings), discovery_backoff * nmos::fields::discovery_backoff_factor(model.settings)), (double)nmos::fields::discovery_backoff_max(model.settings));
                }
                else
                {
                    // "If no Registration APIs are advertised on a network, the Node should assume peer to peer operation unless configured otherwise."
                    mode = peer_to_peer_operation;
                }
                break;

            case initial_registration:
                // "5. The Node registers itself with the Registration API by taking the object it holds under the Node API's /self resource and POSTing this to the Registration API."
                details::initial_registration(self_id, model, grain_id, shutdown, mutex, condition, registration_services, gate);

                if (!registration_services.empty())
                {
                    mode = registered_operation;

                    discovery_backoff = 0;
                }
                else
                {
                    mode = initial_discovery;
                }
                break;

            case registered_operation:
                // "6. The Node persists itself in the registry by issuing heartbeats."
                // "7. The Node registers its other resources (from /devices, /sources etc) with the Registration API."
                details::registered_operation(self_id, model, grain_id, shutdown, mutex, condition, registration_services, gate);

                if (!registration_services.empty())
                {
                    // "A 404 error on heartbeat indicates that the Node performing the heartbeat is not known to the Registration API. [The] Node must re-register each of its resources with the Registration API in order."
                    mode = initial_registration;

                    discovery_backoff = 0;
                }
                else
                {
                    // "Should no further Registration APIs be available or TTLs on advertised services expired, a re-query may be performed."
                    mode = rediscovery;
                }
                break;

            case peer_to_peer_operation:
                details::peer_to_peer_operation(model, grain_id, shutdown, mutex, condition, registration_services, *discovery, *advertiser, gate);

                if (!registration_services.empty())
                {
                    mode = initial_registration;
                }
                break;
            }
        }
    }

    // service advertisement/discovery
    namespace details
    {
        // register the node service with the required TXT records
        void advertise_node_service(mdns::service_advertiser& advertiser, const nmos::settings& settings)
        {
            const auto pri = nmos::fields::pri(settings);
            // no_priority allows the node to run unadvertised
            if (nmos::service_priorities::no_priority != pri)
            {
                auto records = nmos::make_txt_records(nmos::service_types::node);
                nmos::experimental::register_service(advertiser, nmos::service_types::node, settings, records);
            }
        }

        void advertise_node_service(const nmos::settings& settings, nmos::mutex& mutex, mdns::service_advertiser& advertiser)
        {
            advertise_node_service(advertiser, with_read_lock(mutex, [&] { return settings; }));
        }

        std::multimap<service_priority, web::uri> discover_registration_services(mdns::service_discovery& discovery, const std::string& browse_domain, const std::pair<nmos::service_priority, nmos::service_priority>& priorities, const web::uri& fallback_registration_service, const std::chrono::seconds& timeout, slog::base_gate& gate)
        {
            std::multimap<service_priority, web::uri> registration_services;
            
            if (nmos::service_priorities::no_priority != priorities.first)
            {
                slog::log<slog::severities::info>(gate, SLOG_FLF) << "Attempting discovery of a Registration API";

                // hmmm, no way to cancel this currently...
                registration_services = nmos::experimental::resolve_service(discovery, nmos::service_types::registration, browse_domain, nmos::is04_versions::all, true, timeout);

                // erase results with unsuitable priorities (too high or too low) to avoid development and live systems colliding
                // only services between priorities.first and priorities.second (inclusive) should be returned
                registration_services.erase(registration_services.begin(), registration_services.lower_bound(priorities.first));
                registration_services.erase(registration_services.upper_bound(priorities.second), registration_services.end());

                if (!registration_services.empty())
                {
                    slog::log<slog::severities::info>(gate, SLOG_FLF) << "Discovered " << registration_services.size() << " Registration API(s)";
                }
                else
                {
                    slog::log<slog::severities::warning>(gate, SLOG_FLF) << "Did not discover a suitable Registration API via DNS-SD";
                }
            }

            if (registration_services.empty())
            {
                if (!fallback_registration_service.is_empty())
                {
                    registration_services.insert({ nmos::service_priorities::no_priority, fallback_registration_service });
                }
            }

            return registration_services;
        }

        // get the fallback registration service from settings (if present)
        web::uri get_registration_service(const nmos::settings& settings)
        {
            return settings.has_field(nmos::fields::registry_address)
                ? web::uri_builder()
                .set_scheme(U("http"))
                .set_host(nmos::fields::registry_address(settings))
                .set_port(nmos::fields::registration_port(settings))
                .set_path(U("/x-nmos/registration/") + nmos::fields::registry_version(settings))
                .to_uri()
                : web::uri();
        }

        void discover_registration_services(const nmos::settings& settings, nmos::mutex& mutex, std::multimap<service_priority, web::uri>& registration_services, mdns::service_discovery& discovery, slog::base_gate& gate)
        {
            std::string browse_domain;
            std::pair<nmos::service_priority, nmos::service_priority> priorities;
            web::uri fallback_registration_service;
            std::chrono::seconds discovery_interval;
            {
                nmos::read_lock lock(mutex);
                browse_domain = utility::us2s(nmos::fields::domain(settings));
                priorities = { nmos::fields::highest_pri(settings), nmos::fields::lowest_pri(settings) };
                fallback_registration_service = get_registration_service(settings);
                discovery_interval = std::chrono::seconds(nmos::fields::discovery_backoff_max(settings));
            }

            registration_services = details::discover_registration_services(discovery, browse_domain, priorities, fallback_registration_service, discovery_interval, gate);
        }

        // "The Node selects a Registration API to use based on the priority"
        const web::uri& top_registration_service(const std::multimap<service_priority, web::uri>& registration_services)
        {
            return registration_services.begin()->second;
        }

        // "If the chosen Registration API does not respond correctly at any time,
        // another Registration API should be selected from the discovered list."
        void pop_registration_service(std::multimap<service_priority, web::uri>& registration_services)
        {
            registration_services.erase(registration_services.begin());
            // "TTLs on advertised services" may have expired too, so should cache time-to-live values
            // using DNSServiceQueryRecord instead of DNSServiceResolve?
        }
    }

    // a (fake) subscription to keep track of all resource events
    namespace details
    {
        const utility::string_t node_behaviour_resource_path;
        const utility::string_t node_behaviour_topic = node_behaviour_resource_path + U("/");

        nmos::resource make_node_behaviour_subscription(const nmos::id& id)
        {
            using web::json::value;
            value data;
            data[nmos::fields::id] = value::string(id);
            data[nmos::fields::max_update_rate_ms] = 0; // no throttling used at present
            data[nmos::fields::persist] = value::boolean(false); // not to be deleted by someone else
            data[nmos::fields::resource_path] = value::string(node_behaviour_resource_path);
            data[nmos::fields::params] = value::object();
            // no ws_href since subscriptions are inaccessible on the Node API anyway
            return{ nmos::is04_versions::v1_2, nmos::types::subscription, data, true };
        }

        nmos::resource make_node_behaviour_grain(const nmos::id& id, const nmos::id& subscription_id)
        {
            using web::json::value;
            value data;
            data[nmos::fields::id] = value::string(id);
            data[nmos::fields::subscription_id] = value::string(subscription_id);
            data[nmos::fields::message] = details::make_grain(nmos::make_id(), subscription_id, node_behaviour_topic);
            nmos::fields::message_grain_data(data) = value::array();
            return{ nmos::is04_versions::v1_2, nmos::types::grain, data, true };
        }

        struct node_behaviour_grain_guard
        {
            node_behaviour_grain_guard(nmos::resources& resources, nmos::resources::iterator grain, web::json::value& events)
                : resources(resources)
                , grain(grain)
                , events(events)
            {
                // steal the events from the grain
                // reset the grain for next time
                resources.modify(grain, [&](nmos::resource& grain)
                {
                    using std::swap;
                    swap(events, nmos::fields::message_grain_data(grain.data));
                    grain.updated = strictly_increasing_update(resources);
                });
            }

            ~node_behaviour_grain_guard()
            {
                if (0 == events.size()) return;

                // restore any remaining events to the grain
                resources.modify(grain, [&](nmos::resource& grain)
                {
                    // don't overwrite any events that have subsequently been inserted into the grain
                    // (web::json::value has a rather limited interface for the manipulation of arrays)
                    for (const auto& event : nmos::fields::message_grain_data(grain.data).as_array())
                    {
                        web::json::push_back(events, event);
                    }

                    using std::swap;
                    swap(nmos::fields::message_grain_data(grain.data), events);
                    grain.updated = strictly_increasing_update(resources);
                });
            }

            nmos::resources& resources;
            nmos::resources::iterator grain;
            web::json::value& events;
        };
    }

    // registered operation
    namespace details
    {
        web::json::value make_registration_request_body(const nmos::type& type, const web::json::value& data, const nmos::api_version& registry_version)
        {
            // the node behaviour subscription version is currently fixed (see make_node_behaviour_subscription)
            // rather than being set to the registry version, so a downgrade is required if the registry version is lower
            return web::json::value_of(
            {
                { U("type"), web::json::value::string(type.name) },
                { U("data"), nmos::downgrade(nmos::is04_versions::v1_2, type, data, registry_version, registry_version) }
            });
        }

        // server-side (5xx) registration error
        struct registration_service_exception {};

        // should be called when an error condition has been identified, because it will always log
        void handle_registration_error_conditions(const web::http::http_response& response, bool handle_client_error_as_server_error, slog::base_gate& gate, const char* operation)
        {
            // "For HTTP codes 400 and upwards, a JSON format response MUST be returned [in which]
            // the 'code' should always match the HTTP status code. 'error' must always be present
            // and in string format. 'debug' may be null if no further debug information is available"
            // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/docs/2.0.%20APIs.md#error-codes--responses
            // Especially in the case of client (4xx) errors, logging these would be a good idea, but
            // would necessitate blocking for the response body, and extracting them from the json
            // and dealing with potential errors along the way...

            // "A 500 [or other 5xx] error, inability to connect or a timeout indicates a server side or connectivity issue."
            // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/docs/4.1.%20Behaviour%20-%20Registration.md#node-encounters-http-500-or-other-5xx-inability-to-connect-or-a-timeout-on-heartbeat
            if (handle_client_error_as_server_error ? web::http::is_error_status_code(response.status_code()) : web::http::is_server_error_status_code(response.status_code()))
            {
                // this could be regarded as a 'severe' error - presumably it is for the registry
                // on the other hand, since the node has a strategy to recover, it could be regarded as only a 'warning'
                // so on balance, log as an 'error'
                slog::log<slog::severities::error>(gate, SLOG_FLF) << "Registration " << operation << " error: " << response.status_code() << " " << response.reason_phrase();

                throw registration_service_exception();
            }
            // "A 400 [or other 4xx] error [in response to a POST] indicates a client error which is likely
            // to be the result of a validation failure identified by the Registration API."
            // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/docs/4.1.%20Behaviour%20-%20Registration.md#node-encounters-http-400-or-other-4xx-on-registration
            else if (web::http::is_client_error_status_code(response.status_code()))
            {
                // the severity here is trickier, since if it truly indicated a validation failure, this is a 'severe' error
                // but unfortunately, there are circumstances described below where it could be regarded as only a 'warning'
                // so again, until there's a means to distinguish these cases, log as an 'error'
                slog::log<slog::severities::error>(gate, SLOG_FLF) << "Registration " << operation << " error: " << response.status_code() << " " << response.reason_phrase();

                // "The same request must not be re-attempted without corrective action being taken first.
                // Error responses as detailed in the APIs documentation may assist with debugging these issues."
                // In an automated system, the best option seems to be to allow the registry-held representation
                // of the Node's resources to become out of sync with the Node's view, and flag this to the user
                // as visibly as possible.

                // Note that a 400 error can also indicate that the super-resource was not found due to recent
                // garbage collection in the Registration API, even when this has not yet been indicated by a
                // 404 error on heartbeat. Unfortunately, this situation cannot easily be distinguished from a
                // validation failure at this time, another reason not to handle 4xx errors like 5xx errors.

                // Similarly, a 404 error in response to a DELETE indicates either that the resource has already
                // been explicitly deleted (i.e. a real error somewhere), or that it was not found due to recent
                // garbage collection as above.
            }
            else
            {
                // this is a non-error status code, it might even be a successful (2xx) code, but since the
                // calling function didn't expect it, log as an 'error'
                slog::log<slog::severities::error>(gate, SLOG_FLF) << "Registration " << operation << " error: " << response.status_code() << " " << response.reason_phrase();
            }
        }

        void handle_registration_error_conditions(const web::http::http_response& response, slog::base_gate& gate, const char* operation)
        {
            handle_registration_error_conditions(response, false, gate, operation);
        }

        web::http::client::http_client_config config_with_timeout(const std::chrono::seconds& timeout)
        {
            web::http::client::http_client_config config;
            config.set_timeout(timeout);
            return config;
        }

        // make an asynchronous POST or DELETE request on the Registration API specified by the client for the specified resource event
        pplx::task<void> request_registration(web::http::client::http_client client, const web::json::value& event, slog::base_gate& gate, const pplx::cancellation_token& token = pplx::cancellation_token::none())
        {
            // base uri should be like http://example.api.com/x-nmos/registration/{version}
            const auto& registry_version = parse_api_version(web::uri::split_path(client.base_uri().path()).back());

            const auto& path = event.at(U("path")).as_string();
            const auto id_type = get_resource_event_resource(node_behaviour_topic, event);
            const auto event_type = get_resource_event_type(event);

            // An 'added' event calls for registration creation, i.e. a POST request with a 201 'Created' response (200 'OK' is unexpected)
            // A 'removed' event calls for registration deletion, i.e. a DELETE request with a 204 'No Content' response
            // A 'modified' event calls for a registration update, i.e. a POST request with a 200 'OK' response (201 'Created'is unexpected)
            // A 'sync' event is the call for registration creation when first interacting with a registry
            // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/APIs/RegistrationAPI.raml

            const bool creation = resource_added_event == event_type || resource_unchanged_event == event_type;
            const bool update = resource_modified_event == event_type;
            const bool deletion = resource_removed_event == event_type;

            if (creation)
            {
                slog::log<slog::severities::info>(gate, SLOG_FLF) << "Requesting registration creation for " << id_type;

                auto body = make_registration_request_body(id_type.second, event.at(U("post")), registry_version);

                return client.request(web::http::methods::POST, U("/resource"), body, token).then([=, &gate](web::http::http_response response) mutable
                {
                    // hmm, when I tried to make this a task-based continuation in order to just return the response_task argument (in most cases)
                    // the enclosing then call failed to compile

                    // "On first registration with a Registration API this should result in a '201 Created' HTTP response code.
                    // If a Node receives a 200 code in this case, a previous record of the Node can be assumed to still exist."
                    // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/docs/4.1.%20Behaviour%20-%20Registration.md#node-encounters-http-200-on-first-registration

                    if (web::http::status_codes::Created == response.status_code())
                    {
                        // successful registration will be logged by the continuation

                        return pplx::task_from_result(response);
                    }
                    else if (web::http::status_codes::OK == response.status_code())
                    {
                        slog::log<slog::severities::warning>(gate, SLOG_FLF) << "Registration out of sync for " << id_type;

                        slog::log<slog::severities::info>(gate, SLOG_FLF) << "Requesting out of sync registration deletion for " << id_type;

                        // "In order to avoid the registry-held representation of the Node's resources from being out of sync
                        // with the Node's view, an HTTP DELETE should be performed in this situation to explicitly clear the
                        // registry of the Node and any sub-resources."

                        return client.request(web::http::methods::DEL, U("/resource/") + path, token).then([=, &gate](web::http::http_response response) mutable
                        {
                            if (web::http::status_codes::NoContent == response.status_code())
                            {
                                slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "Registration deleted for " << id_type;
                            }
                            else
                            {
                                handle_registration_error_conditions(response, gate, "deletion");
                            }

                            slog::log<slog::severities::info>(gate, SLOG_FLF) << "Re-requesting registration creation for " << id_type;

                            // "A new Node registration after this point should result in the correct 201 response code."
                            return client.request(web::http::methods::POST, U("/resource"), body, token);
                        });
                    }
                    else
                    {
                        // registration errors (4xx, 5xx) will be logged by the continuation

                        return pplx::task_from_result(response);
                    }
                }).then([=, &gate](web::http::http_response response)
                {
                    if (web::http::status_codes::Created == response.status_code())
                    {
                        slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "Registration created for " << id_type;
                    }
                    else
                    {
                        // after a 400 or other 4xx error in response to creating the registration for the node resource itself
                        // there's definitely no point adopting registered operation, so handle this case like a server error
                        const bool initial_registration = nmos::types::node == id_type.second;
                        handle_registration_error_conditions(response, initial_registration, gate, "creation");
                    }
                });
            }
            else if (update)
            {
                slog::log<slog::severities::info>(gate, SLOG_FLF) << "Requesting registration update for " << id_type;

                auto body = make_registration_request_body(id_type.second, event.at(U("post")), registry_version);

                return client.request(web::http::methods::POST, U("/resource"), body, token).then([=, &gate](web::http::http_response response)
                {
                    if (web::http::status_codes::OK == response.status_code())
                    {
                        slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "Registration updated for " << id_type;
                    }
                    else
                    {
                        handle_registration_error_conditions(response, gate, "update");
                    }
                });
            }
            else if (deletion)
            {
                slog::log<slog::severities::info>(gate, SLOG_FLF) << "Requesting registration deletion for " << id_type;

                return client.request(web::http::methods::DEL, U("/resource/") + path, token).then([=, &gate](web::http::http_response response)
                {
                    if (web::http::status_codes::NoContent == response.status_code())
                    {
                        slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "Registration deleted for " << id_type;
                    }
                    else
                    {
                        handle_registration_error_conditions(response, gate, "deletion");
                    }
                });
            }

            // probably an error to get here
            return pplx::task_from_result();
        }

        // asynchronously perform a heartbeat and return a result that indicates whether the heartbeat was successful
        pplx::task<bool> update_node_health(web::http::client::http_client client, const nmos::id& id, slog::base_gate& gate, const pplx::cancellation_token& token = pplx::cancellation_token::none())
        {
            slog::log<slog::severities::too_much_info>(gate, SLOG_FLF) << "Posting registration heartbeat for node: " << id;

            return client.request(web::http::methods::POST, U("/health/nodes/") + id, token).then([=, &gate](pplx::task<web::http::http_response> response_task)
            {
                auto response = response_task.get(); // may throw http_exception

                if (web::http::status_codes::OK == response.status_code())
                {
                    return true;
                }
                else if (web::http::status_codes::NotFound == response.status_code())
                {
                    // although there's a recovery strategy here, so this could be regarded as a 'warning'
                    // it is definitely unexpected, so log it as an 'error'
                    slog::log<slog::severities::error>(gate, SLOG_FLF) << "Registration heartbeat error: " << response.status_code() << " " << response.reason_phrase();

                    // "On encountering this code, a Node must re-register each of its resources with the Registration API in order."
                    // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/docs/4.1.%20Behaviour%20-%20Registration.md#node-encounters-http-404-on-heartbeat
                    return false;
                }
                else
                {
                    handle_registration_error_conditions(response, gate, "heartbeat");

                    // if we get here, it's not a server (5xx) error, so the best option seems to be to continue
                    // even though we don't really know what's going on...
                    return true;
                }
            }, token);
        }

        // there is significant similarity between initial_registration and registered_operation but I'm too tired to refactor again right now...
        void initial_registration(nmos::id& self_id, nmos::model& model, const nmos::id& grain_id, const bool& shutdown, nmos::mutex& mutex, nmos::condition_variable& condition, std::multimap<service_priority, web::uri>& registration_services, slog::base_gate& gate)
        {
            slog::log<slog::severities::info>(gate, SLOG_FLF) << "Attempting initial registration";

            nmos::write_lock lock(mutex);

            const auto grain = nmos::find_resource(model.resources, { grain_id, nmos::types::grain });
            if (model.resources.end() == grain) return;

            std::unique_ptr<web::http::client::http_client> registration_client;

            // "5. The Node registers itself with the Registration API by taking the object it holds under the Node API's /self resource and POSTing this to the Registration API."

            // reset the node behaviour subscription grain; if the node resource has already been added to the model then
            // the first event will be a 'sync' event for the node (and if not, there really should be no events at all!)
            model.resources.modify(grain, [&model](nmos::resource& grain)
            {
                auto& events = nmos::fields::message_grain_data(grain.data);

                // the node behaviour subscription version, resource_path and params are currently fixed (see make_node_behaviour_subscription)
                events = make_resource_events(model.resources, nmos::is04_versions::v1_2, U(""), web::json::value::object());

                grain.updated = strictly_increasing_update(model.resources);
            });

            bool registration_service_error(false);
            bool node_registered(false);

            web::json::value events;

            // background tasks may read/write the above local state by reference
            pplx::cancellation_token_source cancellation_source;
            pplx::task<void> request = pplx::task_from_result();

            tai most_recent_update{};

            for (;;)
            {
                // wait for the thread to be interrupted because there are resource events (or this is the first time through)
                // or because the node was registered successfully
                // or because an error has been encountered with the selected registration service
                // or because the server is being shut down
                condition.wait(lock, [&]{ return shutdown || registration_service_error || node_registered || most_recent_update < grain->updated; });
                if (registration_service_error)
                {
                    pop_registration_service(registration_services);
                    registration_service_error = false;

                    cancellation_source.cancel();
                    // wait without the lock since it is also used by the background tasks
                    details::reverse_lock_guard<nmos::write_lock> unlock{ lock };
                    request.wait();

                    registration_client.reset();
                    cancellation_source = pplx::cancellation_token_source();
                }
                if (shutdown || registration_services.empty() || node_registered) break;

                // "The Node selects a Registration API to use based on the priority"
                if (!registration_client)
                {
                    const auto base_uri = top_registration_service(registration_services);

                    auto registration_config = config_with_timeout(std::chrono::seconds(nmos::fields::registration_request_max(model.settings)));
                    registration_client.reset(new web::http::client::http_client(base_uri, registration_config));
                }

                events = web::json::value::array();
                node_behaviour_grain_guard guard(model.resources, grain, events);

                while (0 != events.size())
                {
                    if (shutdown || registration_service_error || node_registered) break;

                    const auto id_type = get_resource_event_resource(node_behaviour_topic, events.at(0));
                    const auto event_type = get_resource_event_type(events.at(0));

                    // discard events prior to the node 'added' or 'sync' event (shouldn't generally be necessary?)
                    if (!(nmos::types::node == id_type.second && (resource_added_event == event_type || resource_unchanged_event == event_type)))
                    {
                        events.erase(0);
                        continue;
                    }

                    self_id = id_type.first;

                    slog::log<slog::severities::info>(gate, SLOG_FLF) << "Registering nmos-cpp node with the Registration API at: " << registration_client->base_uri().host() << ":" << registration_client->base_uri().port();

                    auto token = cancellation_source.get_token();
                    request = details::request_registration(*registration_client, events.at(0), gate, token).then([&](pplx::task<void> finally)
                    {
                        nmos::write_lock lock(mutex); // in order to update local state

                        try
                        {
                            finally.get();

                            // on success (or an ignored failure), discard the resource event
                            if (0 != events.size())
                            {
                                events.erase(0);
                            }

                            // subsequent events are handled in registered operation
                            node_registered = true;
                        }
                        catch (const web::http::http_exception& e)
                        {
                            slog::log<slog::severities::error>(gate, SLOG_FLF) << "Registration request HTTP error: " << e.what() << " [" << e.error_code() << "]";

                            registration_service_error = true;
                        }
                        catch (const registration_service_exception&)
                        {
                            registration_service_error = true;
                        }
                    });
                    // avoid race condition between condition.notify_all() and request.is_done()
                    request.then([&]
                    {
                        condition.notify_all();
                    });

                    // wait for the request because interactions with the Registration API /resource endpoint must be sequential
                    condition.wait(lock, [&]{ return shutdown || registration_service_error || node_registered || request.is_done(); });
                }

                most_recent_update = grain->updated;
            }

            cancellation_source.cancel();
            // wait without the lock since it is also used by the background tasks
            details::reverse_lock_guard<nmos::write_lock> unlock{ lock };
            request.wait();
        }

        void registered_operation(const nmos::id& self_id, nmos::model& model, const nmos::id& grain_id, const bool& shutdown, nmos::mutex& mutex, nmos::condition_variable& condition, std::multimap<service_priority, web::uri>& registration_services, slog::base_gate& gate)
        {
            slog::log<slog::severities::info>(gate, SLOG_FLF) << "Adopting registered operation";

            nmos::write_lock lock(mutex);

            const auto grain = nmos::find_resource(model.resources, { grain_id, nmos::types::grain });
            if (model.resources.end() == grain) return;

            std::unique_ptr<web::http::client::http_client> registration_client;
            std::unique_ptr<web::http::client::http_client> heartbeat_client;

            bool registration_service_error(false);
            bool node_registered(false);
            bool node_unregistered(false);

            web::json::value events;

            std::chrono::system_clock::time_point heartbeat_time;

            // background tasks may read/write the above local state by reference
            pplx::cancellation_token_source cancellation_source;
            pplx::task<void> request = pplx::task_from_result();
            pplx::task<void> heartbeats = pplx::task_from_result();

            // "7. The Node registers its other resources (from /devices, /sources etc) with the Registration API."

            tai most_recent_update{};

            for (;;)
            {
                // wait for the thread to be interrupted because there are resource events (or this is the first time through)
                // or because the node was unregistered (cleanly, or as a result of missed heartbeats)
                // or because an error has been encountered with the selected registration service
                // or because the server is being shut down
                condition.wait(lock, [&]{ return shutdown || registration_service_error || node_unregistered || most_recent_update < grain->updated; });
                if (registration_service_error)
                {
                    pop_registration_service(registration_services);
                    registration_service_error = false;

                    cancellation_source.cancel();
                    // wait without the lock since it is also used by the background tasks
                    details::reverse_lock_guard<nmos::write_lock> unlock{ lock };
                    request.wait();
                    heartbeats.wait();

                    registration_client.reset();
                    heartbeat_client.reset();
                    cancellation_source = pplx::cancellation_token_source();
                }
                if (shutdown || registration_services.empty() || node_unregistered) break;

                // "The Node selects a Registration API to use based on the priority"
                if (!registration_client)
                {
                    const auto base_uri = top_registration_service(registration_services);

                    auto registration_config = config_with_timeout(std::chrono::seconds(nmos::fields::registration_request_max(model.settings)));
                    registration_client.reset(new web::http::client::http_client(base_uri, registration_config));

                    auto heartbeat_config = config_with_timeout(std::chrono::seconds(nmos::fields::registration_heartbeat_max(model.settings)));
                    heartbeat_client.reset(new web::http::client::http_client(base_uri, heartbeat_config));

                    // "The first interaction with a new Registration API [after a server side or connectivity issue]
                    // should be a heartbeat to confirm whether whether the Node is still present in the registry"

                    const std::chrono::seconds heartbeat_interval(nmos::fields::registration_heartbeat_interval(model.settings));
                    auto token = cancellation_source.get_token();
                    heartbeat_time = std::chrono::system_clock::now();
                    heartbeats = update_node_health(*heartbeat_client, self_id, gate, token).then([&](bool success)
                    {
                        nmos::write_lock lock(mutex); // in order to update local state

                        node_registered = success;
                        if (!node_registered)
                        {
                            node_unregistered = true;
                        }

                        condition.notify_all();
                    }).then([=, &heartbeat_time, &heartbeat_client, &gate]
                    {
                        // "6. The Node persists itself in the registry by issuing heartbeats."

                        return pplx::do_while([=, &heartbeat_time, &heartbeat_client, &gate]
                        {
                            return pplx::complete_at(heartbeat_time + heartbeat_interval, token).then([=, &heartbeat_time, &heartbeat_client, &gate]() mutable
                            {
                                heartbeat_time = std::chrono::system_clock::now();
                                return update_node_health(*heartbeat_client, self_id, gate, token);
                            });
                        }, token);
                    }).then([&](pplx::task<void> finally)
                    {
                        nmos::write_lock lock(mutex); // in order to update local state

                        try
                        {
                            finally.get();

                            node_unregistered = true;
                        }
                        catch (const web::http::http_exception& e)
                        {
                            slog::log<slog::severities::error>(gate, SLOG_FLF) << "Registration heartbeat HTTP error: " << e.what() << " [" << e.error_code() << "]";

                            registration_service_error = true;
                        }
                        catch (const registration_service_exception&)
                        {
                            registration_service_error = true;
                        }

                        condition.notify_all();
                    });

                    // wait for the response from the first heartbeat that the Node is still registered (or not!)
                    condition.wait(lock, [&]{ return shutdown || registration_service_error || node_unregistered || node_registered; });
                    if (shutdown || registration_service_error || node_unregistered) continue;
                }

                events = web::json::value::array();
                node_behaviour_grain_guard guard(model.resources, grain, events);
                most_recent_update = grain->updated;

                while (0 != events.size())
                {
                    if (shutdown || registration_service_error || node_unregistered) break;

                    const auto id_type = get_resource_event_resource(node_behaviour_topic, events.at(0));
                    const auto event_type = get_resource_event_type(events.at(0));

                    auto token = cancellation_source.get_token();
                    request = details::request_registration(*registration_client, events.at(0), gate, token).then([&](pplx::task<void> finally)
                    {
                        nmos::write_lock lock(mutex); // in order to update state variables

                        try
                        {
                            finally.get();

                            // on success (or an ignored failure), discard the resource event
                            if (0 != events.size())
                            {
                                events.erase(0);
                            }

                            // "Following deletion of all other resources, the Node resource may be deleted and heartbeating stopped."
                            // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/docs/4.1.%20Behaviour%20-%20Registration.md#controlled-unregistration
                            if (self_id == id_type.first && resource_removed_event == event_type)
                            {
                                node_unregistered = true;
                            }
                        }
                        catch (const web::http::http_exception& e)
                        {
                            slog::log<slog::severities::error>(gate, SLOG_FLF) << "Registration request HTTP error: " << e.what() << " [" << e.error_code() << "]";

                            registration_service_error = true;
                        }
                        catch (const registration_service_exception&)
                        {
                            registration_service_error = true;
                        }
                    });
                    // avoid race condition between condition.notify_all() and request.is_done()
                    request.then([&]
                    {
                        condition.notify_all();
                    });

                    // wait for the request because interactions with the Registration API /resource endpoint must be sequential
                    condition.wait(lock, [&]{ return shutdown || registration_service_error || node_unregistered || request.is_done(); });
                }
            }

            cancellation_source.cancel();
            // wait without the lock since it is also used by the background tasks
            details::reverse_lock_guard<nmos::write_lock> unlock{ lock };
            request.wait();
            heartbeats.wait();
        }
    }

    // peer to peer operation
    namespace details
    {
        void update_resource_version(nmos::api_resource_versions& ver, const nmos::type& type)
        {
            if (nmos::types::node == type) ++ver.self;
            if (nmos::types::source == type) ++ver.sources;
            if (nmos::types::flow == type) ++ver.flows;
            if (nmos::types::device == type) ++ver.devices;
            if (nmos::types::sender == type) ++ver.senders;
            if (nmos::types::receiver == type) ++ver.receivers;
            // otherwise, error...
        }

        // "When a Node is unable to locate or successfully register with a Registration API it MUST additionally advertise the following mDNS TXT records as part of its Node advertisement."
        void update_node_service(mdns::service_advertiser& advertiser, const nmos::settings& settings, const nmos::api_resource_versions& ver)
        {
            const auto pri = nmos::fields::pri(settings);
            if (nmos::service_priorities::no_priority != pri)
            {
                auto records = nmos::make_txt_records(nmos::service_types::node);
                auto ver_records = nmos::make_ver_records(ver);
                records.insert(records.end(), std::make_move_iterator(ver_records.begin()), std::make_move_iterator(ver_records.end()));
                nmos::experimental::update_service(advertiser, nmos::service_types::node, settings, records);
            }
        }

        // "If a Node is successfully registered with a Registration API it MUST withdraw advertisements of these TXT records."
        void update_node_service(mdns::service_advertiser& advertiser, const nmos::settings& settings)
        {
            const auto pri = nmos::fields::pri(settings);
            if (nmos::service_priorities::no_priority != pri)
            {
                auto records = nmos::make_txt_records(nmos::service_types::node);
                nmos::experimental::update_service(advertiser, nmos::service_types::node, settings, records);
            }
        }

        void peer_to_peer_operation(nmos::model& model, const nmos::id& grain_id, const bool& shutdown, nmos::mutex& mutex, nmos::condition_variable& condition, std::multimap<service_priority, web::uri>& registration_services, mdns::service_discovery& discovery, mdns::service_advertiser& advertiser, slog::base_gate& gate)
        {
            slog::log<slog::severities::info>(gate, SLOG_FLF) << "Adopting peer-to-peer operation";

            nmos::write_lock lock(mutex);

            const auto grain = nmos::find_resource(model.resources, { grain_id, nmos::types::grain });
            if (model.resources.end() == grain) return;

            api_resource_versions ver;
            update_node_service(advertiser, model.settings, ver);

            // intermittently attempting discovery of a Registration API while in peer-to-peer mode seems like a good idea?
            bool registration_services_discovered(false);

            auto discovery_time = std::chrono::system_clock::now();
            const std::string browse_domain = utility::us2s(nmos::fields::domain(model.settings));
            const std::pair<nmos::service_priority, nmos::service_priority> priorities(nmos::fields::highest_pri(model.settings), nmos::fields::lowest_pri(model.settings));
            const web::uri fallback_registration_service(get_registration_service(model.settings));
            const std::chrono::seconds discovery_interval(nmos::fields::discovery_backoff_max(model.settings));

            // background tasks may read/write the above local state by reference
            pplx::cancellation_token_source cancellation_source;
            auto token = cancellation_source.get_token();
            pplx::task<void> background_discovery = pplx::do_while([&]
            {
                return pplx::complete_at(discovery_time + discovery_interval, token).then([&]
                {
                    discovery_time = std::chrono::system_clock::now();
                    registration_services = discover_registration_services(discovery, browse_domain, priorities, fallback_registration_service, discovery_interval, gate);
                    return registration_services.empty();
                });
            }, token).then([&]
            {
                nmos::write_lock lock(mutex); // in order to update local state

                registration_services_discovered = !registration_services.empty();

                condition.notify_all();
            });

            tai most_recent_update{};

            for (;;)
            {
                // wait for the thread to be interrupted because there are resource events (or this is the first time through)
                // or because a Registration API has been discovered so registered operation should be attempted
                // or because the server is being shut down
                condition.wait(lock, [&]{ return shutdown || registration_services_discovered || most_recent_update < grain->updated; });
                if (shutdown || registration_services_discovered) break;

                {
                    auto events = web::json::value::array();
                    node_behaviour_grain_guard guard(model.resources, grain, events);

                    // update the 'ver_' TXT records, without the lock on the resources
                    details::reverse_lock_guard<nmos::write_lock> unlock{ lock };

                    for (const auto& event : events.as_array())
                    {
                        const auto id_type = get_resource_event_resource(node_behaviour_topic, event);
                        update_resource_version(ver, id_type.second);
                    }

                    update_node_service(advertiser, model.settings, ver);

                    // job done
                    events = web::json::value::array();
                }

                most_recent_update = grain->updated;
            }

            // withdraw the 'ver_' TXT records
            update_node_service(advertiser, model.settings);

            cancellation_source.cancel();
            // wait without the lock since it is also used by the background tasks
            details::reverse_lock_guard<nmos::write_lock> unlock{ lock };
            background_discovery.wait();
        }
    }
}
