#include "cpprest/host_utils.h"

#include <iomanip> // for std::setfill
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/host_name.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/range/adaptor/filtered.hpp>
#include "cpprest/asyncrt_utils.h" // for utility::conversions

#if defined(_WIN32)
#if !defined(HOST_UTILS_USE_GETADAPTERSADDRESSES) && !defined(HOST_UTILS_USE_GETADDRINFO)
#define HOST_UTILS_USE_GETADAPTERSADDRESSES
#endif
#ifdef HOST_UTILS_USE_GETADAPTERSADDRESSES
#include <iphlpapi.h>
#include "detail/default_init_allocator.h"

#pragma comment(lib, "IPHLPAPI.lib")
#endif
#else
#include <ifaddrs.h> // for ifaddrs
#include <net/if.h> // for ifa_flags
#include <resolv.h>
#if defined(__linux__)
#include <linux/if_packet.h> // for sockaddr_ll
#elif defined(__APPLE__)
#include <net/if_dl.h> // for sockaddr_dl
#endif
#endif

namespace web
{
    namespace hosts
    {
        namespace experimental
        {
            utility::string_t host_name()
            {
                return utility::conversions::to_string_t(boost::asio::ip::host_name());
            }

#if !defined(_WIN32)
            // get the default domain name for the system
            utility::string_t domain()
            {
                utility::string_t result;

                struct __res_state res{};
                res.options = RES_DEFAULT;
                if (0 == ::res_ninit(&res))
                {
                    result = utility::conversions::to_string_t(res.defdname);
                    ::res_nclose(&res);
                }
                return result;
            }
#endif

            namespace details
            {
                // this just shouldn't be this hard!
                static boost::asio::ip::address from_sockaddr(const sockaddr& address)
                {
                    if (address.sa_family == AF_INET)
                    {
                        auto& addr_in = (const sockaddr_in&)address;
                        auto& addr = addr_in.sin_addr.s_addr;

                        return boost::asio::ip::address_v4(ntohl(addr));
                    }
                    else if (address.sa_family == AF_INET6)
                    {
                        auto& addr_in6 = (const sockaddr_in6&)address;
                        auto& addr6 = addr_in6.sin6_addr.s6_addr;
                        boost::asio::ip::address_v6::bytes_type addr6_bytes;
                        std::copy_n(addr6, addr6_bytes.size(), addr6_bytes.begin());

                        //auto& scope_id = addr_in6.sin6_scope_id;
                        //return boost::asio::ip::address_v6(addr6_bytes, scope_id);
                        return boost::asio::ip::address_v6(addr6_bytes);
                    }
                    else
                    {
                        return{};
                    }
                }
            }

            namespace details
            {
                inline utility::string_t make_physical_address(const uint8_t* data, const size_t size)
                {
                    utility::ostringstream_t physical_address;
                    physical_address << std::hex << std::nouppercase << std::setfill(U('0'));
                    for (size_t i = 0; i < size; ++i)
                    {
                        if (0 != i) physical_address << U('-');
                        physical_address << std::setw(2) << (uint32_t)data[i];
                    }

                    return physical_address.str();
                }
            }

