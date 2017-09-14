// The first "test" is of course whether the header compiles standalone
#include "cpprest/http_utils.h"

#include "bst/test/test.h"

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testGetHostPort)
{
    // no 'Host' header
    {
        web::http::http_request req;
        BST_REQUIRE_EQUAL(std::make_pair(utility::string_t{}, 0), web::http::get_host_port(req));
    }
    // empty 'Host' header
    {
        web::http::http_request req;
        req.headers().add(U("Host"), U(""));
        BST_REQUIRE_EQUAL(std::make_pair(utility::string_t{}, 0), web::http::get_host_port(req));
    }
    // host name alone
    {
        web::http::http_request req;
        req.headers().add(U("Host"), U("foobar"));
        BST_REQUIRE_EQUAL(std::make_pair(utility::string_t{ U("foobar") }, 0), web::http::get_host_port(req));
    }
    // host name and port
    {
        web::http::http_request req;
        req.headers().add(U("Host"), U("foobar:42"));
        BST_REQUIRE_EQUAL(std::make_pair(utility::string_t{ U("foobar") }, 42), web::http::get_host_port(req));
    }
    // host address and port
    {
        web::http::http_request req;
        req.headers().add(U("Host"), U("29.31.37.41:42"));
        BST_REQUIRE_EQUAL(std::make_pair(utility::string_t{ U("29.31.37.41") }, 42), web::http::get_host_port(req));
    }

    // suspect edge cases...

    {
        web::http::http_request req;
        req.headers().add(U("Host"), U("foobar:"));
        BST_REQUIRE_EQUAL(std::make_pair(utility::string_t{ U("foobar") }, 0), web::http::get_host_port(req));
    }
    {
        web::http::http_request req;
        req.headers().add(U("Host"), U(":42"));
        BST_REQUIRE_EQUAL(std::make_pair(utility::string_t{}, 42), web::http::get_host_port(req));
    }
    {
        web::http::http_request req;
        req.headers().add(U("Host"), U("foobar:baz"));
        BST_REQUIRE_EQUAL(std::make_pair(utility::string_t{ U("foobar") }, 0), web::http::get_host_port(req));
    }
}
