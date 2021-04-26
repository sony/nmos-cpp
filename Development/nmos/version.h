#ifndef NMOS_VERSION_H
#define NMOS_VERSION_H

#include "nmos/tai.h"

namespace nmos
{
    // "Core resources such as Sources, Flows, Nodes etc. include a 'version' attribute.
    // As properties of a given Flow or similar will change over its lifetime, the version
    // identifies the instant at which this change took place."
    // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/docs/2.1.%20APIs%20-%20Common%20Keys.md#version

    inline utility::string_t make_version(tai tai = tai_now())
    {
        utility::ostringstream_t os;
        os << tai.seconds << U(':') << tai.nanoseconds;
        return os.str();
    }

    inline tai parse_version(const utility::string_t& timestamp)
    {
        utility::istringstream_t is(timestamp);
        tai tai;
        utility::char_t colon;
        is >> tai.seconds >> colon >> tai.nanoseconds;
        return !is.fail() && U(':') == colon ? tai : nmos::tai{};
    }
}

#endif
