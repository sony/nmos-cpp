#ifndef NMOS_CONTROL_PROTOCOL_STATE_H
#define NMOS_CONTROL_PROTOCOL_STATE_H

#include <map>
#include "cpprest/json_utils.h"
#include "nmos/control_protocol_typedefs.h"
#include "nmos/mutex.h"
#include "nmos/resources.h"

namespace slog { class base_gate; }

namespace nmos
{
    namespace experimental
    {
        struct control_class // NcClassDescriptor
        {
            web::json::value description;
            nmos::nc_class_id class_id;
            utility::string_t name;
            web::json::value fixed_role;

            web::json::value properties; // array of nc_property_descriptor
            web::json::value methods; // array of nc_method_descriptor
            web::json::value events;  // array of nc_event_descriptor
        };

        struct datatype // NcDatatypeDescriptorEnum/NcDatatypeDescriptorPrimitive/NcDatatypeDescriptorStruct/NcDatatypeDescriptorTypeDef
        {
            web::json::value descriptor;
        };

        // nc_class_id vs control_class
        typedef std::map<web::json::value, control_class> control_classes;
        // nc_name vs datatype
        typedef std::map<utility::string_t, datatype> datatypes;

        // methods defnitions
        typedef std::function<web::json::value(nmos::resources& resources, nmos::resources::iterator resource, int32_t handle, const web::json::value& arguments, const control_classes& control_classes, const datatypes& datatypes, slog::base_gate& gate)> method;
        typedef std::map<web::json::value, method> methods; // method_id vs method handler

        struct control_protocol_state
        {
            // mutex to be used to protect the members from simultaneous access by multiple threads
            mutable nmos::mutex mutex;

            experimental::control_classes control_classes;
            experimental::datatypes datatypes;

            nmos::read_lock read_lock() const { return nmos::read_lock{ mutex }; }
            nmos::write_lock write_lock() const { return nmos::write_lock{ mutex }; }

            control_protocol_state();

            // insert control class, false if class already presented
            bool insert(const experimental::control_class& control_class);
            // erase control class of the given class id, false if the required class not found
            bool erase(nc_class_id class_id);

            // insert datatype, false if datatype already presented
            bool insert(const experimental::datatype& datatype);
            // erase datatype of the given datatype name, false if the required datatype not found
            bool erase(const utility::string_t& name);
        };

        // helper functions to create non-standard control class
        //
        // create control class method parameter
        web::json::value make_control_class_method_parameter(const utility::string_t& description, const utility::string_t& name, const utility::string_t& type_name,
            bool is_nullable = false, bool is_sequence = false, const web::json::value& constraints = web::json::value::null());
        // create control class method
        web::json::value make_control_class_method(const utility::string_t& description, const nc_method_id& id, const utility::string_t& name, const utility::string_t& result_datatype,
            const std::vector<web::json::value>& parameters = {}, bool is_deprecated = false);

        // create control class event
        web::json::value make_control_class_event(const utility::string_t& description, const nc_event_id& id, const utility::string_t& name, const utility::string_t& event_datatype,
            bool is_deprecated = false);

        // create control class property
        web::json::value make_control_class_property(const utility::string_t& description, const nc_property_id& id, const utility::string_t& name, const utility::string_t& type_name,
            bool is_read_only = false, bool is_nullable = false, bool is_sequence = false, bool is_deprecated = false, const web::json::value& constraints = web::json::value::null());
        // create control class with fixed role
        control_class make_control_class(const utility::string_t& description, const nc_class_id& class_id, const utility::string_t& name, const utility::string_t& fixed_role, const std::vector<web::json::value>& properties, const std::vector<web::json::value>& methods, const std::vector<web::json::value>& events);
        // create control class with no fixed role
        control_class make_control_class(const utility::string_t& description, const nc_class_id& class_id, const utility::string_t& name, const std::vector<web::json::value>& properties, const std::vector<web::json::value>& methods, const std::vector<web::json::value>& events);
    }
}

#endif
