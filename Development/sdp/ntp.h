#ifndef SDP_NTP_H
#define SDP_NTP_H

#include <cstdint>
#include <chrono>

namespace sdp
{
    // ntp_clock is a std::chrono Clock for generating and manipulating Network Time Protocol (NTP) format timestamps
    // as suggested for Origin session-id and session-version
    // See https://tools.ietf.org/html/rfc4566#section-5.2
    struct ntp_clock
    {
        typedef std::ratio<1, 0x100000000ul> period;

        typedef std::uint64_t rep;

        typedef std::chrono::duration<rep, period> duration;
        typedef std::chrono::time_point<ntp_clock, duration> time_point;

        static const bool is_steady = false;

        static time_point now()
        {
            //static const std::chrono::seconds ntp_offset(2208988800);
            //return time_point(std::chrono::duration_cast<duration>(ntp_offset + std::chrono::system_clock::now().time_since_epoch()));
            // except that with period and ntp_offset, we're operating too close to uint64_t/intmax_t limits so...
            typedef std::chrono::duration<rep> useconds;
            static const useconds ntp_offset(2208988800);
            const auto tse = std::chrono::system_clock::now().time_since_epoch();
            const auto seconds = std::chrono::duration_cast<useconds>(tse);
            const auto fraction = tse - seconds;
            const auto ntp_seconds = ntp_offset.count() + seconds.count();
            const auto ntp_fraction = std::chrono::duration_cast<duration>(fraction).count();
            return time_point(duration(ntp_seconds * period::den / period::num + ntp_fraction));
        }
    };

    inline ntp_clock::rep ntp_now()
    {
        return ntp_clock::now().time_since_epoch().count();
    }
}

#endif
