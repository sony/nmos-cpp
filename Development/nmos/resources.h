#ifndef NMOS_RESOURCES_H
#define NMOS_RESOURCES_H

#include <functional>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include "nmos/resource.h"

// This declares a container type suitable for managing a node's resources, or all the resources in a registry,
// with indices to support the operations required by the Node, Query and Registration APIs.
namespace nmos
{
    // The implementation is currently left public and high-level operations are provided as free functions while
    // the interface settles down...

    namespace tags
    {
        struct id;
        struct type;
        struct created;
        struct updated;
    }

    // the created/updated indices are in descending order to simplify Query API cursor-based paging

    typedef boost::multi_index_container<
        resource,
        boost::multi_index::indexed_by<
            boost::multi_index::hashed_unique<boost::multi_index::tag<tags::id>, boost::multi_index::member<resource_core, id, &resource_core::id>>,
            boost::multi_index::ordered_non_unique<boost::multi_index::tag<tags::type>, boost::multi_index::member<resource_core, type, &resource_core::type>>,
            boost::multi_index::ordered_unique<boost::multi_index::tag<tags::created>, boost::multi_index::member<resource_core, tai, &resource_core::created>, std::greater<tai>>,
            boost::multi_index::ordered_unique<boost::multi_index::tag<tags::updated>, boost::multi_index::member<resource_core, tai, &resource_core::updated>, std::greater<tai>>
        >
    > resources;

    // Resource creation/update/deletion operations

    inline tai most_recent_update(const resources& resources)
    {
        auto& by_updated = resources.get<tags::updated>();
        return (by_updated.empty() ? tai{} : by_updated.begin()->updated);
    }

    inline tai strictly_increasing_update(const resources& resources, tai update = tai_now())
    {
        const auto most_recent = most_recent_update(resources);
        return update > most_recent ? update : tai_from_time_point(time_point_from_tai(most_recent) + tai_clock::duration(1));
    }

    // note, this is now O(N), not O(1), since resource health is mutable and therefore unindexed
    health least_health(const resources& resources);

    // insert a resource (join_sub_resources can be false if related resources are known to be inserted in order)
    std::pair<resources::iterator, bool> insert_resource(resources& resources, resource&& resource, bool join_sub_resources = false);

    // modify a resource
    bool modify_resource(resources& resources, const id& id, std::function<void(resource&)> modifier);

    // erase the resource with the specified id from the specified resources (if present)
    // and return the count of the number of resources erased (including sub-resources)
    resources::size_type erase_resource(resources& resources, const id& id);

    // erase all resources which expired *before* the specified time from the specified resources
    void erase_expired_resources(resources& resources, const health& until_health);

    // find the resource with the specified id in the specified resources (if present) and
    // set the health of the resource and all of its sub-resources, to prevent them expiring
    // note, since health is mutable, no need for the resources parameter to be non-const
    void set_resource_health(const resources& resources, const id& id, health health = health_now());

    // Other helper functions for resources

    // get the super-resource id and type
    std::pair<id, type> get_super_resource(const web::json::value& data, const type& type);

    bool has_resource(const resources& resources, const std::pair<id, type>& id_type);

    resources::const_iterator find_resource(const resources& resources, const std::pair<id, type>& id_type);
    resources::iterator find_resource(resources& resources, const std::pair<id, type>& id_type);

    // strictly, this just returns a node (or the end iterator)
    resources::const_iterator find_self_resource(const resources& resources);
    resources::iterator find_self_resource(resources& resources);

    // get the id of each resource with the specified super-resource
    std::set<nmos::id> get_sub_resources(const resources& resources, const std::pair<id, type>& id_type);
}

#endif
