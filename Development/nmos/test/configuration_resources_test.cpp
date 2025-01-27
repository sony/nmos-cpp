// The first "test" is of course whether the header compiles standalone
#include "nmos/control_protocol_typedefs.h"
#include "nmos/configuration_resources.h"

#include "bst/test/test.h"

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testMakeObjectPropertiesSetValidation)
{
    using web::json::value_of;
    using web::json::value;

    auto role_path = web::json::value_of({ U("root"), U("path1") }).as_array();
    auto status = nmos::nc_restore_validation_status::ok;
    auto notices = value::array();
    auto status_message = U("status message");

    {
        auto object_properties_set_validation = nmos::make_object_properties_set_validation(role_path, status, notices, status_message);

        BST_REQUIRE(object_properties_set_validation.has_field(nmos::fields::nc::path));
        BST_REQUIRE(object_properties_set_validation.has_field(nmos::fields::nc::status));
        BST_REQUIRE(object_properties_set_validation.has_field(nmos::fields::nc::notices));
        BST_REQUIRE(object_properties_set_validation.has_field(nmos::fields::nc::status_message));

        BST_CHECK_EQUAL(role_path, nmos::fields::nc::path(object_properties_set_validation));
        BST_CHECK_EQUAL(status, nmos::fields::nc::status(object_properties_set_validation));
        BST_CHECK_EQUAL(notices.as_array(), nmos::fields::nc::notices(object_properties_set_validation));
        BST_CHECK_EQUAL(value::string(status_message), nmos::fields::nc::status_message(object_properties_set_validation));
    }
    {
        auto object_properties_set_validation = nmos::make_object_properties_set_validation(role_path, status, notices);

        BST_REQUIRE(object_properties_set_validation.has_field(nmos::fields::nc::path));
        BST_REQUIRE(object_properties_set_validation.has_field(nmos::fields::nc::status));
        BST_REQUIRE(object_properties_set_validation.has_field(nmos::fields::nc::notices));
        BST_REQUIRE(object_properties_set_validation.has_field(nmos::fields::nc::status_message));

        BST_CHECK_EQUAL(role_path, nmos::fields::nc::path(object_properties_set_validation));
        BST_CHECK_EQUAL(status, nmos::fields::nc::status(object_properties_set_validation));
        BST_CHECK_EQUAL(notices.as_array(), nmos::fields::nc::notices(object_properties_set_validation));
        BST_CHECK_EQUAL(web::json::value::null(), nmos::fields::nc::status_message(object_properties_set_validation));
    }
    {
        auto object_properties_set_validation = nmos::make_object_properties_set_validation(role_path, status);

        BST_REQUIRE(object_properties_set_validation.has_field(nmos::fields::nc::path));
        BST_REQUIRE(object_properties_set_validation.has_field(nmos::fields::nc::status));
        BST_REQUIRE(object_properties_set_validation.has_field(nmos::fields::nc::notices));
        BST_REQUIRE(object_properties_set_validation.has_field(nmos::fields::nc::status_message));

        BST_CHECK_EQUAL(role_path, nmos::fields::nc::path(object_properties_set_validation));
        BST_CHECK_EQUAL(status, nmos::fields::nc::status(object_properties_set_validation));
        BST_CHECK_EQUAL(web::json::value::array().as_array(), nmos::fields::nc::notices(object_properties_set_validation));
        BST_CHECK_EQUAL(web::json::value::null(), nmos::fields::nc::status_message(object_properties_set_validation));
    }
    {
        auto object_properties_set_validation = nmos::make_object_properties_set_validation(role_path, status, status_message);

        BST_REQUIRE(object_properties_set_validation.has_field(nmos::fields::nc::path));
        BST_REQUIRE(object_properties_set_validation.has_field(nmos::fields::nc::status));
        BST_REQUIRE(object_properties_set_validation.has_field(nmos::fields::nc::notices));
        BST_REQUIRE(object_properties_set_validation.has_field(nmos::fields::nc::status_message));

        BST_CHECK_EQUAL(role_path, nmos::fields::nc::path(object_properties_set_validation));
        BST_CHECK_EQUAL(status, nmos::fields::nc::status(object_properties_set_validation));
        BST_CHECK_EQUAL(web::json::value::array().as_array(), nmos::fields::nc::notices(object_properties_set_validation));
        BST_CHECK_EQUAL(value::string(status_message), nmos::fields::nc::status_message(object_properties_set_validation));
    }
}
