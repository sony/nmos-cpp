#ifndef NMOS_TAI_H
#define NMOS_TAI_H

#include <chrono>
#include <cstdint>
#include <tuple>

namespace nmos
{
    // TAI timestamps are used to construct version field values (<seconds>:<nanoseconds>)
    struct tai
    {
        std::int64_t seconds;
        std::int64_t nanoseconds;

        tai() : seconds(0), nanoseconds(0) {}
        tai(std::int64_t seconds, std::int64_t nanoseconds) : seconds(seconds), nanoseconds(nanoseconds) {}
        auto tied() const -> decltype(std::tie(seconds, nanoseconds)) { return std::tie(seconds, nanoseconds); }
        friend bool operator==(const tai& lhs, const tai& rhs) { return lhs.tied() == rhs.tied(); }
        friend bool operator< (const tai& lhs, const tai& rhs) { return lhs.tied() <  rhs.tied(); }
        friend bool operator> (const tai& lhs, const tai& rhs) { return rhs < lhs; }
        friend bool operator!=(const tai& lhs, const tai& rhs) { return !(lhs == rhs); }
        friend bool operator<=(const tai& lhs, const tai& rhs) { return !(rhs < lhs); }
        friend bool operator>=(const tai& lhs, const tai& rhs) { return !(lhs < rhs); }

        // tai arithmetic is best performed by using the functions, tai_from_time_point, etc., to convert to/from
        // tai_clock::time_point (or tai_clock::duration, since NMOS uses relative timestamps in a few places)
    };

    // tai_clock is a std::chrono Clock for generating and manipulating tai timestamps
    struct tai_clock
    {
        // "It is suggested (although not mandated) that these timestamps are stored with nanosecond resolution."
        typedef std::nano period;

        // Given this rep, this clock has a future range of only 292 years. Good enough.
        typedef int64_t rep;

        typedef std::chrono::duration<rep, period> duration;
        typedef std::chrono::time_point<tai_clock, duration> time_point;

        // "It is important that there are no duplicate creation or update timestamps stored against resources."
        // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/docs/2.5.%20APIs%20-%20Query%20Parameters.md#pagination
        // Unfortunately, this clock is based on the system_clock, so may not produce monotonically increasing
        // time points; nmos::strictly_increasing_update is used to prevent duplicate values in nmos::resources
        static const bool is_steady = std::chrono::system_clock::is_steady;

        static time_point now()
        {
            // "NMOS specifications shall use the PTP/SMPTE Epoch, i.e. 1 January 1970 00:00:00 TAI."
            // See https://github.com/AMWA-TV/nmos-content-model/blob/master/specifications/ContentModel.md#epoch

            // "The [C++] standard does not specify the epoch for any of these clocks. There is a de-facto
            // (unofficial) standard that std::chrono::system_clock::time_point [...] is defined as the time
            // duration that has elapsed since 00:00:00 [UTC], 1 January 1970, not counting leap seconds."
            // Howard Hinnant has "spoken to the current maintainers of VS, gcc and clang, and gotten informal
            // agreement that they will not change their epoch of system_clock."
            // See https://stackoverflow.com/a/29800557
            // Note that on VS 2013 and earlier, the resolution of these clocks is woeful (10ms).
            // Unfortunately, the fact that system_clock is UTC, means that it is offset from the PTP/SMPTE
            // Epoch by a number of seconds that increases every time a leap second is introduced.
            // A nice solution would be to wait for std::chrono::tai_clock (C++20), or to use Howard Hinnant's
            // date library, but for now, just fix the offset to the current value...
            // "As of December 2017, UTC is 37 seconds behind TAI, reflecting the 10-second initial offset and
            // the 27 leap seconds inserted between 1958 and 2017. Thus, 2018-01-01 00:00:00 UTC is equivalent
            // to 2018-01-01 00:00:37 TAI."
            // See https://en.cppreference.com/w/cpp/chrono/tai_clock
            // and https://en.wikipedia.org/wiki/International_Atomic_Time
            // and https://github.com/HowardHinnant/date/issues/129
            // and https://cr.yp.to/proto/utctai.html
            // and https://www.iers.org/SharedDocs/News/EN/BulletinC.html
            // and https://www.ietf.org/timezones/data/leap-seconds.list
            static const duration tai_offset = std::chrono::seconds(37);

            return time_point(tai_offset + std::chrono::system_clock::now().time_since_epoch());
        }
    };

    inline tai tai_from_duration(const tai_clock::duration& d)
    {
        const auto sec = std::chrono::duration_cast<std::chrono::seconds>(d);
        const auto nan = std::chrono::duration_cast<std::chrono::nanoseconds>(d) - sec;

        return{ sec.count(), nan.count() };
    }

    inline tai tai_from_time_point(const tai_clock::time_point& tp)
    {
        return tai_from_duration(tp.time_since_epoch());
    }

    inline tai_clock::duration duration_from_tai(const tai& tai)
    {
        const auto sec = std::chrono::seconds(tai.seconds);
        const auto nan = std::chrono::nanoseconds(tai.nanoseconds);
        return std::chrono::duration_cast<tai_clock::duration>(sec + nan);
    }

    inline tai_clock::time_point time_point_from_tai(const tai& tai)
    {
        return tai_clock::time_point(duration_from_tai(tai));
    }

    inline tai tai_now()
    {
        return tai_from_time_point(tai_clock::now());
    }

    inline tai tai_min()
    {
        // equivalent to tai_from_time_point(tai_clock::time_point())
        return{ 0, 0 };
        // not using tai_from_time_point((tai_clock::time_point::min)())
        // since that is negative, and invalid according to the NMOS schema
    }

    inline tai tai_max()
    {
        return tai_from_time_point((tai_clock::time_point::max)());
        // not using e.g. { (std::numeric_limits<std::int64_t>::max)(), 999999999 }
        // since such values cannot be represented as tai_clock::time_point
    }
}

#endif
