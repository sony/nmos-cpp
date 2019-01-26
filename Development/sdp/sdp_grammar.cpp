#include "sdp/sdp_grammar.h"
#include "sdp/sdp.h"

#include <stdexcept>
#include "bst/regex.h"
#include "cpprest/basic_utils.h"
#include "sdp/json.h"

namespace sdp
{
    // exception explicitly indicating an sdp format or parse error
    // e.g. web::json::json_exception is currently also not unlikely
    struct sdp_exception : std::runtime_error
    {
        sdp_exception(const std::string& message) : std::runtime_error(message) {}
    };

    static sdp_exception sdp_format_error(std::string message)
    {
        return{ "sdp format error - " + std::move(message) };
    }

    static sdp_exception sdp_parse_error(std::string message)
    {
        return{ "sdp parse error - " + std::move(message) };
    }

    namespace grammar
    {
        // for easy reading and simple equality comparison of the json representation
        const bool keep_order = true;

        // SDP structured text conversion to/from json

        inline std::string js2s(const web::json::value& v) { auto s = utility::us2s(v.as_string()); if (!s.empty()) return s; else throw sdp_format_error("expected a non-empty string"); }
        inline web::json::value s2js(const std::string& s) { if (!s.empty()) return web::json::value::string(utility::s2us(s)); else throw sdp_parse_error("expected a non-empty string"); }

        inline std::string jn2s(const web::json::value& v) { return v.as_number(), utility::us2s(v.serialize()); }
        inline web::json::value s2jn(const std::string& s) { auto v = web::json::value::parse(utility::s2us(s)); return v.as_number(), v; }

        // find the first delimiter in str, beginning at pos, and return the substring from pos to the delimiter (or end)
        // set pos to the end of the delimiter
        inline std::string substr_find(const std::string& str, std::string::size_type& pos, const std::string& delimiter = {})
        {
            const std::string::size_type end = !delimiter.empty() ? str.find(delimiter, pos) : std::string::npos;
            std::string substr = std::string::npos != end ? str.substr(pos, end - pos) : str.substr(pos);
            pos = std::string::npos != end ? end + delimiter.size() : std::string::npos;
            return substr;
        }

        // find the first delimiter in str, beginning at pos, and return the substring from pos to the delimiter (or end)
        // set pos to the end of the delimiter
        inline std::string substr_find(const std::string& str, std::string::size_type& pos, const bst::regex& delimiter)
        {
            bst::smatch match;
            const std::string::size_type end = bst::regex_search(str.begin() + pos, str.end(), match, delimiter) ? match[0].first - str.begin() : std::string::npos;
            std::string substr = std::string::npos != end ? str.substr(pos, end - pos) : str.substr(pos);
            pos = std::string::npos != end ? end + (match[0].second - match[0].first) : std::string::npos;
            return substr;
        }

        // <byte-string>
        const converter string_converter{ js2s, s2js };

        const converter number_converter{ jn2s, s2jn };

        // <key>[<separator><value>]
        converter key_value_converter(char separator, const std::pair<utility::string_t, converter>& key_converter, const std::pair<utility::string_t, converter>& value_converter)
        {
            return{
                [=](const web::json::value& v) {
                    const web::json::field_as_value_or value_or_null{ value_converter.first, {} };
                    const auto& value = value_or_null(v);
                    return !value.is_null()
                        ? key_converter.second.format(v.at(key_converter.first)) + separator + value_converter.second.format(value)
                        : key_converter.second.format(v.at(key_converter.first));
                },
                [=](const std::string& s) {
                    const auto eq = s.find(separator);
                    const auto key = key_converter.second.parse(s.substr(0, eq));
                    return std::string::npos != eq
                        ? web::json::value_of({ { key_converter.first, key }, { value_converter.first, value_converter.second.parse(s.substr(eq + 1)) } }, keep_order)
                        : web::json::value_of({ { key_converter.first, key } }, keep_order);
                }
            };
        }

        converter array_converter(const converter& converter, const std::string& delimiter = " ")
        {
            return{
                [=](const web::json::value& v) {
                    std::string s;
                    for (const auto& each : v.as_array())
                    {
                        if (!each.is_null())
                        {
                            if (!s.empty()) s += delimiter;
                            s += converter.format(each);
                        }
                    }
                    return s;
                },
                [=](const std::string& s) {
                    auto v = web::json::value::array();
                    size_t pos = 0;
                    while (std::string::npos != pos && s.size() != pos)
                    {
                        auto each = substr_find(s, pos, delimiter);
                        // leading or repeated delimiters are an error
                        if (each.empty()) throw sdp_parse_error("unexpected delimiter");
                        web::json::push_back(v, converter.parse(each));
                    }
                    return v;
                }
            };
        }

