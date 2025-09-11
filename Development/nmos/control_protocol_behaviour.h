#ifndef NMOS_CONTROL_PROTOCOL_BEHAVIOUR_H
#define NMOS_CONTROL_PROTOCOL_BEHAVIOUR_H


namespace slog
{
    class base_gate;
}

namespace mdns
{
    class service_discovery;
}

namespace nmos
{
    struct node_model;

    namespace experimental
    {
        struct control_protocol_state;

        void control_protocol_behaviour_thread(nmos::node_model& model, control_protocol_state& control_protocol_state, slog::base_gate& gate);
    }
}

#endif