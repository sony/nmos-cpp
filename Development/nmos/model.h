#ifndef NMOS_MODEL_H
#define NMOS_MODEL_H

#include "nmos/mutex.h"
#include "nmos/resources.h"
#include "nmos/settings.h"

// NMOS Node and Registry models
namespace nmos
{
    struct base_model
    {
        // mutex should be used to protect the members of the model from simultaneous access by multiple threads
        mutable nmos::mutex mutex;

        // condition should be used to wait for, and notify other threads about, changes to members of the model
        mutable nmos::condition_variable condition;

        // application-wide configuration
        nmos::settings settings;

        // convenience functions

        nmos::read_lock read_lock() const { return nmos::read_lock{ mutex }; }
        nmos::write_lock write_lock() const { return nmos::write_lock{ mutex }; }
        void notify() const { return condition.notify_all(); }
    };

    struct model : base_model
    {
        // IS-04 resources for this node
        nmos::resources node_resources;
    };

    struct node_model : model
    {
        // IS-05 senders and receivers for this node
        //nmos::resources connection_resources;

        nmos::resources active;
        nmos::resources constraints;
        nmos::resources staged;

        // These are resources that have been modified by an IS-05
        // PATCH request but have not yet been activated. It is
        // separate from staged (above) because of the requirement
        // that an updated transport parameter should override an
        // updated SDP entry, but the SDP entry should take priority
        // if the transport parameters have not been changed.
        nmos::resources patched;

        struct
        {
            std::map<nmos::id, utility::string_t> redirects;
            // Consider adding more here, like the actual SDP string.
        } transportfile;
    };

    struct registry_model : model
    {
        nmos::resources registry_resources;
    };
}

#endif
