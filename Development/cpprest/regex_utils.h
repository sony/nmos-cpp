#ifndef CPPREST_REGEX_UTILS_H
#define CPPREST_REGEX_UTILS_H

#include <map>
#include "bst/regex.h"

// An implementation of named capture on top of std::basic_regex (could be extracted from the cpprest module)
namespace xregex
{
    // parse_regex_named_sub_matches parses a regular expression that is appropriate for bst::regex_match, etc.
    // with the addition of handling the common extension to specify named sub_matches of the form (?<name>...)
    // returning a regex with the name-specifier removed, and a map from each name to its sub_match index
    // See http://xregexp.com/syntax/named_capture_comparison/

    template <typename Char>
    using string_t = std::basic_string<Char>;

    template <typename Char>
    using ostringstream_t = std::basic_ostringstream<Char>;

    template <typename Char>
    using regex_t = bst::basic_regex<Char>;
    template <typename Char>
    using smatch_t = bst::match_results<typename string_t<Char>::const_iterator>;
    template <typename Char>
    using sub_match_t = bst::sub_match<typename string_t<Char>::const_iterator>;

    template <typename Char>
    using named_sub_matches_t = std::map<string_t<Char>, typename smatch_t<Char>::size_type>;
    template <typename Char>
    using regex_named_sub_matches_t = std::pair<string_t<Char>, named_sub_matches_t<Char>>;

    namespace regex_specials
    {
        const char escape = '\\';
        const char sub_match_start = '(';
        const char sub_match_finish = ')';
        const char sub_match_extension = '?';
        const char sub_match_name_start = '<';
        const char sub_match_name_finish = '>';
        const char sub_match_positive_lookahead = '=';
        const char sub_match_negative_lookahead = '!';
        const char sub_match_non_marking = ':';
    }

    template <typename Char>
    string_t<Char> make_named_sub_match(const string_t<Char>& name, const string_t<Char>& sub_match)
    {
        using namespace ::xregex::regex_specials;
        ostringstream_t<Char> os;
        // hmm, sanity-check that name doesn't include sub_match_name_finish?
        os << sub_match_start << sub_match_extension << sub_match_name_start << name << sub_match_name_finish << sub_match << sub_match_finish;
        return os.str(); // i.e. (?<name>sub_match)
    }

    template <typename Char>
    regex_named_sub_matches_t<Char> parse_regex_named_sub_matches(const string_t<Char>& regex)
    {
        using namespace regex_specials;
        enum state_t { normal, escaped, sub_match, extended_sub_match, named_sub_match, unknown_extension };

        regex_named_sub_matches_t<Char> result;

        typename smatch_t<Char>::size_type sub_match_count = 0;
        string_t<Char> sub_match_name;
        state_t state = normal;

        for (auto ch : regex)
        {
            bool copy = true;
            switch (state)
            {
            case normal:
                switch (ch)
                {
                case escape:
                    state = escaped;
                    break;
                case sub_match_start:
                    ++sub_match_count;
                    state = sub_match;
                    break;
                default:
                    //state = normal;
                    break;
                }
                break;
            case escaped:
                state = normal;
                break;
            case sub_match:
                switch (ch)
                {
                case sub_match_extension:
                    copy = false;
                    state = extended_sub_match;
                    break;
                // else identical to normal transitions
                case escape:
                    state = escaped;
                    break;
                case sub_match_start:
                    ++sub_match_count;
                    state = sub_match;
                    break;
                default:
                    state = normal;
                    break;
                }
                break;
            case extended_sub_match:
                switch (ch)
                {
                case sub_match_name_start:
                    copy = false;
                    state = named_sub_match;
                    break;
                case sub_match_positive_lookahead:
                case sub_match_negative_lookahead:
                case sub_match_non_marking:
                    // See http://en.cppreference.com/w/cpp/regex/ecmascript#Assertions
                    // and http://en.cppreference.com/w/cpp/regex/ecmascript#Atoms
                    result.first.push_back(sub_match_extension);
                    --sub_match_count;
                    state = normal;
                    break;
                default:
                    result.first.push_back(sub_match_extension);
                    state = unknown_extension;
                    break;
                }
                break;
            case named_sub_match:
                copy = false;
                switch (ch)
                {
                case sub_match_name_finish:
                    result.second[sub_match_name] = sub_match_count;
                    sub_match_name.clear();
                    state = normal;
                    break;
                default:
                    sub_match_name.push_back(ch);
                    //state = named_sub_match;
                    break;
                }
                break;
            case unknown_extension:
                switch (ch)
                {
                case sub_match_finish:
                    state = normal;
                    break;
                default:
                    //state = unknown_extension;
                    break;
                }
                break;
            default: // unreachable
                break;
            }

            if (copy)
            {
                result.first.push_back(ch);
            }
        }
        return result;
    }
}

#include "cpprest/details/basic_types.h"

// An implementation of named capture for utility::char_t
namespace utility
{
    typedef ::xregex::regex_t<char_t> regex_t; // uregex
    typedef ::xregex::smatch_t<char_t> smatch_t; // usmatch
    typedef ::xregex::sub_match_t<char_t> sub_match_t; // ussub_match

    typedef ::xregex::named_sub_matches_t<char_t> named_sub_matches_t;
    typedef ::xregex::regex_named_sub_matches_t<char_t> regex_named_sub_matches_t;

    inline string_t make_named_sub_match(const string_t& name, const string_t& sub_match)
    {
        return ::xregex::make_named_sub_match(name, sub_match);
    }

    inline regex_named_sub_matches_t parse_regex_named_sub_matches(const string_t& regex)
    {
        return ::xregex::parse_regex_named_sub_matches(regex);
    }
}

#endif
