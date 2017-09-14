#ifndef NMOS_LOG_MANIP_H
#define NMOS_LOG_MANIP_H

#include "nmos/slog.h"
#include "nmos/model.h"

namespace nmos
{
    // Log a model in abbreviated form (relying on logging of utility::string_t, from nmos/slog.h)
    inline slog::log_statement::manip_function put_model(const model& model)
    {
        return slog::log_manip([&](slog::log_statement& s)
        {
            for (auto& resource : model.resources)
            {
                s << resource.type << ' ' << resource.id.substr(0, 6) << ' ' << make_version(resource.created) << ' ' << make_version(resource.updated) << '\n';
                for (auto& sub_resource : resource.sub_resources)
                {
                    s << "  " << sub_resource.substr(0, 6) << '\n';
                }
            }
        });
    }
}

#endif
