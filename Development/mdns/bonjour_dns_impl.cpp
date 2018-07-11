#include "mdns/bonjour_dns_impl.h"

#include <boost/asio/ip/address.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/erase.hpp>
#include "dns_sd.h"
#include "cpprest/basic_utils.h"
#include "cpprest/host_utils.h"
#include "slog/all_in_one.h"

#ifdef _WIN32
// winsock2.h provides htons
#else
// POSIX
#include <arpa/inet.h> // for htons
#endif

#ifdef _WIN32
// "Porting Socket Applications to Winsock" is a useful reference
// See https://msdn.microsoft.com/en-us/library/windows/desktop/ms740096(v=vs.85).aspx
#else
// POSIX
#include <poll.h>
#endif

// dnssd_sock_t was added relatively recently (765.1.2)
typedef decltype(DNSServiceRefSockFD(NULL)) DNSServiceRefSockFD_t;

// kDNSServiceErr_Timeout was added a long time ago (320.5) but did not exist in Tiger
// and is not defined by the Avahi compatibility layer
static const DNSServiceErrorType kDNSServiceErr_Timeout_ = -65568;

// DNSServiceGetAddrInfo was added a long time ago (161.1) but did not exist in Tiger
// and is not defined by the Avahi compatibility layer, so getaddrinfo must be relied on instead
#if _DNS_SD_H+0 >= 1610100
#define HAVE_DNSSERVICEGETADDRINFO
#endif

#ifdef _WIN32
// kIPv6IfIndexBase was added a really long time ago (107.1) in the Windows implementation
// because "Windows does not have a uniform scheme for IPv4 and IPv6 interface indexes"
static const std::uint32_t kIPv6IfIndexBase = 10000000L;
#endif

#ifdef _WIN32
static inline bool dnssd_SocketValid(DNSServiceRefSockFD_t fd) { return fd != INVALID_SOCKET; }
#else
static inline bool dnssd_SocketValid(DNSServiceRefSockFD_t fd) { return fd >= 0; }
#endif

// Wait for a DNSServiceRef to become ready for at most timeout milliseconds (which may be zero)
// Don't use select, which is the recommended practice, to avoid problems with file descriptors
// greater than or equal to FD_SETSIZE.
// See https://developer.apple.com/documentation/dnssd/1804696-dnsserviceprocessresult
static DNSServiceErrorType DNSServiceWaitForReply(DNSServiceRef sdRef, int timeout_millis)
{
    DNSServiceRefSockFD_t fd = DNSServiceRefSockFD(sdRef);
    if (!dnssd_SocketValid(fd))
    {
        return kDNSServiceErr_BadParam;
    }

#ifdef _WIN32
    // FD_SETSIZE doesn't present the same limitation on Windows because it controls the
    // maximum number of file descriptors in the set, not the largest supported value
    // See https://msdn.microsoft.com/en-us/library/windows/desktop/ms740142(v=vs.85).aspx

    fd_set readfds;
    FD_ZERO(&readfds);
PRAGMA_WARNING_PUSH
PRAGMA_WARNING_DISABLE_CONDITIONAL_EXPRESSION_IS_CONSTANT
    FD_SET(fd, &readfds);
PRAGMA_WARNING_POP

    // wait for up to timeout milliseconds (converted to secs and usecs)
    struct timeval tv{ timeout_millis / 1000, (timeout_millis % 1000) * 1000 };
    // first parameter is ignored on Windows but required e.g. on Linux
    int res = select((int)fd + 1, &readfds, (fd_set*)NULL, (fd_set*)NULL, &tv);
#else
    // Use poll in order to be able to wait for a file descriptor larger than FD_SETSIZE
    pollfd pfd_read{ fd, POLLIN, 0 };
    int res = poll(&pfd_read, 1, timeout_millis);
#endif

    // both select and poll return values as follows...
    if (res < 0)
    {
        // WSAGetLastError/errno would give more info
        return kDNSServiceErr_Unknown;
    }
    else if (res == 0)
    {
        // timeout has expired
        return kDNSServiceErr_Timeout_;
    }
    else
    {
        return kDNSServiceErr_NoError;
    }
}

