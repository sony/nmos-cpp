#ifndef NMOS_CPP_NODE_MAIN_GATE_H
#define NMOS_CPP_NODE_MAIN_GATE_H

#include <atomic>
#include <iostream>
#include "nmos/logging_api.h"

namespace
{
    inline void log_to_ostream(std::ostream& os, const slog::async_log_message& message)
    {
        os
            << slog::put_timestamp(message.timestamp()) << ": "
            << slog::put_severity_name(message.level()) << ": "
            << message.thread_id() << ": "
            << message.str()
            << std::endl;
    }

    class main_gate : public slog::base_gate
    {
    public:
        main_gate(nmos::experimental::log_model& model, std::mutex& mutex, std::atomic<slog::severity>& level) : service({ model, mutex }), level(level) {}
        virtual ~main_gate() {}

        virtual bool pertinent(slog::severity level) const { return this->level <= level; }
        virtual void log(const slog::log_message& message) const { service(message); }

    private:
        struct service_function
        {
            nmos::experimental::log_model& model;
            std::mutex& mutex;

            typedef const slog::async_log_message& argument_type;
            void operator()(argument_type message) const
            {
                std::lock_guard<std::mutex> lock(mutex);
                log_to_ostream(std::cout, message);
                nmos::experimental::log_to_model(model, message);
            }
        };

        mutable slog::async_log_service<service_function> service;
        std::atomic<slog::severity>& level;
    };
}

#endif
