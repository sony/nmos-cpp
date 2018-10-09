#ifndef NMOS_MODEL_H
#define NMOS_MODEL_H

#include "nmos/mutex.h"
#include "nmos/resources.h"
#include "nmos/settings.h"
#include "nmos/thread_utils.h"

// NMOS Node and Registry models
namespace nmos
{
    struct base_model
    {
        // mutex should be used to protect the members of the model from simultaneous access by multiple threads
        mutable nmos::mutex mutex;

        // condition should be used to wait for, and notify other threads about, changes to any member of the model
        mutable nmos::condition_variable condition;

        // condition should be used to wait for, and notify other threads when shutdown is initiated
        mutable nmos::condition_variable shutdown_condition;

        // application-wide configuration
        nmos::settings settings;

        // flag indicating whether shutdown has been initiated
        bool shutdown = false;

        // convenience functions

        nmos::read_lock read_lock() const { return nmos::read_lock{ mutex }; }
        nmos::write_lock write_lock() const { return nmos::write_lock{ mutex }; }
        void notify() const { return condition.notify_all(); }

        template <class ReadOrWriteLock, class Predicate>
        void wait(ReadOrWriteLock& lock, Predicate pred)
        {
            condition.wait(lock, pred);
        }

        template <class ReadOrWriteLock, class TimePoint, class Predicate>
        bool wait_until(ReadOrWriteLock& lock, const TimePoint& abs_time, Predicate pred)
        {
            return details::wait_until(condition, lock, abs_time, pred);
        }

        template <class ReadOrWriteLock, class Rep, class Period, class Predicate>
        bool wait_for(ReadOrWriteLock& lock, const std::chrono::duration<Rep, Period>& rel_time, Predicate pred)
        {
            // using wait_until rather than condition.wait_for directly as a workaround for an awful bug in VS2015, resolved in VS2017
            return wait_until(lock, std::chrono::steady_clock::now() + rel_time, pred);
        }

        void controlled_shutdown()
        {
            {
                auto lock = write_lock();
                shutdown = true;
            }
            notify();
            shutdown_condition.notify_all();
        }
    };

    struct model : base_model
    {
        // IS-04 resources for this node
        nmos::resources node_resources;
    };

    struct node_model : model
    {
        // IS-05 senders and receivers for this node
        // Whereas the data of the IS-04 resources corresponds to a particular Node API resource endpoint,
        // each IS-05 resource's data is a json object with an "id" field and a field for each Connection API
        // endpoint of that logical single resource
        // i.e.
        // a "constraints" field, which must have an array value conforming to the v1.0-constraints-schema,
        // "staged" and "active" fields, which must each have a value conforming to the v1.0-sender-response-schema or v1.0-receiver-response-schema,
        // and for senders, also a "transportfile" field, the value of which must be an object, with either
        // "data" and "type" fields, or an "href" field
        // See https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.0/APIs/schemas/v1.0-constraints-schema.json
        // and https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.0/APIs/schemas/v1.0-sender-response-schema.json
        // and https://github.com/AMWA-TV/nmos-device-connection-management/blob/v1.0/APIs/schemas/v1.0-receiver-response-schema.json
        nmos::resources connection_resources;
    };

    struct registry_model : model
    {
        nmos::resources registry_resources;
    };
}

#endif