        // identical to above except that parse_delimiter is a regex pattern
        // hmm, yes, should therefore refactor
        converter array_converter(const converter& converter, const std::string& format_delimiter, const std::string& parse_delimiter)
        {
            return{
                [=](const web::json::value& v) {
                    std::string s;
                    for (const auto& each : v.as_array())
                    {
                        if (!each.is_null())
                        {
                            if (!s.empty()) s += format_delimiter;
                            s += converter.format(each);
                        }
                    }
                    return s;
                },
                [=](const std::string& s) {
                    const bst::regex delimiter{ parse_delimiter };
                    auto v = web::json::value::array();
                    size_t pos = 0;
                    while (std::string::npos != pos && s.size() != pos)
                    {
                        auto each = substr_find(s, pos, delimiter);
                        // leading or repeated delimiters are an error
                        if (each.empty()) throw sdp_parse_error("unexpected delimiter");
                        web::json::push_back(v, converter.parse(each));
                    }
                    return v;
                }
            };
        }

        const converter strings_converter = array_converter(string_converter, " ");

        const converter named_values_converter = array_converter(key_value_converter('=', { sdp::fields::name, string_converter }, { sdp::fields::value, string_converter }), "; ", ";[ \\t]+");

        converter object_converter(const std::vector<std::pair<utility::string_t, converter>>& field_converters, const std::string& delimiter = " ")
        {
            return{
                [=](const web::json::value& v) {
                    std::string s;
                    for (auto& field : field_converters)
                    {
                        if (!s.empty()) s += delimiter;
                        s += field.second.format(!field.first.empty() ? v.at(field.first) : v);
                    }
                    return s;
                },
                [=](const std::string& s) {
                    auto v = web::json::value::object(keep_order);
                    size_t pos = 0;
                    for (auto& field : field_converters)
                    {
                        if (std::string::npos == pos) throw sdp_parse_error("expected a value for " + utility::us2s(field.first));
                        auto each = substr_find(s, pos, delimiter);
                        // leading or repeated delimiters are an error
                        if (each.empty()) throw sdp_parse_error("unexpected delimiter");

                        auto vv = field.second.parse(each);
                        if (!field.first.empty()) v[field.first] = vv;
                        else web::json::insert(v, vv.as_object().begin(), vv.as_object().end());
                    }
                    return v;
                }
            };
        }

        const std::string time_units{ 'd', 'h', 'm', 's' }; // days, hours, minutes, seconds

        const converter typed_time_converter
        {
            [](const web::json::value& v) {
                return jn2s(v.at(sdp::fields::time_value)) + utility::us2s(sdp::fields::time_unit(v));
            },
            [](const std::string& s) {
                return !s.empty() && std::string::npos != time_units.find(s.back())
                    ? web::json::value_of({ { sdp::fields::time_value, s2jn(s.substr(0, s.size() - 1)) }, { sdp::fields::time_unit, s2js({ s.back() }) } }, keep_order)
                    : web::json::value_of({ { sdp::fields::time_value, s2jn(s) } }, keep_order);
            }
        };

        // empty <byte-string> for <att-value> (after the colon) is prohibited so it is used to indicate no <att-value> at all
        // i.e. a property attribute
        const converter default_attribute_converter
        {
            [](const web::json::value& v) {
                return !v.is_null() ? js2s(v) : "";
            },
            [](const std::string& s) {
                return !s.empty() ? s2js(s) : web::json::value::null();
            }
        };

        // SDP lines and descriptions

        inline line required_line(utility::string_t name, char type, converter conv)
        {
            return{ std::move(name), true, false, type, std::move(conv) };
        }

        inline line required_lines(utility::string_t name, char type, converter conv)
        {
            return{ std::move(name), true, true, type, std::move(conv) };
        }

        inline line optional_line(utility::string_t name, char type, converter conv)
        {
            return{ std::move(name), false, false, type, std::move(conv) };
        }

        inline line optional_lines(utility::string_t name, char type, converter conv)
        {
            return{ std::move(name), false, true, type, std::move(conv) };
        }

        inline description required_descriptions(utility::string_t name, std::vector<description::element> elements)
        {
            return{ std::move(name), true, true, std::move(elements) };
        }

