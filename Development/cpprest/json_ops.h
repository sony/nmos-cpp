#ifndef CPPREST_JSON_OPS_H
#define CPPREST_JSON_OPS_H

#include "cpprest/json.h"

// more comparison operators for json::number, json::object, json::array and json::value
namespace web
{
    namespace json
    {
        inline bool operator!=(const web::json::number& lhs, const web::json::number& rhs)
        {
            return !(lhs == rhs);
        }

        inline bool operator<(const web::json::number& lhs, const web::json::number& rhs)
        {
            if (!lhs.is_integral() && !rhs.is_integral()) return lhs.to_double() < rhs.to_double();
            if (lhs.is_int64() && rhs.is_int64()) return lhs.to_int64() < rhs.to_int64();
            if (lhs.is_uint64() && rhs.is_uint64()) return lhs.to_uint64() < rhs.to_uint64();
            return false;
        }

        inline bool operator>(const web::json::number& lhs, const web::json::number& rhs)
        {
            return rhs < lhs;
        }

        inline bool operator<=(const web::json::number& lhs, const web::json::number& rhs)
        {
            return !(rhs > lhs);
        }

        inline bool operator>=(const web::json::number& lhs, const web::json::number& rhs)
        {
            return !(lhs < rhs);
        }

        // json::object comparison respects keep_order
        inline bool operator==(const web::json::object& lhs, const web::json::object& rhs)
        {
            if (lhs.size() != rhs.size())
                return false;
            using std::begin;
            using std::end;
            return std::equal(begin(lhs), end(lhs), begin(rhs));
        }

        inline bool operator!=(const web::json::object& lhs, const web::json::object& rhs)
        {
            return !(lhs == rhs);
        }

        inline bool operator<(const web::json::object& lhs, const web::json::object& rhs)
        {
            using std::begin;
            using std::end;
            return std::lexicographical_compare(begin(lhs), end(lhs), begin(rhs), end(rhs));
        }

        inline bool operator>(const web::json::object& lhs, const web::json::object& rhs)
        {
            return rhs < lhs;
        }

        inline bool operator<=(const web::json::object& lhs, const web::json::object& rhs)
        {
            return !(rhs > lhs);
        }

        inline bool operator>=(const web::json::object& lhs, const web::json::object& rhs)
        {
            return !(lhs < rhs);
        }

        inline bool operator==(const web::json::array& lhs, const web::json::array& rhs)
        {
            if (lhs.size() != rhs.size())
                return false;
            using std::begin;
            using std::end;
            return std::equal(begin(lhs), end(lhs), begin(rhs));
        }

        inline bool operator!=(const web::json::array& lhs, const web::json::array& rhs)
        {
            return !(lhs == rhs);
        }

        inline bool operator<(const web::json::array& lhs, const web::json::array& rhs)
        {
            using std::begin;
            using std::end;
            return std::lexicographical_compare(begin(lhs), end(lhs), begin(rhs), end(rhs));
        }

        inline bool operator>(const web::json::array& lhs, const web::json::array& rhs)
        {
            return rhs < lhs;
        }

        inline bool operator<=(const web::json::array& lhs, const web::json::array& rhs)
        {
            return !(rhs > lhs);
        }

        inline bool operator>=(const web::json::array& lhs, const web::json::array& rhs)
        {
            return !(lhs < rhs);
        }

        inline bool operator<(const web::json::value& lhs, const web::json::value& rhs)
        {
            if (lhs.type() < rhs.type()) return true;
            if (rhs.type() == lhs.type())
            {
                switch (lhs.type())
                {
                case web::json::value::Number: return lhs.as_number() < rhs.as_number();
                case web::json::value::Boolean: return lhs.as_bool() < rhs.as_bool();
                case web::json::value::String: return lhs.as_string() < rhs.as_string();
                case web::json::value::Object: return lhs.as_object() < rhs.as_object();
                case web::json::value::Array: return lhs.as_array() < rhs.as_array();
                case web::json::value::Null: return false;
                default: return false;
                }
            }
            return false;
        }

        inline bool operator>(const web::json::value& lhs, const web::json::value& rhs)
        {
            return rhs < lhs;
        }

        inline bool operator<=(const web::json::value& lhs, const web::json::value& rhs)
        {
            return !(rhs > lhs);
        }

        inline bool operator>=(const web::json::value& lhs, const web::json::value& rhs)
        {
            return !(lhs < rhs);
        }
    }
}

// json::value 'as' accessor
namespace web
{
    namespace json
    {
        namespace details
        {
            // the 'as' accessor is implemented by means of a value_as function object template
            // in order to pass through the various const-ref, ref or value return types
            template <typename T> struct value_as;

