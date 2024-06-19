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
        struct control_class_descriptor // NcClassDescriptor
        {
            utility::string_t description;
            nmos::nc_class_id class_id;
            nmos::nc_name name;
            web::json::value fixed_role;

            web::json::value property_descriptors = web::json::value::array(); // NcPropertyDescriptor array
            std::vector<method> method_descriptors; // NcMethodDescriptor method handler array
            web::json::value event_descriptors = web::json::value::array();  // NcEventDescriptor array

            control_class_descriptor()
                : class_id({ 0 })
            {}

            control_class_descriptor(utility::string_t description, nmos::nc_class_id class_id, nmos::nc_name name, web::json::value fixed_role, web::json::value property_descriptors, std::vector<method> method_descriptors, web::json::value event_descriptors)
                : description(std::move(description))
                , class_id(std::move(class_id))
                , name(std::move(name))
                , fixed_role(std::move(fixed_role))
                , property_descriptors(std::move(property_descriptors))
                , method_descriptors(std::move(method_descriptors))
                , event_descriptors(std::move(event_descriptors))
            {}
        };

        struct datatype_descriptor // NcDatatypeDescriptorEnum/NcDatatypeDescriptorPrimitive/NcDatatypeDescriptorStruct/NcDatatypeDescriptorTypeDef
        {
            web::json::value descriptor;
        };

        typedef std::map<nmos::nc_class_id, control_class_descriptor> control_class_descriptors;
        typedef std::map<nmos::nc_name, datatype_descriptor> datatype_descriptors;

        struct control_protocol_state
        {
            // mutex to be used to protect the members from simultaneous access by multiple threads
            mutable nmos::mutex mutex;

            experimental::control_class_descriptors control_class_descriptors;
            experimental::datatype_descriptors datatype_descriptors;

            nmos::read_lock read_lock() const { return nmos::read_lock{ mutex }; }
            nmos::write_lock write_lock() const { return nmos::write_lock{ mutex }; }

            control_protocol_state(control_protocol_property_changed_handler property_changed = nullptr, get_properties_by_path_handler get_properties_by_path_handler = nullptr, validate_set_properties_by_path_handler validate_set_properties_by_path_handler = nullptr, set_properties_by_path_handler set_properties_by_path_handler = nullptr);
            // insert control class descriptor, false if class descriptor already inserted
            bool insert(const experimental::control_class_descriptor& control_class_descriptor);
            // erase control class of the given class id, false if the required class not found
            bool erase(nc_class_id class_id);

            // insert datatype descriptor, false if datatype descriptor already inserted
            bool insert(const experimental::datatype_descriptor& datatype_descriptor);
            // erase datatype descriptor of the given datatype name, false if the required datatype descriptor not found
            bool erase(const utility::string_t& datatype_name);
        };

        // helper functions to create non-standard control class
        //

        // create control class descriptor with fixed role
        control_class_descriptor make_control_class_descriptor(const utility::string_t& description, const nc_class_id& class_id, const nc_name& name, const utility::string_t& fixed_role, const std::vector<web::json::value>& properties = {}, const std::vector<method>& methods = {}, const std::vector<web::json::value>& events = {});
        // create control class descriptor with no fixed role
        control_class_descriptor make_control_class_descriptor(const utility::string_t& description, const nc_class_id& class_id, const nc_name& name, const std::vector<web::json::value>& properties = {}, const std::vector<method>& methods = {}, const std::vector<web::json::value>& events = {});

        // create control class property descriptor
        web::json::value make_control_class_property_descriptor(const utility::string_t& description, const nc_property_id& id, const nc_name& name, const utility::string_t& type_name,
            bool is_read_only = false, bool is_nullable = false, bool is_sequence = false, bool is_deprecated = false, const web::json::value& constraints = web::json::value::null());

        // create control class method parameter descriptor
        web::json::value make_control_class_method_parameter_descriptor(const utility::string_t& description, const nc_name& name, const utility::string_t& type_name,
            bool is_nullable = false, bool is_sequence = false, const web::json::value& constraints = web::json::value::null());
        // create control class method descriptor
        method make_control_class_method_descriptor(const utility::string_t& description, const nc_method_id& id, const nc_name& name, const utility::string_t& result_datatype,
            const std::vector<web::json::value>& parameters, bool is_deprecated, control_protocol_method_handler method_handler);

        // create control class event descriptor
        web::json::value make_control_class_event_descriptor(const utility::string_t& description, const nc_event_id& id, const nc_name& name, const utility::string_t& event_datatype,
            bool is_deprecated = false);
    }
}

#endif