            std::vector<host_interface> host_interfaces()
            {
                std::vector<host_interface> interfaces;

                // for now, limited to IPv4
                const bool ipv6 = false;

#if defined(_WIN32)
#ifdef HOST_UTILS_USE_GETADAPTERSADDRESSES
                // "The recommended method of calling the GetAdaptersAddresses function is to pre-allocate a 15KB working buffer
                // pointed to by the AdapterAddresses parameter."
                // See https://docs.microsoft.com/en-gb/windows/win32/api/iphlpapi/nf-iphlpapi-getadaptersaddresses
                const auto initial_buffer_size = 15000;
                const auto max_tries = 3;

                std::vector<unsigned char, detail::default_init_allocator<unsigned char>> working_buffer(initial_buffer_size); // default-initialized
                PIP_ADAPTER_ADDRESSES adapter_addresses = (PIP_ADAPTER_ADDRESSES)working_buffer.data();
                ULONG size = (ULONG)working_buffer.size();

                const auto family = ipv6 ? AF_UNSPEC : AF_INET;

                for (auto tries = 0;;)
                {
                    ULONG rv = ::GetAdaptersAddresses(family, GAA_FLAG_INCLUDE_PREFIX, NULL, adapter_addresses, &size);
                    ++tries;
                    if (rv == ERROR_SUCCESS) break;
                    else if (rv == ERROR_BUFFER_OVERFLOW && tries < max_tries)
                    {
                        // the buffer was not big enough; make it bigger and try again
                        working_buffer.resize(size);
                        adapter_addresses = (PIP_ADAPTER_ADDRESSES)working_buffer.data();
                    }
                    else return interfaces; // empty
                }

                for (PIP_ADAPTER_ADDRESSES adapter = adapter_addresses; NULL != adapter; adapter = adapter->Next)
                {
                    if (adapter->OperStatus != IfOperStatusUp) continue;

                    std::vector<utility::string_t> addresses;

                    for (PIP_ADAPTER_UNICAST_ADDRESS address = adapter->FirstUnicastAddress; NULL != address; address = address->Next)
                    {
                        const auto ip_address = details::from_sockaddr(*address->Address.lpSockaddr);

                        if (ip_address.is_unspecified()) continue;
                        if (ip_address.is_loopback()) continue;

                        if (!ipv6 && ip_address.is_v6()) continue;

                        addresses.push_back(utility::conversions::to_string_t(ip_address.to_string()));
                    }

                    interfaces.push_back({
                        (uint32_t)adapter->IfIndex,
                        utility::conversions::to_string_t(adapter->AdapterName),
                        details::make_physical_address(adapter->PhysicalAddress, adapter->PhysicalAddressLength),
                        std::move(addresses),
                        utility::conversions::to_string_t(adapter->DnsSuffix)
                    });
                }
#else
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
                    host_interface interface; // no interface index, name, physical address or domain using getaddrinfo

                    for (auto ptr = results; ptr != NULL; ptr = ptr->ai_next)
                    {
                        const auto ip_address = details::from_sockaddr(*ptr->ai_addr);

                        if (ip_address.is_unspecified()) continue;
                        if (ip_address.is_loopback()) continue;

                        if (!ipv6 && ip_address.is_v6()) continue;

                        interface.addresses.push_back(utility::conversions::to_string_t(ip_address.to_string()));
                    }

                    ::freeaddrinfo(results);

                    interfaces.push_back(std::move(interface));
                }
#endif
#else
                auto nameindex = ::if_nameindex();
                ifaddrs* results = NULL;

