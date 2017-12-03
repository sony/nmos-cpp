#include "nmos/filesystem_route.h"

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
        }

        web::http::experimental::listener::api_router make_filesystem_route(const utility::string_t& filesystem_root, const relative_path_content_type_validator& validate, slog::base_gate& gate)
        {
            using namespace web::http::experimental::listener::api_router_using_declarations;

            api_router filesystem_route;

            filesystem_route.support(U("(?<filesystem-relative-path>/.+)"), web::http::methods::GET, [filesystem_root, validate, &gate](http_request req, http_response res, const string_t&, const route_parameters& parameters)
            {
                slog::log<slog::severities::more_info>(gate, SLOG_FLF) << nmos::api_stash(req, parameters) << "Filesystem request received";
                const auto relative_path = web::uri::decode(parameters.at(U("filesystem-relative-path")));
                const bool naughty = string_t::npos != relative_path.find(U("/.."));
                const auto content_type = validate(relative_path);
                if (naughty || content_type.empty())
                {
                    slog::log<slog::severities::error>(gate, SLOG_FLF) << nmos::api_stash(req, parameters) << "Unexpected request forbidden";
                    set_reply(res, status_codes::Forbidden);
                }
                else
                {
                    const auto filesystem_path = filesystem_root + relative_path;

                    if (bst::filesystem::exists(details::native_path(filesystem_path)))
                    {
                        const utility::size64_t content_length = bst::filesystem::file_size(details::native_path(filesystem_path));
                        return concurrency::streams::fstream::open_istream(filesystem_path, std::ios::in).then([res, content_length, content_type](concurrency::streams::istream is) mutable
                        {
                            set_reply(res, status_codes::OK, is, content_length, content_type);
                            return true;
                        });
                    }
                    else
                    {
                        // unless requests are for files of a supported file type that are actually found in the filesystem, let's just report Forbidden
                        slog::log<slog::severities::error>(gate, SLOG_FLF) << nmos::api_stash(req, parameters) << "Unexpected request forbidden";
                        set_reply(res, status_codes::Forbidden);
                    }
                }

                return pplx::task_from_result(true);
            });

            return filesystem_route;
        }
    }
}
