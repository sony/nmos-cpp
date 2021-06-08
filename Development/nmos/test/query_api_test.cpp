// The first "test" is of course whether the header compiles standalone
#include "cpprest/json_validator.h"

#include <boost/range/adaptor/filtered.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <vector>
#include "bst/test/test.h"
#include "nmos/is04_versions.h"
#include "nmos/json_schema.h"

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testQueryAPISubscriptionsExtensionSchema)
{
    using web::json::value_of;
    using web::json::value;

    const web::json::experimental::json_validator validator
    {
        nmos::experimental::load_json_schema,
        boost::copy_range<std::vector<web::uri>>(nmos::is04_versions::all | boost::adaptors::transformed(nmos::experimental::make_queryapi_subscriptions_post_request_schema_uri))
    };

    // valid subscriptions post request data
    // see https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.3.x/examples/queryapi-subscriptions-post-request.json
    auto data = value_of({
        { U("max_update_rate_ms"), 100 },
        { U("resource_path"), U("/nodes") },
        { U("params"), value_of({
            { U("label"), U("host1") }
        }) },
        { U("persist"), false },
        { U("secure"), false }
    });

    // validate successfully, i.e. no exception
    // see https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.3.x/APIs/schemas/queryapi-subscriptions-post-request.json
    validator.validate(data, nmos::experimental::make_queryapi_subscriptions_post_request_schema_uri(nmos::is04_versions::v1_3));

    // empty path, for experimental extension
    data[U("resource_path")] = value::string(U(""));
    validator.validate(data, nmos::experimental::make_queryapi_subscriptions_post_request_schema_uri(nmos::is04_versions::v1_3));
}
