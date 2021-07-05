// The first "test" is of course whether the header compiles standalone
#include "cpprest/api_router.h"

#include "bst/test/test.h"
#include "cpprest/basic_utils.h" // for utility::us2s, utility::s2us

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testMakeListenerUri)
{
#if defined(_WIN32) && !defined(CPPREST_FORCE_HTTP_LISTENER_ASIO)
    BST_REQUIRE_STRING_EQUAL("http://*:42/", utility::us2s(web::http::experimental::listener::make_listener_uri(42).to_string()));
#else
    BST_REQUIRE_STRING_EQUAL("http://0.0.0.0:42/", utility::us2s(web::http::experimental::listener::make_listener_uri(42).to_string()));
#endif

    BST_REQUIRE_STRING_EQUAL("http://203.0.113.42:42/", utility::us2s(web::http::experimental::listener::make_listener_uri(U("203.0.113.42"), 42).to_string()));

    BST_REQUIRE_STRING_EQUAL("https://203.0.113.42:42/", utility::us2s(web::http::experimental::listener::make_listener_uri(true, U("203.0.113.42"), 42).to_string()));
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testGetRouteRelativePath)
{
    using utility::us2s;
    using web::http::experimental::listener::details::get_route_relative_path;
    using web::http::http_exception;

    web::http::http_request req;
    req.set_request_uri(web::uri(U("http://host:123/foo/bar/baz?qux=quux#quuux")));

    // clear specification

    BST_REQUIRE_STRING_EQUAL("/foo/bar/baz", us2s(get_route_relative_path(req, U(""))));
    BST_REQUIRE_STRING_EQUAL("/bar/baz", us2s(get_route_relative_path(req, U("/foo"))));
    BST_REQUIRE_STRING_EQUAL("", us2s(get_route_relative_path(req, U("/foo/bar/baz"))));
    BST_REQUIRE_THROW(get_route_relative_path(req, U("/qux")), http_exception);

    // less clear specification

    // compatible with http_request::relative_uri(), but should it be "foo/bar/baz"?
    BST_CHECK_STRING_EQUAL("/foo/bar/baz", us2s(get_route_relative_path(req, U("/"))));

    // should it be "/bar/baz"?
    BST_CHECK_STRING_EQUAL("bar/baz", us2s(get_route_relative_path(req, U("/foo/"))));

    // should it throw, no match?
    BST_CHECK_STRING_EQUAL("ar/baz", us2s(get_route_relative_path(req, U("/foo/b"))));

    // should it be ""?
    BST_CHECK_THROW(get_route_relative_path(req, U("/foo/bar/baz/")), http_exception);
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testRouteRegexMatch)
{
    using web::http::experimental::listener::details::match_entire;
    using web::http::experimental::listener::details::match_prefix;
    using web::http::experimental::listener::details::route_regex_match;

    utility::smatch_t route_match;

    BST_REQUIRE(route_regex_match(U("/foo/bar/baz"), route_match, utility::regex_t(U("/f../b../b..")), match_entire));
    BST_REQUIRE(route_regex_match(U("/foo/bar/baz"), route_match, utility::regex_t(U("/f../b../b..")), match_prefix));

    BST_REQUIRE(!route_regex_match(U("/foo/bar/baz/qux"), route_match, utility::regex_t(U("/f../b../b..")), match_entire));
    BST_REQUIRE(route_regex_match(U("/foo/bar/baz/qux"), route_match, utility::regex_t(U("/f../b../b..")), match_prefix));

    BST_REQUIRE(!route_regex_match(U("/foo/bar/qux"), route_match, utility::regex_t(U("/f../b../b..")), match_prefix));
    BST_REQUIRE(!route_regex_match(U("/qux/foo/bar/baz"), route_match, utility::regex_t(U("/f../b../b..")), match_prefix));
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testGetParameters)
{
    using web::http::experimental::listener::details::get_parameters;
    using web::http::experimental::listener::route_parameters;

    const utility::string_t path{ U("ABCD") };
    const utility::regex_t route_regex{ U("A(B(?:C(x)?))((D)?)") };
    utility::named_sub_matches_t parameter_sub_matches;
    parameter_sub_matches[U("BCx")] = 1;
    parameter_sub_matches[U("x")] = 2;
    parameter_sub_matches[U("D")] = 3;

    route_parameters expected;
    expected[U("BCx")] = U("BC");
    expected[U("D")] = U("D");

    utility::smatch_t route_match;
    BST_REQUIRE(bst::regex_match(path, route_match, route_regex));
    BST_REQUIRE(expected == get_parameters(parameter_sub_matches, route_match));
}
