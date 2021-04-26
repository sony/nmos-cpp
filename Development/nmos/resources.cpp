#include "nmos/resources.h"

#include <boost/range/adaptor/reversed.hpp>
#include "nmos/is04_versions.h"
#include "nmos/query_utils.h"

namespace nmos
{
    // Resource creation/update/deletion operations

    // returns the most recent timestamp in the specified resources
    tai most_recent_update(const resources& resources)
    {
        auto& by_updated = resources.get<tags::updated>();
        return (by_updated.empty() ? tai{} : by_updated.begin()->updated);
    }

    // returns the least health of extant and non-extant resources
    // note, this is now O(N), not O(1), since resource health is mutable and therefore unindexed
    std::pair<health, health> least_health(const resources& resources)
    {
        const auto now = health_now();
        std::pair<health, health> results{ now, now };
        for (const auto& resource : resources)
        {
            auto& result = resource.has_data() ? results.first : results.second;

            // since health is mutable, the snapshot may be immediately out of date
            // but it is reasonable to rely on it not being modified to be *less*
            const auto health_snapshot = resource.health.load();
            if (health_snapshot < result)
            {
                result = health_snapshot;
            }
        }
        return results;
    }

    // insert a resource
    std::pair<resources::iterator, bool> insert_resource(resources& resources, resource&& resource, bool join_sub_resources)
    {
        if (join_sub_resources)
        {
            // join this resource to any sub-resources which were inserted out-of-order
            resource.sub_resources = get_sub_resources(resources, { resource.id, resource.type });
        }

        // set the creation and update timestamps, before inserting the resource
        resource.updated = resource.created = nmos::strictly_increasing_update(resources);

        // all types (other than nodes, and subscriptions) must* be a sub-resource of an existing resource
        // (*assuming not out-of-order insertion by the allow_invalid_resources setting)
        auto super_resource = find_resource(resources, get_super_resource(resource));
        if (super_resource != resources.end())
        {
            // this isn't modifying the visible data of the super_resouce, so no resource events need to be generated
            resources.modify(super_resource, [&](nmos::resource& super_resource)
            {
                super_resource.sub_resources.insert(resource.id);
            });
        }

        auto result = resources.insert(std::move(resource));
        // replacement of a deleted or expired resource is also allowed
        // (currently, with no further checks on api_version, type, etc.)
        if (!result.second && !result.first->has_data())
        {
            // if the insertion was banned, resource has not been moved from
            result.second = resources.replace(result.first, std::move(resource));
        }

        if (result.second)
        {
            auto& inserted = *result.first;
            insert_resource_events(resources, inserted.version, inserted.downgrade_version, inserted.type, web::json::value::null(), inserted.data);

            // set the initial health of this resource from the super-resource (if applicable)
            // and update the health of any sub-resources to which the resource has been joined
            // this may emit further resource events...
            auto inserted_health = nmos::health_forever != inserted.health
                ? super_resource != resources.end() ? super_resource->health.load() : inserted.created.seconds
                : nmos::health_forever;
            set_resource_health(resources, inserted.id, inserted_health);
        }
        // else logic error?

        return result;
    }

    // modify a resource
    bool modify_resource(resources& resources, const id& id, std::function<void(resource&)> modifier)
    {
        auto found = resources.find(id);
        if (resources.end() == found || !found->has_data()) return false;

        auto pre = found->data;

        // "If an exception is thrown by some user-provided operation, then the element pointed to by position is erased."
        // This seems too surprising, despite the fact that it means that a modification may have been partially completed,
        // so capture and rethrow.
        // See https://www.boost.org/doc/libs/1_68_0/libs/multi_index/doc/reference/ord_indices.html#modify
        std::exception_ptr modifier_exception;

        auto resource_updated = nmos::strictly_increasing_update(resources);
        auto result = resources.modify(found, [&resource_updated, &modifier, &modifier_exception](resource& resource)
        {
            try
            {
                modifier(resource);
            }
            catch (...)
            {
                modifier_exception = std::current_exception();
            }

            // set the update timestamp
            resource.updated = resource_updated;
        });

        if (result)
        {
            auto& modified = *found;
            insert_resource_events(resources, modified.version, modified.downgrade_version, modified.type, pre, modified.data);
        }

        if (modifier_exception)
        {
            std::rethrow_exception(modifier_exception);
        }

        return result;
    }

