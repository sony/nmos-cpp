#ifndef NMOS_CONTROL_PROTOCOL_UTILS_H
#define NMOS_CONTROL_PROTOCOL_UTILS_H

#include "cpprest/basic_utils.h"
#include "nmos/control_protocol_handlers.h"

namespace nmos
{
    struct control_protocol_resource;

    struct control_protocol_exception : std::runtime_error
    {
        control_protocol_exception(const std::string& message) : std::runtime_error(message) {}
    };

    namespace nc
    {
        namespace details
        {
            // get the runtime property constraints of a given property_id
            web::json::value get_runtime_property_constraints(const nc_property_id& property_id, const web::json::value& runtime_property_constraints_list);

            // get the datatype descriptor of a specific type_name
            web::json::value get_datatype_descriptor(const web::json::value& type_name, get_control_protocol_datatype_descriptor_handler get_control_protocol_datatype);

            // get the datatype property constraints of a given type_name
            web::json::value get_datatype_constraints(const web::json::value& type_name, get_control_protocol_datatype_descriptor_handler get_control_protocol_datatype);

            struct datatype_constraints_validation_parameters
            {
                web::json::value datatype_descriptor;
                get_control_protocol_datatype_descriptor_handler get_control_protocol_datatype_descriptor;
            };
            // multiple levels of constraints validation, may throw nmos::control_protocol_exception
            // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Constraints.html
            void constraints_validation(const web::json::value& value, const web::json::value& runtime_property_constraints, const web::json::value& property_constraints, const datatype_constraints_validation_parameters& params);

            // method parameter constraints validation, may throw nmos::control_protocol_exception
            void method_parameter_constraints_validation(const web::json::value& data, const web::json::value& property_constraints, const datatype_constraints_validation_parameters& params);

            // convert . delimited string into role path object
            web::json::value parse_role_path(const utility::string_t& role_path);
        }

        // is the given class_id a NcBlock
        bool is_block(const nc_class_id& class_id);

        // is the given class_id a NcWorker
        bool is_worker(const nc_class_id& class_id);

        // is the given class_id a NcManager
        bool is_manager(const nc_class_id& class_id);

        // is the given class_id a NcDeviceManager
        bool is_device_manager(const nc_class_id& class_id);

        // is the given class_id a NcClassManager
        bool is_class_manager(const nc_class_id& class_id);

        // construct NcClassId
        nc_class_id make_class_id(const nc_class_id& prefix, int32_t authority_key, const std::vector<int32_t>& suffix);
        nc_class_id make_class_id(const nc_class_id& prefix, const std::vector<int32_t>& suffix); // using default authority_key 0

        // find control class property descriptor (NcPropertyDescriptor)
        web::json::value find_property_descriptor(const nc_property_id& property_id, const nc_class_id& class_id, get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor);

        // get block memeber descriptors
        void get_member_descriptors(const resources& resources, const resource& resource, bool recurse, web::json::array& descriptors);

        // find members with given role name or fragment
        void find_members_by_role(const resources& resources, const resource& resource, const utility::string_t& role, bool match_whole_string, bool case_sensitive, bool recurse, web::json::array& nc_block_member_descriptors);

        // find members with given class id
        void find_members_by_class_id(const resources& resources, const resource& resource, const nc_class_id& class_id, bool include_derived, bool recurse, web::json::array& descriptors);

        // push control protocol resource into other control protocol NcBlock resource
        void push_back(control_protocol_resource& nc_block_resource, const control_protocol_resource& resource);

        // insert a control protocol resource
        std::pair<resources::iterator, bool> insert_resource(resources& resources, resource&& resource);

        // modify a control protocol resource, and insert notification event to all subscriptions
        bool modify_resource(resources& resources, const id& id, std::function<void(resource&)> modifier, const web::json::value& notification_event);

        // erase a control protocol resource
        resources::size_type erase_resource(resources& resources, const id& id);

        // find the control protocol resource which is assoicated with the given IS-04/IS-05/IS-08 resource id
        resources::const_iterator find_resource(resources& resources, type type, const id& id);

        // find resource based on role path.
        resources::const_iterator find_resource_by_role_path(const resources& resources, const web::json::array& role_path);

        // find resource based on role path. Roles in role path string must be delimited with a '.'
        resources::const_iterator find_resource_by_role_path(const resources& resources, const utility::string_t& role_path);

        // method parameters constraints validation, may throw nmos::control_protocol_exception
        void method_parameters_contraints_validation(const web::json::value& arguments, const web::json::value& nc_method_descriptor, get_control_protocol_datatype_descriptor_handler get_control_protocol_datatype_descriptor);

        resources::const_iterator find_touchpoint_resource(const resources& resources, const resource& resource);
    }

    // insert a control protocol resource
    //std::pair<resources::iterator, bool> insert_control_protocol_resource(resources& resources, resource&& resource);

    // erase a control protocol resource
    //resources::size_type erase_control_protocol_resource(resources& resources, const id& id);

    // insert 'value changed', 'sequence item added', 'sequence item changed' or 'sequence item removed' notification events into all grains whose subscriptions match the specified version, type and "pre" or "post" values
    void insert_notification_events(resources& resources, const api_version& version, const api_version& downgrade_version, const type& type, const web::json::value& pre, const web::json::value& post, const web::json::value& event);
}

#endif