        inline description optional_descriptions(utility::string_t name, std::vector<description::element> elements)
        {
            return{ std::move(name), false, true, std::move(elements) };
        }

        // See https://tools.ietf.org/html/rfc4566#section-5.1
        const line protocol_version = required_line(
            sdp::fields::protocol_version,
            'v',
            number_converter
        );

        // See https://tools.ietf.org/html/rfc4566#section-5.2
        const line origin = required_line(
            sdp::fields::origin,
            'o',
            object_converter({
                { sdp::fields::user_name, string_converter },
                { sdp::fields::session_id, number_converter },
                { sdp::fields::session_version, number_converter },
                { sdp::fields::network_type, string_converter },
                { sdp::fields::address_type, string_converter },
                { sdp::fields::unicast_address, string_converter }
            })
        );

        // See https://tools.ietf.org/html/rfc4566#section-5.3
        const line session_name = required_line(
            sdp::fields::session_name,
            's',
            {
                [](const web::json::value& v) { auto s = utility::us2s(v.as_string()); return !s.empty() ? s : " "; },
                [](const std::string& s) { return web::json::value::string(utility::s2us(!s.empty() ? s : " ")); }
            }
        );

        // See https://tools.ietf.org/html/rfc4566#section-5.4
        const line information = optional_line(
            sdp::fields::information,
            'i',
            string_converter
        );

        // See https://tools.ietf.org/html/rfc4566#section-5.5
        const line uri = optional_line(
            sdp::fields::uri,
            'u',
            string_converter
        );

        // See https://tools.ietf.org/html/rfc4566#section-5.6
        const line email_addresses = optional_lines(
            sdp::fields::email_addresses,
            'e',
            string_converter
        );

        // See https://tools.ietf.org/html/rfc4566#section-5.6
        const line phone_numbers = optional_lines(
            sdp::fields::phone_numbers,
            'p',
            string_converter
        );

        // See https://tools.ietf.org/html/rfc4566#section-5.7
        const line connection_data = optional_line(
            sdp::fields::connection_data,
            'c',
            object_converter({
                { sdp::fields::network_type, string_converter },
                { sdp::fields::address_type, string_converter },
                { sdp::fields::connection_address, string_converter },
            })
        );

        // See https://tools.ietf.org/html/rfc4566#section-5.8
        const line bandwidth_information = optional_lines(
            sdp::fields::bandwidth_information,
            'b',
            object_converter({
                { sdp::fields::bandwidth_type, string_converter },
                { sdp::fields::bandwidth, number_converter }
            }, ":")
        );

        const description time_descriptions = required_descriptions(
            sdp::fields::time_descriptions,
            {
                // See https://tools.ietf.org/html/rfc4566#section-5.9
                required_line(
                    sdp::fields::timing,
                    't',
                    object_converter({
                        { sdp::fields::start_time, number_converter },
                        { sdp::fields::stop_time, number_converter }
                    })
                ),
                // See https://tools.ietf.org/html/rfc4566#section-5.10
                optional_lines(
                    sdp::fields::repeat_times,
                    'r',
                    object_converter({
                        { sdp::fields::repeat_interval, typed_time_converter },
                        { sdp::fields::active_duration, typed_time_converter },
                        { sdp::fields::offsets, array_converter(typed_time_converter) }
                    })
                )
            }
        );

        // See https://tools.ietf.org/html/rfc4566#section-5.11
        const line zone_adjustments = optional_line(
            sdp::fields::zone_adjustments,
            'z',
            {
                array_converter(object_converter({
                    { sdp::fields::time, number_converter },
                    { sdp::fields::adjustment, typed_time_converter }
                })).format,
                // custom parser required because the fields of the sub-objects of the array are also space separated, ugh
                [](const std::string& s)
                {
                    auto v = web::json::value::array();
                    size_t pos = 0;
                    do
                    {
                        auto time = substr_find(s, pos, " ");
                        if (std::string::npos == pos) throw sdp_parse_error("expected a value for " + utility::us2s(sdp::fields::adjustment));
                        auto adjustment = substr_find(s, pos, " ");
                        web::json::push_back(v, web::json::value_of({
                            { sdp::fields::time, number_converter.parse(time) },
                            { sdp::fields::adjustment, typed_time_converter.parse(adjustment) }
                        }, keep_order));
                    } while (std::string::npos != pos);
                    return v;
                },
            }
        );

