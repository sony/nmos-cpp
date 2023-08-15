#ifndef NMOS_CONTROL_PROTOCOL_STATE_H
#define NMOS_CONTROL_PROTOCOL_STATE_H

#include <map>
#include "cpprest/json_utils.h"
#include "nmos/control_protocol_class_id.h"  // for nmos::details::nc_class_id definitions
#include "nmos/mutex.h"

namespace nmos
{
    namespace experimental
    {
        struct control_class // NcClassDescriptor
        {
            web::json::value description;
            nmos::details::nc_class_id class_id;
            utility::string_t name;
            web::json::value fixed_role;

            web::json::value properties;  // array of nc_property_descriptor
            web::json::value methods; // array of nc_method_descriptor
            web::json::value events;  // array of nc_event_descriptor

            //control_class(details::nc_class_id class_id, utility::string_t name, web::json::value fixed_role, web::json::value properties, web::json::value methods, web::json::value events)
            //    : description(web::json::value::null())
            //    , class_id(std::move(class_id))
            //    , name(std::move(name))
            //    , fixed_role(std::move(fixed_role))
            //    , properties(std::move(properties))
            //    , methods(std::move(methods))
            //    , events(std::move(events))
            //{}

            //control_class(const utility::string_t& description, details::nc_class_id class_id, utility::string_t name, web::json::value fixed_role, web::json::value properties, web::json::value methods, web::json::value events)
            //    : description(web::json::value::string(description))
            //    , class_id(std::move(class_id))
            //    , name(std::move(name))
            //    , fixed_role(std::move(fixed_role))
            //    , properties(std::move(properties))
            //    , methods(std::move(methods))
            //    , events(std::move(events))
            //{}

        };

        struct datatype // NcDatatypeDescriptorEnum/NcDatatypeDescriptorPrimitive/NcDatatypeDescriptorStruct/NcDatatypeDescriptorTypeDef
        {
            web::json::value descriptor;
        };

        // nc_class_id vs control_class
        typedef std::map<web::json::value, control_class> control_classes;
        // nc_name vs datatype
        typedef std::map<utility::string_t, datatype> datatypes;

        struct control_protocol_state
        {
            // mutex to be used to protect the members from simultaneous access by multiple threads
            mutable nmos::mutex mutex;

            experimental::control_classes control_classes;
            experimental::datatypes datatypes;

            nmos::read_lock read_lock() const { return nmos::read_lock{ mutex }; }
            nmos::write_lock write_lock() const { return nmos::write_lock{ mutex }; }

            control_protocol_state();
        };
    }
}

#endif
