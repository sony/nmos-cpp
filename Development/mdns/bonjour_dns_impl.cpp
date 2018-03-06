#include "mdns/bonjour_dns_impl.h"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/erase.hpp>
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
        : m_gate(gate)
    {
        slog::log<slog::severities::too_much_info>(m_gate, SLOG_FLF) << "Discovery/advertiser instance constructed";
    }

    bonjour_dns_impl::~bonjour_dns_impl()
    {
        // de-register all registered services
        stop();

        slog::log<slog::severities::too_much_info>(m_gate, SLOG_FLF) << "Discovery/advertiser instance destructed";
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

    bool bonjour_dns_impl::register_service(const std::string& name, const std::string& type, std::uint16_t port, const std::string& domain, const std::string& host_name, const txt_records& records)
    {
        bool result = false;

        DNSServiceRef sdRef;
        const std::vector<unsigned char> txt_records = make_txt_records(records);

        DNSServiceErrorType errorCode = DNSServiceRegister(
            &sdRef,
            0, // all interfaces
            0,
            !name.empty() ? name.c_str() : NULL,
            type.c_str(),
            !domain.empty() ? domain.c_str() : NULL,
            !host_name.empty() ? host_name.c_str() : NULL,
            htons(port),
            (std::uint16_t)txt_records.size(),
            !txt_records.empty() ? &txt_records[0] : NULL,
            NULL,
            NULL);

        if (errorCode == kDNSServiceErr_NoError)
        {
            result = true;

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

                DNSServiceErrorType errorCode = DNSServiceUpdateRecord(service.sdRef, NULL, 0, (std::uint16_t)txt_records.size(), &txt_records[0], 0);
                
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
            std::vector<service_discovery::browse_result>& found = *impl->m_found;

            if (0 != (flags & kDNSServiceFlagsAdd))
            {
                mdns::service_discovery::browse_result result{ serviceName, regtype, replyDomain, interfaceIndex };

                found.push_back(result);
            }

            impl->m_more_coming = 0 != (flags & kDNSServiceFlagsMoreComing);
        }
        else
        {
            slog::log<slog::severities::error>(impl->m_gate, SLOG_FLF) << "After DNSServiceBrowse, DNSServiceBrowseReply received error: " << errorCode;
            impl->m_more_coming = false;
            return;
        }
    }

    bool bonjour_dns_impl::browse(std::vector<browse_result>& found, const std::string& type, const std::string& domain, unsigned int timeout_secs)
    {
        m_found = &found;

        DNSServiceRef client = nullptr;

        m_found->clear();
        m_more_coming = false;

        const auto absolute_timeout = std::chrono::system_clock::now() + std::chrono::seconds(timeout_secs);

        DNSServiceErrorType errorCode = DNSServiceBrowse(&client, 0, 0, type.c_str(), !domain.empty() ? domain.c_str() : NULL, browse_reply, this);

        if (errorCode == kDNSServiceErr_NoError)
        {
            do
            {
                // wait for up to timeout seconds for a response
                int wait_millis = (std::max)(0, (int)std::chrono::duration_cast<std::chrono::milliseconds>(absolute_timeout - std::chrono::system_clock::now()).count());

                // get the next browse response
                errorCode = DNSServiceProcessResult(client, wait_millis);

                if (errorCode == kDNSServiceErr_NoError)
                {
                    // callback called, potentially more results coming
                }
                else if (errorCode == kDNSServiceErr_Timeout_)
                {
                    break;
                }
                else
                {
                    slog::log<slog::severities::error>(m_gate, SLOG_FLF) << "After DNSServiceBrowse, DNSServiceProcessResult reported error: " << errorCode;
                    break;
                }

            } while (absolute_timeout > std::chrono::system_clock::now());

            DNSServiceRefDeallocate(client);
        }
        else
        {
            slog::log<slog::severities::error>(m_gate, SLOG_FLF) << "DNSServiceBrowse reported error: " << errorCode;
        }

        return !found.empty();
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

        service_discovery::resolve_result& resolved = *impl->m_resolved;

        if (errorCode == kDNSServiceErr_NoError)
        {
            resolved.host_name = hosttarget;
            slog::log<slog::severities::more_info>(impl->m_gate, SLOG_FLF) << "After DNSServiceResolve, get address for host: " << resolved.host_name;
            // if we used DNSServiceGetAddrInfo here, it would handle the suffix itself?
            const auto ip_addresses = web::http::experimental::host_addresses(utility::s2us(without_suffix(resolved.host_name)));
            if (!ip_addresses.empty())
            {
                resolved.ip_address = utility::us2s(ip_addresses[0]);
            }
            resolved.port = ntohs(port);
            resolved.txt_records = parse_txt_records(txtRecord, txtLen);
        }
        else
        {
            slog::log<slog::severities::error>(impl->m_gate, SLOG_FLF) << "After DNSServiceResolve, DNSServiceResolveReply received error: " << errorCode;
        }
    }

    bool bonjour_dns_impl::resolve(resolve_result& resolved, const std::string& name, const std::string& type, const std::string& domain, std::uint32_t interface_id, unsigned int timeout_secs)
    {
        m_resolved = &resolved;

        DNSServiceRef client = nullptr;

        *m_resolved = resolve_result{};

        slog::log<slog::severities::more_info>(m_gate, SLOG_FLF) << "DNSServiceResolve for name: " << name << " regtype: " << type << " domain: " << domain;

        DNSServiceErrorType errorCode = DNSServiceResolve(&client, 0, interface_id, name.c_str(), type.c_str(), domain.c_str(), resolve_reply, this);

        if (errorCode == kDNSServiceErr_NoError)
        {
            // wait for the resolve response
            errorCode = DNSServiceProcessResult(client, (int)timeout_secs * 1000);

            if (errorCode == kDNSServiceErr_NoError)
            {
                // callback called
            }
            else if (errorCode == kDNSServiceErr_Timeout_)
            {
                // timeout expired
            }
            else
            {
                slog::log<slog::severities::error>(m_gate, SLOG_FLF) << "After DNSServiceResolve, DNSServiceProcessResult reported error: " << errorCode;
            }

            DNSServiceRefDeallocate(client);
        }
        else
        {
            slog::log<slog::severities::error>(m_gate, SLOG_FLF) << "DNSServiceResolve reported error: " << errorCode;
        }

        return !resolved.ip_address.empty();
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
            DNSServiceRefDeallocate(service.sdRef);
            slog::log<slog::severities::too_much_info>(m_gate, SLOG_FLF) << "Advertisement stopped for: " << service.name;
        }

        m_services.clear();
    }
}
