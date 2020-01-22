#ifndef NMOS_NODE_BEHAVIOUR_H
#define NMOS_NODE_BEHAVIOUR_H

namespace slog
{
    class base_gate;
}

namespace mdns
{
    class service_advertiser;
    class service_discovery;
}

// Node behaviour including both registered operation and peer to peer operation
// See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/docs/4.1.%20Behaviour%20-%20Registration.md
// and https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/docs/3.1.%20Discovery%20-%20Registered%20Operation.md
namespace nmos
{
    struct model;

    // Node behaviour using default DNS-SD implementation
    void node_behaviour_thread(nmos::model& model, slog::base_gate& gate);

    // Node behaviour using specified DNS-SD implementation
    void node_behaviour_thread(nmos::model& model, mdns::service_advertiser& advertiser, mdns::service_discovery& discovery, slog::base_gate& gate);
}

#endif
