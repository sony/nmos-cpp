#include "nmos/node_registration.h"

#include "cpprest/http_client.h"
#include "nmos/api_downgrade.h"
#include "nmos/api_utils.h" // for nmos::type_from_resourceType
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
        nmos::resource make_node_registration_subscription(const nmos::id& id)
        {
            using web::json::value;
            value data;
            data[nmos::fields::id] = value::string(id);
            data[nmos::fields::max_update_rate_ms] = 0; // add a setting for this?
            data[nmos::fields::persist] = false; // not to be deleted by someone else
            data[nmos::fields::resource_path] = value::string(U(""));
            data[nmos::fields::params] = value::object();
            // generate a websocket url?
            return{ nmos::is04_versions::v1_2, nmos::types::subscription, data, true };
        }

        nmos::resource make_node_registration_grain(const nmos::id& id, const nmos::id& subscription_id, const nmos::resources& resources)
        {
            using web::json::value;
            value data;
            data[nmos::fields::id] = value::string(id);
            data[nmos::fields::subscription_id] = value::string(subscription_id);
            data[nmos::fields::message] = details::make_grain(nmos::make_id(), subscription_id, U("/"));
            nmos::fields::message_grain_data(data) = make_resource_events(resources, nmos::is04_versions::v1_2, U(""), value::object());
            return{ nmos::is04_versions::v1_2, nmos::types::grain, data, true };
        }

        web::uri make_registration_uri_with_no_version(const nmos::settings& settings)
        {
            // scheme, host and port should come from the mDNS record for the registry's Registration API (via settings)
            // version should be the the highest version supported by both this node and the registry
            return web::uri_builder()
                .set_scheme(U("http"))
                .set_host(nmos::fields::registry_address(settings))
                .set_port(nmos::fields::registration_port(settings))
                .set_path(U("/x-nmos/registration/"))
                .to_uri();
        }

        // make a POST or DELETE request on the Registration API specified by the client for the specified resource event
        // this could be made asynchronous, returning pplx::task<web::http::http_response>
        // however, the logging gateway would need to be passed out of scope to a continuation to perform the response logging
        void request_registration(web::http::client::http_client& client, const nmos::api_version& registry_version, const web::json::value& event, slog::base_gate& gate)
        {
            const auto& path = event.at(U("path")).as_string();
            const auto& data = event.at(U("post"));

            auto slash = path.find('/'); // assert utility::string_t::npos != slash
            nmos::type type = nmos::type_from_resourceType(path.substr(0, slash));
            nmos::id id = path.substr(slash + 1);

            if (!data.is_null())
            {
                // 'create' or 'update'
                const bool creation = event.at(U("pre")) == data;

                slog::log<slog::severities::info>(gate, SLOG_FLF) << "Requesting registration " << (creation ? "creation" : "update") << " for " << type.name << ": " << id;

                // a downgrade is required if the registry version is lower than this resource's version
                auto body = web::json::value_of(
                {
                    { U("type"), web::json::value::string(type.name) },
                    { U("data"), nmos::downgrade(nmos::is04_versions::v1_2, type, data, registry_version, registry_version) }
                });

                // block and wait for the response

                // No trailing slash on the URL
                // See https://github.com/AMWA-TV/nmos-discovery-registration/issues/15
                auto response = client.request(web::http::methods::POST, make_api_version(registry_version) + U("/resource"), body).get();

                if (web::http::status_codes::OK == response.status_code())
                    slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "Registration updated for " << type.name << ": " << id;
                else if (web::http::status_codes::Created == response.status_code())
                    slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "Registration created for " << type.name << ": " << id;
                else if (web::http::is_error_status_code(response.status_code()))
                    slog::log<slog::severities::error>(gate, SLOG_FLF) << "Registration " << (creation ? "creation" : "update") << " rejected for " << type.name << ": " << id;
            }
            else
            {
                // 'delete'

                slog::log<slog::severities::info>(gate, SLOG_FLF) << "Requesting registration deletion for " << type.name << ": " << id;

                // block and wait for the response

                auto response = client.request(web::http::methods::DEL, make_api_version(registry_version) + U("/resource/") + path).get();

                if (web::http::status_codes::NoContent == response.status_code())
                    slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "Registration deleted for " << type.name << ": " << id;
                else if (web::http::is_error_status_code(response.status_code()))
                    slog::log<slog::severities::error>(gate, SLOG_FLF) << "Registration deletion rejected for " << type.name << ": " << id;
            }
        }

        void update_node_health(web::http::client::http_client& client, const nmos::api_version& registry_version, const nmos::id& id, slog::base_gate& gate)
        {
            slog::log<slog::severities::too_much_info>(gate, SLOG_FLF) << "Posting heartbeat for node " << id;

            // block and wait for the response

            auto response = client.request(web::http::methods::POST, make_api_version(registry_version) + U("/health/nodes/") + id).get();

            // Check response to see if re-registration is required!
            if (web::http::status_codes::NotFound == response.status_code())
                slog::log<slog::severities::error>(gate, SLOG_FLF) << "Registration not found for node: " << id;
        }
    }

    void node_registration_thread(nmos::model& model, nmos::mutex& mutex, nmos::condition_variable& condition, bool& shutdown, slog::base_gate& gate)
    {
        using utility::string_t;
        using web::json::value;

        // could start out as a shared/read lock, only upgraded to an exclusive/write lock when a grain in the resources is actually modified
        nmos::write_lock lock(mutex);

        const auto base_uri = details::make_registration_uri_with_no_version(model.settings);
        const auto registry_version = nmos::parse_api_version(nmos::fields::registry_version(model.settings));
        web::http::client::http_client client(base_uri);

        tai most_recent_message{};
        auto earliest_necessary_update = (tai_clock::time_point::max)();

        auto subscription_id = nmos::make_id();
        auto grain_id = nmos::make_id();
        const auto subscription = insert_resource(model.resources, details::make_node_registration_subscription(subscription_id)).first;
        const auto grain = insert_resource(model.resources, details::make_node_registration_grain(grain_id, subscription_id, model.resources)).first;

        for (;;)
        {
            // wait for the thread to be interrupted either because there are resource changes, or because the server is being shut down
            // or because message sending was throttled earlier
            details::wait_until(condition, lock, earliest_necessary_update, [&]{ return shutdown || most_recent_message < most_recent_update(model.resources); });
            if (shutdown) break;
            most_recent_message = most_recent_update(model.resources);

            slog::log<slog::severities::too_much_info>(gate, SLOG_FLF) << "Got notification on node registration thread";

            const auto now = tai_clock::now();

            earliest_necessary_update = (tai_clock::time_point::max)();

            // if the grain has events to send
            if (0 == nmos::fields::message_grain_data(grain->data).size()) continue;

            // throttle messages according to the subscription's max_update_rate_ms
            const auto max_update_rate = std::chrono::milliseconds(nmos::fields::max_update_rate_ms(subscription->data));
            const auto earliest_allowed_update = time_point_from_tai(details::get_grain_timestamp(nmos::fields::message(grain->data))) + max_update_rate;
            if (earliest_allowed_update > now)
            {
                // make sure to send a message as soon as allowed
                if (earliest_allowed_update < earliest_necessary_update)
                {
                    earliest_necessary_update = earliest_allowed_update;
                }
                // just don't do it now!
                continue;
            }

            slog::log<slog::severities::info>(gate, SLOG_FLF) << "Sending " << nmos::fields::message_grain_data(grain->data).size() << " changes to the Registration API";

            value events = value::array();

            // set the grain timestamp
            // steal the events
            // reset the grain for next time
            model.resources.modify(grain, [&most_recent_message, &events, &model](nmos::resource& grain)
            {
                details::set_grain_timestamp(nmos::fields::message(grain.data), most_recent_message);
                using std::swap;
                swap(events, nmos::fields::message_grain_data(grain.data));
                grain.updated = strictly_increasing_update(model.resources);
            });

            // this would be the place to handle e.g. the registration uri in the settings having changed...

            // issue the registration requests, without the lock on the resources and settings
            details::reverse_lock_guard<nmos::write_lock> unlock{ lock };

            for (auto& event : events.as_array())
            {
                // need to implement specified handling of error conditions
                // see https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2.x/docs/4.1.%20Behaviour%20-%20Registration.md#error-conditions
                // for the moment, just log http exceptions...
                try
                {
                    details::request_registration(client, registry_version, event, gate);

                    const auto& path = event.at(U("path")).as_string();
                    auto slash = path.find('/'); // assert utility::string_t::npos != slash
                    nmos::type type = nmos::type_from_resourceType(path.substr(0, slash));
                    nmos::id id = path.substr(slash + 1);

                    if (nmos::types::node == type)
                    {
                        // set the health of the node, to trigger the heartbeat thread
                        nmos::read_lock lock(mutex);
                        set_resource_health(model.resources, id);
                    }
                }
                catch (const web::http::http_exception& e)
                {
                    slog::log<slog::severities::error>(gate, SLOG_FLF) << e.what() << " [" << e.error_code() << "]";
                }
            }
        }
    }

    void node_registration_heartbeat_thread(const nmos::model& model, nmos::mutex& mutex, nmos::condition_variable& condition, bool& shutdown, slog::base_gate& gate)
    {
        // since health is mutable, no need to get an exclusive/write lock
        nmos::read_lock lock(mutex);

        const auto base_uri = details::make_registration_uri_with_no_version(model.settings);
        const auto registry_version = nmos::parse_api_version(nmos::fields::registry_version(model.settings));
        web::http::client::http_client client(base_uri);

        auto resource = nmos::find_self_resource(model.resources);
        auto self_health = model.resources.end() == resource || nmos::health_forever == resource->health ? health_now() : resource->health.load();

        // wait until the next node heartbeat, or the server is being shut down
        while (!condition.wait_until(lock, time_point_from_health(self_health + nmos::fields::registration_heartbeat_interval(model.settings)), [&]{ return shutdown; }))
        {
            auto heartbeat_health = health_now() - nmos::fields::registration_heartbeat_interval(model.settings);
            resource = nmos::find_self_resource(model.resources);
            self_health = model.resources.end() == resource || nmos::health_forever == resource->health ? health_now() : resource->health.load();
            if (self_health > heartbeat_health) continue;

            const auto id = resource->id;
            set_resource_health(model.resources, id);

            // this would be the place to handle e.g. the registration uri in the settings having changed...

            // issue the registration heartbeat, without the lock on the resources and settings
            details::reverse_lock_guard<nmos::read_lock> unlock{ lock };

            // need to implement specified handling of error conditions
            // see https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2.x/docs/4.1.%20Behaviour%20-%20Registration.md#error-conditions
            try
            {
                details::update_node_health(client, registry_version, id, gate);
            }
            catch (const web::http::http_exception& e)
            {
                slog::log<slog::severities::error>(gate, SLOG_FLF) << e.what() << " [" << e.error_code() << "]";
            }
        }
    }
}
