#include "nmos/api_downgrade.h"

#include "nmos/resource.h"

namespace nmos
{
    // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/docs/2.5.%20APIs%20-%20Query%20Parameters.md#downgrade-queries

    bool is_permitted_downgrade(const nmos::resource& resource, const nmos::api_version& version)
    {
        return is_permitted_downgrade(resource, version, version);
    }

    bool is_permitted_downgrade(const nmos::resource& resource, const nmos::api_version& version, const nmos::api_version& downgrade_version)
    {
        return is_permitted_downgrade(resource.version, resource.type, version, downgrade_version);
    }

    bool is_permitted_downgrade(const nmos::api_version& resource_version, const nmos::type& resource_type, const nmos::api_version& version, const nmos::api_version& downgrade_version)
    {
        // Enforce that "downgrade queries may not be performed between major API versions."
        if (version.major != downgrade_version.major) return false;
        if (resource_version.major != version.major) return false;

        // Only "permit old-versioned responses to be provided to clients which are confident that they can handle any missing attributes between the specified API versions"
        // and never perform an upgrade!
        if (resource_version.minor < downgrade_version.minor) return false;

        // Finally, "only ever return subscriptions which were created against the same API version".
        // See https://basecamp.com/1791706/projects/10192586/messages/70664054#comment_544216653
        if (nmos::types::subscription == resource_type && version != resource_version) return false;

        return true;
    }

    web::json::value downgrade(const nmos::resource& resource, const nmos::api_version& version)
    {
        return downgrade(resource, version, version);
    }

    web::json::value downgrade(const nmos::resource& resource, const nmos::api_version& version, const nmos::api_version& downgrade_version)
    {
        return downgrade(resource.version, resource.type, resource.data, version, downgrade_version);
    }

    web::json::value downgrade(const nmos::api_version& resource_version, const nmos::type& resource_type, const web::json::value& resource_data, const nmos::api_version& version, const nmos::api_version& downgrade_version)
    {
        if (!is_permitted_downgrade(resource_version, resource_type, version, downgrade_version)) return web::json::value::null();

        // optimisation for the common case (old-versioned resources, if being permitted, do not get upgraded)
        if (resource_version <= version) return resource_data;

        web::json::value result;

        // This is a simple representation of the backwards-compatible changes that have been made between minor versions
        // of the specification. It just describes in which version each top-level property of each resource type was added.

        // This doesn't cope with some details, like the addition in v1.2 of the "active" property in the "subscription"
        // sub-object of a receiver. However, the schema, "subscription" sub-object does not have "additionalProperties": false
        // so downgrading from v1.2 to v1.1 and keeping the "active" property doesn't cause a schema violation, though this
        // could be an oversight...
        // See nmos-discovery-registration/APIs/schemas/receiver_core.json

        static const std::map<nmos::type, std::map<nmos::api_version, std::vector<utility::string_t>>> resources_versions
        {
            {
                nmos::types::node,
                {
                    { nmos::is04_versions::v1_0, { U("id"), U("version"), U("label"), U("href"), U("hostname"), U("caps"), U("services") } },
                    { nmos::is04_versions::v1_1, { U("description"), U("tags"), U("api"), U("clocks") } },
                    { nmos::is04_versions::v1_2, { U("interfaces") } }
                }
            },
            {
                nmos::types::device,
                {
                    { nmos::is04_versions::v1_0, { U("id"), U("version"), U("label"), U("type"), U("node_id"), U("senders"), U("receivers") } },
                    { nmos::is04_versions::v1_1, { U("description"), U("tags"), U("controls") } }
                }
            },
            {
                nmos::types::source,
                {
                    { nmos::is04_versions::v1_0, { U("id"), U("version"), U("label"), U("description"), U("format"), U("caps"), U("tags"), U("device_id"), U("parents") } },
                    { nmos::is04_versions::v1_1, { U("grain_rate"), U("clock_name"), U("channels") } }
                }
            },
            {
                nmos::types::flow,
                {
                    { nmos::is04_versions::v1_0, { U("id"), U("version"), U("label"), U("description"), U("format"), U("tags"), U("source_id"), U("parents") } },
                    { nmos::is04_versions::v1_1, { U("grain_rate"), U("device_id"), U("media_type"), U("sample_rate"), U("bit_depth"), U("DID_SDID"), U("frame_width"), U("frame_height"), U("interlace_mode"), U("colorspace"), U("transfer_characteristic"), U("components") } }
                }
            },
            {
                nmos::types::sender,
                {
                    { nmos::is04_versions::v1_0, { U("id"), U("version"), U("label"), U("description"), U("flow_id"), U("transport"), U("tags"), U("device_id"), U("manifest_href") } },
                    { nmos::is04_versions::v1_2, { U("caps"), U("interface_bindings"), U("subscription") } }
                }
            },
            {
                nmos::types::receiver,
                {
                    { nmos::is04_versions::v1_0, { U("id"), U("version"), U("label"), U("description"), U("format"), U("caps"), U("tags"), U("device_id"), U("transport"), U("subscription") } },
                    { nmos::is04_versions::v1_2, { U("interface_bindings") } }
                }
            },
            {
                nmos::types::subscription,
                {
                    { nmos::is04_versions::v1_0, { U("id"), U("ws_href"), U("max_update_rate_ms"), U("persist"), U("resource_path"), U("params") } },
                    { nmos::is04_versions::v1_1, { U("secure") } }
                }
            }
        };

        auto& resource_versions = resources_versions.at(resource_type);
        auto version_first = resource_versions.cbegin();
        auto version_last = resource_versions.upper_bound(version);
        for (auto version_properties = version_first; version_last != version_properties; ++version_properties)
        {
            for (auto& property : version_properties->second)
            {
                if (resource_data.has_field(property))
                {
                    result[property] = resource_data.at(property);
                }
            }
        }

        return result;
    }
}