// Read a reply from the daemon, calling the appropriate application callback
// This call will block for at most timeout milliseconds.
static DNSServiceErrorType DNSServiceProcessResult(DNSServiceRef sdRef, int timeout_millis)
{
    DNSServiceErrorType errorCode = DNSServiceWaitForReply(sdRef, timeout_millis);
    return errorCode == kDNSServiceErr_NoError ? DNSServiceProcessResult(sdRef) : errorCode;
}

namespace mdns
{
    bonjour_dns_impl::bonjour_dns_impl(slog::base_gate& gate)
        : m_client(nullptr)
        , m_gate(gate)
    {
        slog::log<slog::severities::too_much_info>(m_gate, SLOG_FLF) << "Discovery/advertiser instance constructed";
    }

    bonjour_dns_impl::~bonjour_dns_impl()
    {
        // de-register all registered services
        stop();

        slog::log<slog::severities::too_much_info>(m_gate, SLOG_FLF) << "Discovery/advertiser instance destructed";
    }

    static std::string make_full_name(const std::string& host_name, const std::string& domain)
    {
        return !host_name.empty() ? host_name + "." + (!domain.empty() ? domain : "local.") : "";
    }

    static std::vector<unsigned char> make_txt_records(const txt_records& records)
    {
        std::vector<unsigned char> txt;

        // create the txt record string in the correct format
        for (const auto& record : records)
        {
            size_t len = record.size();

            if (len > 255)
            {
                // txt record is too long
                continue;
            }

            txt.push_back((unsigned char)len);
            txt.insert(txt.end(), record.begin(), record.end());
        }

        // although a completely empty txt record is invalid, DNSServiceRegister handles this case

        return txt;
    }

    static void DNSSD_API register_address_reply(
        DNSServiceRef         sdRef,
        DNSRecordRef          recordRef,
        DNSServiceFlags       flags,
        DNSServiceErrorType   errorCode,
        void*                 context)
    {
        bonjour_dns_impl* impl = (bonjour_dns_impl*)context;

        if (errorCode == kDNSServiceErr_NoError)
        {
            slog::log<slog::severities::more_info>(impl->m_gate, SLOG_FLF) << "After DNSServiceRegisterRecord, DNSServiceRegisterRecordReply received no error";
        }
        else
        {
            slog::log<slog::severities::error>(impl->m_gate, SLOG_FLF) << "After DNSServiceRegisterRecord, DNSServiceRegisterRecordReply received error: " << errorCode;
            // possible errors include kDNSServiceErr_NameConflict, kDNSServiceErr_AlreadyRegistered
        }
    }

