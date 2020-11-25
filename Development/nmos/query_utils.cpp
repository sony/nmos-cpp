#include "nmos/query_utils.h"

#include <set>
#include <boost/algorithm/string/erase.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/range/adaptor/reversed.hpp>
#include "cpprest/basic_utils.h"
#include "nmos/api_downgrade.h"
#include "nmos/api_utils.h" // for nmos::resourceType_from_type
#include "nmos/rational.h"
#include "nmos/sdp_utils.h" // for nmos::details::make_sampling
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
        , strip(true)
        , match_flags(web::json::match_default)
    {
        // extract the supported advanced query options
        if (basic_query.has_field(U("paging")))
        {
            basic_query.erase(U("paging"));
        }
        if (basic_query.has_field(U("query")))
        {
            auto& advanced = basic_query.at(U("query")).as_object();
            for (auto& field : advanced)
            {
                if (field.first == U("downgrade"))
                {
                    downgrade_version = parse_api_version(field.second.as_string());
                }
                else if (field.first == U("rql"))
                {
                    rql_query = rql::parse_query(field.second.as_string());
                }
                // extract the experimental flag, used to override the default behaviour that resources
                // "must have all [higher-versioned] keys stripped by the Query API before they are returned"
                // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/docs/2.5.%20APIs%20-%20Query%20Parameters.md#downgrade-queries
                else if (field.first == U("strip"))
                {
                    strip = field.second.as_bool();
                }
                // extract the experimental match flags, which extend Basic Queries with really simple per-query control of string matching
                else if (field.first == U("match_type"))
                {
                    match_flags = experimental::parse_match_type(field.second.as_string());
                }
                // taking query.ancestry_id as an example, an error should be reported for unimplemented parameters
                // "A 501 HTTP status code should be returned where an ancestry query is attempted against a Query API which does not implement it."
                // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/docs/2.5.%20APIs%20-%20Query%20Parameters.md#ancestry-queries-optional
                else
                {
                    throw std::runtime_error("unimplemented parameter - query." + utility::us2s(field.first));
                }
            }
            basic_query.erase(U("query"));
        }
    }

    resource_paging::resource_paging(const web::json::value& flat_query_params, const nmos::tai& max_until, size_t default_limit, size_t max_limit)
        : order_by_created(false) // i.e. order by updated timestamp
        , until(nmos::tai_max())
        , since(nmos::tai_min())
        , limit(default_limit)
        , since_specified(false)
    {
        auto query_params = web::json::unflatten(flat_query_params);

        // extract the supported paging options
        if (query_params.has_field(U("paging")))
        {
            auto& paging = query_params.at(U("paging")).as_object();
            for (auto& field : paging)
            {
                if (field.first == U("order"))
                {
                    // paging.order is "create" or "update"
                    // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/APIs/QueryAPI.raml#L40
                    order_by_created = U("create") == field.second.as_string();
                }
                else if (field.first == U("until"))
                {
                    until = nmos::parse_version(field.second.as_string());
                }
                else if (field.first == U("since"))
                {
                    since = nmos::parse_version(field.second.as_string());
                    since_specified = true;
                }
                else if (field.first == U("limit"))
                {
                    // "If the client had requested a page size which the server is unable to honour, the actual page size would be returned"
                    limit = (size_t)field.second.as_integer();
                    if (limit > max_limit) limit = max_limit;
                }
                // as for resource_query, an error is reported for unimplemented parameters
                else
                {
                    throw std::runtime_error("unimplemented parameter - paging." + utility::us2s(field.first));
                }
            }
        }

        if (valid())
        {
            // fix up a valid request which has 'since' in the future (where max_until is 'now') to represent an empty interval at that time
            if (max_until < since) until = since;
            // "These parameters may be used to construct a URL which would return the same set of bounded data on consecutive requests"
            // therefore also fix up a valid request which has 'until' in the future...
            else if (max_until < until) until = max_until;
        }
    }

    bool resource_paging::valid() const
    {
        return since <= until;
    }

    namespace details
    {
        // make user error information (to be used with status_codes::BadRequest)
        utility::string_t make_valid_paging_error(const nmos::resource_paging& paging)
        {
            return U("the value of the 'paging.since' parameter must be less than or equal to the effective value of the 'paging.until' parameter");
        }

        template <typename T>
        static inline T istringstreamed(const utility::string_t& value, const T& default_value = {})
        {
            T result{ default_value }; std::istringstream is(utility::us2s(value)); is >> result; return result;
        }
    }

    // Extend RQL with some NMOS-specific types

    web::json::value equal_to(const web::json::value& lhs, const web::json::value& rhs)
    {
        if (!rql::is_typed_value(lhs) && !rql::is_typed_value(rhs))
        {
            return rql::default_equal_to(lhs, rhs);
        }

        // if either value is null/not found, return indeterminate
        if (lhs.is_null() || rhs.is_null()) return rql::value_indeterminate;

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
            else if (U("rational") == rtype)
            {
                const auto lrat = lvalue.is_string() ? details::istringstreamed<nmos::rational>(lvalue.as_string()) : parse_rational(lvalue);
                const auto rrat = rvalue.is_string() ? details::istringstreamed<nmos::rational>(rvalue.as_string()) : parse_rational(rvalue);
                return lrat == rrat ? rql::value_true : rql::value_false;
            }
            else if (U("sampling") == rtype)
            {
                const auto lsam = lvalue.is_string() ? lvalue.as_string() : details::make_sampling(lvalue.as_array()).name;
                const auto rsam = rvalue.is_string() ? rvalue.as_string() : details::make_sampling(rvalue.as_array()).name;
                return lsam == rsam ? rql::value_true : rql::value_false;
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

        // if either value is null/not found, return indeterminate
        if (lhs.is_null() || rhs.is_null()) return rql::value_indeterminate;

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
            else if (U("rational") == rtype)
            {
                const auto lrat = lvalue.is_string() ? details::istringstreamed<nmos::rational>(lvalue.as_string()) : parse_rational(lvalue);
                const auto rrat = rvalue.is_string() ? details::istringstreamed<nmos::rational>(rvalue.as_string()) : parse_rational(rvalue);
                return lrat < rrat ? rql::value_true : rql::value_false;
            }
        }

        return rql::value_indeterminate;
    }

    static inline utility::string_t erase_tail_copy(const utility::string_t& input, const utility::string_t& tail)
    {
        return boost::algorithm::ends_with(input, tail)
            ? boost::algorithm::erase_tail_copy(input, (int)tail.size())
            : input;
    }

    static inline rql::extractor make_extractor(const web::json::value& value)
    {
        return [&value](web::json::value& results, const web::json::value& key)
        {
            return web::json::extract(value.as_object(), results, key.as_string());
        };
    }

    namespace experimental
    {
        // Sub-query operators
        // Form: relation(<relation-name>, <call-operator>) - Filters for objects where the value identified by the specified relation (i.e. a property) satisfies the specified sub-query

        // Array-friendly sub-query operators
        // Filters for objects as above or where the specified property's value is an array and the value identified by any element of the array satisfies the specified sub-query
        template <typename ResolveRelation>
        web::json::value relation_query(const rql::evaluator& eval, const web::json::value& args, ResolveRelation resolve)
        {
            const auto relation_name = eval(args.at(0)).as_string();
            const auto relation_value = eval(args.at(0), true);
            const auto query = args.at(1);

            const auto& operators = eval.operators;
            const auto rel = [&resolve, &relation_name, &operators, &query](const web::json::value& relation_value)
            {
                // evaluate the call-operator against the specified data
                return rql::evaluator{ make_extractor(resolve(relation_name, relation_value)), operators }(query);
            };

            // cf. rql::details::logical_or
            if (!relation_value.is_array())
            {
                return rel(relation_value);
            }
            bool indeterminate = false;
            for (const auto& rv : relation_value.as_array())
            {
                auto result = rel(rv);
                if (!result.is_boolean())
                {
                    indeterminate = true;
                }
                else if (result.as_bool())
                {
                    return rql::value_true;
                }
            }
            return indeterminate ? rql::value_indeterminate : rql::value_false;
        }

        // Experimental support for the 'rel' operator, in order to allow e.g. matching senders based on their flows' formats
        // rel(<relation-name>, <call-operator>) - Applies the provided call-operator against the linked data of the provided relation-name
        web::json::value rel(const nmos::resources& resources, const rql::evaluator& eval, const web::json::value& args)
        {
            return relation_query(eval, args, [&resources](const utility::string_t& relation_name, const web::json::value& relation_value)
            {
                if (relation_value.is_string())
                {
                    // initially support relation-names that are the '<type>_id' properties, such as a sender's flow_id
                    // and the 'subscription.sender_id' property of receivers (and vice-versa of senders)
                    // other candidates are more complicated for various reasons...
                    // for the 'parents' properties of sources and flows, the resource type is also needed
                    // for 'interface_bindings' and 'clock_name', device_id and node_id are also needed to identify the node resource
                    // some 'href' properties like the 'manifest_href' of a sender could be interesting
                    nmos::type relation_type{ erase_tail_copy(relation_name.substr(relation_name.find_last_of(U('.')) + 1), U("_id")) };
                    nmos::id relation_id{ relation_value.as_string() };

                    auto found = find_resource(resources, { relation_id, relation_type });
                    if (resources.end() == found) return rql::value_indeterminate;

                    // return the linked value
                    return found->data;
                }
                return web::json::value::object();
            });
        }

        // Experimental support for a 'sub' operator, in order to allow e.g. matching on multiple properties of objects in arrays
        // sub(<property>, <call-operator>) - Applies the provided call-operator against the provided property
        web::json::value sub(const rql::evaluator& eval, const web::json::value& args)
        {
            return relation_query(eval, args, [](const utility::string_t& relation_name, const web::json::value& relation_value)
            {
                if (relation_value.is_object())
                {
                    // effectively just changes the extractor context to a sub-object
                    return relation_value;
                }
                return web::json::value::object();
            });
        }
    }

    bool match_rql(const web::json::value& value, const web::json::value& query, const nmos::resources& resources)
    {
        auto operators = rql::default_any_operators(equal_to, less);

        operators[U("rel")] = std::bind(experimental::rel, std::cref(resources), std::placeholders::_1, std::placeholders::_2);
        operators[U("sub")] = experimental::sub;

        return query.is_null() || rql::evaluator{ make_extractor(value), operators }(query) == rql::value_true;
    }

    resource_query::result_type resource_query::operator()(const nmos::api_version& resource_version, const nmos::api_version& resource_downgrade_version, const nmos::type& resource_type, const web::json::value& resource_data, const nmos::resources& resources) const
    {
        // in theory, should be performing match_query against the downgraded resource_data but
        // in practice, I don't think that can make a difference?
        return !resource_data.is_null()
            && (resource_path.empty() || resource_path == U('/') + nmos::resourceType_from_type(resource_type))
            && nmos::is_permitted_downgrade(resource_version, resource_downgrade_version, resource_type, version, downgrade_version)
            && web::json::match_query(resource_data, basic_query, match_flags)
            && match_rql(resource_data, rql_query, resources);
    }

    web::json::value resource_query::downgrade(const nmos::api_version& resource_version, const nmos::api_version& resource_downgrade_version, const nmos::type& resource_type, const web::json::value& resource_data) const
    {
        // when requested, return the resource not stripped
        if (!strip && resource_version.major == version.major && resource_version.minor > version.minor) return resource_data;

        return nmos::downgrade(resource_version, resource_downgrade_version, resource_type, resource_data, version, downgrade_version);
    }

    // Helpers for constructing /subscriptions websocket grains

    namespace details
    {
        bool is_queryable_resource(const nmos::type& type)
        {
            return type != types::subscription && type != types::grain;
        }

        web::json::value make_grain(const nmos::id& source_id, const nmos::id& flow_id, const utility::string_t& topic)
        {
            using web::json::value;

            // seems worthwhile to keep_order for simple visualisation
            value result = value::object(true);

            result[U("grain_type")] = value(U("event"));
            result[U("source_id")] = value::string(source_id);
            result[U("flow_id")] = value::string(flow_id);
            const auto timestamp = value::string(nmos::make_version(tai{}));
            result[nmos::fields::origin_timestamp] = timestamp;
            result[nmos::fields::sync_timestamp] = timestamp;
            result[nmos::fields::creation_timestamp] = timestamp;
            result[U("rate")] = make_rational();
            result[U("duration")] = make_rational();
            value& grain = result[U("grain")] = value::object(true);
            grain[U("type")] = value(U("urn:x-nmos:format:data.event"));
            grain[U("topic")] = value::string(topic);
            grain[U("data")] = value::array();

            return result;
        }

        web::json::value make_resource_event(const utility::string_t& resource_path, const nmos::type& type, const web::json::value& pre, const web::json::value& post)
        {
            // !resource_path.empty() must imply resource_path == U('/') + nmos::resourceType_from_type(type)

            // seems worthwhile to keep_order for simple visualisation
            web::json::value result = web::json::value::object(true);

            const auto id = nmos::fields::id(!pre.is_null() ? pre : post);
            result[U("path")] = web::json::value::string(resource_path.empty() ? nmos::resourceType_from_type(type) + U('/') + id : id);
            if (!pre.is_null()) result[U("pre")] = pre;
            if (!post.is_null()) result[U("post")] = post;

            return result;
        }

        // get the resource id and type from the grain topic and event "path"
        std::pair<nmos::id, nmos::type> get_resource_event_resource(const utility::string_t& topic, const web::json::value& event)
        {
            const auto topic_path = topic + event.at(U("path")).as_string();
            // topic path should be like "/{resourceType}/{resourceId}"
            auto slash = topic_path.find(U('/'), 1);
            if (utility::string_t::npos != slash)
                return{ topic_path.substr(slash + 1), nmos::type_from_resourceType(topic_path.substr(1, slash - 1)) };
            else
                return{};
        }

        // determine the type of the resource event from "pre" and "post"
        resource_event_type get_resource_event_type(const web::json::value& event)
        {
            const bool has_pre = event.has_field(U("pre"));
            const bool has_post = event.has_field(U("post"));

            if (!has_pre && !has_post)
            {
                return resource_continued_nonexistence_event;
            }
            else if (!has_pre && has_post)
            {
                return resource_added_event;
            }
            else if (has_pre && !has_post)
            {
                return resource_removed_event;
            }
            else // if (has_pre && has_post)
            {
                return event.at(U("pre")) != event.at(U("post")) ? resource_modified_event : resource_unchanged_event;
            }
        }
    }

    // make the initial 'sync' resource events for a new grain, including all resources that match the specified version, resource path and flat query parameters
    // optionally, make 'added' resource events instead of 'sync' events
    web::json::value make_resource_events(const nmos::resources& resources, const nmos::api_version& version, const utility::string_t& resource_path, const web::json::value& params, bool sync)
    {
        const resource_query match(version, resource_path, params);

        std::vector<web::json::value> events;

        // resources are traversed in order of nmos::types::all, so that events for super-resources are inserted before events for sub-resources
        // there is no defined ordering between resources of the same type
        for (const auto& type : nmos::types::all)
        {
            if (!details::is_queryable_resource(type)) continue;

            auto& by_type = resources.get<tags::type>();
            const auto range = by_type.equal_range(details::has_data(type));
            for (auto found = range.first; range.second != found; ++found)
            {
                auto& resource = *found;

                if (!match(resource, resources)) continue;

                const auto resource_data = match.downgrade(resource);
                auto event = details::make_resource_event(resource_path, resource.type, sync ? resource_data : web::json::value::null(), resource_data);

                // experimental extension, for the query.strip flag

                // api_version: the API version of the Node API exposing this resource, omitted when equal to the subscription Query API version (an equivalent HTTP response header has been discussed for v1.3)
                // also omitted unless resource_path is empty (since that's also an extension);
                // ironically, the latter is a schema violation, but the former wouldn't be because the schema
                // does not have "additionalProperties": false
                // see nmos-discovery-registration/APIs/schemas/queryapi-subscriptions-websocket.json
                if (resource_path.empty())
                {
                    if (!match.strip || resource.version < match.version)
                    {
                        event[nmos::experimental::fields::api_version] = web::json::value::string(nmos::make_api_version(resource.version));
                    }
                }

                events.push_back(event);
            }
        }

        return web::json::value_from_elements(events);
    }

    // insert 'added', 'removed' or 'modified' resource events into all grains whose subscriptions match the specified version, type and "pre" or "post" values
    void insert_resource_events(nmos::resources& resources, const nmos::api_version& version, const nmos::api_version& downgrade_version, const nmos::type& type, const web::json::value& pre, const web::json::value& post)
    {
        using utility::string_t;
        using web::json::value;

        if (!details::is_queryable_resource(type)) return;

        auto& by_type = resources.get<tags::type>();
        const auto subscriptions = by_type.equal_range(details::has_data(nmos::types::subscription));
        for (auto it = subscriptions.first; subscriptions.second != it; ++it)
        {
            // for each subscription
            const auto& subscription = *it;

            // check whether the resource_path matches the resource type and the query parameters match either the "pre" or "post" resource

            const auto resource_path = nmos::fields::resource_path(subscription.data);
            const resource_query match(subscription.version, resource_path, nmos::fields::params(subscription.data));

            const bool pre_match = match(version, downgrade_version, type, pre, resources);
            const bool post_match = match(version, downgrade_version, type, post, resources);

            if (!pre_match && !post_match) continue;

            // add the event to the grain for each websocket connection to this subscription

            // note: downgrade just returns a copy in the case that version <= match.version
            auto event = details::make_resource_event(resource_path, type,
                pre_match ? match.downgrade(version, downgrade_version, type, pre) : value::null(),
                post_match ? match.downgrade(version, downgrade_version, type, post) : value::null()
                );

            // see explanation in nmos::make_resource_events
            if (resource_path.empty())
            {
                if (!match.strip || version < match.version)
                {
                    event[nmos::experimental::fields::api_version] = web::json::value::string(nmos::make_api_version(version));
                }
            }

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
