#include "mdns/service_discovery.h"

#include <boost/asio/ip/address.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/erase.hpp>
#include "cpprest/basic_utils.h"
#include "cpprest/host_utils.h"
#include "mdns/dns_sd_impl.h"
#include "slog/all_in_one.h"

namespace mdns_details
{
    using namespace mdns;

    struct browse_context
    {
        // browse in-flight state
        std::vector<browse_result>& browsed;
        bool& more_coming;
        slog::base_gate& gate;
    };

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
        browse_context* impl = (browse_context*)context;

        if (errorCode == kDNSServiceErr_NoError)
        {
            auto& results = impl->browsed;

            if (0 != (flags & kDNSServiceFlagsAdd))
            {
                // map kDNSServiceInterfaceIndexLocalOnly to kDNSServiceInterfaceIndexAny, to handle AVAHI_IF_UNSPEC escaping from the Avahi compatibility layer
                results.push_back({ serviceName, regtype, replyDomain, kDNSServiceInterfaceIndexLocalOnly == interfaceIndex ? kDNSServiceInterfaceIndexAny : interfaceIndex });
                const auto& result = results.back();

                slog::log<slog::severities::more_info>(impl->gate, SLOG_FLF) << "After DNSServiceBrowse, DNSServiceBrowseReply got service: " << result.name << " for regtype: " << result.type << " domain: " << result.domain << " on interface: " << result.interface_id;
            }

            impl->more_coming = 0 != (flags & kDNSServiceFlagsMoreComing);
        }
        else
        {
            slog::log<slog::severities::error>(impl->gate, SLOG_FLF) << "After DNSServiceBrowse, DNSServiceBrowseReply received error: " << errorCode;
        }
    }

    static std::vector<browse_result> browse(const std::string& type, const std::string& domain, std::uint32_t interface_id, const std::chrono::steady_clock::duration& latest_timeout_, DNSServiceCancellationToken cancel, slog::base_gate& gate)
    {
        // in order to ensure the daemon actually performs a query rather than just returning results from its cache
        // apply a minimum timeout rather than giving up the first time the more_coming flag is false
        // see https://github.com/lathiat/avahi/blob/v0.7/avahi-core/multicast-lookup.c#L120
        const auto earliest_timeout_ = std::chrono::seconds(1) < latest_timeout_ ? std::chrono::seconds(1) : latest_timeout_;

        std::vector<browse_result> browsed;
        bool more_coming;

        DNSServiceRef client = nullptr;

        const auto now = std::chrono::steady_clock::now();
        const auto latest_timeout = now + latest_timeout_;
        const auto earliest_timeout = now + earliest_timeout_;

        // could use if_indextoname to get a name for the interface (remembering that 0 means "do the right thing", i.e. usually any interface, and there are some other special values too; see dns_sd.h)
        slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "DNSServiceBrowse for regtype: " << type << " domain: " << domain << " on interface: " << interface_id;

        browse_context context{ browsed, more_coming, gate };
        DNSServiceErrorType errorCode = DNSServiceBrowse(&client, 0, interface_id, type.c_str(), !domain.empty() ? domain.c_str() : NULL, browse_reply, &context);

        if (errorCode == kDNSServiceErr_NoError)
        {
            do
            {
                // wait for up to timeout for a response
                const auto& absolute_timeout = browsed.empty() ? latest_timeout : earliest_timeout;
                int wait_millis = (std::max)(0, (int)std::chrono::duration_cast<std::chrono::milliseconds>(absolute_timeout - std::chrono::steady_clock::now()).count());

                // process the next browse responses (callback may be called more than once, or (at least with Avahi!) not at all, in any call to DNSServiceProcessResult)
                more_coming = false;
                errorCode = DNSServiceProcessResult(client, wait_millis, cancel);

                if (errorCode == kDNSServiceErr_NoError)
                {
                    slog::log<slog::severities::too_much_info>(gate, SLOG_FLF) << "After DNSServiceBrowse, DNSServiceProcessResult succeeded";
                    // callback called, potentially more results coming
                }
                else if (errorCode == kDNSServiceErr_Timeout_)
                {
                    slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "After DNSServiceBrowse, DNSServiceProcessResult timed out or was cancelled";
                    break;
                }
                else
                {
                    slog::log<slog::severities::error>(gate, SLOG_FLF) << "After DNSServiceBrowse, DNSServiceProcessResult reported error: " << errorCode;
                    break;
                }

            } while (more_coming || (browsed.empty() ? latest_timeout : earliest_timeout) > std::chrono::steady_clock::now());

            DNSServiceRefDeallocate(client);
        }
        else
        {
            slog::log<slog::severities::error>(gate, SLOG_FLF) << "DNSServiceBrowse reported error: " << errorCode;
        }

        return browsed;
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

    struct resolve_context
    {
        // resolve in-flight state
        std::vector<resolve_result>& resolved;
        bool& more_coming;
        slog::base_gate& gate;
    };

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
        resolve_context* impl = (resolve_context*)context;

        auto& results = impl->resolved;

        if (errorCode == kDNSServiceErr_NoError)
        {
            results.push_back({ hosttarget, ntohs(port), parse_txt_records(txtRecord, txtLen) });
            auto& result = results.back();

            slog::log<slog::severities::more_info>(impl->gate, SLOG_FLF) << "After DNSServiceResolve, DNSServiceResolveReply got host: " << result.host_name << " port: " << (int)result.port;

            impl->more_coming = 0 != (flags & kDNSServiceFlagsMoreComing);
        }
        else
        {
            slog::log<slog::severities::error>(impl->gate, SLOG_FLF) << "After DNSServiceResolve, DNSServiceResolveReply received error: " << errorCode;
        }
    }