    bool bonjour_dns_impl::register_address(const std::string& host_name, const std::string& ip_address_str, const std::string& domain)
    {
        // since empty host_name is valid for other functions, check that logic error here
        if (host_name.empty()) return false;

#if BOOST_VERSION >= 106600
        const auto ip_address = boost::asio::ip::make_address(ip_address_str);
#else
        const auto ip_address = boost::asio::ip::address::from_string(ip_address_str);
#endif
        if (ip_address.is_unspecified()) return false;

        // for now, limited to IPv4
        if (!ip_address.is_v4()) return false;

        bool result = false;

        // the Avahi compatibility layer implementations of DNSServiceCreateConnection and DNSServiceRegisterRecord
        // just return kDNSServiceErr_Unsupported
        // see https://github.com/lathiat/avahi/blob/master/avahi-compat-libdns_sd/unsupported.c
        // an alternative may be to use avahi-publish -a -R {host_name} {ip_address}
        // see https://linux.die.net/man/1/avahi-publish-address

        // lazily create the connection
        DNSServiceErrorType errorCode = nullptr == m_client ? DNSServiceCreateConnection((DNSServiceRef*)&m_client) : kDNSServiceErr_NoError;

        if (errorCode == kDNSServiceErr_NoError)
        {
            // so far as I can tell, attempting to register a host name in a domain other than the multicast .local domain always fails
            // which may be the expected behaviour?
            const auto fullname = make_full_name(host_name, domain);

#if BOOST_VERSION >= 106600
            const auto rdata = htonl(ip_address.to_v4().to_uint());
#else
            const auto rdata = htonl(ip_address.to_v4().to_ulong());
#endif

            // so far as I can tell, this call always returns kDNSServiceErr_BadParam in my Windows environment
            // with a message in the event log for the Bonjour Service:
            // mDNS_Register_internal: TTL CCCC0000 should be 1 - 0x7FFFFFFF    4 ...
            // which looks like a bug in how the ttl value is marshalled between the client stub library and the service?
            DNSRecordRef recordRef;
            errorCode = DNSServiceRegisterRecord(
                (DNSServiceRef)m_client,
                &recordRef,
                kDNSServiceFlagsShared,
                kDNSServiceInterfaceIndexAny,
                fullname.c_str(),
                kDNSServiceType_A,
                kDNSServiceClass_IN,
                sizeof(rdata),
                &rdata,
                0,
                &register_address_reply,
                this);

            if (errorCode == kDNSServiceErr_NoError)
            {
                // process the response (should this be in a loop, like for browse?)
                errorCode = DNSServiceProcessResult((DNSServiceRef)m_client, 1);

                if (errorCode == kDNSServiceErr_NoError)
                {
                    slog::log<slog::severities::too_much_info>(m_gate, SLOG_FLF) << "After DNSServiceRegisterRecord, DNSServiceProcessResult succeeded";

                    result = true;

                    slog::log<slog::severities::info>(m_gate, SLOG_FLF) << "Registered address: " << ip_address.to_string() << " for hostname: " << fullname;
                }
                else
                {
                    slog::log<slog::severities::error>(m_gate, SLOG_FLF) << "After DNSServiceRegisterRecord, DNSServiceProcessResult reported error: " << errorCode;
                }
            }
            else
            {
                slog::log<slog::severities::error>(m_gate, SLOG_FLF) << "DNSServiceRegisterRecord reported error: " << errorCode;
            }
        }
        else
        {
            slog::log<slog::severities::error>(m_gate, SLOG_FLF) << "DNSServiceCreateConnection reported error: " << errorCode;
        }

        return result;
    }

    bool bonjour_dns_impl::register_service(const std::string& name, const std::string& type, std::uint16_t port, const std::string& domain, const std::string& host_name, const txt_records& records)
    {
        bool result = false;

        DNSServiceRef sdRef;
        const auto fullname = make_full_name(host_name, domain);
        const std::vector<unsigned char> txt_records = make_txt_records(records);

        DNSServiceErrorType errorCode = DNSServiceRegister(
            &sdRef,
            0,
            kDNSServiceInterfaceIndexAny,
            !name.empty() ? name.c_str() : NULL,
            type.c_str(),
            !domain.empty() ? domain.c_str() : NULL,
            !fullname.empty() ? fullname.c_str() : NULL,
            htons(port),
            (std::uint16_t)txt_records.size(),
            !txt_records.empty() ? &txt_records[0] : NULL,
            NULL,
            NULL);

        if (errorCode == kDNSServiceErr_NoError)
        {
            result = true;

            // if name were empty, the callback would indicate what name was automatically chosen (and likewise for domain)
            m_services.push_back({ name, type, domain, sdRef });

            slog::log<slog::severities::info>(m_gate, SLOG_FLF) << "Registered advertisement for: " << name << "." << type << (domain.empty() ? "" : "." + domain);
        }
        else
        {
            slog::log<slog::severities::error>(m_gate, SLOG_FLF) << "DNSServiceRegister reported error: " << errorCode << " while registering advertisement for: " << name << "." << type << (domain.empty() ? "" : "." + domain);
        }

        return result;
    }

