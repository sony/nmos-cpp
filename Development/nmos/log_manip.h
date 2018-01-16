#ifndef NMOS_LOG_MANIP_H
#define NMOS_LOG_MANIP_H

#include "nmos/slog.h"
#include "nmos/resources.h"
#include "nmos/version.h"

namespace nmos
{
    // Log resources in abbreviated form (relying on logging of utility::string_t, from nmos/slog.h)
    // where the argument may be nmos::resources, any of its indices, or other range of nmos::resource
    template <typename Resources>
    inline slog::log_statement::manip_function put_resources(const Resources& resources)
    {
        return slog::log_manip([&](slog::log_statement& s)
        {
            for (auto& resource : resources)
            {
                s << resource.type.name << ' ' << resource.id.substr(0, 6) << ' ' << make_version(resource.created) << ' ' << make_version(resource.updated) << ' ' << resource.health << '\n';
                for (auto& sub_resource : resource.sub_resources)
                {
                    s << "  " << sub_resource.substr(0, 6) << '\n';
                }
            }
        });
    }

    inline slog::log_statement::manip_function put_resources_statistics(const nmos::resources& resources)
    {
        return slog::log_manip([&](slog::log_statement& s)
        {
            auto& by_type = resources.get<tags::type>();
            s << by_type.size() << " resources ("
                << by_type.count(types::node) << " nodes, "
                << by_type.count(types::device) << " devices, "
                << by_type.count(types::source) << " sources, "
                << by_type.count(types::flow) << " flows, "
                << by_type.count(types::sender) << " senders, "
                << by_type.count(types::receiver) << " receivers, "
                << by_type.count(types::subscription) << " subscriptions, "
                << by_type.count(types::grain) << " grains), "
                << "most recent update: " << make_version(most_recent_update(resources)) << ", least health: " << least_health(resources);
        });
    }
}

#endif
