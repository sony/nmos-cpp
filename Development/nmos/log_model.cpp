#include "nmos/log_model.h"

#include <boost/range/adaptor/transformed.hpp>
#include "nmos/api_utils.h"
#include "nmos/query_utils.h"
#include "nmos/slog.h"
#include "rql/rql.h"

namespace nmos
{
    namespace experimental
    {
        namespace details
        {
            template <typename T>
            inline utility::string_t ostringstreamed(const T& value)
            {
                std::ostringstream os; os << value; return utility::s2us(os.str());
            }

            inline web::json::value json_from_message(const slog::async_log_message& message, const id& id)
            {
                auto json_message = web::json::value_of({
                    { U("timestamp"), ostringstreamed(slog::put_timestamp(message.timestamp(), "%Y-%m-%dT%H:%M:%06.3SZ")) },
                    { U("level"), message.level() },
                    { U("level_name"), ostringstreamed(slog::put_severity_name(message.level())) },
                    // hmm, could be good to provide a log/settings option to redact thread_id and/or source_location?
                    { U("thread_id"), ostringstreamed(message.thread_id()) },
                    { U("source_location"), web::json::value_of({
                        { U("file"), utility::s2us(message.file()) },
                        { U("line"), message.line() },
                        { U("function"), utility::s2us(message.function()) }
                    }, true) },
                    { U("message"), utility::s2us(message.str()) },
                    { U("id"), id }
                }, true);

                // a few useful optional properties are only stashed in some log messages
                const auto http_method = nmos::get_http_method_stash(message.stream());
                if (!http_method.empty()) json_message[U("http_method")] = web::json::value::string(http_method);
                const auto request_uri = nmos::get_request_uri_stash(message.stream());
                if (!request_uri.is_empty()) json_message[U("request_uri")] = web::json::value::string(request_uri.to_string());
                const auto route_parameters = nmos::get_route_parameters_stash(message.stream());
                if (!route_parameters.empty()) json_message[U("route_parameters")] = web::json::value_from_fields(route_parameters);
                const auto categories = nmos::get_categories_stash(message.stream());
                if (!categories.empty()) json_message[U("tags")][U("category")] = web::json::value_from_elements(categories | boost::adaptors::transformed(utility::s2us));

                return json_message;
            }
        }

        // logically necessary, practically not!
        inline tai strictly_increasing_cursor(const log_events& events, tai cursor = tai_now())
        {
            const auto most_recent = events.empty() ? tai{} : events.front().cursor;
            return cursor > most_recent ? cursor : tai_from_time_point(time_point_from_tai(most_recent) + tai_clock::duration(1));
        }

        void insert_log_event(log_events& events, const slog::async_log_message& message, const id& id, std::size_t max_size)
        {
            if (0 == max_size)
            {
                events.clear();
                return;
            }

            while (events.size() >= max_size)
            {
                events.pop_back();
            }
            events.push_front({ details::json_from_message(message, id), strictly_increasing_cursor(events) });
        }
    }
}