    bool bonjour_dns_impl::update_record(const std::string& name, const std::string& type, const std::string& domain, const txt_records& records)
    {
        bool result = false;

        // try to find a record that matches
        for (const auto& service : m_services)
        {
            if (service.name == name && service.type == type && service.domain == domain)
            {
                const std::vector<unsigned char> txt_records = make_txt_records(records);

                DNSServiceErrorType errorCode = DNSServiceUpdateRecord((DNSServiceRef)service.sdRef, NULL, 0, (std::uint16_t)txt_records.size(), &txt_records[0], 0);

                if (errorCode == kDNSServiceErr_NoError)
                {
                    result = true;
                }

                break;
            }
        }

        return result;
    }

    static void DNSSD_API browse_reply(
        DNSServiceRef         sdRef,
        const DNSServiceFlags flags,
        uint32_t              interfaceIndex,
        DNSServiceErrorType   errorCode,
        const char*           serviceName,
        const char*           regtype,
        const char*           replyDomain,
        void*                 context)
    {
        bonjour_dns_impl* impl = (bonjour_dns_impl*)context;

        if (errorCode == kDNSServiceErr_NoError)
        {
            auto& results = *impl->m_browsed;

            if (0 != (flags & kDNSServiceFlagsAdd))
            {
                // map kDNSServiceInterfaceIndexLocalOnly to kDNSServiceInterfaceIndexAny, to handle AVAHI_IF_UNSPEC escaping from the Avahi compatibility layer
                results.push_back({ serviceName, regtype, replyDomain, kDNSServiceInterfaceIndexLocalOnly == interfaceIndex ? kDNSServiceInterfaceIndexAny : interfaceIndex });
                const auto& result = results.back();

                slog::log<slog::severities::more_info>(impl->m_gate, SLOG_FLF) << "After DNSServiceBrowse, DNSServiceBrowseReply got service: " << result.name << " for regtype: " << result.type << " domain: " << result.domain << " on interface: " << result.interface_id;
            }

            impl->m_more_coming = 0 != (flags & kDNSServiceFlagsMoreComing);
        }
        else
        {
            slog::log<slog::severities::error>(impl->m_gate, SLOG_FLF) << "After DNSServiceBrowse, DNSServiceBrowseReply received error: " << errorCode;
        }
    }

    bool bonjour_dns_impl::browse(std::vector<browse_result>& results, const std::string& type, const std::string& domain, std::uint32_t interface_id, unsigned int latest_timeout_seconds, unsigned int earliest_timeout_seconds)
    {
        m_browsed = &results;

        DNSServiceRef client = nullptr;

        m_browsed->clear();

        const auto latest_timeout = std::chrono::system_clock::now() + std::chrono::seconds(latest_timeout_seconds);
        const auto earliest_timeout = std::chrono::system_clock::now() + std::chrono::seconds(earliest_timeout_seconds);

        // could use if_indextoname to get a name for the interface (remembering that 0 means "do the right thing", i.e. usually any interface, and there are some other special values too; see dns_sd.h)
        slog::log<slog::severities::more_info>(m_gate, SLOG_FLF) << "DNSServiceBrowse for regtype: " << type << " domain: " << domain << " on interface: " << interface_id;

        DNSServiceErrorType errorCode = DNSServiceBrowse(&client, 0, interface_id, type.c_str(), !domain.empty() ? domain.c_str() : NULL, browse_reply, this);

        if (errorCode == kDNSServiceErr_NoError)
        {
            do
            {
                // wait for up to timeout seconds for a response
                const auto& absolute_timeout = m_browsed->empty() ? latest_timeout : earliest_timeout;
                int wait_millis = (std::max)(0, (int)std::chrono::duration_cast<std::chrono::milliseconds>(absolute_timeout - std::chrono::system_clock::now()).count());

                // process the next browse responses (callback may be called more than once, or (at least with Avahi!) not at all, in any call to DNSServiceProcessResult)
                m_more_coming = false;
                errorCode = DNSServiceProcessResult(client, wait_millis);

                if (errorCode == kDNSServiceErr_NoError)
                {
                    slog::log<slog::severities::too_much_info>(m_gate, SLOG_FLF) << "After DNSServiceBrowse, DNSServiceProcessResult succeeded";
                    // callback called, potentially more results coming
                }
                else if (errorCode == kDNSServiceErr_Timeout_)
                {
                    slog::log<slog::severities::more_info>(m_gate, SLOG_FLF) << "After DNSServiceBrowse, DNSServiceProcessResult timed out";
                    break;
                }
                else
                {
                    slog::log<slog::severities::error>(m_gate, SLOG_FLF) << "After DNSServiceBrowse, DNSServiceProcessResult reported error: " << errorCode;
                    break;
                }

            } while ((m_browsed->empty() || m_more_coming ? latest_timeout : earliest_timeout) > std::chrono::system_clock::now());

            DNSServiceRefDeallocate(client);
        }
        else
        {
            slog::log<slog::severities::error>(m_gate, SLOG_FLF) << "DNSServiceBrowse reported error: " << errorCode;
        }

        return !results.empty();
    }

