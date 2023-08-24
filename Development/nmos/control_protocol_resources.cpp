#include "nmos/control_protocol_resources.h"

#include "nmos/control_protocol_resource.h"
#include "nmos/control_protocol_utils.h"
#include "nmos/resource.h"
#include "nmos/is12_versions.h"

namespace nmos
{
    namespace details
    {
        // create block resource
        nmos::resource make_block(nmos::nc_oid oid, const web::json::value& owner, const utility::string_t& role, const utility::string_t& user_label, const web::json::value& touchpoints, const web::json::value& runtime_property_constraints, const web::json::value& members)
        {
            using web::json::value;

            auto data = details::make_nc_block(nc_block_class_id, oid, true, owner, role, value::string(user_label), touchpoints, runtime_property_constraints, true, members);

            return{ is12_versions::v1_0, types::nc_block, std::move(data), true };
        }
    }

    // create block resource
    nmos::resource make_block(nc_oid oid, nc_oid owner, const utility::string_t& role, const utility::string_t& user_label, const web::json::value& touchpoints, const web::json::value& runtime_property_constraints, const web::json::value& members)
    {
        using web::json::value;

        return details::make_block(oid, value(owner), role, user_label, touchpoints, runtime_property_constraints, members);
    }

    // create Root block resource
    nmos::resource make_root_block()
    {
        using web::json::value;

        return details::make_block(1, value::null(), U("root"), U("Root"), value::null(), value::null(), value::array());
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/docs/Framework.html#ncdevicemanager
    nmos::resource make_device_manager(nc_oid oid, const nmos::settings& settings)
    {
        using web::json::value;

        const auto user_label = value::string(U("Device manager"));
        const auto& manufacturer = details::make_nc_manufacturer(nmos::experimental::fields::manufacturer_name(settings));
        const auto& product = details::make_nc_product(nmos::experimental::fields::product_name(settings), nmos::experimental::fields::product_key(settings), nmos::experimental::fields::product_key(settings));
        const auto& serial_number = nmos::experimental::fields::serial_number(settings);
        const auto device_name = value::null();
        const auto device_role = value::null();
        const auto& operational_state = details::make_nc_device_operational_state(nc_device_generic_state::normal_operation, value::null());

        auto data = details::make_nc_device_manager(oid, root_block_oid, user_label, value::null(), value::null(),
            manufacturer, product, serial_number, value::null(), device_name, device_role, operational_state, nc_reset_cause::unknown);

        return{ is12_versions::v1_0, types::nc_device_manager, std::move(data), true };
    }

    // See https://specs.amwa.tv/ms-05-02/branches/v1.0-dev/docs/Framework.html#ncclassmanager
    nmos::resource make_class_manager(nc_oid oid, const nmos::experimental::control_protocol_state& control_protocol_state)
    {
        using web::json::value;

        const auto user_label = value::string(U("Class manager"));

        auto data = details::make_nc_class_manager(oid, root_block_oid, user_label, value::null(), value::null(), control_protocol_state);

        return{ is12_versions::v1_0, types::nc_class_manager, std::move(data), true };
    }

    // add to owner block member
    bool add_member(const utility::string_t& child_description, const nmos::resource& child_block, nmos::resource& parent_block)
    {
        using web::json::value;

        auto& parent = parent_block.data;
        const auto& child = child_block.data;

        web::json::push_back(parent[nmos::fields::nc::members],
            details::make_nc_block_member_descriptor(child_description, nmos::fields::nc::role(child), nmos::fields::nc::oid(child), nmos::fields::nc::constant_oid(child), details::parse_nc_class_id(nmos::fields::nc::class_id(child)), nmos::fields::nc::user_label(child), nmos::fields::nc::oid(parent)));

        return true;
    }
}
