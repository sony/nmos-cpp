#ifndef NMOS_CPP_NODE_MAIN_GATE_H
#define NMOS_CPP_NODE_MAIN_GATE_H

#include <atomic>
#include <ostream>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/find_format.hpp>
#include <boost/algorithm/string/finder.hpp>
#include <boost/algorithm/string/formatter.hpp>
#include "nmos/logging_api.h"

namespace
{
    inline std::string indent_new_lines(const std::string& input)
    {
        return boost::find_format_all_copy(input,
            boost::token_finder(boost::is_any_of("\r\n"), boost::algorithm::token_compress_on),
            boost::const_formatter("\n\t"));
    }

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
                << indent_new_lines(message.str())
                << std::endl;
        });
    }

    class main_gate : public slog::base_gate
    {
    public:
        main_gate(std::ostream& error_log, std::ostream& access_log, nmos::experimental::log_model& model, std::atomic<slog::severity>& level)
            : error_log(error_log), access_log(access_log), model(model), level(level), async_service({ *this }) {}
        virtual ~main_gate() {}

        virtual bool pertinent(slog::severity level) const { return this->level <= level; }
        virtual void log(const slog::log_message& message) const { async_service(message); }

    private:
        std::ostream& error_log;
        std::ostream& access_log;
        nmos::experimental::log_model& model;
        nmos::id_generator generate_id;
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
            auto lock = model.write_lock();
            if (pertinent(message.level()))
            {
                error_log << error_log_format(message);
            }
            if (nmos::categories::access == nmos::get_category_stash(message.stream()))
            {
                access_log << nmos::common_log_format(message);
            }
            nmos::experimental::insert_log_event(model.events, message, generate_id());
        }

        mutable slog::async_log_service<service_function> async_service;
    };
}

#endif
