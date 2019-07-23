#ifndef NMOS_LOG_GATE_H
#define NMOS_LOG_GATE_H

#include <atomic>
#include <ostream>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/find_format.hpp>
#include <boost/algorithm/string/finder.hpp>
#include <boost/algorithm/string/formatter.hpp>
#include <boost/range/algorithm/find.hpp>
#include <boost/range/algorithm/find_if.hpp>
#include "nmos/log_model.h"
#include "nmos/slog.h"

// This is an experimental extension to expose logging via a REST API
// as well as writing an error log to console or file, and an access log
// in Common Log Format
namespace nmos
{
    namespace experimental
    {
        namespace details
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
                    os
                        << slog::put_timestamp(message.timestamp()) << ": "
                        << slog::put_severity_name(message.level()) << ": "
                        << message.thread_id() << ": "
                        << indent_new_lines(message.str())
                        << std::endl;
                });
            }
        }

        // An example logging gateway
        class log_gate : public slog::base_gate
        {
        public:
            log_gate(std::ostream& error_log, std::ostream& access_log, nmos::experimental::log_model& model)
                : error_log(error_log), access_log(access_log), model(model), async_service({ *this }) {}
            virtual ~log_gate() {}

            virtual bool pertinent(slog::severity level) const { return model.level <= level; }
            virtual void log(const slog::log_message& message) const { async_service(message); }

        protected:
            virtual bool pertinent(const std::list<nmos::category>& categories) const
            {
                if (!model.settings.has_field(nmos::fields::logging_categories))
                {
                    return true;
                }

                const auto& pertinent_categories = nmos::fields::logging_categories(model.settings);

                if (categories.empty())
                {
                    static const auto no_category = web::json::value::string(utility::string_t());
                    return pertinent_categories.end() != boost::range::find(pertinent_categories, no_category);
                }

                // this could be made more efficient if there may be many pertinent categories
                return categories.end() != boost::range::find_if(categories, [&](const nmos::category& c)
                {
                    return pertinent_categories.end() != boost::range::find(pertinent_categories, web::json::value::string(utility::s2us(c)));
                });
            }

        private:
            std::ostream& error_log;
            std::ostream& access_log;
            nmos::experimental::log_model& model;
            nmos::id_generator generate_id;

            struct service_function
            {
                nmos::experimental::log_gate& gate;

                typedef const slog::async_log_message& argument_type;
                void operator()(argument_type message) const
                {
                    gate.service(message);
                }
            };

            void service(const slog::async_log_message& message)
            {
                auto lock = model.write_lock();

                auto categories = nmos::get_categories_stash(message.stream());

                if (pertinent(message.level()) && pertinent(categories))
                {
                    error_log << details::error_log_format(message);
                }

                if (categories.end() != boost::range::find(categories, nmos::categories::access))
                {
                    access_log << nmos::common_log_format(message);
                }

                nmos::experimental::insert_log_event(model.events, message, generate_id(), nmos::experimental::fields::logging_limit(model.settings));
            }

            mutable slog::async_log_service<service_function> async_service;
        };
    }
}

#endif
