#include "nmos/resources.h"

#include "nmos/query_utils.h"

namespace nmos
{
    // Resource creation/update/deletion operations

    // note, this is now O(N), not O(1), since resource health is mutable and therefore unindexed
    health least_health(const resources& resources)
    {
        auto result = health_now();
        for (const auto& resource : resources)
        {
            // since health is mutable, the snapshot may be immediately out of date
            // but it is reasonable to rely on it not being modified to be *less*
            const auto health_snapshot = resource.health.load();
            if (health_snapshot < result)
            {
                result = health_snapshot;
            }
        }
        return result;
    }

    // insert a resource
    std::pair<resources::iterator, bool> insert_resource(resources& resources, resource&& resource, bool join_sub_resources)
    {
        if (join_sub_resources)
        {
            // join this resource to any sub-resources which were inserted out-of-order
            resource.sub_resources = get_sub_resources(resources, { resource.id, resource.type });
        }

        // set the creation and update timestamps, and the initial health, before inserting the resource
        resource.updated = resource.created = nmos::strictly_increasing_update(resources);
        if (nmos::health_forever != resource.health)
        {
            resource.health = resource.created.seconds;
        }

        // all types (other than nodes, and subscriptions) must* be a sub-resource of an existing resource
        // (*assuming not out-of-order insertion by the allow_invalid_resources setting)
        auto super_resource = find_resource(resources, get_super_resource(resource.data, resource.type));
        if (super_resource != resources.end())
        {
            // this isn't modifying the visible data of the super_resouce, so no resource events need to be generated
            resources.modify(super_resource, [&](nmos::resource& super_resource)
            {
                super_resource.sub_resources.insert(resource.id);
            });
        }

        auto result = resources.insert(std::move(resource));

        if (result.second)
        {
            auto& resource = *result.first;
            insert_resource_events(resources, resource.version, resource.type, web::json::value::null(), resource.data);
        }
        // else logic error?

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

    // erase all resources which expired *before* the specified time from the specified model
    void erase_expired_resources(resources& resources, const health& until_health)
    {
        auto resource = resources.begin();
        while (resources.end() != resource)
        {
            if (until_health <= resource->health)
            {
                ++resource;
                continue;
            }

            // should also erase all sub-resources of this resource, and remove this resource's id
            // from its super-resource's sub-resources, but this is effectively achieved anyway since
            // health is propagated from nodes (and subscriptions) which don't have a super-resource

            const auto version = resource->version;
            const auto type = resource->type;
            const auto pre = resource->data;

            resource = resources.erase(resource);

            insert_resource_events(resources, version, type, pre, web::json::value::null());
        }
    }

    // find the resource with the specified id in the specified resources (if present) and
    // set the health of the resource and all of its sub-resources, to prevent them expiring
    // note, since health is mutable, no need for the resources parameter to be non-const
    void set_resource_health(const resources& resources, const id& id, health health)
    {
        auto resource = resources.find(id);
        if (resources.end() != resource)
        {
            for (auto& sub_resource : resource->sub_resources)
            {
                set_resource_health(resources, sub_resource, health);
            }

            // since health is mutable, no need for:
            // resources.modify(resource, [&health](nmos::resource& resource){ resource.health = health; });
            resource->health = health;
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
        else if (nmos::types::grain == type)
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

    resources::const_iterator find_resource(const resources& resources, const std::pair<id, type>& id_type)
    {
        if (no_resource == id_type) return resources.end();
        auto resource = resources.find(id_type.first);
        return resources.end() != resource && id_type.second == resource->type ? resource : resources.end();
    }

    resources::iterator find_resource(resources& resources, const std::pair<id, type>& id_type)
    {
        if (no_resource == id_type) return resources.end();
        auto resource = resources.find(id_type.first);
        return resources.end() != resource && id_type.second == resource->type ? resource : resources.end();
    }

    // strictly, this just returns a node (or the end iterator)
    resources::const_iterator find_self_resource(const resources& resources)
    {
        auto& by_type = resources.get<tags::type>();
        const auto nodes = by_type.equal_range(nmos::types::node);
        return nodes.second != nodes.first ? resources.project<0>(nodes.first) : resources.end();
    }

    // strictly, this just returns a node (or the end iterator)
    resources::iterator find_self_resource(resources& resources)
    {
        auto& by_type = resources.get<tags::type>();
        const auto nodes = by_type.equal_range(nmos::types::node);
        return nodes.second != nodes.first ? resources.project<0>(nodes.first) : resources.end();
    }

    // get the id of each resource with the specified super-resource
    std::set<nmos::id> get_sub_resources(const resources& resources, const std::pair<id, type>& id_type)
    {
        std::set<nmos::id> result;
        for (const auto& sub_resource : resources)
        {
            if (id_type == get_super_resource(sub_resource.data, sub_resource.type))
            {
                result.insert(sub_resource.id);
            }
        }
        return result;
    }
}
