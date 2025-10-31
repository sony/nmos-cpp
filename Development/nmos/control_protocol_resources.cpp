#include "nmos/control_protocol_resources.h"

#include "nmos/control_protocol_resource.h"
#include "nmos/control_protocol_utils.h"
#include "nmos/is12_versions.h"

namespace nmos
{
    namespace details
    {
        // create block resource
        control_protocol_resource make_block(nmos::nc_oid oid, const web::json::value& owner, const utility::string_t& role, const utility::string_t& user_label, const utility::string_t& description, const web::json::value& touchpoints, const web::json::value& runtime_property_constraints, const web::json::value& members)
        {
            using web::json::value;

            auto data = nc::details::make_block(nc_block_class_id, oid, true, owner, role, value::string(user_label), description, touchpoints, runtime_property_constraints, true, members);

            return{ is12_versions::v1_0, types::nc_block, std::move(data), true };
        }
    }

    // create block resource
    control_protocol_resource make_block(nc_oid oid, nc_oid owner, const utility::string_t& role, const utility::string_t& user_label, const utility::string_t& description, const web::json::value& touchpoints, const web::json::value& runtime_property_constraints, const web::json::value& members)
    {
        using web::json::value;

        return details::make_block(oid, value(owner), role, user_label, description, touchpoints, runtime_property_constraints, members);
    }

    control_protocol_resource make_rebuildable(control_protocol_resource& control_protocol_resource)
    {
        using web::json::value;

        control_protocol_resource.data[nmos::fields::nc::is_rebuildable] = value::boolean(true);

        return control_protocol_resource;
    }

    control_protocol_resource set_block_allowed_member_classes(control_protocol_resource& control_protocol_resource, const std::vector<nmos::nc_class_id>& allowed_member_classes)
    {
        using web::json::value;

        auto allowed_member_classes_array = value::array();

        for(const auto& class_id: allowed_member_classes)
        {
            web::json::push_back(allowed_member_classes_array, nc::details::make_class_id(class_id));
        }

        control_protocol_resource.data[nmos::fields::nc::allowed_members_classes] = allowed_member_classes_array;

        return control_protocol_resource;
    }

    control_protocol_resource set_object_dependency_paths(control_protocol_resource& control_protocol_resource, const std::vector<nmos::nc_role_path>& dependency_paths)
    {
        using web::json::value;

        auto dependency_path_array = value::array();

        for(const auto& path: dependency_paths)
        {
            auto role_path = value::array();
            for (const auto& path_item : path) { web::json::push_back(role_path, path_item); }
            web::json::push_back(dependency_path_array, role_path);
        }

        control_protocol_resource.data[nmos::fields::nc::dependency_paths] = dependency_path_array;

        return control_protocol_resource;
    }

