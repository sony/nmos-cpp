#include "nmos/query_utils.h"

#include <set>
#include <boost/algorithm/string/split.hpp>
#include <boost/range/adaptor/reversed.hpp>
#include "nmos/api_downgrade.h"
#include "nmos/api_utils.h" // for nmos::resourceType_from_type
#include "nmos/rational.h"
#include "nmos/version.h"
#include "rql/rql.h"

namespace nmos
{
    // Helpers for advanced query options

    namespace experimental
    {
        web::json::match_flag_type parse_match_type(const utility::string_t& match_type)
        {
            std::set<utility::string_t> match_flags;
            boost::algorithm::split(match_flags, match_type, [](utility::char_t c){ return ',' == c; });
            return (match_flags.end() != match_flags.find(U("substr")) ? web::json::match_substr : web::json::match_default)
                | (match_flags.end() != match_flags.find(U("icase")) ? web::json::match_icase : web::json::match_default)
                ;
        }
    }

    resource_query::resource_query(const nmos::api_version& version, const utility::string_t& resource_path, const web::json::value& flat_query_params)
        : version(version)
        , resource_path(resource_path)
        , basic_query(web::json::unflatten(flat_query_params))
        , downgrade_version(version)
        , match_flags(web::json::match_default)
    {
        // extract the supported advanced query options
        if (basic_query.has_field(U("paging")))
        {
            basic_query.erase(U("paging"));
        }
        if (basic_query.has_field(U("query")))
        {
            auto& advanced = basic_query.at(U("query"));
            if (advanced.has_field(U("downgrade")))
            {
                downgrade_version = parse_api_version(web::json::field_as_string{ U("downgrade") }(advanced));
            }
            if (advanced.has_field(U("rql")))
            {
                rql_query = rql::parse_query(web::json::field_as_string{ U("rql") }(advanced));
            }
            // extract the experimental match flags, which extend Basic Queries with really simple per-query control of string matching
            if (advanced.has_field(U("match_type")))
            {
                match_flags = experimental::parse_match_type(web::json::field_as_string{ U("match_type") }(advanced));
            }
            basic_query.erase(U("query"));
        }
    }

    resource_paging::resource_paging(const web::json::value& flat_query_params, const nmos::tai& max_until, size_t default_limit, size_t max_limit)
        : order_by_created(false) // i.e. order by updated timestamp
        , until(max_until)
        , since(nmos::tai_min())
        , limit(default_limit)
        , since_specified(false)
    {
        auto query_params = web::json::unflatten(flat_query_params);

        // extract the supported paging options
        if (query_params.has_field(U("paging")))
        {
            auto& paging = query_params.at(U("paging"));
            if (paging.has_field(U("order")))
            {
                // paging.order is "create" or "update"
                // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/APIs/QueryAPI.raml#L40
                order_by_created = U("create") == web::json::field_as_string{ U("order") }(paging);
            }
            if (paging.has_field(U("until")))
            {
                until = nmos::parse_version(web::json::field_as_string{ U("until") }(paging));
                // "These parameters may be used to construct a URL which would return the same set of bounded data on consecutive requests"
                // therefore the response to a request with 'until' in the future should be fixed up...
                if (until > max_until) until = max_until;
            }
            if (paging.has_field(U("since")))
            {
                since = nmos::parse_version(web::json::field_as_string{ U("since") }(paging));
                since_specified = true;
                // how should a request with 'since' in the future be fixed up?
                if (since > until && until == max_until) until = since;
            }
            if (paging.has_field(U("limit")))
            {
                // "If the client had requested a page size which the server is unable to honour, the actual page size would be returned"
                limit = utility::istringstreamed<size_t>(web::json::field_as_string{ U("limit") }(paging));
                if (limit > max_limit) limit = max_limit;
            }
        }
    }

    bool resource_paging::valid() const
    {
        return since <= until;
    }

    // Extend RQL with some NMOS-specific types

