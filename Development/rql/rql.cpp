#include "rql/rql.h"

#include <stack>
#include <stdexcept>
#include "cpprest/base_uri.h" // for uri::decode
#include "cpprest/basic_utils.h"
#include "cpprest/json_utils.h"
#include "cpprest/regex_utils.h"

namespace rql
{
    // Helper for parsing a Resource Query Language string into a json representation of the abstract syntax tree
    // The resulting json value follows the structure described in the RQL reference implementation for json-native
    // simple types (strings, numbers and booleans) and arrays, as well as in the representation of call-operators
    // being a json object with "name" and "args" fields, the latter an array containing at least one value (since
    // the ABNF for RQL doesn't support empty arrays or argument lists - it does however support the empty string
    // as a value, so e.g. "()" is an array or argument list containing one element, the empty string).
    // See https://github.com/persvr/rql#rqlparser
    // This implementation has one extension in that unrecognized RQL types are transformed to json objects with
    // "type" and (string) "value" fields. This allows typed values to be processed as appropriate by implementations.

    namespace details
    {
        // std::runtime_error takes care of the message during stack unwinding by using reference counted strings
        struct rql_exception : std::runtime_error
        {
            rql_exception(const utility::string_t& message) : std::runtime_error(utility::us2s(message)) {}
        };

        inline rql_exception unexpected_token(utility::char_t unexpected)
        {
            return{ utility::string_t{ U("rql parse error - unexpected ") } + unexpected };
        }

        inline rql_exception expected_value()
        {
            return{ U("rql parse error - expected value") };
        }

        inline rql_exception expected_closing_parenthesis()
        {
            return{ U("rql parse error - expected )") };
        }

        inline rql_exception invalid_typed_value(const utility::string_t& type, const utility::string_t& value)
        {
            return{ U("rql parse error - invalid typed-value, ") + type + U(':') + value };
        }

        web::json::value make_value(const utility::string_t& encoded_type, const utility::string_t& encoded_value)
        {
            using web::json::value;
            using web::json::value_of;

            value result;

            auto decoded_type = web::uri::decode(encoded_type);
            auto decoded_value = web::uri::decode(encoded_value);

            if (decoded_type.empty())
            {
                std::error_code ec;
                // attempt to parse as json, which will handle booleans, numbers (and null)...
                // it'll also mean the value "%22foo%22" is parsed as the string "foo", not "\"foo\""
                // and more exotic values like "%7B%22foo%22%3A%5Btrue%2C42%5D%7D" become structured json
                // which may be undesirable
                result = value::parse(decoded_value, ec);
                // fall back to string
                if (ec)
                {
                    result = value::string(decoded_value);
                }
            }
            else if (U("string") == decoded_type)
            {
                result = value::string(decoded_value);
            }
            else if (U("number") == decoded_type)
            {
                std::error_code ec;
                // attempt to parse as json, and then check the type
                result = value::parse(decoded_value, ec);
                if (ec || !result.is_number())
                {
                    throw details::invalid_typed_value(decoded_type, decoded_value);
                }
            }
            else if (U("boolean") == decoded_type)
            {
                std::error_code ec;
                // attempt to parse as json, and then check the type
                result = value::parse(decoded_value, ec);
                if (ec || !result.is_boolean())
                {
                    throw details::invalid_typed_value(decoded_type, decoded_value);
                }
            }
            else if (U("json") == decoded_type)
            {
                std::error_code ec;
                result = value::parse(decoded_value, ec);
                if (ec)
                {
                    throw details::invalid_typed_value(decoded_type, decoded_value);
                }
            }
            else
            {
                // typed-value
                result = value_of({ { U("type"), value::string(decoded_type) }, { U("value"), value::string(decoded_value) } });
            }

            return result;
        }
    }

    bool is_call_operator(const web::json::value& arg)
    {
        return arg.is_object() && arg.has_field(U("name")) && arg.has_field(U("args"));
    }

    bool is_typed_value(const web::json::value& arg)
    {
        return arg.is_object() && arg.has_field(U("type")) && arg.has_field(U("value"));
    }

