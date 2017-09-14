#ifndef CPPREST_BASIC_UTILS_H
#define CPPREST_BASIC_UTILS_H

#include <codecvt>
#include "cpprest/details/basic_types.h"

#ifndef _TURN_OFF_PLATFORM_STRING
#define US(x) utility::string_t{U(x)}
#endif

// more utility functions dependent on utility::char_t
namespace utility
{
    inline std::string us2s(const string_t& us)
    {
        return std::wstring_convert<std::codecvt_utf8<utility::char_t>, utility::char_t>().to_bytes(us);
    }

    inline string_t s2us(const std::string& s)
    {
        return std::wstring_convert<std::codecvt_utf8<utility::char_t>, utility::char_t>().from_bytes(s);
    }

    template <typename T>
    inline string_t ostringstreamed(const T& value)
    {
        return [&]{ ostringstream_t os; os << value; return os.str(); }();
    }

    template <typename T>
    inline T istringstreamed(const string_t& value)
    {
        return [&]{ T result{}; istringstream_t is(value); is >> result; return result; }();
    }
}

#endif