    static txt_records parse_txt_records(const unsigned char* txtRecord, size_t txtLen)
    {
        txt_records records;

        size_t i = 0;
        while (i < txtLen)
        {
            // each record is preceded by a length byte (so max length of each record is 255)
            size_t len = txtRecord[i++];
            records.emplace_back(&txtRecord[i], &txtRecord[i + len]);
            i += len;
        }

        return records;
    }

#ifdef _WIN32
    inline std::string ierase_tail_copy(const std::string& input, const std::string& tail)
    {
        return boost::algorithm::iends_with(input, tail)
            ? boost::algorithm::erase_tail_copy(input, (int)tail.size())
            : input;
    }

    static const std::string suffix_absolute(".");
    static const std::string suffix_local(".local");
    static std::string without_suffix(const std::string& host_name)
    {
        return ierase_tail_copy(ierase_tail_copy(host_name, suffix_absolute), suffix_local);
    }
#endif

    static void DNSSD_API resolve_reply(
        DNSServiceRef         sdRef,
        const DNSServiceFlags flags,
        uint32_t              interfaceIndex,
        DNSServiceErrorType   errorCode,
        const char*           fullname,
        const char*           hosttarget,
        uint16_t              port,
        uint16_t              txtLen,
        const unsigned char*  txtRecord,
        void*                 context)
    {
        bonjour_dns_impl* impl = (bonjour_dns_impl*)context;

        auto& results = *impl->m_resolved;

        if (errorCode == kDNSServiceErr_NoError)
        {
            results.push_back({ hosttarget, ntohs(port), parse_txt_records(txtRecord, txtLen) });
            auto& result = results.back();

            slog::log<slog::severities::more_info>(impl->m_gate, SLOG_FLF) << "After DNSServiceResolve, DNSServiceResolveReply got host: " << result.host_name << " port: " << (int)result.port;

            impl->m_more_coming = 0 != (flags & kDNSServiceFlagsMoreComing);
        }
        else
        {
            slog::log<slog::severities::error>(impl->m_gate, SLOG_FLF) << "After DNSServiceResolve, DNSServiceResolveReply received error: " << errorCode;
        }
    }

#ifdef HAVE_DNSSERVICEGETADDRINFO
    // this just shouldn't be this hard!
    static boost::asio::ip::address from_sockaddr(const sockaddr& address)
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

    static void DNSSD_API getaddrinfo_reply(
        DNSServiceRef          sdRef,
        const DNSServiceFlags  flags,
        uint32_t               interfaceIndex,
        DNSServiceErrorType    errorCode,
        const char*            hostname,
        const struct sockaddr* address,
        uint32_t               ttl,
        void*                  context)
    {
        bonjour_dns_impl* impl = (bonjour_dns_impl*)context;

        auto& results = *impl->m_ip_addresses;

        if (errorCode == kDNSServiceErr_NoError)
        {
            if (0 != address)
            {
                const auto ip_address = from_sockaddr(*address);
                if (!ip_address.is_unspecified())
                {
                    results.push_back(ip_address.to_string());
                    slog::log<slog::severities::more_info>(impl->m_gate, SLOG_FLF) << "After DNSServiceGetAddrInfo, DNSServiceGetAddrInfoReply got address: " << results.back() << " for host: " << hostname;
                }
            }

            impl->m_more_coming = 0 != (flags & kDNSServiceFlagsMoreComing);
        }
        else
        {
            slog::log<slog::severities::error>(impl->m_gate, SLOG_FLF) << "After DNSServiceGetAddrInfo, DNSServiceGetAddrInfoReply received error: " << errorCode;
        }
    }
#endif

