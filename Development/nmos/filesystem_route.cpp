#include "nmos/filesystem_route.h"

#include <boost/algorithm/string/erase.hpp>
#include "bst/filesystem.h"

#include "cpprest/filestream.h"
#include "nmos/slog.h"

namespace nmos
{
    namespace experimental
    {
        namespace details
        {
#if !defined(BST_FILESYSTEM_MICROSOFT_TR2)
            using native_path = bst_filesystem::path;
#else
            using native_path = bst_filesystem::wpath;
#endif

            // could now be replaced with native_path::extension
            inline utility::string_t extension(const utility::string_t& pathname)
            {
                utility::string_t::size_type dot = pathname.find_last_of(U('.')) + 1;
                utility::string_t::size_type slash = pathname.find_last_of(U("\\/")) + 1;
                return slash < dot ? pathname.substr(dot) : utility::string_t();
            }
        }

        // determines content type based only on file extension, and rejects unexpected file types
        // (another option would be to return "application/octet-stream")
        relative_path_content_type_handler make_relative_path_content_type_handler(const content_types& valid_extensions)
        {
            return [valid_extensions](const utility::string_t& relative_path)
            {
                const auto found = valid_extensions.find(details::extension(relative_path));
                return valid_extensions.end() != found ? found->second : U("");
            };
        }

        // if the path ends with a slash, tries the specified index files in turn and returns the first one found (or empty string if path is invalid for any reason)
        // otherwise returns an external redirect with a trailing slash
        directory_index_redirect_handler make_directory_index_redirect_handler(const utility::string_t& filesystem_root, const directory_index_files& valid_index_files, bool external_redirect)
        {
            return [=](const utility::string_t& relative_path) -> std::pair<utility::string_t, bool>
            {
                if (relative_path.empty() || relative_path.back() != U('/')) return{ relative_path + U('/'), true };

                const auto filesystem_path = filesystem_root + relative_path;
                for (const auto& index : valid_index_files)
                {
                    if (bst::filesystem::is_regular_file(details::native_path(filesystem_path + index)))
                    {
                        return{ relative_path + index, external_redirect };
                    }
                }
                return{};
            };
        }

        web::http::experimental::listener::api_router make_filesystem_route(const utility::string_t& filesystem_root, const relative_path_content_type_handler& content_type_handler, const directory_index_redirect_handler& index_redirect_handler, slog::base_gate& gate_)
        {
            using namespace web::http::experimental::listener::api_router_using_declarations;

            api_router filesystem_route;

            filesystem_route.support(U("(?<filesystem-relative-path>/.*)"), web::http::methods::GET, [filesystem_root, content_type_handler, index_redirect_handler, &gate_](http_request req, http_response res, const string_t&, const route_parameters& parameters)
            {
                nmos::api_gate gate(gate_, req, parameters);
                slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "Filesystem request received";
                auto relative_path = web::uri::decode(parameters.at(U("filesystem-relative-path")));
                const bool naughty = string_t::npos != relative_path.find(U("/.."));
                if (!naughty)
                {
                    // relative path will begin with a slash
                    auto filesystem_path = filesystem_root + relative_path;

                    if (bst::filesystem::is_directory(details::native_path(filesystem_path)))
                    {
                        const auto index_redirect = index_redirect_handler(relative_path);
                        if (!index_redirect.first.empty())
                        {
                            if (index_redirect.second)
                            {
                                set_reply(res, status_codes::TemporaryRedirect); // or status_codes::MovedPermanently?
                                res.headers().add(web::http::header_names::location, boost::algorithm::erase_tail_copy(req.request_uri().path(), (int)relative_path.size()) + index_redirect.first);

                                return pplx::task_from_result(true);
                            }

                            relative_path = index_redirect.first;
                            filesystem_path = filesystem_root + relative_path;
                        }
                    }

                    if (bst::filesystem::is_regular_file(details::native_path(filesystem_path)))
                    {
                        const auto content_type = content_type_handler(relative_path);

                        if (!content_type.empty())
                        {
                            const utility::size64_t content_length = bst::filesystem::file_size(details::native_path(filesystem_path));
                            return concurrency::streams::fstream::open_istream(filesystem_path, std::ios::in).then([res, content_length, content_type](concurrency::streams::istream is) mutable
                            {
                                set_reply(res, status_codes::OK, is, content_length, content_type);
                                return true;
                            });
                        }
                    }
                }

                // unless requests are for files of a supported file type that are actually found in the filesystem, just report not found
                set_reply(res, status_codes::NotFound);

                return pplx::task_from_result(true);
            });

            return filesystem_route;
        }
    }
}
