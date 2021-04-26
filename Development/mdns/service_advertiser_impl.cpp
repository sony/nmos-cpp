#include "mdns/service_advertiser_impl.h"

#include <boost/asio/ip/address.hpp>
#include "mdns/dns_sd_impl.h"
#include "slog/all_in_one.h"

#ifdef _WIN32
// winsock2.h provides htons
#else
// POSIX
#include <arpa/inet.h> // for htons
#endif

namespace mdns_details
{
    using namespace mdns;

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

    struct register_address_context
    {
        slog::base_gate& gate;
    };

    static void DNSSD_API register_address_reply(
        DNSServiceRef         sdRef,
        DNSRecordRef          recordRef,
        DNSServiceFlags       flags,
        DNSServiceErrorType   errorCode,
        void*                 context)
    {
        register_address_context* impl = (register_address_context*)context;

        if (errorCode == kDNSServiceErr_NoError)
        {
            slog::log<slog::severities::more_info>(impl->gate, SLOG_FLF) << "After DNSServiceRegisterRecord, DNSServiceRegisterRecordReply received no error";
        }
        else
        {
            slog::log<slog::severities::error>(impl->gate, SLOG_FLF) << "After DNSServiceRegisterRecord, DNSServiceRegisterRecordReply received error: " << errorCode;
            // possible errors include kDNSServiceErr_NameConflict, kDNSServiceErr_AlreadyRegistered
        }
    }

    static bool register_address(DNSServiceRef client, const std::string& host_name, const std::string& ip_address_, const std::string& domain, std::uint32_t interface_id, slog::base_gate& gate)
    {
        // since empty host_name is valid for other functions, check that logic error here
        if (host_name.empty()) return false;

#if BOOST_VERSION >= 106600
        const auto ip_address = boost::asio::ip::make_address(ip_address_);
#else
        const auto ip_address = boost::asio::ip::address::from_string(ip_address_);
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
        DNSServiceErrorType errorCode = nullptr == client ? DNSServiceCreateConnection(&client) : kDNSServiceErr_NoError;

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
            register_address_context context{ gate };
            errorCode = DNSServiceRegisterRecord(
                (DNSServiceRef)client,
                &recordRef,
                kDNSServiceFlagsShared,
                interface_id, // 0 == kDNSServiceInterfaceIndexAny
                fullname.c_str(),
                kDNSServiceType_A,
                kDNSServiceClass_IN,
                sizeof(rdata),
                &rdata,
                0,
                &register_address_reply,
                &context);

            if (errorCode == kDNSServiceErr_NoError)
            {
                // process the response (should this be in a loop, like for browse?)
                errorCode = DNSServiceProcessResult((DNSServiceRef)client, 1);

                if (errorCode == kDNSServiceErr_NoError)
                {
                    slog::log<slog::severities::too_much_info>(gate, SLOG_FLF) << "After DNSServiceRegisterRecord, DNSServiceProcessResult succeeded";

                    result = true;

                    slog::log<slog::severities::info>(gate, SLOG_FLF) << "Registered address: " << ip_address.to_string() << " for hostname: " << fullname;
                }
                else
                {
                    slog::log<slog::severities::error>(gate, SLOG_FLF) << "After DNSServiceRegisterRecord, DNSServiceProcessResult reported error: " << errorCode;
                }
            }
            else
            {
                slog::log<slog::severities::error>(gate, SLOG_FLF) << "DNSServiceRegisterRecord reported error: " << errorCode;
            }
        }
        else
        {
            slog::log<slog::severities::error>(gate, SLOG_FLF) << "DNSServiceCreateConnection reported error: " << errorCode;
        }

        return result;
    }

    struct service
    {
        service() : sdRef(nullptr) {}

        service(const std::string& name, const std::string& type, const std::string& domain, DNSServiceRef sdRef)
            : name(name)
            , type(type)
            , domain(domain)
            , sdRef(sdRef)
        {}

        std::string name;
        std::string type;
        std::string domain;

        DNSServiceRef sdRef;
    };

    static bool register_service(std::vector<service>& services, const std::string& name, const std::string& type, std::uint16_t port, const std::string& domain, const std::string& host_name, const txt_records& records, slog::base_gate& gate)
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
            services.push_back({ name, type, domain, sdRef });

            slog::log<slog::severities::info>(gate, SLOG_FLF) << "Registered advertisement for: " << name << "." << type << (domain.empty() ? "" : "." + domain);
        }
        else
        {
            slog::log<slog::severities::error>(gate, SLOG_FLF) << "DNSServiceRegister reported error: " << errorCode << " while registering advertisement for: " << name << "." << type << (domain.empty() ? "" : "." + domain);
        }

        return result;
    }

    static bool update_record(const std::vector<service>& services, const std::string& name, const std::string& type, const std::string& domain, const txt_records& records, slog::base_gate& gate)
    {
        bool result = false;

        // try to find a record that matches
        for (const auto& service : services)
        {
            if (service.name == name && service.type == type && service.domain == domain)
            {
                const std::vector<unsigned char> txt_records = make_txt_records(records);

                DNSServiceErrorType errorCode = DNSServiceUpdateRecord(service.sdRef, NULL, 0, (std::uint16_t)txt_records.size(), &txt_records[0], 0);

                if (errorCode == kDNSServiceErr_NoError)
                {
                    result = true;

                    slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "Updated advertisement for: " << name << "." << type << (domain.empty() ? "" : "." + domain);
                }

                break;
            }
        }

        return result;
    }
}

