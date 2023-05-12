#include "nmos/sdp_attributes.h"

#include "cpprest/json_utils.h"

namespace nmos
{
    namespace details
    {
        // hm, forward declaration for function in nmos/sdp_utils.cpp
        std::pair<sdp::address_type, bool> get_address_type_multicast(const utility::string_t& address);
    }

    namespace sdp_attributes
    {
        web::json::value make_extmap(const extmap& extmap)
        {
            using web::json::value_of;

            const bool keep_order = true;

            return value_of({
                { sdp::fields::name, sdp::attributes::extmap },
                { sdp::fields::value, value_of({
                    { sdp::fields::local_id, extmap.local_id },
                    { extmap.direction != sdp::direction{} ? sdp::fields::direction.key : U(""), extmap.direction.name },
                    { sdp::fields::uri, extmap.uri },
                    { !extmap.ext_attributes.empty() ? sdp::fields::extensionattributes.key : U(""), extmap.ext_attributes },
                }, keep_order) }
            }, keep_order);
        }

        extmap parse_extmap(const web::json::value& extmap)
        {
            return{ sdp::fields::local_id(extmap), sdp::direction(sdp::fields::direction(extmap)), sdp::fields::uri(extmap), sdp::fields::extensionattributes(extmap) };
        }

        web::json::value make_hkep(const hkep& hkep)
        {
            using web::json::value_of;

            const bool keep_order = true;

            return value_of({
                { sdp::fields::name, sdp::attributes::hkep },
                { sdp::fields::value, value_of({
                    { sdp::fields::port, hkep.port },
                    { sdp::fields::network_type, sdp::network_types::internet.name },
                    { sdp::fields::address_type, details::get_address_type_multicast(hkep.unicast_address).first.name },
                    { sdp::fields::unicast_address, hkep.unicast_address },
                    { sdp::fields::node_id, hkep.node_id },
                    { sdp::fields::port_id, hkep.port_id },
                }, keep_order) }
            }, keep_order);
        }

        hkep parse_hkep(const web::json::value& hkep)
        {
            return{ sdp::fields::port(hkep), sdp::fields::unicast_address(hkep), sdp::fields::node_id(hkep), sdp::fields::port_id(hkep) };
        }
    }
}