                // On Linux, getaddrinfo does not have the same extension; getifaddrs is used instead
                // Loopback addresses are returned by this function (and "also one AF_PACKET address per
                // interface containing lower-level details about the interface and its physical layer")
                // so those must be filtered out.
                // See http://man7.org/linux/man-pages/man3/getifaddrs.3.html
                if (0 != nameindex && 0 == ::getifaddrs(&results))
                {
                    // for now, get the default domain name from the system resolver, not per-interface
                    auto domain = web::hosts::experimental::domain();

                    for (auto ptr = results; ptr != NULL; ptr = ptr->ifa_next)
                    {
                        if (NULL == ptr->ifa_addr) continue;

                        if (0 == (ptr->ifa_flags & IFF_UP)) continue;
                        // could also check IFF_RUNNING?

                        if (0 != (ptr->ifa_flags & IFF_LOOPBACK)) continue;

                        // find interface index for this address
                        const std::string name(ptr->ifa_name);
                        auto ni = nameindex;
                        for (; 0 != ni->if_index; ++ni)
                        {
                            if (ni->if_name == name) break;
                        }
                        const auto index = (uint32_t)ni->if_index;

                        // find or insert a new interface for this interface index
                        auto interface = std::find_if(interfaces.begin(), interfaces.end(), [index](const host_interface& interface)
                        {
                            return interface.index == index;
                        });
                        if (interfaces.end() == interface)
                        {
                            interface = interfaces.insert(interface, 0 != index ? host_interface(index, utility::conversions::to_string_t(name)) : host_interface());
                        }

#if defined __linux__
                        if (ptr->ifa_addr->sa_family == AF_PACKET)
                        {
                            auto& addr_ll = (const sockaddr_ll&)*ptr->ifa_addr;
                            interface->physical_address = details::make_physical_address(addr_ll.sll_addr, addr_ll.sll_halen);
                        }
                        else
#elif defined __APPLE__
                        if (ptr->ifa_addr->sa_family == AF_LINK)
                        {
                            auto& addr_dl = (const sockaddr_dl&)*ptr->ifa_addr;
                            interface->physical_address = details::make_physical_address((const uint8_t*)LLADDR(&addr_dl), addr_dl.sdl_alen);
                        }
                        else
#endif
                        {
                            if (!ipv6 && ptr->ifa_addr->sa_family == AF_INET6) continue;

                            const auto ip_address = details::from_sockaddr(*ptr->ifa_addr);

                            if (ip_address.is_unspecified()) continue;
                            if (ip_address.is_loopback()) continue;

                            if (!ipv6 && ip_address.is_v6()) continue;

                            interface->addresses.push_back(utility::conversions::to_string_t(ip_address.to_string()));
                        }

                        interface->domain = domain;
                    }

                    ::freeifaddrs(results);
                }
                if (0 != nameindex)
                {
                    ::if_freenameindex(nameindex);
                }
#endif

                // don't return interfaces with no addresses
                return boost::copy_range<std::vector<host_interface>>(interfaces | boost::adaptors::filtered([](const host_interface& interface)
                {
                    return !interface.addresses.empty();
                }));
            }

            std::vector<utility::string_t> host_names(const utility::string_t& address)
            {
                boost::asio::io_service service;
                boost::asio::ip::tcp::resolver resolver(service);
                std::vector<utility::string_t> host_names;
                boost::system::error_code ec;
#if BOOST_VERSION >= 106600
                const auto ip_address = boost::asio::ip::make_address(utility::conversions::to_utf8string(address));
#else
                const auto ip_address = boost::asio::ip::address::from_string(utility::conversions::to_utf8string(address));
#endif
                const auto results = resolver.resolve({ ip_address, 0 }, ec);
#if BOOST_VERSION >= 106600
                for (const auto& re : results)
                {
#else
                for (auto it = results; it != boost::asio::ip::tcp::resolver::iterator{}; ++it)
                {
                    const auto& re = *it;
#endif
                    host_names.push_back(utility::conversions::to_string_t(re.host_name()));
                }
                return host_names; // empty if address cannot be resolved
            }

            std::vector<utility::string_t> host_addresses(const utility::string_t& host_name)
            {
                boost::asio::io_service service;
                boost::asio::ip::tcp::resolver resolver(service);
                std::vector<utility::string_t> addresses;
                boost::system::error_code ec;
                // for now, limited to IPv4
                const auto results = resolver.resolve({ boost::asio::ip::tcp::v4(), utility::conversions::to_utf8string(host_name), "" }, ec);
#if BOOST_VERSION >= 106600
                for (const auto& re : results)
                {
#else
                for (auto it = results; it != boost::asio::ip::tcp::resolver::iterator{}; ++it)
                {
                    const auto& re = *it;
#endif
                    addresses.push_back(utility::conversions::to_string_t(re.endpoint().address().to_string()));
                }
                return addresses; // empty if host_name cannot be resolved
            }
        }
    }
}
