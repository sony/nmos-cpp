#include "nmos/control_protocol_resources.h"

#include "nmos/control_protocol_resource.h"
#include "nmos/control_protocol_utils.h"
#include "nmos/resource.h"
#include "nmos/is12_versions.h"

namespace nmos
{
    // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/docs/Framework.html#ncdevicemanager
    nmos::resource make_device_manager(details::nc_oid oid, nmos::resource& root_block, const nmos::settings& settings)
    {
        using web::json::value;

        auto& root_block_data = root_block.data;
        const auto& owner = nmos::fields::nc::oid(root_block_data);
        const auto user_label = value::string(U("Device manager"));
        const auto description = value::string(U("The device manager offers information about the product this device is representing"));
        const auto& manufacturer = details::make_nc_manufacturer(nmos::experimental::fields::manufacturer_name(settings));
        const auto& product = details::make_nc_product(nmos::experimental::fields::product_name(settings), nmos::experimental::fields::product_key(settings), nmos::experimental::fields::product_key(settings));
        const auto& serial_number = nmos::experimental::fields::serial_number(settings);
        const auto device_name = value::null();
        const auto device_role = value::null();
        const auto& operational_state = details::make_nc_device_operational_state(details::nc_device_generic_state::normal_operation, value::null());

        auto data = details::make_nc_device_manager(oid, owner, user_label, value::null(), value::null(),
            manufacturer, product, serial_number, value::null(), device_name, device_role, operational_state, details::nc_reset_cause::unknown);

        // add NcDeviceManager block_member_descriptor to root block members
        web::json::push_back(root_block_data[nmos::fields::nc::members],
            details::make_nc_block_member_descriptor(description, nmos::fields::nc::role(data), oid, nmos::fields::nc::constant_oid(data), data.at(nmos::fields::nc::class_id), user_label, owner));

        return{ is12_versions::v1_0, types::nc_device_manager, std::move(data), true };
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/docs/Framework.html#ncclassmanager
    nmos::resource make_class_manager(details::nc_oid oid, nmos::resource& root_block, const nmos::experimental::control_protocol_state& control_protocol_state)
    {
        using web::json::value;

        auto& root_block_data = root_block.data;
        const auto& owner = nmos::fields::nc::oid(root_block_data);
        const auto user_label = value::string(U("Class manager"));
        const auto description = value::string(U("The class manager offers access to control class and data type descriptors"));

        auto data = details::make_nc_class_manager(oid, owner, user_label, value::null(), value::null(), control_protocol_state);

        // add NcClassManager block_member_descriptor to root block members
        web::json::push_back(root_block_data[nmos::fields::nc::members],
            details::make_nc_block_member_descriptor(description, nmos::fields::nc::role(data), oid, nmos::fields::nc::constant_oid(data), data.at(nmos::fields::nc::class_id), user_label, owner));

        return{ is12_versions::v1_0, types::nc_class_manager, std::move(data), true };
    }

    // create block resource
    nmos::resource make_block(details::nc_oid oid, const web::json::value& owner, const utility::string_t& role, const utility::string_t& user_label, const web::json::value& touchpoints, const web::json::value& runtime_property_constraints, const web::json::value& members)
    {
        using web::json::value;

        auto data = details::make_nc_block(details::nc_block_class_id, oid, true, owner, role, value::string(user_label), touchpoints, runtime_property_constraints, true, members);

        return{ is12_versions::v1_0, types::nc_block, std::move(data), true };
    }
    nmos::resource make_block(details::nc_oid oid, details::nc_oid owner, const utility::string_t& role, const utility::string_t& user_label, const web::json::value& touchpoints, const web::json::value& runtime_property_constraints, const web::json::value& members)
    {
        using web::json::value;

        return make_block(oid, value(owner), role, user_label, touchpoints, runtime_property_constraints, members);
    }

    // create Root block resource
    nmos::resource make_root_block()
    {
        using web::json::value;

        return make_block(1, value::null(), U("root"), U("Root"));
    }

    // add member to nc_block
    bool add_member_to_block(const utility::string_t& description, const web::json::value& nc_block, web::json::value& parent)
    {
        using web::json::value;

        web::json::push_back(parent[nmos::fields::nc::members],
            details::make_nc_block_member_descriptor(value::string(description), nmos::fields::nc::role(nc_block), nmos::fields::nc::oid(nc_block), nmos::fields::nc::constant_oid(nc_block), nc_block.at(nmos::fields::nc::class_id), nc_block.at(nmos::fields::nc::user_label), nmos::fields::nc::oid(parent)));

        return true;
    }
}
