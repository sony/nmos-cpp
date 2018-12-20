#ifndef NMOS_API_VERSION_H
#define NMOS_API_VERSION_H

#include <cstdint>
#include <tuple>
#include <vector>
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

    namespace is04_versions
    {
        const api_version v1_0{ 1, 0 };
        const api_version v1_1{ 1, 1 };
        const api_version v1_2{ 1, 2 };
        const api_version v1_3{ 1, 3 };

        const std::vector<api_version> all{ nmos::is04_versions::v1_0, nmos::is04_versions::v1_1, nmos::is04_versions::v1_2, nmos::is04_versions::v1_3 };
    }

    namespace is05_versions
    {
        const api_version v1_0{ 1, 0 };
    }

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
