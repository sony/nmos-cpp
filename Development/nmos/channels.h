#ifndef NMOS_CHANNELS_H
#define NMOS_CHANNELS_H

#include "cpprest/json_ops.h"
#include "cpprest/basic_utils.h"
#include "nmos/string_enum.h"

namespace nmos
{
    // Audio channel symbols (used in audio sources) from VSF TR-03 Appendix A
    // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/APIs/schemas/source_audio.json
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
        // Left Surround Sound
        const channel_symbol Lss{ U("Lss") };
        // Right Surround Sound
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

        // Numbered Source Channel
        inline const channel_symbol NSC(unsigned int channel_number)
        {
            return channel_symbol{ U("NSC") + utility::ostringstreamed(channel_number) };
        }

        // Undefined
        inline const channel_symbol Undefined(unsigned int channel_number)
        {
            return channel_symbol{ U("U") + utility::ostringstreamed(channel_number) };
        }

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

    inline web::json::value make_channel(const channel& channel)
    {
        return web::json::value_of({
            { U("label"), web::json::value(channel.label) },
            { U("symbol"), web::json::value(channel.symbol.name) }
        });
    }
}

#endif