    // erase the resource with the specified id from the specified resources (if present)
    // and return the count of the number of resources erased (including sub-resources)
    // resources may optionally be initially "erased" by setting data to null, and remain in this non-extant state until they are explicitly forgotten (or reinserted)
    resources::size_type erase_resource(resources& resources, const id& id, bool forget_now)
    {
        // also erase all sub-resources of this resource, i.e.
        // for a node, all devices with matching node_id
        // for a device, all sources, senders and receivers with matching device_id
        // for a sender, all flows with matching source_id
        // it won't be a very deep recursion...
        resources::size_type count = 0;
        auto found = resources.find(id);
        if (resources.end() != found && found->has_data())
        {
            for (auto& sub_resource : found->sub_resources)
            {
                count += erase_resource(resources, sub_resource, forget_now);
            }

            const auto pre = found->data;

            auto resource_updated = nmos::strictly_increasing_update(resources);
            resources.modify(found, [&resource_updated](resource& resource)
            {
                resource.data = web::json::value::null();

                // set the update timestamp when a resource is deleted
                resource.updated = resource_updated;
            });

            auto& erased = *found;
            insert_resource_events(resources, erased.version, erased.downgrade_version, erased.type, pre, erased.data);

            if (forget_now)
            {
                resources.erase(found);
            }

            ++count;
        }
        return count;
    }

    // forget all erased resources which expired *before* the specified time from the specified resources
    // and return the count of the number of resources forgotten
    // resources may optionally be initially "erased" by setting data to null, and remain in this non-extant state until they are explicitly forgotten (or reinserted)
    resources::size_type forget_erased_resources(resources& resources, const health& forget_health)
    {
        resources::size_type count = 0;
        auto& by_type = resources.get<tags::type>();
        const auto range = by_type.equal_range(false);
        auto found = range.first;
        while (range.second != found)
        {
            if (found->health < forget_health || found->health == health_forever)
            {
                found = by_type.erase(found);
                ++count;
            }
            else
            {
                ++found;
            }
        }
        return count;
    }

    // erase all resources of the specified type which expired *before* the specified time from the specified resources
    // and return the count of the number of resources erased; sub-resources are *not* erased
    // resources may optionally be initially "erased" by setting data to null, and remain in this non-extant state until they are explicitly forgotten (or reinserted)
    // by default, the updated timestamp is not modified but this may be overridden
    resources::size_type erase_expired_resources(resources& resources, const nmos::type& type, const health& expire_health, bool forget_now, bool set_updated)
    {
        resources::size_type count = 0;
        auto& by_type = resources.get<tags::type>();
        const auto range = by_type.equal_range(details::has_data(type));
        auto found = range.first;
        while (range.second != found)
        {
            if (expire_health <= found->health)
            {
                ++found;
            }
            else
            {
                // since modify reorders the resource in this index
                const auto next = std::next(found);

                const auto pre = found->data;

                auto resource_updated = nmos::strictly_increasing_update(resources);
                by_type.modify(found, [&](resource& resource)
                {
                    resource.data = web::json::value::null();

                    // optionally set the update timestamp when a resource is expired
                    if (set_updated)
                    {
                        resource.updated = resource_updated;
                    }
                });

                auto& erased = *found;
                insert_resource_events(resources, erased.version, erased.downgrade_version, erased.type, pre, erased.data);

                if (forget_now)
                {
                    by_type.erase(found);
                }

                found = next;
                ++count;
            }
        }
        return count;
    }

    // erase all resources which expired *before* the specified time from the specified resources
    // and return the count of the number of resources erased
    // resources may optionally be initially "erased" by setting data to null, and remain in this non-extant state until they are explicitly forgotten (or reinserted)
    // by default, the updated timestamp is not modified but this may be overridden
    resources::size_type erase_expired_resources(resources& resources, const health& expire_health, bool forget_now, bool set_updated)
    {
        resources::size_type count = 0;
        // reverse order to ensure sub-resources are erased before super-resources
        for (const auto& type : nmos::types::all | boost::adaptors::reversed)
        {
            count += erase_expired_resources(resources, type, expire_health, forget_now, set_updated);
        }
        return count;
    }