    web::json::value parse_query(const utility::string_t& query)
    {
        using web::json::value;
        using web::json::value_of;

        // parsing state

        value rql = value::array();
        std::stack<value*> nest;
        nest.push(&rql);
        enum { before_arg, after_arg } state = before_arg;

        utility::string_t type;

        // RQL grammar is defined by ABNF as follows...
        // See https://github.com/persvr/rql/blob/master/specification/draft-zyp-rql-00.xml
        //
        // query = and
        // and = operator *( "&" operator )
        // operator = comparison / call-operator / group
        // call-operator = name "(" [ argument *( "," argument ) ] ")"
        // argument = call-operator / value
        // value = *nchar / typed-value / array
        // typed-value = 1*nchar ":" *nchar
        // array = "(" [ value *( "," value ) ] ")"
        // name = *nchar
        // comparison = name ( "=" [ name "=" ] ) value
        // group = "(" ( and / or ) ")"
        // or = operator *( "|" operator )
        // nchar = unreserved / pct-encoded / "*" / "+"
        // pct-encoded   = "%" HEXDIG HEXDIG
        // unreserved    = ALPHA / DIGIT / "-" / "." / "_" / "~"
        //
        // If we rule out the top-level RQL 'and', 'or', 'group' and 'comparison'
        // productions and only implement the normalized call syntax, the ABNF is
        // simplified as follows...
        //
        // query = call-operator
        // call-operator = name "(" [ argument *( "," argument ) ] ")"
        // argument = call-operator / value
        // value = *nchar / typed-value / array
        // typed-value = 1*nchar ":" *nchar
        // array = "(" [ value *( "," value ) ] ")"
        // name = *nchar
        // nchar = unreserved / pct-encoded / "*" / "+"
        // pct-encoded   = "%" HEXDIG HEXDIG
        // unreserved    = ALPHA / DIGIT / "-" / "." / "_" / "~"

        static const utility::regex_t name{ U("([A-Za-z0-9\\-\\._~]|[%][0-9A-Fa-f]{2}|[*+])*") };

        // <aside>
        // On the other hand, Uniform Resource Identifier (URI): Generic Syntax
        // for the Query part (from first '?' to '#') is defined by RFC 3986.
        //
        // query       = *( pchar / "/" / "?" )
        // pchar         = unreserved / pct-encoded / sub-delims / ":" / "@"
        // unreserved    = ALPHA / DIGIT / "-" / "." / "_" / "~"
        // pct-encoded   = "%" HEXDIG HEXDIG
        // sub-delims    = "!" / "$" / "&" / "'" / "(" / ")"
        //                 / "*" / "+" / "," / ";" / "="
        // See https://tools.ietf.org/html/rfc3986#section-3.4
        //
        // This means that "?", "/", "@", "!", "$", "'" and ";" are allowed by
        // the RFC 3986'query' production but not allowed by the RQL grammar.
        // (And on the other hand, RQL has "|" but as far as I can see RFC 3986
        // requires this to be percent encoded.)
        //
        // Should some of those characters therefore be allowed by the RQL
        // 'nchar' production, i.e. usable in 'name' without percent encoding?
        // In particular, '/' for path values, and '$' for regex values would be
        // useful. On the other hand, ';' has been allowed as an alternative to
        // '&' for separating query field=value terms, though e.g. only '&' is
        // allowed by the W3C Recommendation for URL-encoded form data.
        // See https://www.w3.org/TR/html5/forms.html#url-encoded-form-data
        //
        // Let's say not, and stick to the defined RQL grammar...
        // In any case, clients are going to need to be very careful about how
        // they encode RQL in a URL.
        // </aside>

        auto first = query.begin();
        utility::smatch_t match;
        while (query.end() != first)
        {
            if (before_arg == state)
            {
                utility::string_t token;

                if (bst::regex_search(first, query.end(), match, name, bst::regex_constants::match_continuous))
                {
                    token = match.str();
                    first = match[0].second;
                }

                value& args = *nest.top();

                if (query.end() == first)
                {
                    // (typed-)value
                    web::json::push_back(args, details::make_value(type, token));
                    type.clear();
                }
                else
                {
                    utility::char_t punct = *first++;

                    switch (punct)
                    {
                    case U('('):
                        if (!type.empty()) throw details::unexpected_token(punct);
                        // array
                        if (token.empty())
                        {
                            web::json::push_back(args, value::array());
                            nest.push(&web::json::back(args));
                        }
                        // call-operator
                        else
                        {
                            web::json::push_back(args, value_of({ { U("name"), value::string(token) }, { U("args"), value::array() } }));
                            nest.push(&web::json::back(args)[U("args")]);
                        }
                        break;
                    case ',':
                        if (1 == nest.size()) throw details::unexpected_token(punct);
                        // (typed-)value
                        web::json::push_back(args, details::make_value(type, token));
                        type.clear();
                        break;
                    case ')':
                        if (1 >= nest.size()) throw details::unexpected_token(punct);
                        // (typed-)value
                        web::json::push_back(args, details::make_value(type, token));
                        type.clear();
                        nest.pop();
                        state = after_arg;
                        break;
                    case ':':
                        if (token.empty()) throw details::unexpected_token(punct);
                        if (!type.empty()) throw details::unexpected_token(punct);
                        // typed-value
                        type = token;
                        break;
                    default:
                        throw details::unexpected_token(punct);
                    }
                }
            }
            else if (after_arg == state)
            {
                utility::char_t punct = *first++;

                switch (punct)
                {
                case ',':
                    if (1 == nest.size()) throw details::unexpected_token(punct);
                    state = before_arg;
                    break;
                case ')':
                    if (1 >= nest.size()) throw details::unexpected_token(punct);
                    nest.pop();
                    state = after_arg;
                    break;
                default:
                    throw details::unexpected_token(punct);
                }
            }
        }

        if (1 != nest.size()) throw details::expected_closing_parenthesis();
        if (0 == rql.size()) throw details::expected_value();
        return web::json::front(rql);
    }

