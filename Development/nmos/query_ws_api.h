#ifndef NMOS_QUERY_WS_API_H
#define NMOS_QUERY_WS_API_H

#include <mutex>
#include <boost/bimap.hpp>
#include "cpprest/ws_listener.h" // for web::websockets::experimental::listener::connection_id, etc.
#include "nmos/model.h"

namespace slog
{
    class base_gate;
}

// Query API websocket implementation
// See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2-dev/docs/4.2.%20Behaviour%20-%20Querying.md
namespace nmos
{
    struct model;

    typedef boost::bimap<nmos::id, web::websockets::experimental::listener::connection_id> websockets;

    web::websockets::experimental::listener::validate_handler make_query_ws_validate_handler(nmos::model& model, std::mutex& mutex, slog::base_gate& gate);
    web::websockets::experimental::listener::open_handler make_query_ws_open_handler(nmos::model& model, nmos::websockets& websockets, std::mutex& mutex, std::condition_variable& query_ws_events_condition, slog::base_gate& gate);
    web::websockets::experimental::listener::close_handler make_query_ws_close_handler(nmos::model& model, nmos::websockets& websockets, std::mutex& mutex, slog::base_gate& gate);

    void send_query_ws_events_thread(web::websockets::experimental::listener::websocket_listener& listener, nmos::model& model, nmos::websockets& websockets, std::mutex& mutex, std::condition_variable& condition, bool& shutdown, slog::base_gate& gate);

    namespace details
    {
        // set all three timestamps in the message
        void set_subscription_grain_timestamp(web::json::value& message, const nmos::tai& tai);

        // make an empty grain
        web::json::value make_subscription_grain(const nmos::id& source_id, const nmos::id& flow_id, const utility::string_t& topic);
    }
}

#endif