        // See https://tools.ietf.org/html/rfc4566#section-5.12
        // "Its use is NOT RECOMMENDED" (therefore its value is unparsed)
        const line encryption_key = optional_line(
            sdp::fields::encryption_key,
            'k',
            string_converter
        );

        // See https://tools.ietf.org/html/rfc4566#section-5.13
        // and https://tools.ietf.org/html/rfc4566#page-42
        // Note converters are only captured by reference into the returned line!
        line attributes(const attribute_converters& converters, const converter& default_converter)
        {
            return optional_lines(
                sdp::fields::attributes,
                'a',
                converter{
                    [&](const web::json::value& v) {
                        const auto name = sdp::fields::name(v);

                        const auto found = converters.find(name);
                        const auto& format = (converters.end() != found ? found->second : default_converter).format;

                        if (format)
                        {
                            const auto formatted = format(sdp::fields::value(v));
                            // empty <byte-string> for <att-value> (after the colon) is prohibited so it is used to indicate no <att-value> at all
                            // i.e. a property attribute
                            return !formatted.empty() ? utility::us2s(name) + ':' + formatted : utility::us2s(name);
                        }
                        else
                        {
                            if (!sdp::fields::value(v).is_null()) throw sdp_format_error("property attribute has unexpected value");
                            return utility::us2s(name);
                        }
                    },
                    [&](const std::string& s) {
                        auto v = web::json::value::object(keep_order);

                        const auto colon = s.find(':');
                        // empty <token> for <att-field> (before the colon) is prohibited
                        const auto name = utility::s2us(s.substr(0, colon));

                        v[sdp::fields::name] = web::json::value::string(name);

                        // empty <byte-string> for <att-value> (after the colon) is prohibited
                        if (std::string::npos != colon && s.size() == colon + 1) throw sdp_parse_error("expected an attribute value after ':'");

                        const auto found = converters.find(name);
                        const auto& parse = (converters.end() != found ? found->second : default_converter).parse;

                        if (parse)
                        {
                            // empty <byte-string> for <att-value> (after the colon) is prohibited so it is used to indicate no <att-value> at all
                            // i.e. a property attribute
                            auto parsed = parse(std::string::npos != colon ? s.substr(colon + 1) : "");
                            if (!parsed.is_null()) v[sdp::fields::value] = std::move(parsed);
                        }
                        else
                        {
                            if (std::string::npos != colon) throw sdp_parse_error("property attribute has unexpected value");
                        }

                        return v;
                    }
                }
            );
        }

        // See https://tools.ietf.org/html/rfc4566#section-5
        description media_descriptions(const attribute_converters& converters, const converter& converter)
        {
            return optional_descriptions(
                sdp::fields::media_descriptions,
                {
                    // See https://tools.ietf.org/html/rfc4566#section-5.14
                    required_line(
                        sdp::fields::media,
                        'm',
                        object_converter({
                            { sdp::fields::media_type, string_converter },
                            { {}, key_value_converter('/', { sdp::fields::port, number_converter }, { sdp::fields::port_count, number_converter }) },
                            { sdp::fields::protocol, string_converter },
                            { sdp::fields::formats, strings_converter }
                        })
                    ),
                    information,
                    optional_lines(
                        connection_data.name,
                        connection_data.type,
                        connection_data.value_converter
                    ),
                    bandwidth_information,
                    encryption_key,
                    attributes(converters, converter),
                }
            );
        }

        // See https://tools.ietf.org/html/rfc4566#section-5
        description session_description(const attribute_converters& converters, const converter& default_converter)
        {
            return{
                {}, // top-level object
                true,
                false,
                {
                    protocol_version,
                    origin,
                    session_name,
                    information,
                    uri,
                    email_addresses,
                    phone_numbers,
                    connection_data,
                    bandwidth_information,
                    time_descriptions,
                    zone_adjustments,
                    encryption_key,
                    attributes(converters, default_converter),
                    media_descriptions(converters, default_converter)
                }
            };
        }

