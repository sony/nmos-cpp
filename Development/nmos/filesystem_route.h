#ifndef NMOS_FILESYSTEM_ROUTE_H
#define NMOS_FILESYSTEM_ROUTE_H

#include <map>
#include <set>
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
        // the path argument will begin with a slash ('/')
        typedef std::function<utility::string_t(const utility::string_t& relative_path)> relative_path_content_type_handler;

        // map from file extensions to content types (e.g. from "html" to "text/html")
        typedef std::map<utility::string_t, utility::string_t> content_types;

        // determines content type based only on file extension, and rejects unexpected file types
        // (another option would be to return "application/octet-stream")
        relative_path_content_type_handler make_relative_path_content_type_handler(const content_types& valid_extensions);

        // function from directory path to pair of index path (or empty string if path is invalid for any reason) and external redirect flag
        // the path argument will begin with a slash and may end with or without a slash
        typedef std::function<std::pair<utility::string_t, bool>(const utility::string_t& relative_path)> directory_index_redirect_handler;

        // set of index files to look for when the client requests a directory
        typedef std::set<utility::string_t> directory_index_files;

        // if the path ends with a slash, tries the specified index files in turn and returns the first one found (or empty string if path is invalid for any reason)
        // otherwise returns an external redirect with a trailing slash
        directory_index_redirect_handler make_directory_index_redirect_handler(const utility::string_t& filesystem_root, const directory_index_files& valid_index_files = { U("index.html") }, bool external_redirect = false);

        web::http::experimental::listener::api_router make_filesystem_route(const utility::string_t& filesystem_root, const relative_path_content_type_handler& content_type_handler, const directory_index_redirect_handler& index_redirect_handler, slog::base_gate& gate);

        inline web::http::experimental::listener::api_router make_filesystem_route(const utility::string_t& filesystem_root, const relative_path_content_type_handler& content_type_handler, slog::base_gate& gate)
        {
            return make_filesystem_route(filesystem_root, content_type_handler, make_directory_index_redirect_handler(filesystem_root), gate);
        }
    }
}

#endif