    // find the resource with the specified id in the specified resources (if present) and
    // set the health of the resource and all of its sub-resources, to prevent them expiring
    // note, since health is mutable, no need for the resources parameter to be non-const
    void set_resource_health(const resources& resources, const id& id, health health)
    {
        auto found = resources.find(id);
        if (resources.end() != found && found->has_data())
        {
            for (auto& sub_resource : found->sub_resources)
            {
                set_resource_health(resources, sub_resource, health);
            }

            // since health is mutable, no need for:
            // resources.modify(found, [&health](nmos::resource& resource){ resource.health = health; });
            found->health = health;
        }
    }

    static inline std::pair<id, type> no_resource() { return{}; }

    // get the super-resource id and type, according to the guidelines on referential integrity
    // see https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2.1/docs/4.1.%20Behaviour%20-%20Registration.md#referential-integrity
    std::pair<id, type> get_super_resource(const api_version& version, const type& type, const web::json::value& data)
    {
        if (data.is_null())
        {
            return no_resource();
        }
        else if (nmos::types::node == type)
        {
            return no_resource();
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
            // "Version v1.0 Flows do not have a device_id and should be garbage collected based on their parent source_id."
            if (nmos::is04_versions::v1_0 >= version)
            {
                return{ nmos::fields::source_id(data), nmos::types::source };
            }
            return{ nmos::fields::device_id(data), nmos::types::device };
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
            return no_resource();
        }
        else if (nmos::types::grain == type)
        {
            return{ nmos::fields::subscription_id(data), nmos::types::subscription };
        }
        else
        {
            return no_resource();
        }
    }

    bool has_resource(const resources& resources, const std::pair<id, type>& id_type)
    {
        if (no_resource() == id_type) return false;
        auto resource = resources.find(id_type.first);
        return resources.end() != resource && resource->has_data() && id_type.second == resource->type;
    }

    // find resource by id
    resources::const_iterator find_resource(const resources& resources, const id& id)
    {
        if (id.empty()) return resources.end();
        auto resource = resources.find(id);
        return resources.end() != resource && resource->has_data() ? resource : resources.end();
    }

    resources::iterator find_resource(resources& resources, const id& id)
    {
        if (id.empty()) return resources.end();
        auto resource = resources.find(id);
        return resources.end() != resource && resource->has_data() ? resource : resources.end();
    }

    // find resource by id, and matching type
    resources::const_iterator find_resource(const resources& resources, const std::pair<id, type>& id_type)
    {
        auto resource = find_resource(resources, id_type.first);
        return resources.end() != resource && id_type.second == resource->type ? resource : resources.end();
    }

    resources::iterator find_resource(resources& resources, const std::pair<id, type>& id_type)
    {
        auto resource = find_resource(resources, id_type.first);
        return resources.end() != resource && id_type.second == resource->type ? resource : resources.end();
    }

    // strictly, this just returns a node (or the end iterator)
    resources::const_iterator find_self_resource(const resources& resources)
    {
        auto& by_type = resources.get<tags::type>();
        const auto nodes = by_type.equal_range(details::has_data(nmos::types::node));
        return nodes.second != nodes.first ? resources.project<0>(nodes.first) : resources.end();
    }

    // strictly, this just returns a node (or the end iterator)
    resources::iterator find_self_resource(resources& resources)
    {
        auto& by_type = resources.get<tags::type>();
        const auto nodes = by_type.equal_range(details::has_data(nmos::types::node));
        return nodes.second != nodes.first ? resources.project<0>(nodes.first) : resources.end();
    }

    // get the id of each resource with the specified super-resource
    std::set<nmos::id> get_sub_resources(const resources& resources, const std::pair<id, type>& id_type)
    {
        std::set<nmos::id> result;
        for (const auto& sub_resource : resources)
        {
            if (id_type == get_super_resource(sub_resource))
            {
                result.insert(sub_resource.id);
            }
        }
        return result;
    }

    namespace details
    {
        // return true if the resource is "erased" but not forgotten
        bool is_erased_resource(const resources& resources, const std::pair<id, type>& id_type)
        {
            if (no_resource() == id_type) return false;
            auto resource = resources.find(id_type.first);
            return resources.end() != resource && id_type.second == resource->type && !resource->has_data();
        }
    }
}
