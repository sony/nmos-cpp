#ifndef NMOS_SLOG_H
#define NMOS_SLOG_H

#include "cpprest/basic_utils.h"
#include "cpprest/api_router.h" // for web::http::experimental::listener::route_parameters
#include "cpprest/logging_utils.h"
#include "slog/all_in_one.h"

// Enable logging of cpprestsdk's utility::string_t (which is a wide string on Windows)
inline slog::log_statement& operator<<(slog::log_statement& s, const utility::string_t& us)
{
    return s << utility::us2s(us);
}
// hmmm, this shouldn't be necessary?
inline const slog::nolog_statement& operator<<(const slog::nolog_statement& s, const utility::string_t&)
{
    return s;
}

namespace nmos
{
#define DEFINE_STASH_FUNCTIONS(name, stash_type) \
    namespace detail { struct name##_tag : slog::stash_tag<stash_type> {}; } \
    inline slog::ios_stasher<detail::name##_tag> stash_##name(const detail::name##_tag::type& name) \
    { \
        return slog::ios_stasher<detail::name##_tag>(name); \
    } \
    inline detail::name##_tag::type get_##name##_stash(const std::ostream& os) \
    { \
        return slog::get_stash<detail::name##_tag>(os, detail::name##_tag::type{}); \
    }

    DEFINE_STASH_FUNCTIONS(http_method, web::http::method)
    DEFINE_STASH_FUNCTIONS(request_uri, web::uri)
    DEFINE_STASH_FUNCTIONS(route_parameters, web::http::experimental::listener::route_parameters)
    DEFINE_STASH_FUNCTIONS(status_code_tag, web::http::status_code)
#undef DEFINE_STASH_FUNCTIONS

    inline slog::omanip_function api_stash(const web::http::http_request& req, const web::http::experimental::listener::route_parameters& parameters)
    {
        return slog::omanip([&](std::ostream& os)
        {
            os << stash_http_method(req.method()) << stash_request_uri(req.request_uri()) << stash_route_parameters(parameters);
        });
    }

    namespace detail
    {
        inline slog::severity severity_from_level(web::logging::experimental::level level)
        {
            switch (level)
            {
            case web::logging::experimental::levels::devel:     return slog::severities::too_much_info;
            case web::logging::experimental::levels::verbose:   return slog::severities::more_info;
            case web::logging::experimental::levels::info:      return slog::severities::info;
            case web::logging::experimental::levels::warning:   return slog::severities::warning;
            case web::logging::experimental::levels::error:     return slog::severities::error;
            case web::logging::experimental::levels::severe:    return slog::severities::severe;
            default:                                            return slog::severities::error;
            }
        }
    }

    // Adapting web::logging::experimental::callback_function to slog isn't able to provide compile-time filtering or capture source location unfortunately

    SLOG_DETAIL_BEGIN_ANONYMOUS_NAMESPACE_IF_CONFIG_PER_TRANSLATION_UNIT

    inline web::logging::experimental::callback_function make_slog_logging_callback(slog::base_gate& gate)
    {
        return [&gate](web::logging::experimental::level level_, const std::string& message, const std::string& category)
        {
            const slog::severity level = detail::severity_from_level(level_);
            // run-time equivalent of slog::detail::select_pertinent
            if ((SLOG_LOGGING_SEVERITY) <= level)
            {
                // category could be stashed instead?
                slog::detail::logw<slog::log_statement, slog::base_gate>(gate, level, "", 0, "") << (category.empty() ? category : category + ": ") << message;
            }
            // run-time equivalent of slog::detail::select_terminate
            if ((SLOG_TERMINATING_SEVERITY) <= level)
            {
                SLOG_DETAIL_DO_TERMINATE();
            }
        };
    }

    SLOG_DETAIL_END_ANONYMOUS_NAMESPACE_IF_CONFIG_PER_TRANSLATION_UNIT
}

#endif
