#ifndef BST_REGEX_H
#define BST_REGEX_H

// Provide bst::regex, etc. using either std:: or boost:: symbols

#if !defined(BST_REGEX_STD) && !defined(BST_REGEX_BOOST)

#if defined(__GNUC__)

#if __GNUC__  > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 9)
#define BST_REGEX_STD
#else
#define BST_REGEX_BOOST
#endif

#else

#define BST_REGEX_STD

#endif

#endif

#ifndef BST_REGEX_BOOST

#include <regex>

namespace bst_regex = std;

#else

#include <boost/regex.hpp>

namespace bst_regex = boost;

#endif

namespace bst
{
    using bst_regex::basic_regex;
    using bst_regex::regex;
    using bst_regex::wregex;

    using bst_regex::match_results;
    using bst_regex::cmatch;
    using bst_regex::wcmatch;
    using bst_regex::smatch;
    using bst_regex::wsmatch;

    using bst_regex::sub_match;
    using bst_regex::csub_match;
    using bst_regex::wcsub_match;
    using bst_regex::ssub_match;
    using bst_regex::wssub_match;

    using bst_regex::regex_match;
    using bst_regex::regex_search;

    namespace regex_constants
    {
        using bst_regex::regex_constants::syntax_option_type;

        using bst_regex::regex_constants::icase;
        using bst_regex::regex_constants::nosubs;
        using bst_regex::regex_constants::optimize;
        using bst_regex::regex_constants::collate;
        using bst_regex::regex_constants::ECMAScript;
        using bst_regex::regex_constants::basic;
        using bst_regex::regex_constants::extended;
        using bst_regex::regex_constants::awk;
        using bst_regex::regex_constants::grep;
        using bst_regex::regex_constants::egrep;

        using bst_regex::regex_constants::match_flag_type;

        using bst_regex::regex_constants::match_default;
        using bst_regex::regex_constants::match_not_bol;
        using bst_regex::regex_constants::match_not_eol;
        using bst_regex::regex_constants::match_not_bow;
        using bst_regex::regex_constants::match_not_eow;
        using bst_regex::regex_constants::match_any;
        using bst_regex::regex_constants::match_not_null;
        using bst_regex::regex_constants::match_continuous;
        using bst_regex::regex_constants::match_prev_avail;

        using bst_regex::regex_constants::format_default;
        using bst_regex::regex_constants::format_sed;
        using bst_regex::regex_constants::format_no_copy;
        using bst_regex::regex_constants::format_first_only;

        using bst_regex::regex_constants::error_type;

        using bst_regex::regex_constants::error_collate;
        using bst_regex::regex_constants::error_ctype;
        using bst_regex::regex_constants::error_escape;
        using bst_regex::regex_constants::error_backref;
        using bst_regex::regex_constants::error_brack;
        using bst_regex::regex_constants::error_paren;
        using bst_regex::regex_constants::error_brace;
        using bst_regex::regex_constants::error_badbrace;
        using bst_regex::regex_constants::error_range;
        using bst_regex::regex_constants::error_space;
        using bst_regex::regex_constants::error_badrepeat;
        using bst_regex::regex_constants::error_complexity;
        using bst_regex::regex_constants::error_stack;
    }
}

#endif
