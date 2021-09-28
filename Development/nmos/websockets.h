#ifndef NMOS_WEBSOCKETS_H
#define NMOS_WEBSOCKETS_H

#include <boost/bimap/bimap.hpp>
#include <boost/bimap/unordered_set_of.hpp>
#include "cpprest/ws_listener.h" // for web::websockets::experimental::listener::connection_id, etc.
#include "nmos/id.h"

namespace nmos
{
    // This declares a container type suitable for managing websocket connections associated with subscriptions
    // to NMOS APIs, as used in the IS-04 Query WebSocket API and IS-07 Events WebSocket API.
    typedef boost::bimaps::bimap<boost::bimaps::unordered_set_of<nmos::id>, web::websockets::experimental::listener::connection_id> websockets;
}

#endif
