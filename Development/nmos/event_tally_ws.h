#ifndef EVENT_TALLY_WS_H
#define EVENT_TALLY_WS_H

namespace slog
{
    class base_gate;
}

namespace nmos
{
    struct node_model;

    namespace event_tally 
    {

        void event_tally_ws_thread(nmos::node_model& model, slog::base_gate& gate);

    }

 }

#endif //EVENT_TALLY_WS_H