#ifdef HAVE_DNSSERVICEGETADDRINFO
    struct getaddrinfo_context
    {
        // getaddrinfo in-flight state
        std::vector<std::string>& ip_addresses;
        bool& more_coming;
        slog::base_gate& gate;
    };

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
        getaddrinfo_context* impl = (getaddrinfo_context*)context;

        auto& results = impl->ip_addresses;

        if (errorCode == kDNSServiceErr_NoError)
        {
            if (0 != address)
            {
                const auto ip_address = from_sockaddr(*address);
                if (!ip_address.is_unspecified())
                {
                    results.push_back(ip_address.to_string());
                    slog::log<slog::severities::more_info>(impl->gate, SLOG_FLF) << "After DNSServiceGetAddrInfo, DNSServiceGetAddrInfoReply got address: " << results.back() << " for host: " << hostname;
                }
            }

            impl->more_coming = 0 != (flags & kDNSServiceFlagsMoreComing);
        }
        else
        {
            slog::log<slog::severities::error>(impl->gate, SLOG_FLF) << "After DNSServiceGetAddrInfo, DNSServiceGetAddrInfoReply received error: " << errorCode;
        }
    }
#endif

    static std::vector<resolve_result> resolve(const std::string& name, const std::string& type, const std::string& domain, std::uint32_t interface_id, const std::chrono::steady_clock::duration& latest_timeout_, DNSServiceCancellationToken cancel, slog::base_gate& gate)
    {
        const auto earliest_timeout_ = std::chrono::seconds(0);

        std::vector<resolve_result> resolved;
        bool more_coming;

        DNSServiceRef client = nullptr;

        const auto now = std::chrono::steady_clock::now();
        const auto latest_timeout = now + latest_timeout_;
        const auto earliest_timeout = now + earliest_timeout_;

        // could use if_indextoname to get a name for the interface (remembering that 0 means "do the right thing", i.e. usually any interface, and there are some other special values too; see dns_sd.h)
        slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "DNSServiceResolve for name: " << name << " regtype: " << type << " domain: " << domain << " on interface: " << interface_id;

        resolve_context context{ resolved, more_coming, gate };
        DNSServiceErrorType errorCode = DNSServiceResolve(&client, 0, interface_id, name.c_str(), type.c_str(), domain.c_str(), (DNSServiceResolveReply)resolve_reply, &context);

        if (errorCode == kDNSServiceErr_NoError)
        {
            do
            {
                // wait for up to timeout for a response
                const auto& absolute_timeout = resolved.empty() ? latest_timeout : earliest_timeout;
                int wait_millis = (std::max)(0, (int)std::chrono::duration_cast<std::chrono::milliseconds>(absolute_timeout - std::chrono::steady_clock::now()).count());

                // process the next resolve responses (callback may be called more than once, or not at all, in any call to DNSServiceProcessResult)
                more_coming = false;
                errorCode = DNSServiceProcessResult(client, wait_millis, cancel);

                if (errorCode == kDNSServiceErr_NoError)
                {
                    slog::log<slog::severities::too_much_info>(gate, SLOG_FLF) << "After DNSServiceResolve, DNSServiceProcessResult succeeded";
                    // callback called, potentially more results coming
                }
                else if (errorCode == kDNSServiceErr_Timeout_)
                {
                    slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "After DNSServiceResolve, DNSServiceProcessResult timed out or was cancelled";
                    break;
                }
                else
                {
                    slog::log<slog::severities::error>(gate, SLOG_FLF) << "After DNSServiceResolve, DNSServiceProcessResult reported error: " << errorCode;
                    break;
                }

            } while (more_coming || (resolved.empty() ? latest_timeout : earliest_timeout) > std::chrono::steady_clock::now());

            DNSServiceRefDeallocate(client);

            for (auto& r : resolved)
            {
#ifdef HAVE_DNSSERVICEGETADDRINFO
                client = nullptr;

                // for now, limited to IPv4
                const DNSServiceProtocol protocol = kDNSServiceProtocol_IPv4;

#ifdef _WIN32
                if (protocol == kDNSServiceProtocol_IPv4 && interface_id >= kIPv6IfIndexBase)
                {
                    // no point trying in this case!
                    slog::log<slog::severities::too_much_info>(gate, SLOG_FLF) << "DNSServiceGetAddrInfo not tried for hostname: " << r.host_name << " on interface: " << interface_id;
                }
                else
#endif
                {
                    slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "DNSServiceGetAddrInfo for hostname: " << r.host_name << " on interface: " << interface_id;

                    getaddrinfo_context context{ r.ip_addresses, more_coming, gate };
                    errorCode = DNSServiceGetAddrInfo(&client, 0, interface_id, protocol, r.host_name.c_str(), getaddrinfo_reply, &context);

                    if (errorCode == kDNSServiceErr_NoError)
                    {
                        do
                        {
                            // wait for up to timeout for a response
                            const auto& absolute_timeout = r.ip_addresses.empty() ? latest_timeout : earliest_timeout;
                            int wait_millis = (std::max)(0, (int)std::chrono::duration_cast<std::chrono::milliseconds>(absolute_timeout - std::chrono::steady_clock::now()).count());

                            // process the next lookup responses (callback may be called more than once, or potentially not at all, in any call to DNSServiceProcessResult)
                            more_coming = false;
                            errorCode = DNSServiceProcessResult(client, wait_millis, cancel);

                            if (errorCode == kDNSServiceErr_NoError)
                            {
                                slog::log<slog::severities::too_much_info>(gate, SLOG_FLF) << "After DNSServiceGetAddrInfo, DNSServiceProcessResult succeeded";
                                // callback called, potentially more results coming
                            }
                            else if (errorCode == kDNSServiceErr_Timeout_)
                            {
                                slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "After DNSServiceGetAddrInfo, DNSServiceProcessResult timed out or was cancelled";
                                break;
                            }
                            else
                            {
                                slog::log<slog::severities::error>(gate, SLOG_FLF) << "After DNSServiceGetAddrInfo, DNSServiceProcessResult reported error: " << errorCode;
                                break;
                            }

                        } while (more_coming || (r.ip_addresses.empty() ? latest_timeout : earliest_timeout) > std::chrono::steady_clock::now());

                        DNSServiceRefDeallocate(client);
                    }
                    else
                    {
                        slog::log<slog::severities::error>(gate, SLOG_FLF) << "DNSServiceGetAddrInfo reported error: " << errorCode;
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
                    slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "After DNSServiceResolve, got " << r.ip_addresses.size() << " address(es) for hostname: " << r.host_name;
                }
            }
        }
        else
        {
            slog::log<slog::severities::error>(gate, SLOG_FLF) << "DNSServiceResolve reported error: " << errorCode;
        }

        return resolved;
    }
}

