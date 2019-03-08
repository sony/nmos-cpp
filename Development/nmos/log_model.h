#ifndef NMOS_LOG_MODEL_H
#define NMOS_LOG_MODEL_H

#include <atomic>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include "nmos/id.h"
#include "nmos/json_fields.h" // only for nmos::fields::id
#include "nmos/mutex.h"
#include "nmos/settings.h"
#include "slog/all_in_one.h" // for slog::async_log_message and slog::severity, etc.

// This is an experimental extension to expose logging via a REST API
namespace nmos
{
    namespace experimental
    {
        // Log events just consist of their json data, plus some API metadata
        struct log_event
        {
            log_event(web::json::value data, const nmos::tai& cursor) : data(std::move(data)), id(nmos::fields::id(this->data)), cursor(cursor) {}

            // event data
            web::json::value data;

            // unique id, just to allow the API to provide access to single events
            nmos::id id;

            // unique cursor, just to allow the API to provide paginated access to events
            nmos::tai cursor;
        };

        namespace tags
        {
            struct id;
            struct sequenced;
        }

        namespace details
        {
            typedef boost::multi_index::member<log_event, id, &log_event::id> log_event_id_extractor;
            // could use an ordered_unique index on cursor, rather than the sequenced index?
        }

        typedef boost::multi_index_container<
            log_event,
            boost::multi_index::indexed_by<
                boost::multi_index::sequenced<boost::multi_index::tag<tags::sequenced>>,
                boost::multi_index::hashed_unique<boost::multi_index::tag<tags::id>, details::log_event_id_extractor>
            >
        > log_events;

        struct log_model
        {
            // mutex to be used to protect the members of the model from simultaneous access by multiple threads
            mutable nmos::mutex mutex;

            // application-wide configuration
            nmos::settings settings;

            // the logging level is a special case because we want to turn it into an atomic value
            // that can be read by logging statements without locking the mutex protecting the settings
            std::atomic<slog::severity> level{ nmos::fields::logging_level.default_value };

            // log events themselves
            nmos::experimental::log_events events;

            // convenience functions

            nmos::read_lock read_lock() const { return nmos::read_lock{ mutex }; }
            nmos::write_lock write_lock() const { return nmos::write_lock{ mutex }; }
        };

        // push a log event into the model keeping a maximum size (lock the mutex before calling this)
        void insert_log_event(log_events& events, const slog::async_log_message& message, const id& id, std::size_t max_size = 1234);
    }
}

#endif
