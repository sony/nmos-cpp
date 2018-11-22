#ifndef NMOS_SLOG_H
#define NMOS_SLOG_H

#include "cpprest/basic_utils.h"
#include "cpprest/api_router.h" // for web::http::experimental::listener::route_parameters
#include "cpprest/logging_utils.h"
#include "nmos/id.h"
#include "nmos/type.h"
#include "slog/all_in_one.h"

namespace slog
{
    // Enable logging of cpprestsdk's utility::string_t when it's an alias for std::wstring (as on Windows) rather than std::string
    // (defined in namespace slog to ensure slog::nolog_statement::operator<< also finds it via ADL)
    inline log_statement& operator<<(log_statement& s, const utf16string& u16s)
    {
        return s << utility::conversions::to_utf8string(u16s);
    }
}

namespace nmos
{
    // (defined in an associated namespace to ensure slog::nolog_statement::operator<< also finds it via ADL)
    inline slog::log_statement& operator<<(slog::log_statement& s, const std::pair<nmos::id, nmos::type>& id_type)
    {
        return s << id_type.second.name << ": " << id_type.first;
    }

    // Log message categories
    typedef std::string category;

    namespace categories
    {
        const category unknown{};
        const category access{ "access" };
        // other categories may be defined ad-hoc
    }

#define DEFINE_STASH_FUNCTIONS(name, stash_type) \
    namespace details { struct name##_tag : slog::stash_tag<stash_type> {}; } \
    inline slog::ios_stasher<details::name##_tag> stash_##name(const details::name##_tag::type& name) \
    { \
        return slog::ios_stasher<details::name##_tag>(name); \
    } \
    inline details::name##_tag::type get_##name##_stash(const std::ostream& os, const details::name##_tag::type& default_value = stash_type()) \
    { \
        return slog::get_stash<details::name##_tag>(os, default_value); \
    }

    DEFINE_STASH_FUNCTIONS(category, category)
    DEFINE_STASH_FUNCTIONS(remote_address, utility::string_t)
    DEFINE_STASH_FUNCTIONS(http_method, web::http::method)
    DEFINE_STASH_FUNCTIONS(request_uri, web::uri)
    DEFINE_STASH_FUNCTIONS(route_parameters, web::http::experimental::listener::route_parameters)
    DEFINE_STASH_FUNCTIONS(http_version, web::http::http_version)
    DEFINE_STASH_FUNCTIONS(status_code, web::http::status_code)
    DEFINE_STASH_FUNCTIONS(response_length, utility::size64_t)
#undef DEFINE_STASH_FUNCTIONS

    inline slog::omanip_function api_stash(const web::http::http_request& req, const web::http::experimental::listener::route_parameters& parameters)
    {
        return slog::omanip([&](std::ostream& os)
        {
            os << stash_http_method(req.method()) << stash_request_uri(req.request_uri()) << stash_route_parameters(parameters);
        });
    }

    inline utility::string_t make_http_protocol(const web::http::http_version& http_version)
    {
        utility::ostringstream_t result;
        result << U("HTTP/") << (int)http_version.major << U(".") << (int)http_version.minor;
        return result.str();
    }

    inline slog::omanip_function common_log_format(const slog::async_log_message& message)
    {
        return slog::omanip([&](std::ostream& os)
        {
            // Produce a line in the Common Log Format, i.e.
            // remotehost rfc931 authuser [date] "request" status bytes
            // E.g.
            // 127.0.0.1 - - [18/Oct/2017:09:07:36 +0100] "GET /x-nmos/query/v1.2/nodes/ HTTP/1.1" 200 2326
            // (where the two '-' characters indicate the user identifiers are unknown)
            // See https://www.w3.org/Daemon/User/Config/Logging.html#common-logfile-format
            // and https://httpd.apache.org/docs/1.3/logs.html#common

#if !defined(_MSC_VER) || _MSC_VER >= 1900
            static const char* time_format = "%d/%b/%Y:%T %z";
#else
            // %T isn't supported by VS2013, and it also treats %z as %Z; both issues are resolved in VS2015
            static const char* time_format = "%d/%b/%Y:%H:%M:%S";
#endif

            os
                << utility::us2s(get_remote_address_stash(message.stream(), U("-"))) << " "
                << "- "
                << "- "
                << "[" << slog::put_timestamp(message.timestamp(), time_format) << "] "
                << "\"" << utility::us2s(get_http_method_stash(message.stream())) << " "
                << utility::us2s(get_request_uri_stash(message.stream()).to_string()) << " "
                << utility::us2s(make_http_protocol(get_http_version_stash(message.stream()))) << "\" "
                << get_status_code_stash(message.stream()) << " "
                << get_response_length_stash(message.stream()) // output "-" for 0 or unknown?
                << std::endl;
        });
    }

    inline slog::omanip_function common_log_stash(const web::http::http_request& req, const web::http::http_response& res)
    {
        return slog::omanip([&](std::ostream& os)
        {
            // Stash fields for the Common Log Format
            os
                << stash_remote_address(req.remote_address())
                << stash_http_method(req.method())
                << stash_request_uri(req.request_uri())
                << stash_http_version(req.http_version())
                << stash_status_code(res.status_code())
                << stash_response_length(res.headers().content_length());
        });
    }

    namespace details
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

    inline web::logging::experimental::log_handler make_slog_logging_callback(slog::base_gate& gate)
    {
        return [&gate](web::logging::experimental::level level_, const std::string& message, const std::string& category)
        {
            const slog::severity level = details::severity_from_level(level_);
            // run-time equivalent of slog::detail::select_pertinent
            if ((SLOG_LOGGING_SEVERITY) <= level)
            {
                slog::detail::logw<slog::log_statement, slog::base_gate>(gate, level, "", 0, "") << stash_category(category) << message;
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
