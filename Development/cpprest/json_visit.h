#ifndef CPPREST_JSON_VISIT_H
#define CPPREST_JSON_VISIT_H

#include <array>
#include <cinttypes>
#include <functional>
#include <stack>
#include <type_traits>
#include "cpprest/json.h"
#include "cpprest/json_escape.h"

// json visiting
namespace web
{
    namespace json
    {
        struct number_tag {};
        struct boolean_tag {};
        struct string_tag {};
        struct object_tag {};
        struct array_tag {};
        struct null_tag {};

        template <typename Visitor, typename Value>
        inline void visit(Visitor&& visitor, Value&& value)
        {
            switch (value.type())
            {
            case value::Number:
                visitor(value, number_tag{});
                break;
            case value::Boolean:
                visitor(value, boolean_tag{});
                break;
            case value::String:
                visitor(value, string_tag{});
                break;
            case value::Object:
                visitor(value, object_tag{});
                break;
            case value::Array:
                visitor(value, array_tag{});
                break;
            case value::Null:
                visitor(value, null_tag{});
                break;
            default:
                break;
            }
        }

        struct enter_object_tag {};
        struct enter_field_tag {};
        struct field_separator_tag {};
        struct leave_field_tag {};
        struct object_separator_tag {};
        struct leave_object_tag {};

        template <typename Visitor, typename Value>
        inline void visit_object(Visitor&& visitor, Value&& value)
        {
            visitor(enter_object_tag{});
            if (0 != value.size())
            {
                auto& o = value.as_object();
                auto first = o.begin(), last = o.end() - 1;
                for (; last != first; ++first)
                {
                    visit_field(visitor, first->first, first->second);
                    visitor(object_separator_tag{});
                }
                visit_field(visitor, first->first, first->second);
            }
            visitor(leave_object_tag{});
        }

        template <typename Visitor, typename String, typename Value>
        inline void visit_field(Visitor&& visitor, String&& key, Value&& value)
        {
            visitor(enter_field_tag{});
            visit(visitor, typename std::remove_cv<typename std::remove_reference<Value>::type>::type(key));
            visitor(field_separator_tag{});
            visit(visitor, value);
            visitor(leave_field_tag{});
        }

        struct enter_array_tag {};
        struct enter_element_tag {};
        struct leave_element_tag {};
        struct array_separator_tag {};
        struct leave_array_tag {};

        template <typename Visitor, typename Value>
        inline void visit_array(Visitor&& visitor, Value&& value)
        {
            visitor(enter_array_tag{});
            if (0 != value.size())
            {
                auto& a = value.as_array();
                auto first = a.begin(), last = a.end() - 1;
                for (; last != first; ++first)
                {
                    visit_element(visitor, *first);
                    visitor(array_separator_tag{});
                }
                visit_element(visitor, *first);
            }
            visitor(leave_array_tag{});
        }

        template <typename Visitor, typename Value>
        inline void visit_element(Visitor&& visitor, Value&& element)
        {
            visitor(enter_element_tag{});
            visit(visitor, element);
            visitor(leave_element_tag{});
        }

        struct double_tag {};
        struct signed_tag {};
        struct unsigned_tag {};

        template <typename Visitor, typename Value>
        inline void visit_number(Visitor&& visitor, Value&& value)
        {
            const auto& number = value.as_number();
            // cf. web::json::number::type
            if (!number.is_integral()) visitor(value, double_tag{});
            else if (number.is_int64()) visitor(value, signed_tag{});
            else visitor(value, unsigned_tag{});
        }

        // value_assigning_visitor can be used to construct a new value based on a valid sequence of visit callbacks
        struct value_assigning_visitor
        {
            value_assigning_visitor(web::json::value& value)
                : cursor()
                , name()
            {
                cursor.push(value);
            }

            // visit callbacks
            void operator()(web::json::value value, web::json::number_tag) { assign(std::move(value)); }
            void operator()(web::json::value value, web::json::boolean_tag) { assign(std::move(value)); }
            void operator()(web::json::value value, web::json::string_tag) { if (name) push_field(value.as_string()); else assign(std::move(value)); }
            void operator()(web::json::value value, web::json::object_tag) { assign(std::move(value)); }
            void operator()(web::json::value value, web::json::array_tag) { assign(std::move(value)); }
            void operator()(web::json::value value, web::json::null_tag) { assign(std::move(value)); }

