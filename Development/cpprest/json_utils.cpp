#include "cpprest/json_utils.h"

#include <boost/algorithm/string/find.hpp>
#include <boost/algorithm/string/predicate.hpp>

// json query helpers
namespace web
{
    namespace json
    {
        namespace details
        {
            inline utility::string_t::size_type count(utility::string_t::size_type first, utility::string_t::size_type last)
            {
                return utility::string_t::npos == last ? utility::string_t::npos : last - first;
            }
        }

        // insert a field into the specified object at the specified key path (splitting it on '.' and inserting sub-objects as necessary)
        // only if the value doesn't already contain a field matching that key path (except for the required sub-objects or null values)
        bool insert(web::json::object& object, const utility::string_t& key_path, const web::json::value& field_value)
        {
            web::json::object* pobject = &object;
            utility::string_t::size_type key_first = 0;
            do
            {
                const utility::string_t::size_type key_last = key_path.find_first_of(U("."), key_first);
                const utility::string_t key = key_path.substr(key_first, details::count(key_first, key_last));
                if (utility::string_t::npos != key_last)
                {
                    auto& field = (*pobject)[key];
                    // do not replace (non-null) values for duplicate keys; other policies (replace, promote to array, or throw) would be possible...
                    if (field.is_null()) field = web::json::value::object();
                    else if (!field.is_object()) return false;
                    pobject = &field.as_object();
                }
                else
                {
                    auto& field = (*pobject)[key];
                    // do not replace (non-null) values for duplicate keys; other policies (replace, promote to array, or throw) would be possible...
                    if (field.is_null()) field = field_value;
                    else return false;
                }
                key_first = utility::string_t::npos != key_last ? key_last + 1 : key_last;
            } while (utility::string_t::npos != key_first);
            return true;
        }

        // find the value of a field or fields from the specified object, splitting the key path on '.' and searching arrays as necessary
        // returns true if the value has at least one field matching the key path
        // if any arrays are encountered on the key path, results is an array, otherwise it's a non-array value
        bool extract(const web::json::object& object, web::json::value& results, const utility::string_t& key_path)
        {
            bool match = false;
            results = web::json::value::null();

            std::list<const web::json::object*> pobjects(1, &object);
            utility::string_t::size_type key_first = 0;
            do
            {
                const utility::string_t::size_type key_last = key_path.find_first_of(U("."), key_first);
                const utility::string_t key = key_path.substr(key_first, details::count(key_first, key_last));
                if (utility::string_t::npos != key_last)
                {
                    // not the leaf key, so map each object to the specified field, searching arrays and filtering out other types
                    for (auto it = pobjects.begin(); pobjects.end() != it; it = pobjects.erase(it))
                    {
                        auto& object = **it;
                        auto found = object.find(key);
                        if (object.end() != found)
                        {
                            auto& field = found->second;
                            if (field.is_array())
                            {
                                // encountered an array
                                if (!results.is_array())
                                {
                                    results = web::json::value::array();
                                }

                                for (auto& element : field.as_array())
                                {
                                    if (element.is_object())
                                    {
                                        pobjects.insert(it, &element.as_object());
                                    }
                                }
                            }
                            else if (field.is_object())
                            {
                                pobjects.insert(it, &field.as_object());
                            }
                        }
                    }
                }
                else
                {
                    // leaf key, so map each object to the specified field, merging arrays into results
                    for (auto& pobject : pobjects)
                    {
                        auto& object = *pobject;
                        auto found = object.find(key);
                        if (object.end() != found)
                        {
                            auto& field = found->second;
                            if (!results.is_array())
                            {
                                // field must be the only result(s); i.e. match currently false
                                // field may be any type of value though, including null or an array
                                results = field;
                            }
                            else
                            {
                                if (field.is_array())
                                {
                                    for (auto& element : field.as_array())
                                    {
                                        web::json::push_back(results, element);
                                    }
                                }
                                else
                                {
                                    web::json::push_back(results, field);
                                }
                            }

                            // found a match!
                            if (!match) match = true;
                        }
                    }
                }
                key_first = utility::string_t::npos != key_last ? key_last + 1 : key_last;
            } while (utility::string_t::npos != key_first);

            return match;
        }