    web::json::value equal_to(const web::json::value& lhs, const web::json::value& rhs)
    {
        if (!rql::is_typed_value(lhs) && !rql::is_typed_value(rhs))
        {
            return rql::default_equal_to(lhs, rhs);
        }

        auto& ltype = (rql::is_typed_value(lhs) ? lhs : rhs).at(U("type")).as_string();
        auto& rtype = (rql::is_typed_value(rhs) ? rhs : lhs).at(U("type")).as_string();
        if (ltype == rtype)
        {
            auto& lvalue = rql::is_typed_value(lhs) ? lhs.at(U("value")) : lhs;
            auto& rvalue = rql::is_typed_value(rhs) ? rhs.at(U("value")) : rhs;
            if (U("api_version") == rtype)
            {
                return parse_api_version(lvalue.as_string()) == parse_api_version(rvalue.as_string()) ? rql::value_true : rql::value_false;
            }
            else if (U("version") == rtype)
            {
                return parse_version(lvalue.as_string()) == parse_version(rvalue.as_string()) ? rql::value_true : rql::value_false;
            }
        }

        return rql::value_indeterminate;
    }

    web::json::value less(const web::json::value& lhs, const web::json::value& rhs)
    {
        if (!rql::is_typed_value(lhs) && !rql::is_typed_value(rhs))
        {
            return rql::default_less(lhs, rhs);
        }

        auto& ltype = (rql::is_typed_value(lhs) ? lhs : rhs).at(U("type")).as_string();
        auto& rtype = (rql::is_typed_value(rhs) ? rhs : lhs).at(U("type")).as_string();
        if (ltype == rtype)
        {
            auto& lvalue = rql::is_typed_value(lhs) ? lhs.at(U("value")) : lhs;
            auto& rvalue = rql::is_typed_value(rhs) ? rhs.at(U("value")) : rhs;
            if (U("api_version") == rtype)
            {
                return parse_api_version(lvalue.as_string()) < parse_api_version(rvalue.as_string()) ? rql::value_true : rql::value_false;
            }
            else if (U("version") == rtype)
            {
                return parse_version(lvalue.as_string()) < parse_version(rvalue.as_string()) ? rql::value_true : rql::value_false;
            }
        }

        return rql::value_indeterminate;
    }

    bool match_rql(const web::json::value& value, const web::json::value& query)
    {
        return query.is_null() || rql::evaluator
        {
            [&value](web::json::value& results, const web::json::value& key)
            {
                return web::json::extract(value.as_object(), results, key.as_string());
            },
            rql::default_any_operators(equal_to, less)
        }(query) == web::json::value::boolean(true);
    }

    resource_query::result_type resource_query::operator()(const nmos::api_version& resource_version, const nmos::type& resource_type, const web::json::value& resource_data) const
    {
        return (resource_path.empty() || resource_path == U('/') + nmos::resourceType_from_type(resource_type))
            && nmos::is_permitted_downgrade(resource_version, resource_type, version, downgrade_version)
            && web::json::match_query(resource_data, basic_query, match_flags)
            && match_rql(resource_data, rql_query);
    }

    // Helpers for constructing /subscriptions websocket grains

    namespace details
    {
        bool is_queryable_resource(const nmos::type& type)
        {
            return type != types::subscription && type != types::grain;
        }

        void set_grain_timestamp(web::json::value& message, const nmos::tai& tai)
        {
            const auto timestamp = web::json::value::string(nmos::make_version(tai));
            message[U("origin_timestamp")] = timestamp;
            message[U("sync_timestamp")] = timestamp;
            message[U("creation_timestamp")] = timestamp;
        }

        nmos::tai get_grain_timestamp(const web::json::value& message)
        {
            return parse_version(nmos::fields::sync_timestamp(message));
        }

