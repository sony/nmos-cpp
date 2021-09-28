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
                s << resource.type.name << ' ' << resource.id.substr(0, 6) << ' ' << make_version(resource.created) << ' ' << make_version(resource.updated) << ' ' << resource.health << (resource.has_data() ? "" : " (non-extant)") << '\n';
                for (auto& sub_resource : resource.sub_resources)
                {
                    // note that this information may be out-of-date because in some circumstances a resource is *not* removed from its super-resource's sub-resources
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
            s << by_type.count(true) << " resources ("
                << by_type.count(details::has_data(types::node)) << " nodes, "
                << by_type.count(details::has_data(types::device)) << " devices, "
                << by_type.count(details::has_data(types::source)) << " sources, "
                << by_type.count(details::has_data(types::flow)) << " flows, "
                << by_type.count(details::has_data(types::sender)) << " senders, "
                << by_type.count(details::has_data(types::receiver)) << " receivers, "
                << by_type.count(details::has_data(types::subscription)) << " subscriptions, "
                << by_type.count(details::has_data(types::grain)) << " grains), "
                << "most recent update: " << make_version(most_recent_update(resources)) << ", least health: " << least_health(resources).first << ", "
                << by_type.count(false) << " non-extant resources";
        });
    }
}

#endif
