#ifndef NMOS_REGISTRY_SERVER_H
#define NMOS_REGISTRY_SERVER_H

namespace slog
{
    class base_gate;
}

namespace nmos
{
    struct registry_model;

    struct server;

    namespace experimental
    {
        struct log_model;

        // Construct a server instance for an NMOS Registry instance, implementing the IS-04 Registration and Query APIs, the Node API, the IS-10 System API
        // and the experimental Logging API and Settings API, according to the specified data models
        nmos::server make_registry_server(nmos::registry_model& registry_model, nmos::experimental::log_model& log_model, slog::base_gate& gate);
    }
}

#endif