    bool bonjour_dns_impl::resolve(std::vector<resolve_result>& resolved, const std::string& name, const std::string& type, const std::string& domain, std::uint32_t interface_id, unsigned int latest_timeout_seconds, unsigned int earliest_timeout_seconds)
    {
        m_resolved = &resolved;

        DNSServiceRef client = nullptr;

        m_resolved->clear();

        const auto latest_timeout = std::chrono::system_clock::now() + std::chrono::seconds(latest_timeout_seconds);
        const auto earliest_timeout = std::chrono::system_clock::now() + std::chrono::seconds(earliest_timeout_seconds);

        // could use if_indextoname to get a name for the interface (remembering that 0 means "do the right thing", i.e. usually any interface, and there are some other special values too; see dns_sd.h)
        slog::log<slog::severities::more_info>(m_gate, SLOG_FLF) << "DNSServiceResolve for name: " << name << " regtype: " << type << " domain: " << domain << " on interface: " << interface_id;

        DNSServiceErrorType errorCode = DNSServiceResolve(&client, 0, interface_id, name.c_str(), type.c_str(), domain.c_str(), (DNSServiceResolveReply)resolve_reply, this);

        if (errorCode == kDNSServiceErr_NoError)
        {
            do
            {
                // wait for up to timeout seconds for a response
                const auto& absolute_timeout = m_resolved->empty() ? latest_timeout : earliest_timeout;
                int wait_millis = (std::max)(0, (int)std::chrono::duration_cast<std::chrono::milliseconds>(absolute_timeout - std::chrono::system_clock::now()).count());

                // process the next resolve responses (callback may be called more than once, or not at all, in any call to DNSServiceProcessResult)
                m_more_coming = false;
                errorCode = DNSServiceProcessResult(client, wait_millis);

                if (errorCode == kDNSServiceErr_NoError)
                {
                    slog::log<slog::severities::too_much_info>(m_gate, SLOG_FLF) << "After DNSServiceResolve, DNSServiceProcessResult succeeded";
                    // callback called, potentially more results coming
                }
                else if (errorCode == kDNSServiceErr_Timeout_)
                {
                    slog::log<slog::severities::more_info>(m_gate, SLOG_FLF) << "After DNSServiceResolve, DNSServiceProcessResult timed out";
                    break;
                }
                else
                {
                    slog::log<slog::severities::error>(m_gate, SLOG_FLF) << "After DNSServiceResolve, DNSServiceProcessResult reported error: " << errorCode;
                    break;
                }

            } while ((m_resolved->empty() || m_more_coming ? latest_timeout : earliest_timeout) > std::chrono::system_clock::now());

            DNSServiceRefDeallocate(client);

            for (auto& r : *m_resolved)
            {
#ifdef HAVE_DNSSERVICEGETADDRINFO
                m_ip_addresses = &r.ip_addresses;

                client = nullptr;

                // for now, limited to IPv4
                const DNSServiceProtocol protocol = kDNSServiceProtocol_IPv4;

#ifdef _WIN32
                if (protocol == kDNSServiceProtocol_IPv4 && interface_id >= kIPv6IfIndexBase)
                {
                    // no point trying in this case!
                    slog::log<slog::severities::too_much_info>(m_gate, SLOG_FLF) << "DNSServiceGetAddrInfo not tried for hostname: " << r.host_name << " on interface: " << interface_id;
                }
                else
#endif
                {
                    slog::log<slog::severities::more_info>(m_gate, SLOG_FLF) << "DNSServiceGetAddrInfo for hostname: " << r.host_name << " on interface: " << interface_id;

                    errorCode = DNSServiceGetAddrInfo(&client, 0, interface_id, protocol, r.host_name.c_str(), getaddrinfo_reply, this);

                    if (errorCode == kDNSServiceErr_NoError)
                    {
                        do
                        {
                            // wait for up to timeout seconds for a response
                            const auto& absolute_timeout = m_ip_addresses->empty() ? latest_timeout : earliest_timeout;
                            int wait_millis = (std::max)(0, (int)std::chrono::duration_cast<std::chrono::milliseconds>(absolute_timeout - std::chrono::system_clock::now()).count());

                            // process the next lookup responses (callback may be called more than once, or potentially not at all, in any call to DNSServiceProcessResult)
                            m_more_coming = false;
                            errorCode = DNSServiceProcessResult(client, wait_millis);

                            if (errorCode == kDNSServiceErr_NoError)
                            {
                                slog::log<slog::severities::too_much_info>(m_gate, SLOG_FLF) << "After DNSServiceGetAddrInfo, DNSServiceProcessResult succeeded";
                                // callback called, potentially more results coming
                            }
                            else if (errorCode == kDNSServiceErr_Timeout_)
                            {
                                slog::log<slog::severities::more_info>(m_gate, SLOG_FLF) << "After DNSServiceGetAddrInfo, DNSServiceProcessResult timed out";
                                break;
                            }
                            else
                            {
                                slog::log<slog::severities::error>(m_gate, SLOG_FLF) << "After DNSServiceGetAddrInfo, DNSServiceProcessResult reported error: " << errorCode;
                                break;
                            }

                        } while ((m_ip_addresses->empty() || m_more_coming ? latest_timeout : earliest_timeout) > std::chrono::system_clock::now());

                        DNSServiceRefDeallocate(client);
                    }
                    else
                    {
                        slog::log<slog::severities::error>(m_gate, SLOG_FLF) << "DNSServiceGetAddrInfo reported error: " << errorCode;
                    }
                }
#endif

                if (r.ip_addresses.empty())
                {
                    // hmmm, plain old getaddrinfo uses all name resolution mechanisms so isn't specific to a particular interface
#ifdef _WIN32
                    // on Windows, resolution of multicast .local domain names doesn't seem to work even with the Bonjour service running?
                    const auto ip_addresses = web::http::experimental::host_addresses(utility::s2us(without_suffix(r.host_name)));
#else
                    // on Linux, the name-service switch should be configured to use Avahi to resolve multicast .local domain names
                    // by including 'mdns4' or 'mdns4_minimal' in the hosts stanza of /etc/nsswitch.conf
                    const auto ip_addresses = web::http::experimental::host_addresses(utility::s2us(r.host_name));
#endif
                    std::transform(ip_addresses.begin(), ip_addresses.end(), std::back_inserter(r.ip_addresses), utility::us2s);
                    slog::log<slog::severities::more_info>(m_gate, SLOG_FLF) << "After DNSServiceResolve, got " << r.ip_addresses.size() << " address(es) for hostname: " << r.host_name;
                }
            }
        }
        else
        {
            slog::log<slog::severities::error>(m_gate, SLOG_FLF) << "DNSServiceResolve reported error: " << errorCode;
        }

        return !resolved.empty();
    }

    void bonjour_dns_impl::start()
    {
        slog::log<slog::severities::too_much_info>(m_gate, SLOG_FLF) << "Advertisement started for " << m_services.size() << " service(s)";
    }

    void bonjour_dns_impl::stop()
    {
        // De-register anything that is registered
        for (const auto& service : m_services)
        {
            DNSServiceRefDeallocate((DNSServiceRef)service.sdRef);
            slog::log<slog::severities::too_much_info>(m_gate, SLOG_FLF) << "Advertisement stopped for: " << service.name;
        }

        m_services.clear();

        if (nullptr != m_client)
        {
            DNSServiceRefDeallocate((DNSServiceRef)m_client);

            m_client = nullptr;
        }
    }
}
