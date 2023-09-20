#ifndef NMOS_CONTROL_PROTOCOL_UTILS_H
#define NMOS_CONTROL_PROTOCOL_UTILS_H

#include "cpprest/basic_utils.h"
#include "nmos/control_protocol_handlers.h"

namespace nmos
{
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
    void get_member_descriptors(const resources& resources, resources::iterator resource, bool recurse, web::json::array& descriptors);

    // find members with given role name or fragment
    void find_members_by_role(const resources& resources, resources::iterator resource, const utility::string_t& role, bool match_whole_string, bool case_sensitive, bool recurse, web::json::array& nc_block_member_descriptors);

    // find members with given class id
    void find_members_by_class_id(const resources& resources, resources::iterator resource, const nc_class_id& class_id, bool include_derived, bool recurse, web::json::array& descriptors);

    // add block (NcBlock) to other block (NcBlock)
    bool push_back(resource& parent_block, const resource& child_block);

    // modify a resource, and insert notification event to all subscriptions
    bool modify_resource(resources& resources, const id& id, std::function<void(resource&)> modifier, const web::json::value& notification_event);
}

#endif
