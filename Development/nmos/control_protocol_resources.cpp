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

            auto data = details::make_nc_block(nc_block_class_id, oid, true, owner, role, value::string(user_label), description, touchpoints, runtime_property_constraints, true, members);

            return{ is12_versions::v1_0, types::nc_block, std::move(data), true };
        }
    }

    // create block resource
    control_protocol_resource make_block(nc_oid oid, nc_oid owner, const utility::string_t& role, const utility::string_t& user_label, const utility::string_t& description, const web::json::value& touchpoints, const web::json::value& runtime_property_constraints, const web::json::value& members)
    {
        using web::json::value;

        return details::make_block(oid, value(owner), role, user_label, description, touchpoints, runtime_property_constraints, members);
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

        const auto& manufacturer = details::make_nc_manufacturer(nmos::experimental::fields::manufacturer_name(settings));
        const auto& product = details::make_nc_product(nmos::experimental::fields::product_name(settings), nmos::experimental::fields::product_key(settings), nmos::experimental::fields::product_key(settings));
        const auto& serial_number = nmos::experimental::fields::serial_number(settings);
        const auto device_name = value::null();
        const auto device_role = value::null();
        const auto& operational_state = details::make_nc_device_operational_state(nc_device_generic_state::normal_operation, value::null());

        auto data = details::make_nc_device_manager(oid, root_block_oid, value::string(U("Device manager")), U("The device manager offers information about the product this device is representing"), value::null(), value::null(),
            manufacturer, product, serial_number, value::null(), device_name, device_role, operational_state, nc_reset_cause::unknown);

        return{ is12_versions::v1_0, types::nc_device_manager, std::move(data), true };
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0.x/docs/Framework.html#ncclassmanager
    control_protocol_resource make_class_manager(nc_oid oid, const nmos::experimental::control_protocol_state& control_protocol_state)
    {
        using web::json::value;

        auto data = details::make_nc_class_manager(oid, root_block_oid, value::string(U("Class manager")), U("The class manager offers access to control class and data type descriptors"), value::null(), value::null(), control_protocol_state);

        return{ is12_versions::v1_0, types::nc_class_manager, std::move(data), true };
    }

    // Monitoring feature set control classes
    //
    // See https://specs.amwa.tv/nmos-control-feature-sets/branches/main/monitoring/#ncreceivermonitor
    control_protocol_resource make_receiver_monitor(nc_oid oid, bool constant_oid, nc_oid owner, const utility::string_t& role, const utility::string_t& user_label, const utility::string_t& description, const web::json::value& touchpoints, const web::json::value& runtime_property_constraints, bool enabled,
        nc_connection_status::status connection_status, const utility::string_t& connection_status_message, nc_payload_status::status payload_status, const utility::string_t& payload_status_message)
    {
        auto data = details::make_receiver_monitor(nc_receiver_monitor_class_id, oid, constant_oid, owner, role, user_label, description, touchpoints, runtime_property_constraints, enabled, connection_status, connection_status_message, payload_status, payload_status_message);

        return{ is12_versions::v1_0, types::nc_receiver_monitor, std::move(data), true };
    }
    // See https://specs.amwa.tv/nmos-control-feature-sets/branches/main/monitoring/#ncreceivermonitorprotected
    control_protocol_resource make_receiver_monitor_protected(nc_oid oid, bool constant_oid, nc_oid owner, const utility::string_t& role, const utility::string_t& user_label, const utility::string_t& description, const web::json::value& touchpoints, const web::json::value& runtime_property_constraints, bool enabled,
        nc_connection_status::status connection_status, const utility::string_t& connection_status_message, nc_payload_status::status payload_status, const utility::string_t& payload_status_message, bool signal_protection_status)
    {
        using web::json::value;

        auto data = details::make_receiver_monitor(nc_receiver_monitor_protected_class_id, oid, constant_oid, owner, role, user_label, description, touchpoints, runtime_property_constraints, enabled, connection_status, connection_status_message, payload_status, payload_status_message);
        data[nmos::fields::nc::signal_protection_status] = value::boolean(signal_protection_status);

        return{ is12_versions::v1_0, types::nc_receiver_monitor_protected, std::move(data), true };
    }

    // Identification feature set control classes
    //
    // See https://specs.amwa.tv/nmos-control-feature-sets/branches/main/identification/#ncidentbeacon
    control_protocol_resource make_ident_beacon(nc_oid oid, bool constant_oid, nc_oid owner, const utility::string_t& role, const utility::string_t& user_label, const utility::string_t& description, const web::json::value& touchpoints, const web::json::value& runtime_property_constraints, bool enabled,
        bool active)
    {
        using web::json::value;

        auto data = nmos::details::make_nc_worker(nc_ident_beacon_class_id, oid, constant_oid, owner, role, value::string(user_label), description, touchpoints, runtime_property_constraints, enabled);
        data[nmos::fields::nc::active] = value::boolean(active);

        return{ is12_versions::v1_0, types::nc_ident_beacon, std::move(data), true };
    }

    // Device Configuration feature set control classes
    //
    // TODO: add link
    control_protocol_resource make_bulk_properties_manager(nc_oid oid)
    {
        using web::json::value;

        auto data = details::make_nc_bulk_properties_manager(oid, root_block_oid, value::string(U("Bulk properties manager")), U("The bulk properties manager offers a central model for getting and setting properties of multiple role paths"), value::null(), value::null());

        return{ is12_versions::v1_0, types::nc_bulk_properties_manager, std::move(data), true };
    }
}
