#ifndef NMOS_NODE_BEHAVIOUR_H
#define NMOS_NODE_BEHAVIOUR_H

namespace slog
{
    class base_gate;
}

// Node behaviour including both registered operation and peer to peer operation
// See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/docs/4.1.%20Behaviour%20-%20Registration.md
// and https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/docs/3.1.%20Discovery%20-%20Registered%20Operation.md
namespace nmos
{
    struct model;

    void node_behaviour_thread(nmos::model& model, slog::base_gate& gate);
}

#endif