namespace mdns
{
    namespace details
    {
        struct cancellation_guard
        {
            cancellation_guard(const pplx::cancellation_token& source)
                : source(source)
                , target(nullptr)
            {
                if (source.is_cancelable())
                {
                    DNSServiceCreateCancellationToken(&target);
                    reg = source.register_callback([this] { DNSServiceCancel(target); });
                }
            }

            ~cancellation_guard()
            {
                if (pplx::cancellation_token_registration{} != reg) source.deregister_callback(reg);
                DNSServiceCancellationTokenDeallocate(target);
            }

            pplx::cancellation_token source;
            DNSServiceCancellationToken target;
            pplx::cancellation_token_registration reg;
        };

        class service_discovery_impl
        {
        public:
            explicit service_discovery_impl(slog::base_gate& gate)
                : gate(gate)
            {
            }

            ~service_discovery_impl()
            {
            }

            pplx::task<std::vector<browse_result>> browse(const std::string& type, const std::string& domain, std::uint32_t interface_id, const std::chrono::steady_clock::duration& timeout, const pplx::cancellation_token& token)
            {
                return pplx::create_task([=]
                {
                    cancellation_guard guard(token);
                    auto result = mdns_details::browse(type, domain, interface_id, timeout, guard.target, this->gate);
                    // when this task is cancelled, make sure it doesn't just return an empty/partial result
                    if (token.is_canceled()) pplx::cancel_current_task();
                    return result;
                }, token);
            }