            // visit_object callbacks
            void operator()(web::json::enter_object_tag) { assign(value::object()); }
            void operator()(web::json::enter_field_tag) { name = true; }
            void operator()(web::json::field_separator_tag) { name = false; }
            void operator()(web::json::leave_field_tag) { pop(); }
            void operator()(web::json::object_separator_tag) {}
            void operator()(web::json::leave_object_tag) {}

            // visit_array callbacks
            void operator()(web::json::enter_array_tag) { assign(value::array()); }
            void operator()(web::json::enter_element_tag) { push_element(); }
            void operator()(web::json::leave_element_tag) { pop(); }
            void operator()(web::json::array_separator_tag) {}
            void operator()(web::json::leave_array_tag) {}

        protected:
            std::stack<std::reference_wrapper<web::json::value>> cursor;
            bool name;

            web::json::value& top() { return cursor.top(); }
            void assign(web::json::value&& value) { top() = std::move(value); }
            void push_field(const utility::string_t& name) { cursor.push(top()[name] = value::null()); }
            void push_element() { cursor.push(top()[top().size()] = value::null()); }
            void pop() { cursor.pop(); }
        };
    }
}

// json formatting visitors
namespace web
{
    namespace json
    {
        namespace details
        {
            template <typename CharType>
            struct literals
            {
                static const CharType enter_object = '{';
                static const CharType field_separator = ':';
                static const CharType object_separator = ',';
                static const CharType leave_object = '}';

                static const CharType enter_array = '[';
                static const CharType array_separator = ',';
                static const CharType leave_array = ']';

                static const CharType quotation_mark = '\"';

                // null-terminated strings
                static const CharType value_null[5];
                static const CharType value_true[5];
                static const CharType value_false[6];
            };

            template <typename CharType>
            const CharType literals<CharType>::value_null[] = { 'n', 'u', 'l', 'l', 0 };
            template <typename CharType>
            const CharType literals<CharType>::value_true[] = { 't', 'r', 'u', 'e', 0 };
            template <typename CharType>
            const CharType literals<CharType>::value_false[] = { 'f', 'a', 'l', 's', 'e', 0 };

            // write json numbers as null-terminated strings to a buffer
            template <typename CharType>
            struct num_put
            {
                // maximum required buffer capacity for double, based on a '-', no loss, '.', "e-123", null terminator
                static const size_t capacity_double = 1 + (std::numeric_limits<double>::digits10 + 2) + 1 + 5 + 1;
                // maximum required buffer capacity for signed, based on a '-', no loss, null terminator
                static const size_t capacity_signed = 1 + (std::numeric_limits<int64_t>::digits10 + 1) + 1;
                // maximum required buffer capacity for unsigned, based on no loss, null terminator
                static const size_t capacity_unsigned = (std::numeric_limits<uint64_t>::digits10 + 1) + 1;

                static size_t put(CharType* buffer, size_t capacity, double v, std::streamsize precision = 0);
                static size_t put(CharType* buffer, size_t capacity, int64_t v);
                static size_t put(CharType* buffer, size_t capacity, uint64_t v);
            };

            template <>
            inline size_t num_put<char>::put(char* buffer, size_t capacity, double v, std::streamsize precision)
            {
                return std::snprintf(buffer, capacity, "%.*g", (int)precision, v);
            }

            template <>
            inline size_t num_put<char>::put(char* buffer, size_t capacity, int64_t v)
            {
                return std::snprintf(buffer, capacity, "%" PRId64, v);
            }

            template <>
            inline size_t num_put<char>::put(char* buffer, size_t capacity, uint64_t v)
            {
                return std::snprintf(buffer, capacity, "%" PRIu64, v);
            }

#ifdef _WIN32
            template <>
            inline size_t num_put<wchar_t>::put(wchar_t* buffer, size_t capacity, double v, std::streamsize precision)
            {
                return std::swprintf(buffer, capacity, L"%.*g", (int)precision, v);
            }

