#ifndef SDP_SDP_GRAMMAR_H
#define SDP_SDP_GRAMMAR_H

#include <functional>
#include <unordered_map>
#include <vector>
#include <boost/variant.hpp>
#include "cpprest/json.h"

// SDP: Session Description Protocol
// See https://tools.ietf.org/html/rfc4566
namespace sdp
{
    namespace grammar
    {
        // SDP structured text conversion to/from json
        struct converter
        {
            typedef std::function<std::string(const web::json::value& value)> formatter;
            typedef std::function<web::json::value(const std::string& value)> parser;

            // make SDP structured text out of the json representation
            formatter format;

            // parse SDP structured text into the json representation
            parser parse;
        };

        // "An SDP session description consists of a number of lines of text of
        // the form: <type>=<value>
        // where <type> MUST be exactly one case-significant character and
        // <value> is structured text whose format depends on <type>."
        // See https://tools.ietf.org/html/rfc4566#section-5
        struct line
        {
            utility::string_t name; // used as the json field name

            bool required; // "must have at least one"
            bool list; // "may have more than one", i.e. json array

            // "<type> MUST be exactly one case-significant character"
            char type; // used in the SDP session description

            // "In general, <value> is either a number of fields delimited by
            // a single space character or a free format string"
            converter value_converter;
        };

        // "An SDP session description consists of a session-level section
        // followed by zero or more media-level sections."
        // In fact the terminology of RFC 4566 is inconsistent throughout, but
        // what works is to consider that a description consists of a number
        // of elements, both lines and sub-descriptions.
        struct description
        {
            typedef boost::variant<description, line> element;

            utility::string_t name; // used as the json field name

            bool required; // "must have at least one"
            bool list; // "may have more than one"

            // "Some lines in each description are REQUIRED and some are OPTIONAL,
            // but all MUST appear in exactly the order given here."
            std::vector<element> elements;
        };

        // "Attributes are the primary means for extending SDP."
        // "Application writers may add new attributes as they are required"
        // See https://tools.ietf.org/html/rfc4566#section-5.13
        // and https://tools.ietf.org/html/rfc4566#section-6
        // and https://tools.ietf.org/html/rfc4566#page-42
        // "A property attribute is simply of the form "a=<flag>"."
        // It therefore requires no format or parse function, so a
        // default-constructed converter is appropriate.
        typedef std::unordered_map<utility::string_t, converter> attribute_converters;

        // default attribute converters which includes those defined by RFC 4566
        // plus a few other favourites ;-)
        // See https://tools.ietf.org/html/rfc4566#section-6
        attribute_converters get_default_attribute_converters();

        // construct a grammar with the specified attribute converters
        description session_description(const attribute_converters& converters, const converter& default_converter);
    }

    // make a complete SDP session description out of its json representation, using the specified grammar
    std::string make_session_description(const web::json::value& session_description, const grammar::description& grammar);

    // parse a complete SDP session description into its json representation, using the specified grammar
    web::json::value parse_session_description(const std::string& session_description, const grammar::description& grammar);
}

#endif