    // create Root block resource
    control_protocol_resource make_root_block()
    {
        using web::json::value;

        return details::make_block(nmos::root_block_oid, value::null(), nmos::root_block_role, U("Root"), U("Root block"), value::null(), value::null(), value::array());
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncdevicemanager
    control_protocol_resource make_device_manager(nc_oid oid, const nmos::settings& settings)
    {
        using web::json::value;

        const auto& manufacturer = nc::details::make_manufacturer(nmos::experimental::fields::manufacturer_name(settings));
        const auto& product = nc::details::make_product(nmos::experimental::fields::product_name(settings), nmos::experimental::fields::product_key(settings), nmos::experimental::fields::product_key(settings));
        const auto& serial_number = nmos::experimental::fields::serial_number(settings);
        const auto device_name = value::null();
        const auto device_role = value::null();
        const auto& operational_state = nc::details::make_device_operational_state(nc_device_generic_state::normal_operation, value::null());

        auto data = nc::details::make_device_manager(oid, root_block_oid, value::string(U("Device manager")), U("The device manager offers information about the product this device is representing"), value::null(), value::null(),
            manufacturer, product, serial_number, value::null(), device_name, device_role, operational_state, nc_reset_cause::unknown);

        return{ is12_versions::v1_0, types::nc_device_manager, std::move(data), true };
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncclassmanager
    control_protocol_resource make_class_manager(nc_oid oid, const nmos::experimental::control_protocol_state& control_protocol_state)
    {
        using web::json::value;

        auto data = nc::details::make_class_manager(oid, root_block_oid, value::string(U("Class manager")), U("The class manager offers access to control class and data type descriptors"), value::null(), value::null(), control_protocol_state);

        return{ is12_versions::v1_0, types::nc_class_manager, std::move(data), true };
    }

    // Monitoring feature set control classes
    //
    // See https://specs.amwa.tv/nmos-control-feature-sets/branches/main/monitoring/#ncreceivermonitor
    control_protocol_resource make_receiver_monitor(nc_oid oid, bool constant_oid, nc_oid owner, const utility::string_t& role, const utility::string_t& user_label, const utility::string_t& description, const web::json::value& touchpoints, const web::json::value& runtime_property_constraints, bool enabled,
        nc_overall_status::status overall_status, const utility::string_t& overall_status_message, nc_link_status::status link_status, const utility::string_t& link_status_message, nc_connection_status::status connection_status, const utility::string_t& connection_status_message, nc_synchronization_status::status synchronization_status, const utility::string_t& synchronization_status_message, const web::json::value& synchronization_source_id, nc_stream_status::status stream_status, const utility::string_t& stream_status_message, uint32_t status_reporting_delay, bool auto_reset_monitor)
    {
        auto data = nc::details::make_receiver_monitor(nc_receiver_monitor_class_id, oid, constant_oid, owner, role, user_label, description, touchpoints, runtime_property_constraints, enabled, overall_status, overall_status_message, link_status, link_status_message, connection_status, connection_status_message, synchronization_status, synchronization_status_message, synchronization_source_id, stream_status, stream_status_message, status_reporting_delay, auto_reset_monitor);

        return{ is12_versions::v1_0, types::nc_status_monitor, std::move(data), true };
    }

    // See https://specs.amwa.tv/nmos-control-feature-sets/branches/main/monitoring/#ncsendermonitor
    control_protocol_resource make_sender_monitor(nc_oid oid, bool constant_oid, nc_oid owner, const utility::string_t& role, const utility::string_t& user_label, const utility::string_t& description, const web::json::value& touchpoints, const web::json::value& runtime_property_constraints, bool enabled,
        nc_overall_status::status overall_status, const utility::string_t& overall_status_message, nc_link_status::status link_status, const utility::string_t& link_status_message, nc_transmission_status::status transmission_status, const utility::string_t& transmission_status_message, nc_synchronization_status::status synchronization_status, const utility::string_t& synchronization_status_message, const web::json::value& synchronization_source_id, nc_essence_status::status essence_status, const utility::string_t& essence_status_message, uint32_t status_reporting_delay, bool auto_reset_counters)
    {
        auto data = nc::details::make_sender_monitor(nc_sender_monitor_class_id, oid, constant_oid, owner, role, user_label, description, touchpoints, runtime_property_constraints, enabled, overall_status, overall_status_message, link_status, link_status_message, transmission_status, transmission_status_message, synchronization_status, synchronization_status_message, synchronization_source_id, essence_status, essence_status_message, status_reporting_delay, auto_reset_counters);

        return{ is12_versions::v1_0, types::nc_status_monitor, std::move(data), true };
    }

    // Identification feature set control classes
    //
    // See https://specs.amwa.tv/nmos-control-feature-sets/branches/main/identification/#ncidentbeacon
    control_protocol_resource make_ident_beacon(nc_oid oid, bool constant_oid, nc_oid owner, const utility::string_t& role, const utility::string_t& user_label, const utility::string_t& description, const web::json::value& touchpoints, const web::json::value& runtime_property_constraints, bool enabled,
        bool active)
    {
        using web::json::value;

        auto data = nc::details::make_worker(nc_ident_beacon_class_id, oid, constant_oid, owner, role, value::string(user_label), description, touchpoints, runtime_property_constraints, enabled);
        data[nmos::fields::nc::active] = value::boolean(active);

        return{ is12_versions::v1_0, types::nc_ident_beacon, std::move(data), true };
    }

    // Device Configuration feature set control classes
    //
    // See https://specs.amwa.tv/nmos-control-feature-sets/branches/main/device-configuration/#ncbulkpropertiesmanager
    control_protocol_resource make_bulk_properties_manager(nc_oid oid)
    {
        using web::json::value;

        auto data = nc::details::make_bulk_properties_manager(oid, root_block_oid, value::string(U("Bulk properties manager")), U("The bulk properties manager offers a central model for getting and setting properties of multiple role paths"), value::null(), value::null());

        return{ is12_versions::v1_0, types::nc_bulk_properties_manager, std::move(data), true };
    }
}
