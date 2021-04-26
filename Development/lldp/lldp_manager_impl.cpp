#include "lldp/lldp_manager.h"

#include <thread>
#include <pcap.h>
#include "cpprest/basic_utils.h"
#include "cpprest/host_utils.h"
#include "lldp/lldp_frame.h"
#include "slog/all_in_one.h"

#ifndef PCAP_OPENFLAG_PROMISCUOUS
#define PCAP_OPENFLAG_PROMISCUOUS 1
#endif

#ifndef PCAP_NETMASK_UNKNOWN
#define PCAP_NETMASK_UNKNOWN 0xffffffff
#endif

namespace lldp
{
    namespace details
    {
        pcap_t* open_device(const std::string& interface_id)
        {
            char errbuf[PCAP_ERRBUF_SIZE];
            pcap_if_t* devices{ nullptr };
            pcap_if_t* device{ nullptr };
            pcap_t* handle{ nullptr };

            // retrieve the device list from the local machine
#ifdef _WIN32
            if (-1 == pcap_findalldevs_ex(PCAP_SRC_IF_STRING, nullptr, &devices, errbuf))
#else
            if (-1 == pcap_findalldevs(&devices, errbuf))
#endif
            {
                throw lldp::lldp_exception("unable to retrieve LLDP device list: " + std::string(errbuf));
            }

            // select device for LLDP operation
            device = [devices, interface_id]() {
                pcap_if_t* device{ nullptr };
                for (device = devices; device != nullptr; device = device->next)
                {
                    if (std::string::npos != std::string(device->name).find(interface_id))
                    {
                        return device;
                    }
                }
                return device;
            }();

            if (!device)
            {
                throw lldp::lldp_exception("no LLDP device found for " + interface_id);
            }

#ifdef _WIN32
            if ((handle = pcap_open(device->name, 65535, PCAP_OPENFLAG_PROMISCUOUS | PCAP_OPENFLAG_NOCAPTURE_LOCAL, 1000, nullptr, errbuf)) == nullptr)
#else
            if ((handle = pcap_open_live(device->name, 65535, PCAP_OPENFLAG_PROMISCUOUS, 1000, errbuf)) == nullptr)
#endif
            {
                pcap_freealldevs(devices);
                throw lldp::lldp_exception("failed to open LLDP agent for " + interface_id + ": " + std::string(errbuf));
            }

            bpf_program fp{ 0, nullptr };
            std::stringstream ss_filter;
            ss_filter << std::hex << std::nouppercase << "ether[12:2] = 0x" << lldp_ether_type;

            // compile a LLDP frame filter
            if (-1 == pcap_compile(handle, &fp, ss_filter.str().data(), 1, PCAP_NETMASK_UNKNOWN))
            {
                pcap_freealldevs(devices);
                throw lldp::lldp_exception("failed to compile the LLDP frame filter: " + std::string(pcap_geterr(handle)));
            }

            // associate the compiled LLDP filter to capture
            if (-1 == pcap_setfilter(handle, &fp))
            {
                pcap_freecode(&fp);
                pcap_freealldevs(devices);
                throw lldp::lldp_exception("failed to associate the compiled LLDP filter to capture: " + std::string(pcap_geterr(handle)));
            }

            // free LLDP frame filter
            pcap_freecode(&fp);

            // free the device list
            pcap_freealldevs(devices);

            return handle;
        }

        struct receive_context
        {
            const std::string& interface_id;
            std::vector<uint8_t>& source_mac_address;
            const lldp_handler& handler;
            slog::base_gate& gate;
        };

        static void on_received_frame(u_char* user, const pcap_pkthdr* header, const u_char* bytes)
        {
            // hmm, not much we can do if no user context, or no packet header or data
            if (!user || !header || !bytes) return;
            auto context = reinterpret_cast<receive_context*>(user);

            try
            {
                if (context->handler)
                {
                    const auto lldp_frame = parse_lldp_frame(bytes, header->len);
                    if (context->source_mac_address != lldp_frame.source_mac_address)
                    {
                        context->handler(context->interface_id, lldp_frame.lldpdu);
                    }
                }
            }
            catch (const lldp_exception& e)
            {
                slog::log<slog::severities::error>(context->gate, SLOG_FLF) << "Unable to process the LLDP frame: " << e.what();
            }
        }

        void receive_thread(pcap_t* handle, std::shared_ptr<receive_context> context)
        {
            pcap_loop(handle, -1, on_received_frame, reinterpret_cast<u_char*>(context.get()));
        }

        struct transmit_context
        {
            pcap_t* handle;
            std::vector<uint8_t> frame;
            slog::base_gate& gate;
         };

        static void on_transmit_frame(std::shared_ptr<transmit_context> context)
        {
            if (-1 == pcap_sendpacket(context->handle, context->frame.data(), (int)context->frame.size()))
            {
                slog::log<slog::severities::error>(context->gate, SLOG_FLF) << "Unable to transmit the LLDP frame: " << pcap_geterr(context->handle);
            }
        }

