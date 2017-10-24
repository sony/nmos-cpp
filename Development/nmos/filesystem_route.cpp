#include "nmos/filesystem_route.h"

#if !defined(_MSC_VER) || _MSC_VER >= 1900
#include <experimental/filesystem>
#else
#include <filesystem>
#endif
#include "cpprest/filestream.h"
#include "nmos/slog.h"

#if !defined(_MSC_VER) || _MSC_VER >= 1900
namespace filesystem = std::experimental::filesystem;
typedef filesystem::path path_t;
#else
namespace filesystem = std::tr2::sys;
typedef filesystem::wpath path_t;
#endif

namespace nmos
{
    namespace experimental
    {
        web::http::experimental::listener::api_router make_filesystem_route(const utility::string_t& filesystem_root, const relative_path_content_type_validator& validate, slog::base_gate& gate)
        {
            using namespace web::http::experimental::listener::api_router_using_declarations;

            api_router filesystem_route;

            filesystem_route.support(U("(?<filesystem-relative-path>/.+)"), web::http::methods::GET, [filesystem_root, validate, &gate](const http_request& req, http_response& res, const string_t&, const route_parameters& parameters)
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

                    if (filesystem::exists(path_t(filesystem_path)))
                    {
                        // this was written as a continuation on the task from open_istream, but api_router can now call multiple handlers
                        // to modify the response, so the asynchronicity needs more consideration...
                        const utility::size64_t content_length = filesystem::file_size(path_t(filesystem_path));
                        pplx::task<concurrency::streams::istream> tis = concurrency::streams::fstream::open_istream(filesystem_path, std::ios::in);
                        concurrency::streams::istream is = tis.get();
                        set_reply(res, status_codes::OK, is, content_length, content_type);
                    }
                    else
                    {
                        // unless requests are for files of a supported file type that are actually found in the filesystem, let's just report Forbidden
                        slog::log<slog::severities::error>(gate, SLOG_FLF) << nmos::api_stash(req, parameters) << "Unexpected request forbidden";
                        set_reply(res, status_codes::Forbidden);
                    }
                }

                return true;
            });

            return filesystem_route;
        }
    }
}
