#ifndef NMOS_CONTROL_PROTOCOL_STATE_H
#define NMOS_CONTROL_PROTOCOL_STATE_H

#include <map>
#include "cpprest/json_utils.h"
#include "nmos/mutex.h"

namespace nmos
{
    namespace experimental
    {
        struct control_class
        {
            web::json::value properties;  // array of nc_property_descriptor
            web::json::value methods; // array of nc_method_descriptor
            web::json::value events;  // array of nc_event_descriptor
        };

        typedef std::map<web::json::value, control_class> control_classes;

        struct control_protocol_state
        {
            // mutex to be used to protect the members from simultaneous access by multiple threads
            mutable nmos::mutex mutex;

            experimental::control_classes control_classes;

            nmos::read_lock read_lock() const { return nmos::read_lock{ mutex }; }
            nmos::write_lock write_lock() const { return nmos::write_lock{ mutex }; }

            control_protocol_state();
        };
    }
}

#endif
