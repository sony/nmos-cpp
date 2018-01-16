#ifndef NMOS_QUERY_UTILS_H
#define NMOS_QUERY_UTILS_H

#include <boost/range/any_range.hpp>
#include "nmos/paging_utils.h"
#include "nmos/resources.h"

namespace nmos
{
    // Helpers for advanced query options

    namespace experimental
    {
        web::json::match_flag_type parse_match_type(const utility::string_t& match_type);
    }

    // Predicate to match resources against a query
    struct resource_query
    {
        typedef const nmos::resource& argument_type;
        typedef bool result_type;

        resource_query(const nmos::api_version& version, const utility::string_t& resource_path, const web::json::value& flat_query_params);

        result_type operator()(argument_type resource) const { return (*this)(resource.version, resource.type, resource.data); }

        result_type operator()(const nmos::api_version& resource_version, const nmos::type& resource_type, const web::json::value& resource_data) const;

        // the Query API version (since a registry being queried may contain resources of more than one version of IS-04 Discovery and Registration)
        nmos::api_version version;

        // resource_path may be empty (matching all resource types) or e.g. "/nodes"
        utility::string_t resource_path;

        // the query/exemplar object for a Basic Query
        web::json::value basic_query;

        // the client's minimum supported version of IS-04 Discovery and Registration
        nmos::api_version downgrade_version;

        // a representation of the RQL abstract syntax tree for an Advanced Query
        web::json::value rql_query;

        // flags that affect the Basic Query (experimental)
        web::json::match_flag_type match_flags;
    };

    // Cursor-based paging parameters
    struct resource_paging
    {
        explicit resource_paging(const web::json::value& flat_query_params, const nmos::tai& max_until = nmos::tai_max(), size_t default_limit = (std::numeric_limits<size_t>::max)(), size_t max_limit = (std::numeric_limits<size_t>::max)());

        // determine if the range [until, since) and limit are valid
        bool valid() const;

        // "Specify whether paging should be based upon initial resource creation time, or when it was last modified"
        bool order_by_created;

        // "Return only the results which were created/updated up until the time specified (inclusive)"
        nmos::tai until;

        // "Return only the results which have been created/updated since the time specified (non-inclusive)"
        nmos::tai since;

        // "Restrict the response to the specified number of results"
        size_t limit;

        // "Where both 'since' and 'until' parameters are specified, the 'since' value takes precedence
        // where a resulting data set is constrained by the server's value of 'limit'"
        bool since_specified;

        template <typename Predicate>
        boost::any_range<const nmos::resource, boost::bidirectional_traversal_tag, const nmos::resource&, std::ptrdiff_t> page(const nmos::resources& resources, Predicate match)
        {
            if (order_by_created)
            {
                return paging::cursor_based_page(resources.get<tags::created>(), match, until, since, limit, !since_specified);
            }
            else
            {
                return paging::cursor_based_page(resources.get<tags::updated>(), match, until, since, limit, !since_specified);
            }
        }
    };

    // Cursor-based paging customisation points

    inline nmos::tai extract_cursor(const nmos::resources::index<tags::created>::type&, nmos::resources::index_iterator<tags::created>::type it) { return it->created; }
    inline nmos::tai extract_cursor(const nmos::resources::index<tags::updated>::type&, nmos::resources::index_iterator<tags::updated>::type it) { return it->updated; }

    inline nmos::resources::index_iterator<tags::created>::type lower_bound(const nmos::resources::index<tags::created>::type& index, const nmos::tai& timestamp) { return index.lower_bound(timestamp); }
    inline nmos::resources::index_iterator<tags::updated>::type lower_bound(const nmos::resources::index<tags::updated>::type& index, const nmos::tai& timestamp) { return index.lower_bound(timestamp); }

    // Helpers for constructing /subscriptions websocket grains

    // make the initial 'sync' resource events for a new grain, including all resources that match the specified version, resource path and flat query parameters
    web::json::value make_resource_events(const nmos::resources& resources, const nmos::api_version& version, const utility::string_t& resource_path, const web::json::value& params);

    // insert 'added', 'removed' or 'modified' resource events into all grains whose subscriptions match the specified version, type and "pre" or "post" values
    void insert_resource_events(nmos::resources& resources, const nmos::api_version& version, const nmos::type& type, const web::json::value& pre, const web::json::value& post);

    namespace fields
    {
        const web::json::field_as_value message{ U("message") };
        const web::json::field_path<web::json::value> message_grain_data{ { U("message"), U("grain"), U("data") } };
    }

    namespace details
    {
        // resource_path may be empty (matching all resource types) or e.g. "/nodes"
        web::json::value make_resource_event(const utility::string_t& resource_path, const nmos::type& type, const web::json::value& pre, const web::json::value& post);

        // set all three timestamps in the message
        void set_grain_timestamp(web::json::value& message, const nmos::tai& tai);

        nmos::tai get_grain_timestamp(const web::json::value& message);

        // make an empty grain
        web::json::value make_grain(const nmos::id& source_id, const nmos::id& flow_id, const utility::string_t& topic);
    }
}

#endif
