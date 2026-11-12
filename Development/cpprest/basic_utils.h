#ifndef CPPREST_BASIC_UTILS_H
#define CPPREST_BASIC_UTILS_H

#include <stdexcept> // for std::range_error
#include "cpprest/asyncrt_utils.h" // for cpprest/details/basic_types.h and utility::conversions

namespace utility
{
    namespace conversions
    {
        namespace details
        {
            // non-throwing overload of function in cpprest/asyncrt_utils.h
            template <typename Source>
            utility::string_t print_string(const Source& val, const utility::string_t& default_str)
            {
                utility::ostringstream_t oss;
                oss.imbue(std::locale::classic());
                oss << val;
                return !oss.fail() ? oss.str() : default_str;
            }

            // non-throwing overload of function in cpprest/asyncrt_utils.h
            template <typename Target>
            Target scan_string(const utility::string_t& str, const Target& default_val)
            {
                Target t;
                utility::istringstream_t iss(str);
                iss.imbue(std::locale::classic());
                iss >> t;
                return !iss.fail() ? t : default_val;
            }
        }

        // Encode the given byte array into a base64url string
        // using the alternative alphabet and skipping the padding
        // as per https://tools.ietf.org/html/rfc4648#section-5
        inline utility::string_t to_base64url(const std::vector<unsigned char>& data)
        {
            auto str = utility::conversions::to_base64(data);
            auto it = str.begin();
            for (; str.end() != it; ++it)
            {
                auto& c = *it;
                if (U('=') == c) break;
                if (U('+') == c) c = U('-');
                else if (U('/') == c) c = U('_');
            }
            str.erase(it, str.end());
            return str;
        }

        // Decode the given base64url string to a byte array
        // using the alternative alphabet and skipping the padding
        // as per https://tools.ietf.org/html/rfc4648#section-5
        inline std::vector<unsigned char> from_base64url(utility::string_t str)
        {
            for (auto& c : str)
            {
                if (U('-') == c) c = U('+');
                else if (U('_') == c) c = U('/');
            }
            auto m4 = str.size() % 4;
            if (0 != m4) str.insert(str.end(), 4 - m4, U('='));
            return utility::conversions::from_base64(str);
        }
    }
}

#ifndef _TURN_OFF_PLATFORM_STRING
#define US(x) utility::string_t{_XPLATSTR(x)}
#endif

// more convenient utility functions dependent on utility::char_t
namespace utility
{
    inline std::string us2s(const string_t& us)
    {
        return conversions::to_utf8string(us);
    }

    inline string_t s2us(const std::string& s)
    {
        return conversions::to_string_t(s);
    }

    namespace details
    {
        // Decode a narrow string that is not valid UTF-8, using the system narrow encoding
        // (the Windows ANSI code page) where there is one, and Latin-1 otherwise
        // Latin-1 cannot fail, so neither can this; no input is rejected and nothing is discarded
        string_t system_narrow_to_us(const std::string& s);
    }

    // Convert a narrow string of uncertain encoding to string_t
    // s2us requires valid UTF-8 and throws std::range_error otherwise; use this instead at
    // boundaries where the narrow string comes from the platform rather than from this codebase,
    // e.g. std::exception::what(), system error messages, or __FILE__, none of which are
    // guaranteed to be UTF-8 - on Windows they are typically in the ANSI code page
    // Never throws, and never discards a message
    inline string_t s2us_lenient(const std::string& s)
    {
        if (s.empty()) return{};
        try
        {
            // conversions::to_utf16string is the same decoder that s2us itself uses on a wide
            // build, on every platform, so this accepts exactly what s2us accepts and the
            // fallback handles exactly what it would have rejected
            // note this deliberately discards the result and converts s below, rather than
            // converting the decoded value, so that whenever s2us would have succeeded this
            // returns precisely what s2us returns, on a narrow build as well as a wide one
            (void)conversions::to_utf16string(s);
        }
        catch (const std::range_error&)
        {
            return details::system_narrow_to_us(s);
        }
        return conversions::to_string_t(s);
    }

    template <typename T>
    inline string_t ostringstreamed(const T& value)
    {
        return conversions::details::print_string(value, {});
    }

    template <typename T>
    inline T istringstreamed(const string_t& value, const T& default_value = {})
    {
        return conversions::details::scan_string(value, default_value);
    }
}

#endif
