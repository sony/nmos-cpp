#ifndef CPPREST_BASIC_UTILS_H
#define CPPREST_BASIC_UTILS_H

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