    // Helpers for evaluating RQL

    evaluator::evaluator(extractor extract)
        : extract(extract)
        , operators(default_operators())
    {
    }

    evaluator::evaluator(extractor extract, rql::operators operators)
        : extract(extract)
        , operators(operators)
    {
    }

    web::json::value evaluator::operator()(const web::json::value& arg, bool extract_value) const
    {
        // arg is a call-operator
        if (is_call_operator(arg))
        {
            const auto& name = arg.at(U("name")).as_string();
            const auto& args = arg.at(U("args"));
            // throws json_exception if not an array
            args.as_array();

            // throws out_of_range if operator not found
            return operators.at(name)(*this, args);
        }
        // arg is a value used as property key
        else if (extract_value)
        {
            web::json::value extracted;
            extract(extracted, arg);
            return extracted;
        }
        // arg is a value
        else
        {
            return arg;
        }
    }

    // Helpers for json value comparison, implementing three-valued (tribool) logic

    web::json::value default_equal_to(const web::json::value& lhs, const web::json::value& rhs)
    {
        if (lhs.type() != rhs.type())
        {
            return value_indeterminate;
        }
        else if (lhs.is_number())
        {
            // json::number equality operator compares different sub-types (signed, unsigned, double) as unequal
            return lhs.as_double() == rhs.as_double() ? value_true : value_false;
        }
        else
        {
            // could (should?) override null == null is true?
            return lhs == rhs ? value_true : value_false;
        }
    }

    web::json::value default_less(const web::json::value& lhs, const web::json::value& rhs)
    {
        if (lhs.type() != rhs.type())
        {
            return value_indeterminate;
        }
        else if (lhs.is_string())
        {
            return lhs.as_string() < rhs.as_string() ? value_true : value_false;
        }
        else if (lhs.is_number())
        {
            // json::number doesn't implement ordering relations
            return lhs.as_double() < rhs.as_double() ? value_true : value_false;
        }
        else
        {
            // don't define ordering of boolean, object and array, as well as null?
            return value_indeterminate;
        }
    }

    // Other helpers

    namespace details
    {
        web::json::value logical_not(const web::json::value& arg)
        {
            if (!arg.is_boolean())
            {
                return value_indeterminate;
            }

            return !arg.as_bool() ? value_true : value_false;
        }

        template <typename ThreeStatePredicate>
        inline web::json::value logical_or(const web::json::value& args, ThreeStatePredicate predicate)
        {
            if (!args.is_array())
            {
                return predicate(args);
            }
            bool indeterminate = false;
            for (const auto& arg : args.as_array())
            {
                auto result = predicate(arg);
                if (!result.is_boolean())
                {
                    indeterminate = true;
                }
                else if (result.as_bool())
                {
                    return value_true;
                }
            }
            return indeterminate ? value_indeterminate : value_false;
        }

        template <typename ThreeStatePredicate>
        inline web::json::value logical_and(const web::json::value& args, ThreeStatePredicate predicate)
        {
            // all elements satisfy the predicate if none of the arguments do not satisfy the predicate
            return logical_not(logical_or(args, std::bind(logical_not, std::bind(predicate, std::placeholders::_1))));
        }

        template <typename ThreeStateBinaryPredicate>
        inline web::json::value includes(const web::json::value& lhs, const web::json::value& rhs, ThreeStateBinaryPredicate predicate)
        {
            // if all comparisons are indeterminate, return false
            return logical_or(lhs, std::bind(predicate, std::placeholders::_1, rhs)) == value_true ? value_true : value_false;
        }

