#ifndef NMOS_CHANNELS_H
#define NMOS_CHANNELS_H

#include <vector>
#include "nmos/string_enum.h"

namespace web
{
    namespace json
    {
        class value;
    }
}

namespace nmos
{
    // Audio channel symbols (used in audio sources) from VSF TR-03 Appendix A
    // See https://specs.amwa.tv/is-04/releases/v1.2.0/APIs/schemas/with-refs/source_audio.html
    // and http://www.videoservicesforum.org/download/technical_recommendations/VSF_TR-03_2015-11-12.pdf
    DEFINE_STRING_ENUM(channel_symbol)
    namespace channel_symbols
    {
        // Left
        const channel_symbol L{ U("L") };
        // Right
        const channel_symbol R{ U("R") };
        // Center
        const channel_symbol C{ U("C") };
        // Low Frequency Effects
        const channel_symbol LFE{ U("LFE") };
        // Left Surround
        const channel_symbol Ls{ U("Ls") };
        // Right Surround
        const channel_symbol Rs{ U("Rs") };
        // Left Side Surround 
        const channel_symbol Lss{ U("Lss") };
        // Right Side Surround
        const channel_symbol Rss{ U("Rss") };
        // Left Rear Surround
        const channel_symbol Lrs{ U("Lrs") };
        // Right Rear Surround
        const channel_symbol Rrs{ U("Rrs") };
        // Left Center
        const channel_symbol Lc{ U("Lc") };
        // Right Center
        const channel_symbol Rc{ U("Rc") };
        // Center Surround
        const channel_symbol Cs{ U("Cs") };
        // Hearing Impaired
        const channel_symbol HI{ U("HI") };
        // Visually Impaired - Narrative
        const channel_symbol VIN{ U("VIN") };
        // Mono One
        const channel_symbol M1{ U("M1") };
        // Mono Two
        const channel_symbol M2{ U("M2") };
        // Left Total
        const channel_symbol Lt{ U("Lt") };
        // Right Total
        const channel_symbol Rt{ U("Rt") };
        // Left Surround Total
        const channel_symbol Lst{ U("Lst") };
        // Right Surround Total
        const channel_symbol Rst{ U("Rst") };
        // Surround
        const channel_symbol S{ U("S") };

        // Numbered Source Channel (001..127)
        // "due to original regex [in source_audio.json] allowing NSC000, but NSC001-NSC128 possibly
        // being preferable for consistency with U01-U64", it's OK to use NSC000 and NSC128 also!
        // see https://github.com/AMWA-TV/is-04/pull/145
        const channel_symbol NSC(unsigned int channel_number);

        // Undefined channel (01..64)
        const channel_symbol Undefined(unsigned int channel_number);

#include "cpprest/details/push_undef_u.h"
        inline const channel_symbol U(unsigned int channel_number)
        {
            return Undefined(channel_number);
        }
#include "cpprest/details/pop_u.h"
    }

    struct channel
    {
        utility::string_t label;
        nmos::channel_symbol symbol;
    };

    web::json::value make_channel(const channel& channel);
    channel parse_channel(const web::json::value& channel);

    // Audio channel order convention groupings
    // See SMPTE ST 2110-30:2017 Table 1 - Channel Order Convention Grouping Symbols
    namespace channel_symbols
    {
        // Mono
        const std::vector<channel_symbol> M{ M1 };
        // Dual Mono
        const std::vector<channel_symbol> DM{ M1, M2 };
        // Standard Stereo
        const std::vector<channel_symbol> ST{ L, R };
        // Matrix Stereo
        const std::vector<channel_symbol> LtRt{ Lt, Rt };
        // 5.1 Surround
        const std::vector<channel_symbol> S51{ L, R, C, LFE, Ls, Rs };
        // 7.1 Surround
        const std::vector<channel_symbol> S71{ L, R, C, LFE, Lss, Rss, Lrs, Rrs };

        // 22.2 Surround ('222') and SDI audio group ('SGRP') cannot be indicated exactly using the channel symbols defined in NMOS
        // so these are the closest representation, using the channel symbol for the same loudspeaker position where there is one,
        // and otherwise the Undefined channel symbol numbered by the channel's position in the group

        // 22.2 Surround
        // See https://pub.smpte.org/pub/st2036-2/st2036-2-2008.pdf
        // and https://pub.smpte.org/pub/st428-12/st0428-12-2013.pdf
        // and https://static.vsf.tv/download/technical_recommendations/VSF_TR-03_2015-11-12.pdf
        const std::vector<channel_symbol> S222{
            // FL, FR, FC, LFE1, BL, BR
            L, R, C, LFE, Lrs, Rrs,
            // FLc, FRc, BC, LFE2, SiL, SiR
            Lc, Rc, Cs, Undefined(10), Lss, Rss,
            // TpFL, TpFR, TpFC, TpC, TpBL, TpBR
            Undefined(13), Undefined(14), Undefined(15), Undefined(16), Undefined(17), Undefined(18),
            // TpSiL, TpSiR, TpBC, BtFC, BtFL, BtFR
            Undefined(19), Undefined(20), Undefined(21), Undefined(22), Undefined(23), Undefined(24)
        };
        // One SDI audio group
        const std::vector<channel_symbol> SGRP{ Undefined(1), Undefined(2), Undefined(3), Undefined(4) };
    }

    // See SMPTE ST 2110-30:2017 Section 6.2.2 Channel Order Convention
    utility::string_t make_fmtp_channel_order(const std::vector<channel_symbol>& channels);
    std::vector<nmos::channel_symbol> parse_fmtp_channel_order(const utility::string_t& channel_order);
}

#endif
