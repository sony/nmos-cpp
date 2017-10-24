#ifndef CPPREST_BASIC_UTILS_H
#define CPPREST_BASIC_UTILS_H

#include "cpprest/asyncrt_utils.h" // for cpprest/details/basic_types.h and utility::conversions

#ifndef _TURN_OFF_PLATFORM_STRING
#define US(x) utility::string_t{U(x)}
#endif

// more utility functions dependent on utility::char_t
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
        return [&]{ ostringstream_t os; os << value; return os.str(); }();
    }

    template <typename T>
    inline T istringstreamed(const string_t& value, const T& default_value = {})
    {
        return [&]{ T result{ default_value }; istringstream_t is(value); is >> result; return result; }();
    }
}

#endif