        inline web::json::value matches(const web::json::value& target, const utility::string_t& pattern, bool icase)
        {
            if (!target.is_string())
            {
                return value_indeterminate;
            }
            else
            {
                // verbose conditional expression avoids compiler warnings on all platforms
                const auto flags = icase ? utility::regex_t::flag_type(utility::regex_t::icase) : utility::regex_t::flag_type(0);

                // throws bst::regex_error if the pattern is not valid
                utility::regex_t regex(pattern, flags);

                return bst::regex_search(target.as_string(), regex) ? value_true : value_false;
            }
        }
    }

    namespace functions
    {
        // Logical operators (three-valued logic)
        // See https://doc.apsstandard.org/6.0/spec/rql/#logical-operators

        // and(<call-operator>, <call-operator>, ...) - Short-circuit conjunction
        web::json::value logical_and(const evaluator& eval, const web::json::value& args)
        {
            return details::logical_and(args, eval);
        }

        // or(<call-operator>, <call-operator>, ...) - Short-circuit disjunction
        web::json::value logical_or(const evaluator& eval, const web::json::value& args)
        {
            return details::logical_or(args, eval);
        }

        // not(<call-operator>) - Negation
        web::json::value logical_not(const evaluator& eval, const web::json::value& args)
        {
            return details::logical_not(eval(args.at(0)));
        }

        // Relational operators
        // Standard form: relation(<property>, <value>) - Filters for objects where the specified property's value and the provided value satisfy the specified relation
        // Extended form: relation(<call-operator>, <value|call-operator>) - Filters for objects where the result of the first specified call-operator and the second specified call-operator or value satisfy the specified relation
        // See https://doc.apsstandard.org/6.0/spec/rql/#relational-operators

        template <typename ThreeStateBinaryPredicate>
        web::json::value eq(const evaluator& eval, const web::json::value& args, ThreeStateBinaryPredicate predicate)
        {
            return predicate(eval(args.at(0), true), eval(args.at(1)));
        }

        template <typename ThreeStateBinaryPredicate>
        web::json::value ne(const evaluator& eval, const web::json::value& args, ThreeStateBinaryPredicate predicate)
        {
            return details::logical_not(predicate(eval(args.at(0), true), eval(args.at(1))));
        }

        template <typename ThreeStateCompare>
        web::json::value gt(const evaluator& eval, const web::json::value& args, ThreeStateCompare compare)
        {
            return compare(eval(args.at(1)), eval(args.at(0), true));
        }

        template <typename ThreeStateCompare>
        web::json::value ge(const evaluator& eval, const web::json::value& args, ThreeStateCompare compare)
        {
            return details::logical_not(compare(eval(args.at(0), true), eval(args.at(1))));
        }

        template <typename ThreeStateCompare>
        web::json::value lt(const evaluator& eval, const web::json::value& args, ThreeStateCompare compare)
        {
            return compare(eval(args.at(0), true), eval(args.at(1)));
        }

        template <typename ThreeStateCompare>
        web::json::value le(const evaluator& eval, const web::json::value& args, ThreeStateCompare compare)
        {
            return details::logical_not(compare(eval(args.at(1)), eval(args.at(0), true)));
        }

        // Array-friendly relational operators
        // Filters for objects as above or where the specified property's value is an array and any element of the array and the provided value satisfy the specified relation

        template <typename ThreeStateBinaryPredicate>
        web::json::value any_eq(const evaluator& eval, const web::json::value& args, ThreeStateBinaryPredicate predicate)
        {
            return details::logical_or(eval(args.at(0), true), std::bind(predicate, std::placeholders::_1, eval(args.at(1))));
        }

        template <typename ThreeStateBinaryPredicate>
        web::json::value any_ne(const evaluator& eval, const web::json::value& args, ThreeStateBinaryPredicate predicate)
        {
            return details::logical_or(eval(args.at(0), true), std::bind(details::logical_not, std::bind(predicate, std::placeholders::_1, eval(args.at(1)))));
        }

        template <typename ThreeStateCompare>
        web::json::value any_gt(const evaluator& eval, const web::json::value& args, ThreeStateCompare compare)
        {
            return details::logical_or(eval(args.at(0), true), std::bind(compare, eval(args.at(1)), std::placeholders::_1));
        }