        web::json::value make_grain(const nmos::id& source_id, const nmos::id& flow_id, const utility::string_t& topic)
        {
            using web::json::value;

            // seems worthwhile to keep_order for simple visualisation
            value result = value::object(true);

            result[U("grain_type")] = JU("event");
            result[U("source_id")] = value::string(source_id);
            result[U("flow_id")] = value::string(flow_id);
            set_grain_timestamp(result, {});
            result[U("rate")] = make_rational();
            result[U("duration")] = make_rational();
            value& grain = result[U("grain")] = value::object(true);
            grain[U("type")] = JU("urn:x-nmos:format:data.event");
            grain[U("topic")] = value::string(topic);
            grain[U("data")] = value::array();

            return result;
        }

        web::json::value make_resource_event(const utility::string_t& resource_path, const nmos::type& type, const web::json::value& pre, const web::json::value& post)
        {
            // !resource_path.empty() must imply resource_path == U('/') + nmos::resourceType_from_type(type)

            // seems worthwhile to keep_order for simple visualisation
            web::json::value result = web::json::value::object(true);

            const auto id = (!pre.is_null() ? pre : post).at(U("id")).as_string();
            result[U("path")] = web::json::value::string(resource_path.empty() ? nmos::resourceType_from_type(type) + U('/') + id : id);
            if (!pre.is_null()) result[U("pre")] = pre;
            if (!post.is_null()) result[U("post")] = post;

            return result;
        }
    }

    // make the initial 'sync' resource events for a new grain, including all resources that match the specified version, resource path and flat query parameters
    web::json::value make_resource_events(const nmos::resources& resources, const nmos::api_version& version, const utility::string_t& resource_path, const web::json::value& params)
    {
        const resource_query match(version, resource_path, params);

        std::vector<web::json::value> events;

        // resources are traversed in order of increasing creation timestamp, so that events for super-resources are inserted before events for sub-resources
        // (assuming not out-of-order insertion by the allow_invalid_resources setting, hmm)
        auto& by_created = resources.get<tags::created>();
        for (const auto& resource : by_created | boost::adaptors::reversed)
        {
            if (!details::is_queryable_resource(resource.type)) continue;

            if (match(resource))
            {
                events.push_back(details::make_resource_event(resource_path, resource.type, resource.data, resource.data));
            }
        }

        return web::json::value_from_elements(events);
    }

    // insert 'added', 'removed' or 'modified' resource events into all grains whose subscriptions match the specified version, type and "pre" or "post" values
    void insert_resource_events(nmos::resources& resources, const nmos::api_version& version, const nmos::type& type, const web::json::value& pre, const web::json::value& post)
    {
        using utility::string_t;
        using web::json::value;

        if (!details::is_queryable_resource(type)) return;

        auto& by_type = resources.get<tags::type>();
        const auto subscriptions = by_type.equal_range(nmos::types::subscription);
        for (auto it = subscriptions.first; subscriptions.second != it; ++it)
        {
            // for each subscription
            const auto& subscription = *it;

            // check whether the resource_path matches the resource type and the query parameters match either the "pre" or "post" resource

            const auto resource_path = nmos::fields::resource_path(subscription.data);
            resource_query match(subscription.version, resource_path, subscription.data.at(U("params")));

            const bool pre_match = match(version, type, pre);
            const bool post_match = match(version, type, post);

            if (!pre_match && !post_match) continue;

            // add the event to the grain for each websocket connection to this subscription

            const value event = details::make_resource_event(resource_path, type, pre_match ? pre : value::null(), post_match ? post : value::null());

            for (const auto& id : subscription.sub_resources)
            {
                auto grain = find_resource(resources, { id, nmos::types::grain });
                if (resources.end() == grain) continue; // check websocket connection is still open

                resources.modify(grain, [&resources, &event](nmos::resource& grain)
                {
                    auto& events = nmos::fields::message_grain_data(grain.data);
                    web::json::push_back(events, event);
                    grain.updated = strictly_increasing_update(resources);
                });
            }
        }
    }
}
