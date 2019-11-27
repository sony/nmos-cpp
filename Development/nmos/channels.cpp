#include "nmos/channels.h"

#include <iomanip>
#include <boost/algorithm/string/join.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include "nmos/json_fields.h"

namespace nmos
{
    namespace channel_symbols
    {
        // Numbered Source Channel (001..127)
        const channel_symbol NSC(unsigned int channel_number)
        {
            utility::ostringstream_t symbol;
            symbol << U("NSC") << std::setfill(U('0')) << std::setw(3) << channel_number;
            return channel_symbol{ symbol.str() };
        }

        // Undefined channel (01..64)
        const channel_symbol Undefined(unsigned int channel_number)
        {
            utility::ostringstream_t symbol;
            symbol << U("U") << std::setfill(U('0')) << std::setw(2) << channel_number;
            return channel_symbol{ symbol.str() };
        }
    }

    web::json::value make_channel(const channel& channel)
    {
        return web::json::value_of({
            { nmos::fields::label, channel.label },
            { nmos::fields::symbol, channel.symbol.name }
        });
    }

    channel parse_channel(const web::json::value& channel)
    {
        return{
            nmos::fields::label(channel),
            channel_symbol{ nmos::fields::symbol(channel) }
        };
    }

    // Audio channel order convention grouping symbols
    // See SMPTE ST 2110-30:2017 Table 1 - Channel Order Convention Grouping Symbols
    DEFINE_STRING_ENUM(channel_group_symbol)
    namespace channel_group_symbols
    {
        // Mono
        const channel_group_symbol M{ U("M") };
        // Dual Mono
        const channel_group_symbol DM{ U("DM") };
        // Standard Stereo
        const channel_group_symbol ST{ U("ST") };
        // Matrix Stereo
        const channel_group_symbol LtRt{ U("LtRt") };
        // 5.1 Surround
        const channel_group_symbol S51{ U("51") };
        // 7.1 Surround
        const channel_group_symbol S71{ U("71") };
        // 22.2 Surround
        const channel_group_symbol S222{ U("222") };
        // One SDI audio group
        const channel_group_symbol SGRP{ U("SGRP") };

        // Undefined group (01..64)
        // "Undefined is a special designation which is used to signal groups of channels whose mix is unknown or un-representable,
        // or otherwise non-conformant to the other defined groupings and orderings in the convention."
        const channel_group_symbol Undefined(unsigned int channel_count)
        {
            utility::ostringstream_t symbol;
            symbol << U("U") << std::setfill(U('0')) << std::setw(2) << channel_count;
            return channel_group_symbol{ symbol.str() };
        }

#include "cpprest/details/push_undef_u.h"
        inline const channel_group_symbol U(unsigned int channel_count)
        {
            return Undefined(channel_count);
        }
#include "cpprest/details/pop_u.h"
    }

    namespace details
    {
        // carefully ordered 'largest' to 'smallest'
        static const std::vector<std::pair<std::vector<channel_symbol>, channel_group_symbol>> channel_groups{
            { channel_symbols::S71, channel_group_symbols::S71 },
            { channel_symbols::S51, channel_group_symbols::S51 },
            { channel_symbols::LtRt, channel_group_symbols::LtRt },
            { channel_symbols::ST, channel_group_symbols::ST },
            { channel_symbols::DM, channel_group_symbols::DM },
            { channel_symbols::M, channel_group_symbols::M }
        };

        std::vector<channel_group_symbol> make_channel_group_symbols(const std::vector<channel_symbol>& channels)
        {
            std::vector<channel_group_symbol> group_symbols;

            unsigned int undefined_count = 0;
            auto channel = channels.begin();
            while (channels.end() != channel)
            {
                auto group = channel_groups.begin();
                while (channel_groups.end() != group)
                {
                    auto ch = channel;
                    auto group_ch = group->first.begin();
                    while (channels.end() != ch && group->first.end() != group_ch && *ch == *group_ch)
                    {
                        ++ch;
                        ++group_ch;
                    }
                    if (group->first.end() == group_ch)
                    {
                        channel = ch;
                        break;
                    }
                    ++group;
                }
                if (channel_groups.end() != group)
                {
                    if (0 != undefined_count)
                    {
                        group_symbols.push_back(nmos::channel_group_symbols::Undefined(undefined_count));
                        undefined_count = 0;
                    }
                    group_symbols.push_back(group->second);
                }
                else
                {
                    ++undefined_count;
                    ++channel;
                }
            }
            if (0 != undefined_count)
            {
                group_symbols.push_back(nmos::channel_group_symbols::Undefined(undefined_count));
            }

            return group_symbols;
        }
    }

    // See SMPTE ST 2110-30:2017 Section 6.2.2 Channel Order Convention
    utility::string_t make_fmtp_channel_order(const std::vector<channel_symbol>& channels)
    {
        utility::ostringstream_t channel_order;

        channel_order << U("SMPTE2110.(") << boost::algorithm::join(details::make_channel_group_symbols(channels) | boost::adaptors::transformed([](const channel_group_symbol& symbol)
        {
            return symbol.name;
        }), U(",")) << U(")");

        return channel_order.str();
    }
}
