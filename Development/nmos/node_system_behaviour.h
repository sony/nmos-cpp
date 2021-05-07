#ifndef NMOS_NODE_SYSTEM_BEHAVIOUR_H
#define NMOS_NODE_SYSTEM_BEHAVIOUR_H

#include <functional>
#include "nmos/certificate_handlers.h"

namespace web
{
    namespace json
    {
        class value;
    }

    class uri;
}

namespace slog
{
    class base_gate;
}

namespace mdns
{
    class service_discovery;
}

// Node behaviour with the System API
// See https://github.com/AMWA-TV/nmos-system/blob/v1.0/README.md
namespace nmos
{
    struct model;
    struct resource;

    // a system_global_handler is a notification that a change in the system global configuration resource has been identified
    // system uri should be like http://api.example.com/x-nmos/system/{version}
    // or empty if errors have been encountered when interacting with all discoverable System APIs
    // this callback should not throw exceptions
    typedef std::function<void(const web::uri& system_uri, const web::json::value& system_global)> system_global_handler;

    // uses the default DNS-SD implementation
    // callbacks from this function are called with the model locked, and may read or write directly to the model
    void node_system_behaviour_thread(nmos::model& model, load_ca_certificates_handler load_ca_certificates, system_global_handler system_changed, slog::base_gate& gate);

    // uses the specified DNS-SD implementation
    // callbacks from this function are called with the model locked, and may read or write directly to the model
    void node_system_behaviour_thread(nmos::model& model, load_ca_certificates_handler load_ca_certificates, system_global_handler system_changed, mdns::service_discovery& discovery, slog::base_gate& gate);
}

#endif
