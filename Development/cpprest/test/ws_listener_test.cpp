// The first "test" is of course whether the header compiles standalone
#include "cpprest/ws_listener.h"

#include "bst/test/test.h"

BST_TEST_CASE(testWebSocketListenerCloseOpen)
{
    for (auto port = 49152; port <= 65535; ++port)
    {
        web::websockets::experimental::listener::websocket_listener ws(web::uri_builder(U("ws://localhost")).set_port(port).to_uri());
        try
        {
            ws.open().wait();
        }
        catch (const web::websockets::websocket_exception&)
        {
            // could well be that port is already in use, so just try the next one
            continue;
        }
        ws.close().wait();
        // it now ought to be possible to reopen a closed websocket_listener
        // i.e. this open task ought not to result in an exception!
        ws.open().wait();
        ws.close().wait();
        return;
    }
    // hmm, ran out of dynamic ports?!
    BST_REQUIRE(false);
}
