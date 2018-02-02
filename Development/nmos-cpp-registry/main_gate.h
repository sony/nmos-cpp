#ifndef NMOS_CPP_REGISTRY_MAIN_GATE_H
#define NMOS_CPP_REGISTRY_MAIN_GATE_H

#include <atomic>
#include <ostream>
#include <boost/algorithm/string/replace.hpp>
#include "nmos/logging_api.h"

namespace
{
    inline slog::omanip_function error_log_format(const slog::async_log_message& message)
    {
        return slog::omanip([&](std::ostream& os)
        {
            auto category = nmos::get_category_stash(message.stream());
            os
                << slog::put_timestamp(message.timestamp()) << ": "
                << slog::put_severity_name(message.level()) << ": "
                << message.thread_id() << ": "
                << (category.empty() ? "" : category + ": ")
                << boost::replace_all_copy(message.str(), "\n", "\n\t") // indent multi-line messages
                << std::endl;
        });
    }

    class main_gate : public slog::base_gate
    {
    public:
        main_gate(std::ostream& error_log, std::ostream& access_log, nmos::experimental::log_model& model, nmos::mutex& mutex, std::atomic<slog::severity>& level)
            : error_log(error_log), access_log(access_log), model(model), mutex(mutex), level(level), async_service({ *this }) {}
        virtual ~main_gate() {}

        virtual bool pertinent(slog::severity level) const { return this->level <= level; }
        virtual void log(const slog::log_message& message) const { async_service(message); }

    private:
        std::ostream& error_log;
        std::ostream& access_log;
        nmos::experimental::log_model& model;
        nmos::mutex& mutex;
        std::atomic<slog::severity>& level;

        struct service_function
        {
            main_gate& gate;

            typedef const slog::async_log_message& argument_type;
            void operator()(argument_type message) const
            {
                gate.service(message);
            }
        };

        void service(const slog::async_log_message& message)
        {
            nmos::write_lock lock(mutex);
            if (pertinent(message.level()))
            {
                error_log << error_log_format(message);
            }
            if (nmos::categories::access == nmos::get_category_stash(message.stream()))
            {
                access_log << nmos::common_log_format(message);
            }
            nmos::experimental::log_to_model(model, message);
        }

        mutable slog::async_log_service<service_function> async_service;
    };
}

#endif