        // default attribute converters which includes those defined by RFC 4566
        // plus a few other favourites ;-)
        // See https://tools.ietf.org/html/rfc4566#section-6
        attribute_converters get_default_attribute_converters()
        {
            return{
                // See https://tools.ietf.org/html/rfc4566#section-6
                { sdp::attributes::cat, string_converter },
                { sdp::attributes::keywds, string_converter },
                { sdp::attributes::tool, string_converter },
                { sdp::attributes::ptime, number_converter },
                { sdp::attributes::maxptime, number_converter },
                {
                    sdp::attributes::rtpmap, // <payload type> <encoding name>/<clock rate>[/<encoding parameters>]
                    {
                        [](const web::json::value& v) {
                            std::string s;
                            s += number_converter.format(v.at(sdp::fields::payload_type));
                            s += " " + string_converter.format(v.at(sdp::fields::encoding_name));
                            s += "/" + number_converter.format(v.at(sdp::fields::clock_rate));
                            if (v.has_field(sdp::fields::encoding_parameters)) s += "/" + number_converter.format(v.at(sdp::fields::encoding_parameters));
                            return s;
                        },
                        [](const std::string& s) {
                            auto v = web::json::value::object(keep_order);
                            size_t pos = 0;
                            v[sdp::fields::payload_type] = number_converter.parse(substr_find(s, pos, " "));
                            v[sdp::fields::encoding_name] = string_converter.parse(substr_find(s, pos, "/"));
                            v[sdp::fields::clock_rate] = number_converter.parse(substr_find(s, pos, "/"));
                            if (std::string::npos != pos) v[sdp::fields::encoding_parameters] = number_converter.parse(substr_find(s, pos));
                            return v;
                        },
                    }
                },
                { sdp::attributes::recvonly, {} },
                { sdp::attributes::sendrecv, {} },
                { sdp::attributes::sendonly, {} },
                { sdp::attributes::inactive, {} },
                { sdp::attributes::orient, string_converter }, // portrait|landscape|seascape
                { sdp::attributes::type, string_converter }, // broadcast|meeting|moderated|test|H332
                { sdp::attributes::charset, string_converter },
                { sdp::attributes::sdplang, string_converter },
                { sdp::attributes::lang, string_converter },
                { sdp::attributes::framerate, number_converter }, // floating point
                { sdp::attributes::quality, number_converter }, // integer
                {
                    sdp::attributes::fmtp, // <format> <format specific parameters>
                    {
                        [](const web::json::value& v) {
                            std::string s;
                            s += string_converter.format(v.at(sdp::fields::format));
                            s += " ";
                            // <format specific parameters> are required but may be empty
                            const auto& params = v.at(sdp::fields::format_specific_parameters);
                            if (0 != params.size()) s += named_values_converter.format(params) + "; ";
                            return s;
                        },
                        [](const std::string& s) {
                            auto v = web::json::value::object(keep_order);
                            size_t pos = 0;
                            const bst::regex whitespace{ "[ \\t]+" };
                            v[sdp::fields::format] = string_converter.parse(substr_find(s, pos, whitespace));
                            // handle no space after <format> if there are no <format specific parameters>
                            auto params = std::string::npos != pos ? substr_find(s, pos) : "";
                            // named_values_converter ignores a (correct, probably?) trailing "; " and equally copes if it's not present
                            // but needs a helping hand with a trailing ";" but no space
                            if (!params.empty() && ';' == params.back()) params.push_back(' ');
                            v[sdp::fields::format_specific_parameters] = named_values_converter.parse(params);
                            return v;
                        },
                    }
                },
                // See https://tools.ietf.org/html/rfc4570
                {
                    sdp::attributes::source_filter, // <filter-mode> <nettype> <address-types> <dest-address> <src-list>
                    {
                        [](const web::json::value& v) {
                            std::string s;
                            s += " " + string_converter.format(v.at(sdp::fields::filter_mode));
                            s += " " + string_converter.format(v.at(sdp::fields::network_type));
                            s += " " + string_converter.format(v.at(sdp::fields::address_types));
                            s += " " + string_converter.format(v.at(sdp::fields::destination_address));
                            s += " " + strings_converter.format(v.at(sdp::fields::source_addresses));
                            return s;
                        },
                        [](const std::string& s) {
                            auto v = web::json::value::object(keep_order);
                            size_t pos = (!s.empty() && ' ' == s.front()) ? 1 : 0;
                            v[sdp::fields::filter_mode] = string_converter.parse(substr_find(s, pos, " "));
                            v[sdp::fields::network_type] = string_converter.parse(substr_find(s, pos, " "));
                            v[sdp::fields::address_types] = string_converter.parse(substr_find(s, pos, " "));
                            v[sdp::fields::destination_address] = string_converter.parse(substr_find(s, pos, " "));
                            v[sdp::fields::source_addresses] = strings_converter.parse(substr_find(s, pos));
                            return v;
                        },
                    }
                },
                // See https://tools.ietf.org/html/rfc5888
                {
                    sdp::attributes::group, // <semantics>[ <identification-tag>]*
                    {
                        [](const web::json::value& v) {
                            std::string s;
                            s += string_converter.format(v.at(sdp::fields::semantics));
                            const auto& mids = v.at(sdp::fields::mids);
                            if (0 != mids.size()) s += " " + strings_converter.format(v.at(sdp::fields::mids));
                            return s;
                        },
                        [](const std::string& s) {
                            auto v = web::json::value::object(keep_order);
                            size_t pos = 0;
                            v[sdp::fields::semantics] = string_converter.parse(substr_find(s, pos, " "));
                            auto mids = std::string::npos != pos ? substr_find(s, pos) : "";
                            v[sdp::fields::mids] = strings_converter.parse(mids);
                            return v;
                        },
                    }
                },
                { sdp::attributes::mid, string_converter },
                // See https://tools.ietf.org/html/rfc7273
                {
                    sdp::attributes::ts_refclk,
                    // a=ts-refclk:ntp=<ntp server addr>
                    // a=ts-refclk:ntp=/traceable/
                    // a=ts-refclk:ptp=<ptp version>:<ptp gmid>[:<ptp domain>]
                    // a=ts-refclk:ptp=<ptp version>:traceable
                    // a=ts-refclk:gps
                    // a=ts-refclk:gal
                    // a=ts-refclk:glonass
                    // a=ts-refclk:local
                    // a=ts-refclk:private[:traceable]
                    // See SMPTE ST 2110-10:2017 Professional Media Over Managed IP Networks: System Timing and Definitions, Section 8.2 Reference Clock
                    // a=ts-refclk:localmac=<mac-address-of-sender>
                    {
                        [](const web::json::value& v) {
                            std::string s;
                            s += string_converter.format(v.at(sdp::fields::clock_source));
                            const sdp::ts_refclk_source clock_source{ sdp::fields::clock_source(v) };
                            if (sdp::ts_refclk_sources::ntp == clock_source)
                            {
                                s += '=';
                                if (sdp::fields::traceable(v)) s += "/traceable/";
                                else s += string_converter.format(v.at(sdp::fields::ntp_server));
                            }
                            else if (sdp::ts_refclk_sources::ptp == clock_source)
                            {
                                s += '=';
                                s += string_converter.format(v.at(sdp::fields::ptp_version)) + ':';
                                if (sdp::fields::traceable(v)) s += "traceable";
                                else s += string_converter.format(v.at(sdp::fields::ptp_server)); // <ptp gmid>[:<ptp domain>]
                            }
                            else if (sdp::ts_refclk_sources::private_clock == clock_source)
                            {
                                if (sdp::fields::traceable(v)) s += ":traceable";
                            }
                            else if (sdp::ts_refclk_sources::local_mac == clock_source)
                            {
                                s += '=';
                                s += string_converter.format(v.at(sdp::fields::mac_address));
                            }
                            return s;
                        },
                        [](const std::string& s) {
                            auto v = web::json::value::object(keep_order);
                            size_t pos = s.find_first_of("=:");
                            v[sdp::fields::clock_source] = string_converter.parse(s.substr(0, pos));
                            const sdp::ts_refclk_source clock_source{ utility::s2us(s.substr(0, pos)) };
                            if (sdp::ts_refclk_sources::ntp == clock_source)
                            {
                                if (std::string::npos == pos) throw sdp_parse_error("expected a value for " + utility::us2s(sdp::fields::ntp_server));
                                const auto server = s.substr(++pos);
                                if ("/traceable/" == server) v[sdp::fields::traceable] = web::json::value::boolean(true);
                                else v[sdp::fields::ntp_server] = string_converter.parse(server);
                            }
                            else if (sdp::ts_refclk_sources::ptp == clock_source)
                            {
                                if (std::string::npos == pos) throw sdp_parse_error("expected a value for " + utility::us2s(sdp::fields::ptp_version));
                                v[sdp::fields::ptp_version] = string_converter.parse(substr_find(s, ++pos, ":"));
                                if (std::string::npos == pos) throw sdp_parse_error("expected a value for " + utility::us2s(sdp::fields::ptp_server));
                                const auto server = s.substr(pos);
                                if ("traceable" == server) v[sdp::fields::traceable] = web::json::value::boolean(true);
                                else v[sdp::fields::ptp_server] = string_converter.parse(server);
                            }
                            else if (sdp::ts_refclk_sources::gps == clock_source || sdp::ts_refclk_sources::glonass == clock_source || sdp::ts_refclk_sources::galileo == clock_source)
                            {
                                v[sdp::fields::traceable] = web::json::value::boolean(true);
                            }
                            else if (sdp::ts_refclk_sources::private_clock == clock_source)
                            {
                                if (std::string::npos != pos && ":traceable" == s.substr(pos)) v[sdp::fields::traceable] = web::json::value::boolean(true);
                            }
                            else if (sdp::ts_refclk_sources::local_mac == clock_source)
                            {
                                if (std::string::npos == pos) throw sdp_parse_error("expected a value for " + utility::us2s(sdp::fields::mac_address));
                                const auto mac_address = s.substr(++pos);
                                v[sdp::fields::mac_address] = string_converter.parse(mac_address);
                            }
                            return v;
                        }
                    }
                },
                {
                    sdp::attributes::mediaclk,
                    string_converter // sorry, cannot summon the energy
                }
            };
        }

