// The first "test" is of course whether the header compiles standalone
#include "nmos/connection_api.h"

#include "bst/test/test.h"
#include "nmos/is05_versions.h"
#include "nmos/json_fields.h"
#include "nmos/transport.h"
#include "nmos/type.h"

namespace nmos
{
    namespace details
    {
        // Internal helpers under test; kept out of the public header
        void validate_staged_core(const nmos::api_version& version, const nmos::type& type, const web::json::value& staged);
        void validate_staged_constraints(const nmos::type& type, const web::json::value& constraints, const nmos::transport& transport_base, const web::json::value& staged);
    }
}

namespace
{
    web::json::value make_staged(const utility::string_t& key, const web::json::value& param)
    {
        using web::json::value_of;

        return value_of({ { nmos::fields::transport_params, value_of({ value_of({ { key, param } }) }) } });
    }

    const utility::string_t ext_example_param{ U("ext_example_param") };
    const web::json::value source_id = web::json::value::string(U("aaaaaaaa-1111-4aaa-a111-aaaaaaaaaaaa"));
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testValidateStagedCoreExtRegister)
{
    using web::json::value;

    const auto validate = [](const nmos::type& type, const value& staged)
    {
        nmos::details::validate_staged_core(nmos::is05_versions::v1_1, type, staged);
    };

    // the registered ext_ parameters are validated according to the Transport Parameters register
    validate(nmos::types::receiver, make_staged(nmos::fields::ext_link_offset_delay, 30));
    validate(nmos::types::receiver, make_staged(nmos::fields::ext_link_offset_delay, value::string(U("auto"))));
    BST_REQUIRE_THROW(validate(nmos::types::receiver, make_staged(nmos::fields::ext_link_offset_delay, value::null())), web::json::json_exception);
    BST_REQUIRE_THROW(validate(nmos::types::receiver, make_staged(nmos::fields::ext_link_offset_delay, value::string(U("foo")))), web::json::json_exception);
    BST_REQUIRE_THROW(validate(nmos::types::receiver, make_staged(nmos::fields::ext_link_offset_delay, -1)), web::json::json_exception);

    validate(nmos::types::sender, make_staged(nmos::fields::ext_is_07_source_id, source_id));
    BST_REQUIRE_THROW(validate(nmos::types::sender, make_staged(nmos::fields::ext_is_07_source_id, value::null())), web::json::json_exception);
    BST_REQUIRE_THROW(validate(nmos::types::sender, make_staged(nmos::fields::ext_is_07_source_id, value::string(U("foo")))), web::json::json_exception);
    validate(nmos::types::receiver, make_staged(nmos::fields::ext_is_07_source_id, value::null()));

    // other ext_ parameters are not restricted
    validate(nmos::types::receiver, make_staged(ext_example_param, value::null()));
    validate(nmos::types::receiver, make_staged(ext_example_param, value::string(U("foo"))));
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testValidateStagedExtConstraints)
{
    using web::json::value;
    using web::json::value_of;

    const auto validate = [](const nmos::type& type, const value& constraints, const nmos::transport& transport_base, const value& staged)
    {
        nmos::details::validate_staged_constraints(type, value_of({ constraints }), transport_base, staged);
    };

    // based on the active IPMX Receiver example in VSF TR-10-8:2026 Section 8
    const auto lod_constraints = value_of({
        { nmos::fields::interface_ip, value_of({ { U("enum"), value_of({ U("192.168.55.85") }) } }) },
        { nmos::fields::rtp_enabled, value_of({ { U("enum"), value_of({ true }) } }) },
        { nmos::fields::ext_link_offset_delay, value_of({ { U("minimum"), 15 }, { U("maximum"), 52 } }) }
    });

    validate(nmos::types::receiver, lod_constraints, nmos::transports::rtp, make_staged(nmos::fields::ext_link_offset_delay, 30));
    validate(nmos::types::receiver, lod_constraints, nmos::transports::rtp, make_staged(nmos::fields::ext_link_offset_delay, value::string(U("auto"))));
    BST_REQUIRE_THROW(validate(nmos::types::receiver, lod_constraints, nmos::transports::rtp, make_staged(nmos::fields::ext_link_offset_delay, 10)), web::json::json_exception);
    BST_REQUIRE_THROW(validate(nmos::types::receiver, lod_constraints, nmos::transports::rtp, make_staged(nmos::fields::ext_link_offset_delay, 60)), web::json::json_exception);

    // other parameters are unchanged
    validate(nmos::types::receiver, lod_constraints, nmos::transports::rtp, make_staged(nmos::fields::interface_ip, value::string(U("auto"))));
    BST_REQUIRE_THROW(validate(nmos::types::receiver, lod_constraints, nmos::transports::rtp, make_staged(nmos::fields::rtp_enabled, value::string(U("auto")))), web::json::json_exception);
    BST_REQUIRE_THROW(validate(nmos::types::receiver, lod_constraints, nmos::transports::rtp, make_staged(nmos::fields::rtp_enabled, value::null())), web::json::json_exception);

    // ext_link_offset_delay permits "auto" but not null, in addition to the values in an enum constraint
    const auto lod_enum_constraints = value_of({
        { nmos::fields::ext_link_offset_delay, value_of({ { U("enum"), value_of({ 15, 30, 52 }) } }) }
    });

    validate(nmos::types::receiver, lod_enum_constraints, nmos::transports::rtp, make_staged(nmos::fields::ext_link_offset_delay, value::string(U("auto"))));
    BST_REQUIRE_THROW(validate(nmos::types::receiver, lod_enum_constraints, nmos::transports::rtp, make_staged(nmos::fields::ext_link_offset_delay, value::null())), web::json::json_exception);

    // "auto" and null are permitted for unregistered ext_ parameters, in addition to the values in an enum constraint
    const auto example_constraints = value_of({
        { ext_example_param, value_of({ { U("enum"), value_of({ 15, 30, 52 }) } }) }
    });

    validate(nmos::types::receiver, example_constraints, nmos::transports::rtp, make_staged(ext_example_param, 30));
    validate(nmos::types::receiver, example_constraints, nmos::transports::rtp, make_staged(ext_example_param, value::string(U("auto"))));
    validate(nmos::types::receiver, example_constraints, nmos::transports::rtp, make_staged(ext_example_param, value::null()));
    BST_REQUIRE_THROW(validate(nmos::types::receiver, example_constraints, nmos::transports::rtp, make_staged(ext_example_param, 20)), web::json::json_exception);

    // the IS-07 ext_ parameters do not permit "auto", and permit null only for a Receiver
    const auto is_07_constraints = value_of({
        { nmos::fields::ext_is_07_source_id, value_of({ { U("enum"), value_of({ source_id }) } }) },
        { nmos::fields::ext_is_07_rest_api_url, value_of({ { U("enum"), value_of({ U("http://example.com/x-nmos/events/v1.0/sources/aaaaaaaa-1111-4aaa-a111-aaaaaaaaaaaa/") }) } }) }
    });

    validate(nmos::types::sender, is_07_constraints, nmos::transports::websocket, make_staged(nmos::fields::ext_is_07_source_id, source_id));
    BST_REQUIRE_THROW(validate(nmos::types::sender, is_07_constraints, nmos::transports::websocket, make_staged(nmos::fields::ext_is_07_source_id, value::string(U("auto")))), web::json::json_exception);
    BST_REQUIRE_THROW(validate(nmos::types::sender, is_07_constraints, nmos::transports::websocket, make_staged(nmos::fields::ext_is_07_source_id, value::null())), web::json::json_exception);

    validate(nmos::types::receiver, is_07_constraints, nmos::transports::websocket, make_staged(nmos::fields::ext_is_07_source_id, value::null()));
    validate(nmos::types::receiver, is_07_constraints, nmos::transports::websocket, make_staged(nmos::fields::ext_is_07_rest_api_url, value::null()));
    BST_REQUIRE_THROW(validate(nmos::types::receiver, is_07_constraints, nmos::transports::websocket, make_staged(nmos::fields::ext_is_07_source_id, value::string(U("auto")))), web::json::json_exception);
    validate(nmos::types::receiver, is_07_constraints, nmos::transports::mqtt, make_staged(nmos::fields::ext_is_07_rest_api_url, value::null()));
}
