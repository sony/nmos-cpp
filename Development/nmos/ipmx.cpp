#include "nmos/ipmx.h"

#include <stdexcept>
#include <boost/asio/ip/address_v4.hpp>
#include "cpprest/basic_utils.h"

namespace nmos
{
    namespace ipmx
    {
        // Default multicast IPv4 address of a media stream of an IPMX Sender, 239.S.C.D, given the IPv4 address A.B.C.D
        // of its media network port and the stream number S, which must be greater than 0 and less than 128
        // See VSF TR-10-9:2025 Section 17.1
        utility::string_t make_default_multicast_address_v4(const utility::string_t& interface_address, int stream_number)
        {
            if (stream_number <= 0 || 128 <= stream_number) throw std::invalid_argument("IPMX stream number must be greater than 0 and less than 128");

#if BOOST_VERSION >= 106600
            auto bytes = boost::asio::ip::make_address_v4(utility::us2s(interface_address)).to_bytes();
#else
            auto bytes = boost::asio::ip::address_v4::from_string(utility::us2s(interface_address)).to_bytes();
#endif
            bytes[0] = 239;
            bytes[1] = (unsigned char)stream_number;
            return utility::s2us(boost::asio::ip::address_v4(bytes).to_string());
        }
    }
}