        template <typename ThreeStateCompare>
        web::json::value any_ge(const evaluator& eval, const web::json::value& args, ThreeStateCompare compare)
        {
            return details::logical_or(eval(args.at(0), true), std::bind(details::logical_not, std::bind(compare, std::placeholders::_1, eval(args.at(1)))));
        }

        template <typename ThreeStateCompare>
        web::json::value any_lt(const evaluator& eval, const web::json::value& args, ThreeStateCompare compare)
        {
            return details::logical_or(eval(args.at(0), true), std::bind(compare, std::placeholders::_1, eval(args.at(1))));
        }

        template <typename ThreeStateCompare>
        web::json::value any_le(const evaluator& eval, const web::json::value& args, ThreeStateCompare compare)
        {
            return details::logical_or(eval(args.at(0), true), std::bind(details::logical_not, std::bind(compare, eval(args.at(1)), std::placeholders::_1)));
        }

        // Set relation functions
        // Standard form: relation(<property>, <array-of-values>) - Filters for objects where the specified property's value and the provided array satisfy the specified relation
        // Extended form: relation(<call-operator>, <array-of-values|call-operator>) - Filters for objects where the result of the first specified call-operator and the result of the second specified call-operator or array satisfy the specified relation

        // in(<property>, <array-of-values>) - Filters for objects where the specified property's value is in the provided array
        template <typename ThreeStateBinaryPredicate>
        web::json::value in(const evaluator& eval, const web::json::value& args, ThreeStateBinaryPredicate predicate)
        {
            return details::includes(eval(args.at(1)), eval(args.at(0), true), predicate);
        }

        // out(<property>, <array-of-values>) - Filters for objects where the specified property's value is not in the provided array
        template <typename ThreeStateBinaryPredicate>
        web::json::value out(const evaluator& eval, const web::json::value& args, ThreeStateBinaryPredicate predicate)
        {
            return details::logical_not(details::includes(eval(args.at(1)), eval(args.at(0), true), predicate));
        }

        // contains(<property>, <value>) - Filters for objects where the specified property's value is an array and the array contains the provided value
        template <typename ThreeStateBinaryPredicate>
        web::json::value contains(const evaluator& eval, const web::json::value& args, ThreeStateBinaryPredicate predicate)
        {
            return details::includes(eval(args.at(0), true), eval(args.at(1)), predicate);
        }

        // excludes(<property>, <value>) - Filters for objects where the specified property's value is an array and the array does not contain the provided value
        template <typename ThreeStateBinaryPredicate>
        web::json::value excludes(const evaluator& eval, const web::json::value& args, ThreeStateBinaryPredicate predicate)
        {
            return details::logical_not(details::includes(eval(args.at(0), true), eval(args.at(1)), predicate));
        }

        // Additional filter functions

        // null(<property>) - Filters for objects where the specified property's value is null
        web::json::value null(const evaluator& eval, const web::json::value& args)
        {
            auto arg = args.at(0);

            // arg is a call-operator
            if (is_call_operator(arg))
            {
                return web::json::value::null() == eval(arg) ? value_true : value_false;
            }
            else
            {
                // can't use evaluator::operator() precisely because it doesn't distinguish a null value from property key not found
                web::json::value extracted;
                if (!eval.extract(extracted, arg))
                {
                    return value_indeterminate;
                }
                return web::json::value::null() == extracted ? value_true : value_false;
            }
        }

        // matches(<property>, <pattern>[, <options>]) - Filters for objects where the specified property's value is a string which contains a match for the specified regex pattern/options
        web::json::value matches(const evaluator& eval, const web::json::value& args)
        {
            auto target = eval(args.at(0), true);
            // throws web::json::json_exception if pattern or options are not strings
            auto pattern = eval(args.at(1)).as_string();
            auto icase = args.size() > 2 ? args.at(2).as_string() == U("i") : false;
            return details::matches(target, pattern, icase);
        }

        // any_matches(<property>, <pattern>[, <options>]) - Filters for objects as above or where the specified property's value is an array and any element of the array is a string which contains a match for the specified regex pattern/options
        web::json::value any_matches(const evaluator& eval, const web::json::value& args)
        {
            auto target = eval(args.at(0), true);
            // throws web::json::json_exception if pattern or options are not strings
            auto pattern = eval(args.at(1)).as_string();
            auto icase = args.size() > 2 ? args.at(2).as_string() == U("i") : false;
            return details::logical_or(target, std::bind(details::matches, std::placeholders::_1, pattern, icase));
        }

        // Other helpers