        const attribute_converters default_attribute_converters = get_default_attribute_converters();
        const description default_grammar = session_description(default_attribute_converters, default_attribute_converter);
    }

    void write_line(std::ostream& os, const web::json::value& line, const grammar::line& grammar)
    {
        if (grammar.list)
        {
            // if required, must be a non-empty array; otherwise, must be either a (possibly empty) array or null
            const bool valid = grammar.required ? line.is_array() && 0 < line.size() : line.is_array() || line.is_null();
            if (!valid) throw sdp_format_error("expected an array for " + utility::us2s(grammar.name));
            if (line.is_null()) return;

            for (const auto& each : line.as_array())
            {
                os << grammar.type << '=' << grammar.value_converter.format(each) << "\r\n";
            }
        }
        else
        {
            // if required, must be non-null
            const bool valid = !grammar.required || !line.is_null();
            if (!valid) throw sdp_format_error("expected a value for " + utility::us2s(grammar.name));
            if (line.is_null()) return;

            os << grammar.type << '=' << grammar.value_converter.format(line) << "\r\n";
        }
    }

    void write_elements(std::ostream& os, const web::json::value& description, const std::vector<grammar::description::element>& elements);

    void write_description(std::ostream& os, const web::json::value& description, const grammar::description& grammar)
    {
        if (grammar.list)
        {
            // if required, must be a non-empty array; otherwise, must be either a (possibly empty) array or null
            const bool valid = grammar.required ? description.is_array() && 0 < description.size() : description.is_array() || description.is_null();
            if (!valid) throw sdp_format_error("expected an array for " + utility::us2s(grammar.name));
            if (description.is_null()) return;

            for (const auto& each : description.as_array())
            {
                if (!each.is_object()) throw sdp_format_error("expected an object for each " + utility::us2s(grammar.name));
                write_elements(os, each, grammar.elements);
            }
        }
        else
        {
            // if required, must be an object; otherwise, must be either an object or null
            const bool valid = grammar.required ? description.is_object() : description.is_object() || description.is_null();
            if (!valid) throw sdp_format_error("expected an object for " + utility::us2s(grammar.name));
            if (description.is_null()) return;

            write_elements(os, description, grammar.elements);
        }
    }

