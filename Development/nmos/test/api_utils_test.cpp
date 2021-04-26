// The first "test" is of course whether the header compiles standalone
#include "nmos/api_utils.h"

#include "bst/test/test.h"

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testAddCorsPreflightHeaders)
{
    // Access-Control-Max-Age
    {
        web::http::http_request req;
        web::http::http_response res;
        BST_REQUIRE(&res == &nmos::details::add_cors_preflight_headers(req, res));
        BST_REQUIRE_EQUAL(1, res.headers().size());
        BST_REQUIRE_EQUAL(U("86400"), res.headers()[U("Access-Control-Max-Age")]);
    }
    // Access-Control-Allow-Headers
    {
        web::http::http_request req;
        req.headers().add(U("Access-Control-Request-Headers"), U("X-My-Custom-Header"));
        req.headers().add(U("Access-Control-Request-Headers"), U("X-Another-Custom-Header"));
        web::http::http_response res;
        BST_REQUIRE(&res == &nmos::details::add_cors_preflight_headers(req, res));
        BST_REQUIRE_EQUAL(2, res.headers().size());
        BST_REQUIRE_EQUAL(U("86400"), res.headers()[U("Access-Control-Max-Age")]);
        BST_REQUIRE_EQUAL(U("X-My-Custom-Header, X-Another-Custom-Header"), res.headers()[U("Access-Control-Allow-Headers")]);
    }
    // Access-Control-Allow-Methods
    {
        web::http::http_request req;
        web::http::http_response res;
        res.headers().add(U("Allow"), U("GET, POST, TRUMPET"));
        BST_REQUIRE(&res == &nmos::details::add_cors_preflight_headers(req, res));
        BST_REQUIRE_EQUAL(2, res.headers().size());
        BST_REQUIRE_EQUAL(U("86400"), res.headers()[U("Access-Control-Max-Age")]);
        BST_REQUIRE_EQUAL(U("GET, POST, TRUMPET"), res.headers()[U("Access-Control-Allow-Methods")]);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testAddCorsHeaders)
{
    // Access-Control-Allow-Origin
    {
        web::http::http_response res;

        BST_REQUIRE(&res == &nmos::details::add_cors_headers(res));
        BST_REQUIRE_EQUAL(U("*"), res.headers()[U("Access-Control-Allow-Origin")]);
        BST_REQUIRE(!res.headers().has(U("Access-Control-Expose-Headers")));
    }
    // Access-Control-Expose-Headers (no safelisted)
    {
        web::http::http_response res;
        res.headers().add(U("X-My-Custom-Header"), U("foo bar"));
        res.headers().add(U("Content-Language"), U("en-GB")); // safelisted
        res.headers().add(U("X-Another-Custom-Header"), U(""));
        res.headers().add(U("X-My-Custom-Header"), U("baz")); // update to previous value
        res.headers().add(U("Last-Modified"), U("Tue, 14 Mar 2017 16:03:42 GMT")); // safelisted

        BST_REQUIRE(&res == &nmos::details::add_cors_headers(res));
        BST_REQUIRE_EQUAL(U("*"), res.headers()[U("Access-Control-Allow-Origin")]);
        BST_REQUIRE_EQUAL(U("X-Another-Custom-Header, X-My-Custom-Header"), res.headers()[U("Access-Control-Expose-Headers")]); // currently sorted lexicographically
    }
    // Access-Control-Expose-Headers (no CORS)
    {
        web::http::http_response res;
        res.headers().add(U("Access-Control-Allow-Credentials"), U("true")); // CORS header

        BST_REQUIRE(&res == &nmos::details::add_cors_headers(res));
        BST_REQUIRE_EQUAL(U("*"), res.headers()[U("Access-Control-Allow-Origin")]);
        BST_REQUIRE(!res.headers().has(U("Access-Control-Expose-Headers")));
    }
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testMakeErrorResponseBody)
{
    // default user info (reason phrase), null debug info
    {
        web::json::value actual = nmos::make_error_response_body(406, U(""), U(""));
        BST_REQUIRE_EQUAL(406, actual[U("code")].as_integer());
        BST_REQUIRE_EQUAL(U("Not Acceptable"), actual[U("error")].as_string());
        BST_REQUIRE(actual[U("debug")].is_null());
    }
    // custom user info, custom debug info
    {
        web::json::value actual = nmos::make_error_response_body(406, U("Inconceivable"), U("You keep using that word. I do not think it means what you think it means."));
        BST_REQUIRE_EQUAL(406, actual[U("code")].as_integer());
        BST_REQUIRE_EQUAL(U("Inconceivable"), actual[U("error")].as_string());
        BST_REQUIRE_EQUAL(U("You"), actual[U("debug")].as_string().substr(0, 3));
    }
    // unknown status code
    {
        web::json::value actual = nmos::make_error_response_body(599);
        BST_REQUIRE_EQUAL(599, actual[U("code")].as_integer());
        BST_REQUIRE_EQUAL(U(""), actual[U("error")].as_string()); // did an unknown status code be ought to mapped to a generic non-empty error message?
    }
    // successful status code perhaps ought to throw?
}
