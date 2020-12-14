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

    // Function object to match resources against a query
    struct resource_query
    {
        typedef bool result_type;

        resource_query(const nmos::api_version& version, const utility::string_t& resource_path, const web::json::value& flat_query_params);

        // evaluate the query against the specified resource in the context of the specified resources
        result_type operator()(const nmos::resource& resource, const nmos::resources& resources) const { return (*this)(resource.version, resource.downgrade_version, resource.type, resource.data, resources); }

        web::json::value downgrade(const nmos::resource& resource) const { return downgrade(resource.version, resource.downgrade_version, resource.type, resource.data); }

        result_type operator()(const nmos::api_version& resource_version, const nmos::api_version& resource_downgrade_version, const nmos::type& resource_type, const web::json::value& resource_data, const nmos::resources& resources) const;

        web::json::value downgrade(const nmos::api_version& resource_version, const nmos::api_version& resource_downgrade_version, const nmos::type& resource_type, const web::json::value& resource_data) const;

        // the Query API version (since a registry being queried may contain resources of more than one version of IS-04 Discovery and Registration)
        nmos::api_version version;

        // resource_path may be empty (matching all resource types) or e.g. "/nodes"
        utility::string_t resource_path;

        // the query/exemplar object for a Basic Query
        web::json::value basic_query;

        // the client's minimum supported version of IS-04 Discovery and Registration
        nmos::api_version downgrade_version;

        // whether resources of a higher API version are stripped of higher-version keys (false is experimental)
        bool strip;

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

    namespace details
    {
        // make user error information (to be used with status_codes::BadRequest)
        utility::string_t make_valid_paging_error(const nmos::resource_paging& paging);
    }

    // Cursor-based paging customisation points

    inline nmos::tai extract_cursor(const nmos::resources::index<tags::created>::type&, nmos::resources::index<tags::created>::type::const_iterator it) { return it->created; }
    inline nmos::tai extract_cursor(const nmos::resources::index<tags::updated>::type&, nmos::resources::index<tags::updated>::type::const_iterator it) { return it->updated; }

    inline nmos::resources::index<tags::created>::type::const_iterator lower_bound(const nmos::resources::index<tags::created>::type& index, const nmos::tai& timestamp) { return index.lower_bound(timestamp); }
    inline nmos::resources::index<tags::updated>::type::const_iterator lower_bound(const nmos::resources::index<tags::updated>::type& index, const nmos::tai& timestamp) { return index.lower_bound(timestamp); }

    // Helpers for constructing /subscriptions websocket grains
    // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/docs/4.2.%20Behaviour%20-%20Querying.md

    // make the initial 'sync' resource events for a new grain, including all resources that match the specified version, resource path and flat query parameters
    // optionally, make 'added' resource events instead of 'sync' events
    web::json::value make_resource_events(const nmos::resources& resources, const nmos::api_version& version, const utility::string_t& resource_path, const web::json::value& params, bool sync = true);

    // insert 'added', 'removed' or 'modified' resource events into all grains whose subscriptions match the specified version, type and "pre" or "post" values
    void insert_resource_events(nmos::resources& resources, const nmos::api_version& version, const nmos::api_version& downgrade_version, const nmos::type& type, const web::json::value& pre, const web::json::value& post);

    namespace fields
    {
        const web::json::field_as_string_or query_rql{ U("query.rql"), {} };
        const web::json::field_as_string_or paging_limit{ U("paging.limit"), {} };

        const web::json::field_as_value message{ U("message") };
        const web::json::field_path<utility::string_t> grain_topic{ { U("grain"), U("topic") } };
        const web::json::field_path<web::json::value> grain_data{ { U("grain"), U("data") } };
        const web::json::field_path<web::json::value> message_grain_data{ { U("message"), U("grain"), U("data") } };
    }

    namespace experimental
    {
        namespace fields
        {
            const web::json::field_as_string_or query_strip{ U("query.strip"), {} };
        }
    }

    namespace details
    {
        // get the resource id and type from the grain topic and event "path"
        std::pair<nmos::id, nmos::type> get_resource_event_resource(const utility::string_t& topic, const web::json::value& event);

        enum resource_event_type
        {
            resource_continued_nonexistence_event, // not used; neither existed previously, nor any longer (we may co-opt this for e.g. replicating health)
            resource_added_event,
            resource_removed_event,
            resource_modified_event,
            resource_unchanged_event // also known as 'sync'
        };

        // determine the type of the resource event from "pre" and "post"
        resource_event_type get_resource_event_type(const web::json::value& event);

        // resource_path may be empty (matching all resource types) or e.g. "/nodes"
        web::json::value make_resource_event(const utility::string_t& resource_path, const nmos::type& type, const web::json::value& pre, const web::json::value& post);

        // make an empty grain
        web::json::value make_grain(const nmos::id& source_id, const nmos::id& flow_id, const utility::string_t& topic);
    }
}

#endif