        // count(<property>) - Count of the fields of the object value, or elements in the array value, of the specified property
        web::json::value count(const evaluator& eval, const web::json::value& args)
        {
            auto arg = eval(args.at(0), true);
            if (!arg.is_object() && !arg.is_array())
            {
                return value_indeterminate;
            }
            return web::json::value::number(arg.size());
        }

        // get(<call-operator>) - Value of the property specified by the result of the call-operator (rather than that value directly)
        web::json::value get(const evaluator& eval, const web::json::value& args)
        {
            return eval(eval(args.at(0)), true);
        }

        // value(<value>) - Explicit value as specified (used where the value might otherwise specify a property)
        web::json::value value(const evaluator& eval, const web::json::value& args)
        {
            return eval(args.at(0));
        }
    }

    namespace details
    {
        template <typename ThreeStateBinaryPredicate, typename ThreeStateCompare>
        operators default_operators(ThreeStateBinaryPredicate equal_to, ThreeStateCompare less)
        {
            using namespace std::placeholders;
            return
            {
                { U("and"), functions::logical_and },
                { U("or"), functions::logical_or },
                { U("not"), functions::logical_not },
                { U("eq"), std::bind(functions::eq<ThreeStateBinaryPredicate>, _1, _2, equal_to) },
                { U("ne"), std::bind(functions::ne<ThreeStateBinaryPredicate>, _1, _2, equal_to) },
                { U("gt"), std::bind(functions::gt<ThreeStateCompare>, _1, _2, less) },
                { U("ge"), std::bind(functions::ge<ThreeStateCompare>, _1, _2, less) },
                { U("lt"), std::bind(functions::lt<ThreeStateCompare>, _1, _2, less) },
                { U("le"), std::bind(functions::le<ThreeStateCompare>, _1, _2, less) },
                { U("in"), std::bind(functions::in<ThreeStateBinaryPredicate>, _1, _2, equal_to) },
                { U("out"), std::bind(functions::out<ThreeStateBinaryPredicate>, _1, _2, equal_to) },
                { U("contains"), std::bind(functions::contains<ThreeStateBinaryPredicate>, _1, _2, equal_to) },
                { U("excludes"), std::bind(functions::excludes<ThreeStateBinaryPredicate>, _1, _2, equal_to) },
                { U("null"), functions::null },
                { U("matches"), functions::matches },
                { U("count"), functions::count },
                { U("get"), functions::get },
                { U("value"), functions::value }
            };
        }

        template <typename ThreeStateBinaryPredicate, typename ThreeStateCompare>
        operators default_any_operators(ThreeStateBinaryPredicate equal_to, ThreeStateCompare less)
        {
            using namespace std::placeholders;
            return
            {
                { U("and"), functions::logical_and },
                { U("or"), functions::logical_or },
                { U("not"), functions::logical_not },
                { U("eq"), std::bind(functions::any_eq<ThreeStateBinaryPredicate>, _1, _2, equal_to) },
                { U("ne"), std::bind(functions::any_ne<ThreeStateBinaryPredicate>, _1, _2, equal_to) },
                { U("gt"), std::bind(functions::any_gt<ThreeStateCompare>, _1, _2, less) },
                { U("ge"), std::bind(functions::any_ge<ThreeStateCompare>, _1, _2, less) },
                { U("lt"), std::bind(functions::any_lt<ThreeStateCompare>, _1, _2, less) },
                { U("le"), std::bind(functions::any_le<ThreeStateCompare>, _1, _2, less) },
                { U("in"), std::bind(functions::in<ThreeStateBinaryPredicate>, _1, _2, equal_to) },
                { U("out"), std::bind(functions::out<ThreeStateBinaryPredicate>, _1, _2, equal_to) },
                { U("contains"), std::bind(functions::contains<ThreeStateBinaryPredicate>, _1, _2, equal_to) },
                { U("excludes"), std::bind(functions::excludes<ThreeStateBinaryPredicate>, _1, _2, equal_to) },
                { U("null"), functions::null },
                { U("matches"), functions::any_matches },
                { U("count"), functions::count },
                { U("get"), functions::get },
                { U("value"), functions::value }
            };
        }
    }

    operators default_operators()
    {
        return details::default_operators(default_equal_to, default_less);
    }

    operators default_operators(comparator equal_to, comparator less)
    {
        return details::default_operators(equal_to, less);
    }

    operators default_any_operators()
    {
        return details::default_any_operators(default_equal_to, default_less);
    }

    operators default_any_operators(comparator equal_to, comparator less)
    {
        return details::default_any_operators(equal_to, less);
    }
}
