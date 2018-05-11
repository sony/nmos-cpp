#include "cpprest/host_utils.h"

#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/host_name.hpp>
#include <boost/asio/ip/tcp.hpp>
#include "cpprest/basic_utils.h" // for utility::s2us, etc.

#if !defined(_WIN32)
#include <ifaddrs.h> // for ifaddrs
#include <net/if.h> // for ifa_flags
#endif

namespace web
{
    namespace http
    {
        namespace experimental
        {
            utility::string_t host_name()
            {
                return utility::s2us(boost::asio::ip::host_name());
            }

            namespace details
            {
                // this just shouldn't be this hard!
                boost::asio::ip::address from_sockaddr(const sockaddr& address)
                {
                    if (address.sa_family == AF_INET)
                    {
                        auto addr_in = (const sockaddr_in&)address;
                        auto addr = addr_in.sin_addr.s_addr;

                        return boost::asio::ip::address_v4(ntohl(addr));
                    }
                    else if (address.sa_family == AF_INET6)
                    {
                        auto addr_in6 = (const sockaddr_in6&)address;
                        auto addr6 = addr_in6.sin6_addr.s6_addr;
                        boost::asio::ip::address_v6::bytes_type addr6_bytes;
                        std::copy_n(addr6, addr6_bytes.size(), addr6_bytes.begin());

                        //auto scope_id = addr_in6.sin6_scope_id;
                        //return boost::asio::ip::address_v6(addr6_bytes, scope_id);
                        return boost::asio::ip::address_v6(addr6_bytes);
                    }
                    else
                    {
                        return{};
                    }
                }
            }

            std::vector<utility::string_t> interface_addresses()
            {
                std::vector<utility::string_t> addresses;

                // for now, limited to IPv4
                const bool ipv6 = false;

#if defined(_WIN32)
                addrinfo hints = {};
                hints.ai_family = ipv6 ? AF_UNSPEC : AF_INET;

                addrinfo* results = NULL;

                // On Windows, getaddrinfo supports an extension so that "If the pNodeName parameter contains
                // an empty string, all registered addresses on the local computer are returned."
                // Loopback addresses appear to be returned in some circumstances (e.g. no active interfaces)
                // so those must be filtered out.
                // See https://msdn.microsoft.com/en-us/library/windows/desktop/ms738520(v=vs.85).aspx
                if (0 == ::getaddrinfo("", NULL, &hints, &results))
                {
                    for (auto ptr = results; ptr != NULL; ptr = ptr->ai_next)
                    {
                        const auto ip_address = details::from_sockaddr(*ptr->ai_addr);

                        if (ip_address.is_unspecified()) continue;
                        if (ip_address.is_loopback()) continue;

                        if (!ipv6 && ip_address.is_v6()) continue;

                        addresses.push_back(utility::conversions::to_string_t(ip_address.to_string()));
                    }

                    ::freeaddrinfo(results);
                }
#else
                ifaddrs* results = NULL;

                // On Linux, getaddrinfo does not have the same extension; getifaddrs is used instead
                // Loopback addresses are returned by this function (and "also one AF_PACKET address per
                // interface containing lower-level details about the interface and its physical layer")
                // so those must be filtered out.
                // See http://man7.org/linux/man-pages/man3/getifaddrs.3.html
                if (0 == ::getifaddrs(&results))
                {
                    for (auto ptr = results; ptr != NULL; ptr = ptr->ifa_next)
                    {
                        if (NULL == ptr->ifa_addr) continue;

                        if (0 == (ptr->ifa_flags & IFF_UP)) continue;
                        // could also check IFF_RUNNING?

                        if (0 != (ptr->ifa_flags & IFF_LOOPBACK)) continue;

                        if (!ipv6 && ptr->ifa_addr->sa_family == AF_INET6) continue;

                        const auto ip_address = details::from_sockaddr(*ptr->ifa_addr);

                        if (ip_address.is_unspecified()) continue;
                        if (ip_address.is_loopback()) continue;

                        if (!ipv6 && ip_address.is_v6()) continue;

                        addresses.push_back(utility::conversions::to_string_t(ip_address.to_string()));
                    }

                    ::freeifaddrs(results);
                }
#endif

                return addresses;
            }

            std::vector<utility::string_t> host_addresses(const utility::string_t& host_name)
            {
                boost::asio::io_service service;
                boost::asio::ip::tcp::resolver resolver(service);
                std::vector<utility::string_t> addresses;
                boost::system::error_code ec;
                // for now, limited to IPv4
                const auto results = resolver.resolve({ boost::asio::ip::tcp::v4(), utility::us2s(host_name), "" }, ec);
#if BOOST_VERSION >= 106600
                for (const auto& re : results)
                {
#else
                for (auto it = results; it != boost::asio::ip::tcp::resolver::iterator{}; ++it)
                {
                    const auto& re = *it;
#endif
                    addresses.push_back(utility::s2us(re.endpoint().address().to_string()));
                }
                return addresses; // empty if host_name cannot be resolved
            }
        }
    }
}
