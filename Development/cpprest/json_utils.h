#ifndef CPPREST_JSON_UTILS_H
#define CPPREST_JSON_UTILS_H

#include "cpprest/json_ops.h" // for web::json::value, web::json::as, etc.

// since some of the constructors are explicit, provide helpers
#ifndef _TURN_OFF_PLATFORM_STRING
#define J(x) web::json::value{x}
#define JU(x) J(U(x))
#endif

// json field accessor helpers
namespace web
{
    namespace json
    {
        template <typename T> struct field
        {
            utility::string_t key;
            operator const utility::string_t&() const { return key; }
            template <typename V> auto operator()(V& value) const -> decltype(as<T>(value))
            {
                return as<T>(value.at(key));
            }
        };

        template <typename T> struct field_path
        {
            std::vector<utility::string_t> key_path;
            template <typename V> auto operator()(V& value) const -> decltype(as<T>(value))
            {
                auto pv = &value;
                for (auto& key : key_path)
                {
                    pv = &pv->at(key);
                }
                return as<T>(*pv);
            }
        };

        typedef field<web::json::value> field_as_value;
        typedef field<web::json::object> field_as_object;
        typedef field<web::json::array> field_as_array;
        typedef field<web::json::number> field_as_number;
        typedef field<utility::string_t> field_as_string;
        typedef field<bool> field_as_bool;
        typedef field<int> field_as_integer;

        template <typename T> struct field_with_default
        {
            utility::string_t key;
            T default_value;
            operator const utility::string_t&() const { return key; }
            auto operator()(const web::json::value& value) const -> decltype(as<T>(value))
            {
                web::json::object::const_iterator it;
                return value.is_object() && value.as_object().end() != (it = value.as_object().find(key)) ? as<T>(it->second) : default_value;
            }
        };

        typedef field_with_default<web::json::value> field_as_value_or;
        typedef field_with_default<utility::string_t> field_as_string_or;
        typedef field_with_default<bool> field_as_bool_or;
        typedef field_with_default<int> field_as_integer_or;
    }
}

// more json initialization helpers
// web::json::value has a rather limited interface for the construction of objects and arrays;
// value_from_fields is a helper function similar to the web::json::value::object factory function
// but which can handle e.g. std::map<utility:string_t, utility:string_t>
// value_from_elements is similarly related to the web::json::value::array factory function
namespace web
{
    namespace json
    {
        template <typename KeyValuePairs>
        inline web::json::value value_from_fields(const KeyValuePairs& fields, bool keep_order = false)
        {
            web::json::value result = web::json::value::object(keep_order);
            insert(result, std::begin(fields), std::end(fields));
            return result;
        }

        template <typename Values>
        inline web::json::value value_from_elements(const Values& elements)
        {
            web::json::value result = web::json::value::array();
            for (const auto& element : elements)
            {
                push_back(result, element);
            }
            return result;
        }
    }
}

// json serialization helpers
namespace web
{
    namespace json
    {
        // serialize a forward range of json values (e.g. vector, list) as an array
        template <typename ForwardRange>
        inline void serialize_array(utility::ostream_t& os, const ForwardRange& elements)
        {
            os.put(_XPLATSTR('['));
            bool empty = true;
            for (const auto& element : elements)
            {
                if (!empty)
                {
                    os.put(_XPLATSTR(','));
                }
                else
                {
                    empty = false;
                }
                element.serialize(os);
            }
            os.put(_XPLATSTR(']'));
        }

        // serialize a forward range of pairs of strings and json values (e.g. map) as an object
        template <typename ForwardRange>
        inline void serialize_object(utility::ostream_t& os, const ForwardRange& fields)
        {
            os.put(_XPLATSTR('{'));
            bool empty = true;
            for (const auto& field : fields)
            {
                if (!empty)
                {
                    os.put(_XPLATSTR(','));
                }
                else
                {
                    empty = false;
                }
                value::string(std::get<0>(field)).serialize(os);
                os.put(_XPLATSTR(':'));
                std::get<1>(field).serialize(os);
            }
            os.put(_XPLATSTR('}'));
        }

        // serialize a forward range of json values (e.g. vector, list) as an array
        template <typename ForwardRange>
        inline utility::string_t serialize_array(const ForwardRange& elements)
        {
            utility::ostringstream_t os;
            serialize_array(os, elements);
            return os.str();
        }

        // serialize a forward range of pairs of strings and json values (e.g. map) as an object
        template <typename ForwardRange>
        inline utility::string_t serialize_object(const ForwardRange& fields)
        {
            utility::ostringstream_t os;
            serialize_object(os, fields);
            return os.str();
        }

