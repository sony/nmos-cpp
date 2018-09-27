#include "nmos/node_api_target_handler.h"

#include <boost/algorithm/string/trim.hpp>
#include "cpprest/http_client.h"
#include "nmos/slog.h"

namespace nmos
{
    node_api_target_handler make_node_api_target_handler(nmos::resources& resources, nmos::mutex& mutex, nmos::condition_variable& condition, connect_function connect, slog::base_gate& gate)
    {
        return [&, connect](const nmos::id& receiver_id, const web::json::value& sender_data)
        {
            using web::json::value;
            using web::json::value_of;

            if (sender_data.size() != 0)
            {
                // get sdp from sender, and then use this to connect

                // hmm, should add json validation of sender data

                const auto sender_id = nmos::fields::id(sender_data);
                const auto manifest_href = nmos::fields::manifest_href(sender_data);

                web::http::client::http_client client(manifest_href);
                return client.request(web::http::methods::GET).then([&gate](web::http::http_response res)
                {
                    if (res.status_code() != web::http::status_codes::OK)
                    {
                        throw web::http::http_exception(U("no sender sdp retrieved"));
                    }

                    // extract_string doesn't know about "application/sdp"
                    auto content_type = res.headers().content_type();
                    auto semicolon = content_type.find(U(';'));
                    if (utility::string_t::npos != semicolon) content_type.erase(semicolon);
                    boost::algorithm::trim(content_type);

                    if (content_type.empty())
                    {
                        slog::log<slog::severities::warning>(gate, SLOG_FLF) << "Missing Content-Type: should be application/sdp";
                    }
                    else if (U("application/sdp") != content_type)
                    {
                        throw web::http::http_exception(U("Incorrect Content-Type: ") + content_type + U(", should be application/sdp"));
                    }

                    return res.extract_string(true);
                }).then([connect, receiver_id](const utility::string_t& sdp)
                {
                    return connect(receiver_id, sdp);
                }).then([&, receiver_id, sender_id]
                {
                    nmos::write_lock lock(mutex);

                    modify_resource(resources, receiver_id, [&sender_id](nmos::resource& resource)
                    {
                        resource.data[nmos::fields::version] = web::json::value::string(nmos::make_version());
                        nmos::fields::subscription(resource.data) = value_of({
                            { nmos::fields::active, true },
                            { nmos::fields::sender_id, sender_id }
                        });
                    });

                    // notify anyone who cares...
                    condition.notify_all();
                });
            }
            else
            {
                // no sender data means disconnect

                return connect(receiver_id, U("")).then([&, receiver_id]
                {
                    nmos::write_lock lock(mutex);

                    modify_resource(resources, receiver_id, [](nmos::resource& resource)
                    {
                        resource.data[nmos::fields::version] = web::json::value::string(nmos::make_version());
                        nmos::fields::subscription(resource.data) = value_of({
                            { nmos::fields::active, false },
                            { nmos::fields::sender_id, {} }
                        });
                    });

                    // notify anyone who cares...
                    condition.notify_all();
                });
            }
        };
    }
}
