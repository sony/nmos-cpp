#ifndef NMOS_CONTROL_PROTOCOL_STATE_H
#define NMOS_CONTROL_PROTOCOL_STATE_H

#include <map>
#include "cpprest/json_utils.h"
#include "nmos/control_protocol_handlers.h"
#include "nmos/control_protocol_typedefs.h"
#include "nmos/mutex.h"

namespace slog { class base_gate; }

namespace nmos
{
    namespace experimental
    {
        struct control_class // NcClassDescriptor
        {
            utility::string_t description;
            nmos::nc_class_id class_id;
            nmos::nc_name name;
            web::json::value fixed_role;

            web::json::value properties = web::json::value::array(); // array of NcPropertyDescriptor
            std::vector<method> methods; // vector of NcMethodDescriptor and method_handler
            web::json::value events = web::json::value::array();  // array of NcEventDescriptor

            control_class()
                : class_id({ 0 })
            {}

            control_class(utility::string_t description, nmos::nc_class_id class_id, nmos::nc_name name, web::json::value fixed_role, web::json::value properties, std::vector<method> methods, web::json::value events)
                : description(std::move(description))
                , class_id(std::move(class_id))
                , name(std::move(name))
                , fixed_role(std::move(fixed_role))
                , properties(std::move(properties))
                , methods(std::move(methods))
                , events(std::move(events))
            {}
        };

        struct datatype // NcDatatypeDescriptorEnum/NcDatatypeDescriptorPrimitive/NcDatatypeDescriptorStruct/NcDatatypeDescriptorTypeDef
        {
            web::json::value descriptor;
        };

        typedef std::map<nmos::nc_class_id, control_class> control_classes;
        typedef std::map<nmos::nc_name, datatype> datatypes;

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
        web::json::value make_control_class_method_parameter(const utility::string_t& description, const nc_name& name, const utility::string_t& type_name,
            bool is_nullable = false, bool is_sequence = false, const web::json::value& constraints = web::json::value::null());
        // create control class method
        web::json::value make_control_class_method(const utility::string_t& description, const nc_method_id& id, const nc_name& name, const utility::string_t& result_datatype,
            const std::vector<web::json::value>& parameters = {}, bool is_deprecated = false);

        // create control class event
        web::json::value make_control_class_event(const utility::string_t& description, const nc_event_id& id, const nc_name& name, const utility::string_t& event_datatype,
            bool is_deprecated = false);

        // create control class property
        web::json::value make_control_class_property(const utility::string_t& description, const nc_property_id& id, const nc_name& name, const utility::string_t& type_name,
            bool is_read_only = false, bool is_nullable = false, bool is_sequence = false, bool is_deprecated = false, const web::json::value& constraints = web::json::value::null());

        // create control class with fixed role
         control_class make_control_class(const utility::string_t& description, const nc_class_id& class_id, const nc_name& name, const utility::string_t& fixed_role, const std::vector<web::json::value>& properties = {}, const std::vector<std::pair<web::json::value, nmos::experimental::method_handler>>& methods = {}, const std::vector<web::json::value>& events = {});
        // create control class with no fixed role
        control_class make_control_class(const utility::string_t& description, const nc_class_id& class_id, const nc_name& name, const std::vector<web::json::value>& properties = {}, const std::vector<std::pair<web::json::value, nmos::experimental::method_handler>>& methods = {}, const std::vector<web::json::value>& events = {});
    }
}

#endif