    void write_elements(std::ostream& os, const web::json::value& description, const std::vector<grammar::description::element>& elements)
    {
        for (auto& element : elements)
        {
            if (const sdp::grammar::line* sub_grammar = boost::get<sdp::grammar::line>(&element))
            {
                const web::json::field_as_value_or value_or_null{ sub_grammar->name, {} };
                write_line(os, value_or_null(description), *sub_grammar);
            }
            else if (const sdp::grammar::description* sub_grammar = boost::get<sdp::grammar::description>(&element))
            {
                const web::json::field_as_value_or value_or_null{ sub_grammar->name, {} };
                write_description(os, value_or_null(description), *sub_grammar);
            }
        }
    }

    // make a complete SDP session description out of its json representation, using the specified grammar
    std::string make_session_description(const web::json::value& session_description, const grammar::description& grammar)
    {
        std::ostringstream result;
        write_description(result, session_description, grammar);
        return result.str();
    }

    web::json::value read_equals_value(std::istream& is, const grammar::converter& value_converter)
    {
        if ('=' != is.get()) throw sdp_parse_error("expected '='");

        std::string line;
        std::getline(is, line, '\n');
        if ('\r' == line.back()) line.pop_back();
        // else throw sdp_parse_error("expected CRLF");

        return value_converter.parse(line);
    }

