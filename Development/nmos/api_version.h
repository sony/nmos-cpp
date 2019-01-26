#ifndef NMOS_API_VERSION_H
#define NMOS_API_VERSION_H

#include <cstdint>
#include <tuple>
#include "cpprest/details/basic_types.h"

namespace nmos
{
    // APIs are versioned, and since a Node or Registry may communicate resources via multiple versions of an API,
    // the native API version of each resource must be tracked

    struct api_version
    {
        std::uint32_t major;
        std::uint32_t minor;

        api_version() : major{ 0 }, minor{ 0 } {}
        api_version(std::uint32_t major, std::uint32_t minor) : major{ major }, minor{ minor } {}
        auto tied() const -> decltype(std::tie(major, minor)) { return std::tie(major, minor); }
        friend bool operator==(const api_version& lhs, const api_version& rhs) { return lhs.tied() == rhs.tied(); }
        friend bool operator< (const api_version& lhs, const api_version& rhs) { return lhs.tied() <  rhs.tied(); }
        friend bool operator> (const api_version& lhs, const api_version& rhs) { return rhs < lhs; }
        friend bool operator!=(const api_version& lhs, const api_version& rhs) { return !(lhs == rhs); }
        friend bool operator<=(const api_version& lhs, const api_version& rhs) { return !(rhs < lhs); }
        friend bool operator>=(const api_version& lhs, const api_version& rhs) { return !(lhs < rhs); }
    };

    inline utility::string_t make_api_version(const api_version& version)
    {
        utility::ostringstream_t os;
        os << U('v') << version.major << U('.') << version.minor;
        return os.str();
    }

    inline api_version parse_api_version(const utility::string_t& version)
    {
        utility::istringstream_t is(version);
        api_version result;
        utility::char_t v, point;
        is >> v >> result.major >> point >> result.minor;
        return !is.fail() && U('v') == v && U('.') == point ? result : api_version{};
    }
}

#endif