        // construct an ordered parameters object from a URI-encoded query string of '&' or ';' separated terms expected to be field=value pairs
        // field names will be URI-decoded, but values will be left as-is!
        // cf. web::uri::split_query
        web::json::value value_from_query(const utility::string_t& query)
        {
            web::json::value result = web::json::value::object(true); // keep order
            utility::string_t::size_type field_first = 0;
            do
            {
                const utility::string_t::size_type field_last = query.find_first_of(U("=&;"), field_first);
                const utility::string_t::size_type value_first = utility::string_t::npos != field_last && U('=') == query[field_last] ? field_last + 1 : field_last;
                const utility::string_t::size_type value_last = query.find_first_of(U("&;"), value_first);
                const utility::string_t field = uri::decode(query.substr(field_first, details::count(field_first, field_last)));
                // could distinguish the two cases (no '=' vs. from empty value) but at the moment, map both to empty string
                const utility::string_t value = utility::string_t::npos == value_first ? utility::string_t() : query.substr(value_first, details::count(value_first, value_last));
                if (!field.empty() || !value.empty())
                {
                    result[field] = web::json::value::string(value);
                }
                field_first = utility::string_t::npos != value_last ? value_last + 1 : value_last;
            } while (utility::string_t::npos != field_first);
            return result;
        }

        // construct a query string of '&' separated terms from a parameters object
        // field names will be URI-encoded, but values will be left as-is!
        utility::string_t query_from_value(const web::json::value& params)
        {
            utility::ostringstream_t query;
            bool first = true;
            for (auto& param : params.as_object())
            {
                if (first) first = false; else query << U('&');
                query << uri::encode_uri(param.first, uri::components::query) << U('=') << param.second.as_string();
            }
            return query.str();
        }

        // construct a query/exemplar object from a parameters object, by constructing nested sub-objects from '.'-separated field names
        web::json::value unflatten(const web::json::value& params)
        {
            web::json::value result = web::json::value::object();
            for (auto& param : params.as_object())
            {
                // later duplicate query terms are ignored, as a result of current insert policy above
                insert(result.as_object(), param.first, param.second);
            }
            return result;
        }

        // compare a value against a query/exemplar
        bool match_query(const web::json::value& value, const web::json::value& query, match_flag_type match_flags)
        {
            if (value.is_array())
            {
                // one of the value's elements must match the query
                for (auto& element : value.as_array())
                {
                    if (match_query(element, query, match_flags))
                    {
                        return true;
                    }
                }
                return false;
            }
            else if (value.is_object() && query.is_object())
            {
                // value must have fields matching all of the query fields, but other value fields are ignored
                for (auto& query_field : query.as_object())
                {
                    if (!value.has_field(query_field.first) || !match_query(value.at(query_field.first), query_field.second, match_flags))
                    {
                        return false;
                    }
                }
                return true;
            }
            else if (value.type() == query.type())
            {
                if (query.is_string())
                {
                    return 0 != (match_substr & match_flags)
                        // value must contain the query as a substring (optionally case-insensitive)
                        ? 0 != (match_icase & match_flags)
                            ? (bool)boost::algorithm::ifind_first(value.as_string(), query.as_string())
                            : (bool)boost::algorithm::find_first(value.as_string(), query.as_string())
                        // value must be an exact match (optionally case-insensitive)
                        : 0 != (match_icase & match_flags)
                            ? boost::algorithm::iequals(value.as_string(), query.as_string())
                            : boost::algorithm::equals(value.as_string(), query.as_string());
                }
                else
                {
                    // value must be an exact match
                    return value == query;
                }
            }
            else if (query.is_string()) // && !value.is_object() could be a good idea
            {
                // last resort; treat the query string as serialized json...
                // it'd feel less risky to compare value.serialize() to the query string but
                // that wouldn't handle different string representations, hmm.
                std::error_code ec;
                web::json::value parsed_query = web::json::value::parse(query.as_string(), ec);
                return !ec && match_query(value, parsed_query, match_flags);
            }
            else
            {
                return false;
            }
        }
    }
}
