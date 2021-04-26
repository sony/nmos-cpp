#ifndef NMOS_HEALTH_H
#define NMOS_HEALTH_H

#include "nmos/tai.h"

namespace nmos
{
    // Like version, health is also stored as a timestamp, but only to the nearest second
    typedef std::int64_t health;

    // some resources may (temporarily or permanently) be exempt from expiration
    const health health_forever = (std::numeric_limits<health>::max)();

    inline health health_now()
    {
        return std::chrono::duration_cast<std::chrono::seconds>(
            tai_clock::now().time_since_epoch()).count();
    }

    inline tai_clock::time_point time_point_from_health(health health)
    {
        return tai_clock::time_point(std::chrono::seconds(health));
    }
}

#endif