    void read_line(std::istream& is, int& line_number, web::json::value& line, const grammar::line& grammar)
    {
        if (grammar.list)
        {
            const auto peek_type = is.peek();

            // if required, must be correct type
            const bool valid = !grammar.required || grammar.type == peek_type;
            if (!valid) throw sdp_parse_error("expected a line for " + utility::us2s(grammar.name));
            if (grammar.type != peek_type) return;

            auto lines = web::json::value::array();
            do
            {
                is.get();
                web::json::push_back(lines, read_equals_value(is, grammar.value_converter));
                ++line_number;
            } while (grammar.type == is.peek());
            line = std::move(lines);
        }
        else
        {
            const auto peek_type = is.peek();

            // if required, must be correct type
            const bool valid = !grammar.required || grammar.type == peek_type;
            if (!valid) throw sdp_parse_error("expected a line for " + utility::us2s(grammar.name));
            if (grammar.type != peek_type) return;

            is.get();
            line = read_equals_value(is, grammar.value_converter);
            ++line_number;
        }
    }

    void read_elements(std::istream& is, int& line_number, web::json::value& description, const std::vector<grammar::description::element>& elements);

    void read_description(std::istream& is, int& line_number, web::json::value& description, const grammar::description& grammar)
    {
        // for simplicity of implementation, thankfully a description grammar always begins with a required line
        if (grammar.elements.empty()) throw std::logic_error("a description grammar  must not be empty");
        const sdp::grammar::line* sub_grammar = boost::get<sdp::grammar::line>(&grammar.elements.front());
        if (!sub_grammar || !sub_grammar->required) throw std::logic_error("a description grammar must begin with a required line");

        if (grammar.list)
        {
            const auto peek_type = is.peek();

            // if required, must be correct type
            const bool valid = !grammar.required || sub_grammar->type == peek_type;
            if (!valid) throw sdp_parse_error("expected a line for " + utility::us2s(sub_grammar->name));
            if (sub_grammar->type != peek_type) return;

            description = web::json::value::array();
            do
            {
                web::json::push_back(description, web::json::value::object(sdp::grammar::keep_order));
                read_elements(is, line_number, web::json::back(description), grammar.elements);
            } while (sub_grammar->type == is.peek());
        }
        else
        {
            const auto peek_type = is.peek();

            // if required, must be correct type
            const bool valid = !grammar.required || sub_grammar->type == peek_type;
            if (!valid) throw sdp_parse_error("expected a line for " + utility::us2s(sub_grammar->name));
            if (sub_grammar->type != peek_type) return;

            description = web::json::value::object(sdp::grammar::keep_order);
            read_elements(is, line_number, description, grammar.elements);
        }
    }

    void read_elements(std::istream& is, int& line_number, web::json::value& description, const std::vector<grammar::description::element>& elements)
    {
        for (auto& element : elements)
        {
            if (const sdp::grammar::line* sub_grammar = boost::get<sdp::grammar::line>(&element))
            {
                web::json::value v;
                read_line(is, line_number, v, *sub_grammar);
                if (!v.is_null()) description[sub_grammar->name] = std::move(v);
            }
            else if (const sdp::grammar::description* sub_grammar = boost::get<sdp::grammar::description>(&element))
            {
                web::json::value v;
                read_description(is, line_number, v, *sub_grammar);
                if (!v.is_null()) description[sub_grammar->name] = std::move(v);
            }
        }
    }

    // parse a complete SDP session description into its json representation, using the specified grammar
    web::json::value parse_session_description(const std::string& session_description, const grammar::description& grammar)
    {
        std::istringstream is(session_description);
        int line_number = 1; // could stash this in the stream?
        try
        {
            web::json::value result;
            read_description(is, line_number, result, grammar);
            if (!is.eof()) throw sdp_parse_error("unexpected characters before end-of-file");
            return result;
        }
        catch (const sdp_exception& e)
        {
            std::ostringstream os; os << e.what() << " at line " << line_number;
            throw sdp_exception(os.str());
        }
    }

    // make a complete SDP session description out of its json representation, using the default attribute formatters
    std::string make_session_description(const web::json::value& session_description)
    {
        return make_session_description(session_description, sdp::grammar::default_grammar);
    }

    // parse a complete SDP session description into its json representation, using the default attribute parsers
    web::json::value parse_session_description(const std::string& session_description)
    {
        return parse_session_description(session_description, sdp::grammar::default_grammar);
    }
}
