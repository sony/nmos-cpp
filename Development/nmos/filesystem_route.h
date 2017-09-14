#ifndef NMOS_FILESYSTEM_ROUTE_H
#define NMOS_FILESYSTEM_ROUTE_H

#include <map>
#include "cpprest/api_router.h"

namespace slog
{
    class base_gate;
}

// This is a helper to simplify usage of api_router to serve local filesystem files
namespace nmos
{
    namespace experimental
    {
        // function from relative path to content type (or empty string if path is invalid for any reason)
        typedef std::function<utility::string_t(const utility::string_t&)> relative_path_content_type_validator;

        // map from file extensions to content types (e.g. from "html" to "text/html")
        typedef std::map<utility::string_t, utility::string_t> content_types;

        namespace details
        {
            inline utility::string_t extension(const utility::string_t& pathname)
            {
                utility::string_t::size_type dot = pathname.find_last_of(U('.')) + 1;
                utility::string_t::size_type slash = pathname.find_last_of(U("\\/")) + 1;
                return slash < dot ? pathname.substr(dot) : utility::string_t();
            }
        }

        // A reasonable default validator is to reject unexpected file types (another option would be to return "application/octet-stream")
        inline relative_path_content_type_validator make_relative_path_content_type_validator(const content_types& valid_extensions)
        {
            return [valid_extensions](const utility::string_t& relative_path)
            {
                const auto found = valid_extensions.find(details::extension(relative_path));
                return valid_extensions.end() != found ? found->second : U("");
            };
        }

        web::http::experimental::listener::api_router make_filesystem_route(const utility::string_t& filesystem_root, const relative_path_content_type_validator& validate, slog::base_gate& gate);
    }
}

#endif