        class lldp_agent_impl
        {
            pcap_t* handle;
            std::string interface_id; // for receive callback

            bool config_transmit;
            bool config_receive;
            bool active_transmit;
            bool active_receive;

            std::vector<uint8_t> source_mac_address;

            // transmit operation
            std::chrono::seconds transmit_interval;
            std::vector<uint8_t> transmit_mac_address;
            lldp_data_unit transmit_lldpdu;
            pplx::cancellation_token_source transmit_cancellation_source;
            pplx::task<void> transmit_task;

            // receive operation
            // possibly, multiple agents could share a single thread by using pcap_dispatch instead of pcap_loop?
            std::thread receive_thread;
            const lldp_handler& receive_handler;

            slog::base_gate& gate;

        public:
            lldp_agent_impl(pcap_t* handle, const std::string& interface_id, const std::vector<uint8_t>& destination_mac_address, const std::vector<uint8_t>& source_mac_address, const std::chrono::seconds& transmit_interval, const lldp_handler& receive_handler, slog::base_gate& gate)
                : handle(handle)
                , interface_id(interface_id)
                , config_transmit(false)
                , config_receive(false)
                , active_transmit(false)
                , active_receive(false)
                , source_mac_address(source_mac_address)
                , transmit_interval(transmit_interval)
                , transmit_mac_address(destination_mac_address)
                , receive_handler(receive_handler)
                , gate(gate)
            {
            }

            ~lldp_agent_impl()
            {
                configure_status(unmanaged);
                activate_configuration();
            }

            void configure_status(lldp::management_status status)
            {
                config_transmit = 0 != (status & management_status::transmit);
                config_receive = 0 != (status & management_status::receive);
            }

            void configure_data(lldp::lldp_data_unit data)
            {
                transmit_lldpdu = std::move(data);
            }

            // actually stop or start...
            void activate_configuration()
            {
                if (config_transmit) start_transmit();
                else if (!config_transmit && active_transmit) stop_transmit();

                if (config_receive && !active_receive) start_receive();
                else if (!config_receive && active_receive) stop_receive();
            }

        private:
            void start_transmit()
            {
                // cancel any current LLDP frame transmit task and immediately transmit the updated LLDP frame
                if (active_transmit)
                {
                    transmit_cancellation_source.cancel();
                    transmit_task.wait();
                }

                transmit_cancellation_source = pplx::cancellation_token_source();

                const lldp_frame frame{ transmit_mac_address, source_mac_address, transmit_lldpdu };
                std::shared_ptr<transmit_context> context(new transmit_context{ handle, make_lldp_frame(frame), gate });
                on_transmit_frame(context);

                auto token = transmit_cancellation_source.get_token();

                const auto interval = transmit_interval;
                transmit_task = pplx::do_while([interval, context, token]
                {
                    return pplx::complete_after(interval, token).then([context]
                    {
                        on_transmit_frame(context);
                        return true;
                    });
                }, token);

                active_transmit = true;
            }

            void stop_transmit()
            {
                // cancel any current LLDP frame transmit task and immediately transmit an LLDP frame with TTL=0
                transmit_cancellation_source.cancel();
                transmit_task.wait();

                const lldp_frame frame{ transmit_mac_address, source_mac_address, shutdown_data_unit(transmit_lldpdu.chassis_id, transmit_lldpdu.port_id) };
                std::shared_ptr<transmit_context> context(new transmit_context{ handle, make_lldp_frame(frame), gate });
                on_transmit_frame(context);

                active_transmit = false;
            }

            void start_receive()
            {
                if (!receive_thread.joinable())
                {
                    std::shared_ptr<receive_context> context(new receive_context{ interface_id, source_mac_address, receive_handler, gate });

                    // start a thread to receive LLDP frames
                    receive_thread = std::thread(&lldp::details::receive_thread, handle, context);
                }

                active_receive = true;
            }

            void stop_receive()
            {
                pcap_breakloop(handle);

                if (receive_thread.joinable())
                {
                    receive_thread.join();
                }

                active_receive = false;
            }
        };

        class lldp_manager_impl
        {
        public:
            explicit lldp_manager_impl(lldp_config config, slog::base_gate& gate)
                : config(std::move(config))
                , gate(gate)
                , opened(false)
            {}

            void set_handler(lldp_handler handler)
            {
                user_handler = std::move(handler);
            }

            pplx::task<void> open()
            {
                std::lock_guard<std::mutex> lock(mutex);

                if (opened) return pplx::task_from_result();

                opened = true;

                for (auto& agent : agents)
                {
                    agent.second->activate_configuration();
                }

                return pplx::task_from_result();
            }

