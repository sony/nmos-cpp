#ifndef NMOS_VPID_CODE_H
#define NMOS_VPID_CODE_H

#include <cstdint>

namespace nmos
{
    // SMPTE ST 352 (Video) Payload Identification Codes for Serial Digital Interfaces
    // Byte 1: Payload and Digital Interface Identification
    typedef uint8_t vpid_code;

    // SMPTE ST 352 Video Payload ID Codes for Serial Digital Interfaces
    // See https://smpte-ra.org/video-payload-id-codes-serial-digital-interfaces
    namespace vpid_codes
    {
        // 483/576-line interlaced payloads on 270 Mb/s and 360 Mb/s serial digital interfaces
        const vpid_code vpid_270Mbps = 129;
        // 483/576-line extended payloads on 360 Mb/s single-link and 270 Mb/s dual-link serial digital interfaces
        const vpid_code vpid_360Mbps = 130;
        // 483/576-line payloads on a 540 Mb/s serial digital interface
        const vpid_code vpid_540Mbps = 131;
        // 483/576-line payloads on a 1.485 Gb/s (nominal) serial digital interface
        const vpid_code vpid_1_5Gbps = 132;

        // extensible enum
    }
}

#endif
