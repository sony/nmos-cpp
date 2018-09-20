#ifndef NMOS_CHANNELS_H
#define NMOS_CHANNELS_H

#include "cpprest/json_utils.h"
#include "cpprest/basic_utils.h"
#include "nmos/string_enum.h"

namespace nmos
{
    // Audio channel symbols (used in audio sources)
    // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/APIs/schemas/source_audio.json
    DEFINE_STRING_ENUM(channel_symbol)
    namespace channel_symbols
    {
        const channel_symbol L{ U("L") };
        const channel_symbol R{ U("R") };
        const channel_symbol C{ U("C") };
        const channel_symbol LFE{ U("LFE") };
        const channel_symbol Ls{ U("Ls") };
        const channel_symbol Rs{ U("Rs") };
        const channel_symbol Lss{ U("Lss") };
        const channel_symbol Rss{ U("Rss") };
        const channel_symbol Lrs{ U("Lrs") };
        const channel_symbol Rrs{ U("Rrs") };
        const channel_symbol Lc{ U("Lc") };
        const channel_symbol Rc{ U("Rc") };
        const channel_symbol Cs{ U("Cs") };
        const channel_symbol HI{ U("HI") };
        const channel_symbol VIN{ U("VIN") };
        const channel_symbol M1{ U("M1") };
        const channel_symbol M2{ U("M2") };
        const channel_symbol Lt{ U("Lt") };
        const channel_symbol Rt{ U("Rt") };
        const channel_symbol Lst{ U("Lst") };
        const channel_symbol Rst{ U("Rst") };
        const channel_symbol S{ U("S") };

        inline const channel_symbol NSC(unsigned int channel_number)
        {
            return channel_symbol{ U("NSC") + utility::ostringstreamed(channel_number) };
        }

        inline const channel_symbol Undefined(unsigned int channel_number)
        {
            return channel_symbol{ U("U") + utility::ostringstreamed(channel_number) };
        }
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