            pplx::task<void> close()
            {
                std::lock_guard<std::mutex> lock(mutex);

                if (!opened) return pplx::task_from_result();

                for (auto& agent : agents)
                {
                    agent.second->configure_status(lldp::unmanaged);
                    agent.second->activate_configuration();
                }

                opened = false;

                return pplx::task_from_result();
            }

            pplx::task<bool> configure_interface(const std::string& interface_id, management_status status)
            {
                try
                {
                    const auto interface_name = utility::s2us(interface_id);
                    const auto interfaces = web::hosts::experimental::host_interfaces();
                    const auto interface_ = std::find_if(interfaces.begin(), interfaces.end(), [&](const web::hosts::experimental::host_interface& interface_)
                    {
                        return interface_name == interface_.name;
                    });

                    if (interfaces.end() == interface_) throw lldp_exception("interface " + interface_id + " not found");

                    lldp_data_unit data(
                        lldp::make_mac_address_chassis_id(utility::us2s(interfaces.front().physical_address)),
                        lldp::make_mac_address_port_id(utility::us2s(interface_->physical_address))
                    );

                    return configure_interface(interface_id, status, data);
                }
                catch (const lldp_exception& e)
                {
                    slog::log<slog::severities::error>(gate, SLOG_FLF) << "LLDP error: " << e.what();
                    return pplx::task_from_result(false);
                }
            }

            pplx::task<bool> configure_interface(const std::string& interface_id, management_status status, const lldp_data_unit& data)
            {
                std::lock_guard<std::mutex> lock(mutex);

                try
                {
                    auto agent = agents.find(interface_id);
                    if (agent == agents.end())
                    {
                        // hmm, frustrating duplication of the code above!
                        const auto interface_name = utility::s2us(interface_id);
                        const auto interfaces = web::hosts::experimental::host_interfaces();
                        const auto interface_ = std::find_if(interfaces.begin(), interfaces.end(), [&](const web::hosts::experimental::host_interface& interface_)
                        {
                            return interface_name == interface_.name;
                        });

                        if (interfaces.end() == interface_) throw lldp_exception("interface " + interface_id + " not found");

                        const auto destination_mac_address = make_mac_address(config.destination_address());
                        if (destination_mac_address.empty()) throw lldp_exception("invalid destination MAC address");
                        const auto source_mac_address = make_mac_address(utility::us2s(interface_->physical_address));
                        if (source_mac_address.empty()) throw lldp_exception("invalid source MAC address");

                        std::shared_ptr<lldp_agent_impl> agent_impl(new lldp_agent_impl(open_device(interface_id), interface_id, destination_mac_address, source_mac_address, config.transmit_interval(), user_handler, gate));
                        agent = agents.insert(std::make_pair(interface_id, agent_impl)).first;
                    }

                    agent->second->configure_status(status);
                    agent->second->configure_data(data);

                    if (opened)
                    {
                        agent->second->activate_configuration();
                    }

                    return pplx::task_from_result(true);
                }
                catch (const lldp_exception& e)
                {
                    slog::log<slog::severities::error>(gate, SLOG_FLF) << "LLDP error: " << e.what();
                    return pplx::task_from_result(false);
                }
            }

            const lldp_config& configuration() const
            {
                return config;
            }

        private:
            lldp_config config;
            slog::base_gate& gate;
            lldp_handler user_handler;
            std::mutex mutex;
            bool opened;
            std::map<std::string, std::shared_ptr<lldp_agent_impl>> agents;
        };
    }

    lldp_manager::lldp_manager(slog::base_gate& gate)
        : impl(new details::lldp_manager_impl(lldp_config(), gate))
    {
    }

    lldp_manager::lldp_manager(lldp_config config, slog::base_gate& gate)
        : impl(new details::lldp_manager_impl(std::move(config), gate))
    {
    }

    lldp_manager::lldp_manager(lldp_manager&& other)
        : impl(std::move(other.impl))
    {
    }

    lldp_manager& lldp_manager::operator=(lldp_manager&& other)
    {
        if (this != &other)
        {
            impl = std::move(other.impl);
        }
        return *this;
    }

    lldp_manager::~lldp_manager()
    {
        if (impl)
        {
            try
            {
                close().wait();
            }
            catch (...)
            {
            }
        }
    }

    void lldp_manager::set_handler(lldp_handler handler)
    {
        impl->set_handler(std::move(handler));
    }

    pplx::task<void> lldp_manager::open()
    {
        return impl->open();
    }

    pplx::task<void> lldp_manager::close()
    {
        return impl->close();
    }

    pplx::task<bool> lldp_manager::configure_interface(const std::string& interface_id, management_status status)
    {
        return impl->configure_interface(interface_id, status);
    }

    pplx::task<bool> lldp_manager::configure_interface(const std::string& interface_id, management_status status, const lldp_data_unit& data)
    {
        return impl->configure_interface(interface_id, status, data);
    }

    const lldp_config& lldp_manager::configuration() const
    {
        return impl->configuration();
    }
}