            template <>
            inline size_t num_put<wchar_t>::put(wchar_t* buffer, size_t capacity, int64_t v)
            {
                return std::swprintf(buffer, capacity, L"%" PRId64, v);
            }

            template <>
            inline size_t num_put<wchar_t>::put(wchar_t* buffer, size_t capacity, uint64_t v)
            {
                return std::swprintf(buffer, capacity, L"%" PRIu64, v);
            }
#endif
        }

        // basic_ostream_visitor can be used to serialize a value with no extraneous whitespace
        // it should support CharType of at least utility::char_t and char (when different!)
        template <typename CharType>
        struct basic_ostream_visitor
        {
            typedef CharType char_type;
            typedef details::literals<char_type> literals;
            typedef details::num_put<char_type> num_put;

            basic_ostream_visitor(std::basic_ostream<char_type>& os) : os(os)
            {
                // this is *almost* always what the user wants
                os.precision(std::numeric_limits<double>::digits10 + 2);
            }

            // visit callbacks
            void operator()(const web::json::value& value, web::json::number_tag)
            {
                visit_number(*this, value);
            }
            void operator()(const web::json::value& value, web::json::boolean_tag)
            {
                if (value.as_bool())
                    put(literals::value_true);
                else
                    put(literals::value_false);
            }
            void operator()(const web::json::value& value, web::json::string_tag)
            {
                put(literals::quotation_mark);
                const auto cs = escape_characters(value.as_string());
                put(cs.data(), cs.size());
                put(literals::quotation_mark);
            }
            void operator()(const web::json::value& value, web::json::object_tag)
            {
                web::json::visit_object(*this, value);
            }
            void operator()(const web::json::value& value, web::json::array_tag)
            {
                web::json::visit_array(*this, value);
            }
            void operator()(const web::json::value& value, web::json::null_tag)
            {
                put(literals::value_null);
            }

            // visit_number callbacks
            void operator()(const web::json::value& value, web::json::double_tag)
            {
                std::array<char_type, num_put::capacity_double> cs;
                put(cs.data(), num_put::put(cs.data(), cs.size(), value.as_number().to_double(), os.precision()));
            }
            void operator()(const web::json::value& value, web::json::signed_tag)
            {
                std::array<char_type, num_put::capacity_signed> cs;
                put(cs.data(), num_put::put(cs.data(), cs.size(), value.as_number().to_int64()));
            }
            void operator()(const web::json::value& value, web::json::unsigned_tag)
            {
                std::array<char_type, num_put::capacity_unsigned> cs;
                put(cs.data(), num_put::put(cs.data(), cs.size(), value.as_number().to_uint64()));
            }

            // visit_object callbacks
            void operator()(web::json::enter_object_tag) { put(literals::enter_object); }
            void operator()(web::json::enter_field_tag) {}
            void operator()(web::json::field_separator_tag) { put(literals::field_separator); }
            void operator()(web::json::leave_field_tag) {}
            void operator()(web::json::object_separator_tag) { put(literals::object_separator); }
            void operator()(web::json::leave_object_tag) { put(literals::leave_object); }

            // visit_array callbacks
            void operator()(web::json::enter_array_tag) { put(literals::enter_array); }
            void operator()(web::json::enter_element_tag) {}
            void operator()(web::json::leave_element_tag) {}
            void operator()(web::json::array_separator_tag) { put(literals::array_separator); }
            void operator()(web::json::leave_array_tag) { put(literals::leave_array); }

            static std::basic_string<char_type> escape_characters(const utility::string_t& str);

        protected:
            std::basic_ostream<char_type>& os;

            void put(char_type c) { os.rdbuf()->sputc(c); }
            void put(const char_type* cs, size_t n) { os.rdbuf()->sputn(cs, n); }

            void put(size_t n, char_type c) { while (n-- > 0) put(c); }
            template <size_t N>
            void put(const char_type(& cs)[N]) { put(cs, N - 1); }
        };