        // filter, transform and serialize a forward range (e.g. vector, list) of json values as an array
        // deprecated, use serialize_array with e.g. boost::adaptors::filtered/transformed
        template <typename ForwardRange, typename Pred, typename Transform>
        inline void serialize_if(utility::ostream_t& os, const ForwardRange& range, Pred pred, Transform transform)
        {
            os << _XPLATSTR('[');
            bool empty = true;
            for (const auto& element : range)
            {
                if (pred(element))
                {
                    if (!empty)
                    {
                        os << _XPLATSTR(',');
                    }
                    else
                    {
                        empty = false;
                    }
                    transform(element).serialize(os);
                }
            }
            os << _XPLATSTR(']');
        }

        // deprecated, use serialize_array with e.g. boost::adaptors::filtered/transformed
        template <typename ForwardRange, typename Pred, typename Transform>
        inline utility::string_t serialize_if(const ForwardRange& range, Pred pred, Transform transform)
        {
            utility::ostringstream_t os;
            serialize_if(os, range, pred, transform);
            return os.str();
        }

        // deprecated, use serialize_array with e.g. boost::adaptors::filtered
        template <typename ForwardRange, typename Pred>
        inline utility::string_t serialize_if(const ForwardRange& range, Pred pred)
        {
            utility::ostringstream_t os;
            using std::begin;
            serialize_if(os, range, pred, [](const typename std::iterator_traits<decltype(begin(range))>::value_type& element) { return element; });
            return os.str();
        }

        // deprecated, use serialize_array with e.g. boost::adaptors::transformed
        template <typename ForwardRange, typename Transform>
        inline utility::string_t serialize(const ForwardRange& range, Transform transform)
        {
            using std::begin;
            return serialize_if(range, [](const typename std::iterator_traits<decltype(begin(range))>::value_type& element) { return true; }, transform);
        }

        // deprecated, use serialize_array
        template <typename ForwardRange>
        inline utility::string_t serialize(const ForwardRange& range)
        {
            using std::begin;
            return serialize_if(range, [](const typename std::iterator_traits<decltype(begin(range))>::value_type& element) { return true; });
        }
    }
}

// json parsing helpers
namespace web
{
    namespace json
    {
        namespace experimental
        {
            // preprocess a json-like string to remove C++/JavaScript-style comments
            utility::string_t preprocess(const utility::string_t& value);
        }
    }
}

// json query/patch helpers
namespace web
{
    namespace json
    {
        // insert a field into the specified object at the specified key path (splitting it on '.' and inserting sub-objects as necessary)
        // only if the object doesn't already contain a field matching that key path (except for the required sub-objects or null values)
        bool insert(web::json::object& object, const utility::string_t& key_path, const web::json::value& field_value);

        // find the value of a field or fields from the specified object, splitting the key path on '.' and searching arrays as necessary
        // returns true if the object has at least one field matching the key path
        // if any arrays are encountered on the key path, results is an array, otherwise it's a non-array value
        bool extract(const web::json::object& object, web::json::value& results, const utility::string_t& key_path);

        // match_flag_type is a bitmask
        enum match_flag_type
        {
            match_default = 0x0000, // string matches must be exact
            match_substr =  0x0001, // substring matches are OK
            match_icase =   0x0002  // case-insensitive matches are OK
        };
        // so for some convenience...
        inline match_flag_type operator|(match_flag_type lhs, match_flag_type rhs) { return match_flag_type((int)lhs | (int)rhs); }

        // construct an ordered parameters object from a URI-encoded query string of '&' or ';' separated terms expected to be field=value pairs
        // field names will be URI-decoded, but values will be left as-is!
        // cf. web::uri::split_query
        web::json::value value_from_query(const utility::string_t& query);

        // construct a query string of '&' separated terms from a parameters object
        // field names will be URI-encoded, but string values will be left as-is
        // other value types will be serialized before being encoded!
        utility::string_t query_from_value(const web::json::value& value);

        // construct a query/exemplar object from a parameters object, by constructing nested sub-objects from '.'-separated field names
        web::json::value unflatten(const web::json::value& value);

        // compare a value against a query/exemplar
        bool match_query(const web::json::value& value, const web::json::value& query, match_flag_type match_flags = match_default);

        // merge source into target value
        void merge_patch(web::json::value& value, const web::json::value& patch, bool permissive = false);
    }
}

#endif
