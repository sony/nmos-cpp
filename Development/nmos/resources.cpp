#include "nmos/resources.h"

#include "nmos/query_utils.h"

namespace nmos
{
    // Resource creation/update/deletion operations

    // insert a resource
    std::pair<resources::iterator, bool> insert_resource(resources& resources, resource&& resource)
    {
        // set the creation and update timestamps, and the initial health, before inserting the resource
        resource.updated = resource.created = nmos::strictly_increasing_update(resources);
        if (nmos::health_forever != resource.health)
        {
            resource.health = resource.created.seconds;
        }
        auto result = resources.insert(std::move(resource));

        if (result.second)
        {
            auto& resource = *result.first;
            insert_resource_events(resources, resource.version, resource.type, web::json::value::null(), resource.data);
        }

        return result;
    }

    // modify a resource
    bool modify_resource(resources& resources, const id& id, std::function<void(resource&)> modifier)
    {
        auto found = resources.find(id);
        auto pre = found->data;

        // set the update timestamp before applying the modifier
        auto resource_updated = nmos::strictly_increasing_update(resources);
        auto result = resources.modify(found, [&resource_updated, &modifier](resource& resource) {
            resource.updated = resource_updated;
            modifier(resource);
        });

        if (result)
        {
            auto& resource = *found;
            insert_resource_events(resources, resource.version, resource.type, pre, resource.data);
        }

        return result;
    }

    // erase the resource with the specified id from the specified resources (if present)
    // and return the count of the number of resources erased (including sub-resources)
    resources::size_type erase_resource(resources& resources, const id& id)
    {
        // also erase all sub-resources of this resource, i.e.
        // for a node, all devices with matching node_id
        // for a device, all sources, senders and receivers with matching device_id
        // for a sender, all flows with matching source_id
        // it won't be a very deep recursion...
        resources::size_type count = 0;
        auto resource = resources.find(id);
        if (resources.end() != resource)
        {
            for (auto& sub_resource : resource->sub_resources)
            {
                count += erase_resource(resources, sub_resource);
            }

            const auto version = resource->version;
            const auto type = resource->type;
            const auto pre = resource->data;

            resources.erase(resource);

            insert_resource_events(resources, version, type, pre, web::json::value::null());

            ++count;
        }
        return count;
    }

    // erase all resources which expired at or before the specified time from the specified model
    void erase_expired_resources(resources& resources, const health& expiration_health)
    {
        typedef resources::index<tags::health>::type by_health;
        by_health& healths = resources.get<tags::health>();
        by_health::const_iterator unexpired = healths.lower_bound(expiration_health);

        for (auto resource = healths.begin(); unexpired != resource; ++resource)
        {
            const auto version = resource->version;
            const auto type = resource->type;
            const auto pre = resource->data;

            insert_resource_events(resources, version, type, pre, web::json::value::null());
        }

        // should remove each of these resources from their super-resource's sub-resources
        // but this is unnecessary because health is propagated from nodes (and subscriptions)
        // which don't have a super-resource
        healths.erase(healths.begin(), unexpired);
    }

    // find the resource with the specified id in the specified resources (if present) and
    // set the health of the resource and all of its sub-resources, to prevent them expiring
    void set_resource_health(resources& resources, const id& id, health health)
    {
        auto resource = resources.find(id);
        if (resources.end() != resource)
        {
            for (auto& sub_resource : resource->sub_resources)
            {
                set_resource_health(resources, sub_resource, health);
            }

            resources.modify(resource, [&health](nmos::resource& resource){ resource.health = health; });
        }
    }

    static const std::pair<id, type> no_resource{};

    // get the super-resource id and type
    std::pair<id, type> get_super_resource(const web::json::value& data, const type& type)
    {
        if (nmos::types::node == type)
        {
            return no_resource;
        }
        else if (nmos::types::device == type)
        {
            return{ nmos::fields::node_id(data), nmos::types::node };
        }
        else if (nmos::types::source == type)
        {
            return{ nmos::fields::device_id(data), nmos::types::device };
        }
        else if (nmos::types::flow == type)
        {
            return{ nmos::fields::source_id(data), nmos::types::source };
        }
        else if (nmos::types::sender == type)
        {
            return{ nmos::fields::device_id(data), nmos::types::device };
        }
        else if (nmos::types::receiver == type)
        {
            return{ nmos::fields::device_id(data), nmos::types::device };
        }
        else if (nmos::types::subscription == type)
        {
            return no_resource;
        }
        else if (nmos::types::websocket == type)
        {
            return{ nmos::fields::subscription_id(data), nmos::types::subscription };
        }
        else
        {
            return no_resource;
        }
    }

    bool has_resource(const resources& resources, const std::pair<id, type>& id_type)
    {
        if (no_resource == id_type) return false;
        auto resource = resources.find(id_type.first);
        return resources.end() != resource && id_type.second == resource->type;
    }

    // only need a non-const version so far...
    resources::iterator find_resource(resources& resources, const std::pair<id, type>& id_type)
    {
        if (no_resource == id_type) return resources.end();
        auto resource = resources.find(id_type.first);
        return resources.end() != resource && id_type.second == resource->type ? resource : resources.end();
    }
}
