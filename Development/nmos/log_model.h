#ifndef NMOS_LOG_MODEL_H
#define NMOS_LOG_MODEL_H

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include "nmos/id.h"
#include "nmos/json_fields.h" // only for nmos::fields::id
#include "nmos/mutex.h"

namespace slog
{
    class async_log_message;
}

// This is an experimental extension to expose logging via a REST API
namespace nmos
{
    namespace experimental
    {
        // Log events just consist of their json data, plus some API metadata
        struct event
        {
            event(web::json::value data, const nmos::tai& cursor) : data(std::move(data)), id(nmos::fields::id(this->data)), cursor(cursor) {}

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
            typedef boost::multi_index::member<event, id, &event::id> event_id_extractor;
            // could use an ordered_unique index on cursor, rather than the sequenced index?
        }

        typedef boost::multi_index_container<
            event,
            boost::multi_index::indexed_by<
                boost::multi_index::sequenced<boost::multi_index::tag<tags::sequenced>>,
                boost::multi_index::hashed_unique<boost::multi_index::tag<tags::id>, details::event_id_extractor>
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

        // push a log event into the model keeping a maximum size (lock the mutex before calling this)
        void insert_log_event(events& events, const slog::async_log_message& message, const id& id);
    }
}

#endif