        template <>
        inline std::basic_string<utility::char_t> basic_ostream_visitor<utility::char_t>::escape_characters(const utility::string_t& str)
        {
            return details::escape_characters(str);
        }

#ifdef _UTF16_STRINGS
        // utility::char_t != char
        template <>
        inline std::basic_string<char> basic_ostream_visitor<char>::escape_characters(const utility::string_t& str)
        {
            return details::escape_characters(utility::conversions::to_utf8string(str));
        }
#endif

        typedef basic_ostream_visitor<utility::char_t> ostream_visitor;

        // pretty_visitor can be used to serialize a value for human readability
        // it should support CharType of at least utility::char_t and char (when different!)
        template <typename CharType>
        struct basic_pretty_visitor : basic_ostream_visitor<CharType>
        {
            typedef basic_ostream_visitor<CharType> base;
            typedef typename base::char_type char_type;

            basic_pretty_visitor(std::basic_ostream<char_type>& os, size_t indent = 4)
                : base(os)
                , indent(indent)
                , depth(0)
                , empty()
            {}

            // visit call backs
            using base::operator();
            void operator()(const web::json::value& value, web::json::object_tag)
            {
                web::json::visit_object(*this, value);
            }
            void operator()(const web::json::value& value, web::json::array_tag)
            {
                web::json::visit_array(*this, value);
            }

            // visit_object callbacks
            void operator()(web::json::enter_object_tag tag) { base::operator()(tag); ++depth; empty = true; }
            void operator()(web::json::enter_field_tag tag) { if (empty) end_line(); start_line(); base::operator()(tag); }
            void operator()(web::json::field_separator_tag tag) { base::operator()(tag); put(' '); }
            void operator()(web::json::leave_field_tag tag) { base::operator()(tag); empty = false; }
            void operator()(web::json::object_separator_tag tag) { base::operator()(tag); end_line(); }
            void operator()(web::json::leave_object_tag tag) { if (!empty) end_line(); --depth; if (!empty) start_line(); base::operator()(tag); }

            // visit_array callbacks
            void operator()(web::json::enter_array_tag tag) { base::operator()(tag); ++depth; empty = true; }
            void operator()(web::json::enter_element_tag tag) { if (empty) end_line(); start_line(); base::operator()(tag); }
            void operator()(web::json::leave_element_tag tag) { base::operator()(tag); empty = false; }
            void operator()(web::json::array_separator_tag tag) { base::operator()(tag); end_line(); }
            void operator()(web::json::leave_array_tag tag) { if (!empty) end_line(); --depth; if (!empty) start_line(); base::operator()(tag); }

        protected:
            using base::put;

            size_t indent;

            size_t depth;
            bool empty;

            void start_line() { put(depth * indent, ' '); }
            void end_line() { put('\n'); }
        };

        typedef basic_pretty_visitor<utility::char_t> pretty_visitor;

        namespace experimental
        {
            // sample stylesheet for the HTML generated by html_visitor
            extern const char* html_stylesheet;

            // sample script for the HTML generated by html_visitor
            extern const char* html_script;

            namespace details
            {
                template <typename CharType>
                struct html_entities
                {
                    // null-terminated strings
                    static const CharType amp[6];
                    static const CharType lt[5];
                    static const CharType gt[5];
                    static const CharType quot[7];
                    static const CharType apos[7];
                };
                template <typename CharType>
                const CharType html_entities<CharType>::amp[] = { '&', 'a', 'm', 'p', ';', 0 };
                template <typename CharType>
                const CharType html_entities<CharType>::lt[] = { '&', 'l', 't', ';', 0 };
                template <typename CharType>
                const CharType html_entities<CharType>::gt[] = { '&', 'g', 't', ';', 0 };
                template <typename CharType>
                const CharType html_entities<CharType>::quot[] = { '&', 'q', 'u', 'o', 't', ';', 0 };
                template <typename CharType>
                const CharType html_entities<CharType>::apos[] = { '&', 'a', 'p', 'o', 's', ';', 0 };

