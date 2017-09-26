#ifndef NMOS_TAI_H
#define NMOS_TAI_H

#include <chrono>

namespace nmos
{
    // TAI timestamps are used to construct version field values (<seconds>:<nanoseconds>)

    struct tai
    {
        std::int64_t seconds;
        std::int64_t nanoseconds;

        auto tied() const -> decltype(std::tie(seconds, nanoseconds)) { return std::tie(seconds, nanoseconds); }
        friend bool operator==(const tai& lhs, const tai& rhs) { return lhs.tied() == rhs.tied(); }
        friend bool operator< (const tai& lhs, const tai& rhs) { return lhs.tied() <  rhs.tied(); }
        friend bool operator> (const tai& lhs, const tai& rhs) { return rhs < lhs; }
        friend bool operator!=(const tai& lhs, const tai& rhs) { return !(lhs == rhs); }
        friend bool operator<=(const tai& lhs, const tai& rhs) { return !(rhs < lhs); }
        friend bool operator>=(const tai& lhs, const tai& rhs) { return !(lhs < rhs); }
    };

    // tai_clock has a nanosecond period, which is used by the nmos::resources model to implement the requirement
    // from the NMOS specification "that there are no duplicate creation or update timestamps stored against resources.
    // It is suggested (although not mandated) that these timestamps are stored with nanosecond resolution."
    // Given the rep, this clock has a range of only 292 years. Good enough.
    struct tai_clock
    {
        typedef int64_t rep;
        typedef std::nano period;
        typedef std::chrono::duration<rep, period> duration;
        typedef std::chrono::time_point<std::chrono::high_resolution_clock, duration> time_point;
        static const bool is_steady = false;

        static time_point now()
        {
            return time_point(std::chrono::high_resolution_clock::now());
        }
    };

    inline tai tai_from_time_point(const tai_clock::time_point& tp)
    {
        const auto tse = tp.time_since_epoch();
        const auto sec = std::chrono::duration_cast<std::chrono::seconds>(tse);
        const auto nan = std::chrono::duration_cast<std::chrono::nanoseconds>(tse) - sec;

        return{ sec.count(), nan.count() };
    }

    inline tai_clock::time_point time_point_from_tai(const tai& tai)
    {
        const auto sec = std::chrono::seconds(tai.seconds);
        const auto nan = std::chrono::nanoseconds(tai.nanoseconds);
        const auto tse = std::chrono::duration_cast<tai_clock::duration>(sec + nan);

        return tai_clock::time_point(tse);
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
