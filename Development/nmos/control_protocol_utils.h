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

    namespace details
    {
        // get the runtime property constraints of a given property_id
        web::json::value get_runtime_property_constraints(const nc_property_id& property_id, const web::json::value& runtime_property_constraints_list);

        // get the datatype descriptor of a specific type_name
        web::json::value get_datatype_descriptor(const web::json::value& type_name, get_control_protocol_datatype_handler get_control_protocol_datatype);

        // get the datatype property constraints of a given type_name
        web::json::value get_datatype_constraints(const web::json::value& type_name, get_control_protocol_datatype_handler get_control_protocol_datatype);

        struct datatype_constraints_validation_parameters
        {
            web::json::value datatype_descriptor;
            get_control_protocol_datatype_handler get_control_protocol_datatype;
        };
        // multiple levels of constraints validation, may throw nmos::control_protocol_exception
        // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Constraints.html
        void constraints_validation(const web::json::value& value, const web::json::value& runtime_property_constraints, const web::json::value& property_constraints, const datatype_constraints_validation_parameters& params);

        // method parameter constraints validation, may throw nmos::control_protocol_exception
        void method_parameter_constraints_validation(const web::json::value& data, const web::json::value& property_constraints, const datatype_constraints_validation_parameters& params);
    }

    // is the given class_id a NcBlock
    bool is_nc_block(const nc_class_id& class_id);

    // is the given class_id a NcWorker
    bool is_nc_worker(const nc_class_id& class_id);

    // is the given class_id a NcManager
    bool is_nc_manager(const nc_class_id& class_id);

    // is the given class_id a NcDeviceManager
    bool is_nc_device_manager(const nc_class_id& class_id);

    // is the given class_id a NcClassManager
    bool is_nc_class_manager(const nc_class_id& class_id);

    // construct NcClassId
    nc_class_id make_nc_class_id(const nc_class_id& prefix, int32_t authority_key, const std::vector<int32_t>& suffix);
    nc_class_id make_nc_class_id(const nc_class_id& prefix, const std::vector<int32_t>& suffix); // using default authority_key 0

    // find control class property (NcPropertyDescriptor)
    web::json::value find_property(const nc_property_id& property_id, const nc_class_id& class_id, get_control_protocol_class_handler get_control_protocol_class);

    // get block memeber descriptors
    void get_member_descriptors(const resources& resources, const resource& resource, bool recurse, web::json::array& descriptors);

    // find members with given role name or fragment
    void find_members_by_role(const resources& resources, const resource& resource, const utility::string_t& role, bool match_whole_string, bool case_sensitive, bool recurse, web::json::array& nc_block_member_descriptors);

    // find members with given class id
    void find_members_by_class_id(const resources& resources, const resource& resource, const nc_class_id& class_id, bool include_derived, bool recurse, web::json::array& descriptors);

    // push control protocol resource into other control protocol NcBlock resource
    void push_back(control_protocol_resource& nc_block_resource, const control_protocol_resource& resource);

    // modify a control protocol resource, and insert notification event to all subscriptions
    bool modify_control_protocol_resource(resources& resources, const id& id, std::function<void(resource&)> modifier, const web::json::value& notification_event);

    // find the control protocol resource which is assoicated with the given IS-04/IS-05/IS-08 resource id
    resources::const_iterator find_control_protocol_resource(resources& resources, type type, const id& id);

    // method parameters constraints validation, may throw nmos::control_protocol_exception
    void method_parameters_contraints_validation(const web::json::value& arguments, const web::json::value& nc_method_descriptor, get_control_protocol_datatype_handler get_control_protocol_datatype);
}

#endif