            pplx::task<std::vector<resolve_result>> resolve(const std::string& name, const std::string& type, const std::string& domain, std::uint32_t interface_id, const std::chrono::steady_clock::duration& timeout, const pplx::cancellation_token& token)
            {
                return pplx::create_task([=]
                {
                    cancellation_guard guard(token);
                    auto result = mdns_details::resolve(name, type, domain, interface_id, timeout, guard.target, this->gate);
                    // when this task is cancelled, make sure it doesn't just return an empty/partial result
                    if (token.is_canceled()) pplx::cancel_current_task();
                    return result;
                }, token);
            }

        private:
            slog::base_gate& gate;
        };
    }

    service_discovery::service_discovery(slog::base_gate& gate)
        : impl(new details::service_discovery_impl(gate))
    {
    }

    service_discovery::service_discovery(service_discovery&& other)
        : impl(std::move(other.impl))
    {
    }

    service_discovery& service_discovery::operator=(service_discovery&& other)
    {
        if (this != &other)
        {
            impl = std::move(other.impl);
        }
        return *this;
    }

    service_discovery::~service_discovery()
    {
    }

    pplx::task<std::vector<browse_result>> service_discovery::browse(const std::string& type, const std::string& domain, std::uint32_t interface_id, const std::chrono::steady_clock::duration& timeout, const pplx::cancellation_token& token)
    {
        return impl->browse(type, domain, interface_id, timeout, token);
    }

    pplx::task<std::vector<resolve_result>> service_discovery::resolve(const std::string& name, const std::string& type, const std::string& domain, std::uint32_t interface_id, const std::chrono::steady_clock::duration& timeout, const pplx::cancellation_token& token)
    {
        return impl->resolve(name, type, domain, interface_id, timeout, token);
    }
}