                template <typename CharType>
                inline std::basic_string<CharType> html_escape(const std::basic_string<CharType>& unescaped)
                {
                    typedef details::html_entities<CharType> html_entities;
                    std::basic_string<CharType> escaped;
                    escaped.reserve(unescaped.size());
                    for (auto c : unescaped)
                    {
                        switch (c)
                        {
                            // escape everywhere
                        case '&': escaped.append(html_entities::amp); break;
                        case '<': escaped.append(html_entities::lt); break;
                        case '>': escaped.append(html_entities::gt); break;
                            // escape in attributes
                        case '"': escaped.append(html_entities::quot); break;
                        case '\'': escaped.append(html_entities::apos); break;
                            // unescaped
                        default: escaped.push_back(c); break;
                        }
                    }
                    return escaped;
                }
            }

            // html_visitor can be used to serialize a value to HTML, to allow formatting and syntax highlighting using a stylesheet
            template <typename CharType>
            struct basic_html_visitor : basic_ostream_visitor<CharType>
            {
                typedef basic_ostream_visitor<CharType> base;
                typedef typename base::char_type char_type;
                typedef typename base::literals literals;
                typedef details::html_entities<char_type> html_entities;

                basic_html_visitor(std::basic_ostream<char_type>& os)
                    : base(os)
                    , empty()
                    , name()
                {}

                // visit callbacks
                void operator()(const web::json::value& value, web::json::number_tag tag)
                {
                    start_span("number"); base::operator()(value, tag); end_span();
                }
                void operator()(const web::json::value& value, web::json::boolean_tag tag)
                {
                    start_span("boolean"); base::operator()(value, tag); end_span();
                }
                void operator()(const web::json::value& value, web::json::string_tag tag)
                {
                    start_span(name ? "name" : "string");
                    os << html_entities::quot;
                    os << escape(escape_characters(value.as_string()));
                    os << html_entities::quot;
                    end_span();
                }
                void operator()(const web::json::value& value, web::json::object_tag)
                {
                    web::json::visit_object(*this, value);
                }
                void operator()(const web::json::value& value, web::json::array_tag)
                {
                    web::json::visit_array(*this, value);
                }
                void operator()(const web::json::value& value, web::json::null_tag tag)
                {
                    start_span("null"); base::operator()(value, tag); end_span();
                }

                // visit_object callbacks
                void operator()(web::json::enter_object_tag) { start_span("object"); start_compound(literals::enter_object, "<ul>"); }
                void operator()(web::json::enter_field_tag) { if (!empty) os << literals::object_separator << "</li>"; os << "<li>"; name = true; }
                void operator()(web::json::field_separator_tag) { name = false; os << literals::field_separator << ' '; }
                void operator()(web::json::leave_field_tag) { empty = false; }
                void operator()(web::json::object_separator_tag) {}
                void operator()(web::json::leave_object_tag) { end_compound("</ul>", literals::leave_object); end_span(); }

                // visit_array callbacks
                void operator()(web::json::enter_array_tag) { start_span("array"); start_compound(literals::enter_array, "<ol>"); }
                void operator()(web::json::enter_element_tag) { if (!empty) os << literals::array_separator << "</li>"; os << "<li>"; }
                void operator()(web::json::leave_element_tag) { empty = false; }
                void operator()(web::json::array_separator_tag) {}
                void operator()(web::json::leave_array_tag) { end_compound("</ol>", literals::leave_array); end_span(); }

                using base::escape_characters;
                static std::basic_string<char_type> escape(const std::basic_string<char_type>& unescaped) { return details::html_escape(unescaped); }

            protected:
                using base::os;

                bool empty;
                bool name;

                void start_span(const char* clazz) { os << "<span class=\"" << clazz << "\">"; }
                void end_span() { os << "</span>"; }

                void start_compound(char_type enter, const char* start) { start_span("toggle"); os << enter; end_span(); start_span("normal"); os << start; empty = true; }
                void end_compound(const char* end, char_type leave) { if (!empty) os << "</li>"; os << end; end_span(); start_span("elided"); if (!empty) os << "&#x2026;"; end_span(); os << leave; }
            };

            typedef basic_html_visitor<utility::char_t> html_visitor;
        }
    }
}

#endif