namespace mdns
{
    namespace details
    {
        // hm, 'final' may be appropriate here rather than 'override'?
        class service_advertiser_impl_ : public service_advertiser_impl
        {
        public:
            explicit service_advertiser_impl_(slog::base_gate& gate)
                : client(nullptr)
                , gate(gate)
            {
            }

            ~service_advertiser_impl_() override
            {
                try
                {
                    close().wait();
                }
                catch (...) {}
            }

            pplx::task<void> open() override
            {
                // this might be the right place to create the client connection, rather than doing it lazily in register_address?
                return pplx::task_from_result();
            }

            pplx::task<void> close() override
            {
                std::lock_guard<std::mutex> lock(mutex);

                // De-register anything that is registered
                for (const auto& service : services)
                {
                    DNSServiceRefDeallocate(service.sdRef);
                    slog::log<slog::severities::too_much_info>(gate, SLOG_FLF) << "Advertisement stopped for: " << service.name;
                }

                services.clear();

                if (nullptr != client)
                {
                    DNSServiceRefDeallocate(client);

                    client = nullptr;
                }

                return pplx::task_from_result();
            }

            pplx::task<bool> register_address(const std::string& host_name, const std::string& ip_address, const std::string& domain, std::uint32_t interface_id) override
            {
                return pplx::create_task([=]
                {
                    // "Note: client is responsible for serializing access to these structures if
                    // they are shared between concurrent threads."
                    // See dns_sd.h
                    std::lock_guard<std::mutex> lock(mutex);
                    return mdns_details::register_address(client, host_name, ip_address, domain, interface_id, gate);
                });
            }

            pplx::task<bool> register_service(const std::string& name, const std::string& type, std::uint16_t port, const std::string& domain, const std::string& host_name, const txt_records& txt_records) override
            {
                return pplx::create_task([=]
                {
                    // Lock also required here, for services
                    std::lock_guard<std::mutex> lock(mutex);
                    return mdns_details::register_service(services, name, type, port, domain, host_name, txt_records, gate);
                });
            }

            pplx::task<bool> update_record(const std::string& name, const std::string& type, const std::string& domain, const txt_records& txt_records) override
            {
                return pplx::create_task([=]
                {
                    // Lock also required here, for services
                    std::lock_guard<std::mutex> lock(mutex);
                    return mdns_details::update_record(services, name, type, domain, txt_records, gate);
                });
            }

        private:
            DNSServiceRef client;
            std::vector<mdns_details::service> services;
            std::mutex mutex;
            slog::base_gate& gate;
        };
    }

    service_advertiser::service_advertiser(std::unique_ptr<details::service_advertiser_impl> impl)
        : impl(std::move(impl))
    {
    }

    service_advertiser::service_advertiser(slog::base_gate& gate)
        : impl(new details::service_advertiser_impl_(gate))
    {
    }

    service_advertiser::service_advertiser(service_advertiser&& other)
        : impl(std::move(other.impl))
    {
    }

    service_advertiser& service_advertiser::operator=(service_advertiser&& other)
    {
        if (this != &other)
        {
            impl = std::move(other.impl);
        }
        return *this;
    }

    service_advertiser::~service_advertiser()
    {
    }

    pplx::task<void> service_advertiser::open()
    {
        return impl->open();
    }

    pplx::task<void> service_advertiser::close()
    {
        return impl->close();
    }

    pplx::task<bool> service_advertiser::register_address(const std::string& host_name, const std::string& ip_address, const std::string& domain, std::uint32_t interface_id)
    {
        return impl->register_address(host_name, ip_address, domain, interface_id);
    }

    pplx::task<bool> service_advertiser::register_service(const std::string& name, const std::string& type, std::uint16_t port, const std::string& domain, const std::string& host_name, const txt_records& txt_records)
    {
        return impl->register_service(name, type, port, domain, host_name, txt_records);
    }

    pplx::task<bool> service_advertiser::update_record(const std::string& name, const std::string& type, const std::string& domain, const txt_records& txt_records)
    {
        return impl->update_record(name, type, domain, txt_records);
    }
}
