#ifndef NMOS_LOGGING_API_H
#define NMOS_LOGGING_API_H

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include "cpprest/api_router.h"
#include "nmos/json_fields.h" // only for nmos::fields::id
#include "nmos/id.h"
#include "nmos/mutex.h"
#include "nmos/slog.h" // for slog::base_gate and slog::async_log_message

// This is an experimental extension to expose logging via a REST API
namespace nmos
{
    namespace experimental
    {
        // Log events just consist of their json data for now

        struct event
        {
            web::json::value data;
        };

        namespace tags
        {
            struct id;
            struct sequenced;
        }

        namespace details
        {
            struct event_id
            {
                typedef id result_type;

                result_type operator()(const event& event) const
                {
                    return nmos::fields::id(event.data);
                }
            };
        }

        typedef boost::multi_index_container<
            event,
            boost::multi_index::indexed_by<
                boost::multi_index::sequenced<boost::multi_index::tag<tags::sequenced>>,
                boost::multi_index::hashed_unique<boost::multi_index::tag<tags::id>, details::event_id>
            >
        > events;

        struct log_model
        {
            mutable nmos::mutex mutex;
            nmos::experimental::events events;

            // convenience functions

            nmos::read_lock read_lock() const { return nmos::read_lock{ mutex }; }
            nmos::write_lock write_lock() const { return nmos::write_lock{ mutex }; }
        };

        web::http::experimental::listener::api_router make_logging_api(nmos::experimental::log_model& model, slog::base_gate& gate);

        // push a log event into the model keeping a maximum size (lock the mutex before calling this)
        void insert_log_event(events& events, const slog::async_log_message& message, const id& id);
    }
}

#endif