            template <> struct value_as<web::json::value>
            {
                const web::json::value& operator()(const web::json::value& value) const { return value; }
                web::json::value& operator()(web::json::value& value) const { return value; }
            };
            template <> struct value_as<web::json::number>
            {
                const web::json::number& operator()(const web::json::value& value) const { return value.as_number(); }
            };
            template <> struct value_as<web::json::object>
            {
                const web::json::object& operator()(const web::json::value& value) const { return value.as_object(); }
                web::json::object& operator()(web::json::value& value) const { return value.as_object(); }
            };
            template <> struct value_as<web::json::array>
            {
                const web::json::array& operator()(const web::json::value& value) const { return value.as_array(); }
                web::json::array& operator()(web::json::value& value) const { return value.as_array(); }
            };
            template <> struct value_as<utility::string_t>
            {
                const utility::string_t& operator()(const web::json::value& value) const { return value.as_string(); }
            };
            template <> struct value_as<bool>
            {
                bool operator()(const web::json::value& value) const { return value.as_bool(); }
            };
            template <> struct value_as<int>
            {
                int operator()(const web::json::value& value) const { return value.as_integer(); }
            };
            template <> struct value_as<double>
            {
                double operator()(const web::json::value& value) const { return value.as_double(); }
            };
            template <> struct value_as<uint32_t>
            {
                uint32_t operator()(const web::json::value& value) const { return value.as_number().to_uint32(); }
            };
            template <> struct value_as<int64_t>
            {
                int64_t operator()(const web::json::value& value) const { return value.as_number().to_int64(); }
            };
            template <> struct value_as<uint64_t>
            {
                uint64_t operator()(const web::json::value& value) const { return value.as_number().to_uint64(); }
            };
        }

        template <typename T, typename V>
        inline auto as(V& value) -> decltype(details::value_as<T>{}(value))
        {
            return details::value_as<T>{}(value);
        }
    }
}

// more json::value accessors and operations
namespace web
{
    namespace json
    {
        // insert a field into the specified value (which must be an object or null),
        // only if the value doesn't already contain a field with that key
        template <typename KeyValuePair>
        inline bool insert(web::json::value& value, const KeyValuePair& field)
        {
            if (!value.has_field(field.first))
            {
                value[field.first] = web::json::value{ field.second };
                return true;
            }
            else
            {
                return false;
            }
        }

        // insert each field into the specified value
        template <typename InputIterator>
        inline void insert(web::json::value& value, InputIterator first, InputIterator last)
        {
            for (auto it = first; last != it; ++it)
            {
                insert(value, *it);
            }
        }

        template <typename Value>
        inline void push_back(web::json::value& value, const Value& element)
        {
            value[value.size()] = web::json::value{ element };
        }

        inline void push_back(web::json::value& value, web::json::value&& element)
        {
            value[value.size()] = std::move(element);
        }

        inline void pop_back(web::json::value& value)
        {
            value.erase(value.size() - 1);
        }

        inline void pop_front(web::json::value& value)
        {
            value.erase(0);
        }

        inline web::json::value& front(web::json::value& value)
        {
            return value.at(0);
        }

        inline const web::json::value& front(const web::json::value& value)
        {
            return value.at(0);
        }

        inline web::json::value& back(web::json::value& value)
        {
            return value.at(value.size() - 1);
        }

        inline const web::json::value& back(const web::json::value& value)
        {
            return value.at(value.size() - 1);
        }

        inline bool empty(const web::json::value& value)
        {
            return 0 == value.size();
        }
    }
}

// json::value initialization
// web::json::value has a rather limited interface for the construction of objects and arrays;
// value_of allows construction of both objects and array from a braced-init-list
namespace web
{
    namespace json
    {
        namespace details
        {
            // value_init shouldn't be used explicitly, it exists to allow implicit conversions
            // from bool and string in braced-init-list arguments to web::json::value_of
            struct value_init : public value
            {
                using value::value;

                value_init() {}
                value_init(const value& v) : value(v) {}
                value_init(value&& v) : value(std::move(v)) {}

                value_init(bool b) : value(b) {}
                value_init(utility::string_t s) : value(std::move(s)) {}
                value_init(const utility::char_t* s) : value(s) {}
            };
        }

        // this function allows terse construction of object values using a braced-init-list
        // e.g. value_of({ { U("foo"), 42 }, { U("bar"), U("baz") }, { U("qux"), true } })
        inline web::json::value value_of(std::initializer_list<std::pair<utility::string_t, web::json::details::value_init>> fields, bool keep_order = false, bool omit_empty_keys = true)
        {
            web::json::value result = web::json::value::object(keep_order);
            for (auto& field : fields)
            {
                if (!field.first.empty() || !omit_empty_keys) insert(result, field);
            }
            return result;
        }

        // this function allows terse construction of array values using a braced-init-list
        // (it is a template specialization to resolve ambiguous calls in favour of the non-template function,
        // since gcc considers the explicit two-arg value constructors for elements of the braced-init-list)
        template <typename = void>
        inline web::json::value value_of(std::initializer_list<web::json::details::value_init> elements)
        {
            web::json::value result = web::json::value::array();
            for (auto& element : elements)
            {
                push_back(result, element);
            }
            return result;
        }

        namespace detail { class ambiguous; }
        // this overload intentionally makes value_of({}) ambiguous
        // since it's unclear whether that's intended to be an empty object or an empty array
        web::json::value value_of(std::initializer_list<detail::ambiguous>);
    }
}

#endif
