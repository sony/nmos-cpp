#ifndef LLDP_LLDP_MANAGER_H
#define LLDP_LLDP_MANAGER_H

#include "lldp/lldp.h"
#include "pplx/pplx_utils.h"

namespace slog
{
    class base_gate;
}

namespace lldp
{
    class lldp_config
    {
    public:
        // default configuration
        lldp_config();

        const std::chrono::seconds& transmit_interval() const { return transmit_interval_; }
        void set_transmit_interval(const std::chrono::seconds& interval) { transmit_interval_ = interval; }

        //uint8_t max_credit() const { return max_credit_; }
        //void set_max_credit(uint8_t credit) { max_credit_ = credit; }

        //uint8_t fast_count() const { return fast_count_; }
        //void set_fast_count(uint8_t count) { fast_count_ = count; }

        //const std::chrono::seconds& fast_interval() const { return fast_interval_; }
        //void set_fast_interval(const std::chrono::seconds& interval) { fast_interval_ = interval; }

        const std::string& destination_address() const { return destination_address_; }
        void set_destination_address(const std::string& mac_address) { destination_address_ = mac_address; }

    private:
        std::chrono::seconds transmit_interval_;
        //uint8_t max_credit_;
        //uint8_t fast_count_;
        //std::chrono::seconds fast_interval_;
        std::string destination_address_;
    };

    inline lldp_config::lldp_config()
        : transmit_interval_(std::chrono::seconds(30))
        //, max_credit_(5)
        //, fast_count_(4)
        //, fast_interval_(std::chrono::seconds(1))
        , destination_address_(group_mac_addresses::nearest_bridge)
    {
    }

    namespace details
    {
        class lldp_manager_impl;
    }

    // This is a minimal viable Link Layer Discovery Protocol implementation to support both sending and receiving LLDP packets
    // when using a full LLDP agent, such as lldpd or lldpad on Linux, is not feasible
    class lldp_manager
    {
    public:
        explicit lldp_manager(slog::base_gate& gate); // or web::logging::experimental::log_handler to avoid the dependency on slog?
        explicit lldp_manager(lldp_config config, slog::base_gate& gate); // or web::logging::experimental::log_handler to avoid the dependency on slog?
        ~lldp_manager();

        void set_handler(lldp_handler handler);

        // start LLDP management for all configured interfaces
        pplx::task<void> open();
        // stop LLDP management for all configured interfaces
        pplx::task<void> close();

        // interfaces may be (re)configured before or after calling open()
        // if status includes transmit, the data is defaulted
        // management of a single interface may be stopped by passing management_status::unmanaged, and if currently transmitting, this will attempt to send an LLDP frame with TTL=0
        pplx::task<bool> configure_interface(const std::string& interface_id, management_status status = transmit_receive);

        // interfaces may be (re)configured before or after calling open()
        // if status includes transmit, the specified data is used
        // management of a single interface may be stopped by passing management_status::unmanaged, and if currently transmitting, this will attempt to send an LLDP frame with TTL=0
        pplx::task<bool> configure_interface(const std::string& interface_id, management_status status, const lldp_data_unit& data);

        lldp_manager(lldp_manager&& other);
        lldp_manager& operator=(lldp_manager&& other);

        const lldp_config& configuration() const;

    private:
        lldp_manager(const lldp_manager& other);
        lldp_manager& operator=(const lldp_manager& other);

        std::unique_ptr<details::lldp_manager_impl> impl;
    };

    // RAII helper for LLDP management sessions
    typedef pplx::open_close_guard<lldp_manager> lldp_manager_guard;
}

#endif
