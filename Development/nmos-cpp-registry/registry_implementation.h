#ifndef NMOS_CPP_REGISTRY_REGISTRY_IMPLEMENTATION_H
#define NMOS_CPP_REGISTRY_REGISTRY_IMPLEMENTATION_H

namespace slog
{
    class base_gate;
}

namespace nmos
{
    struct registry_model;

    namespace experimental
    {
        struct registry_implementation;
        struct ocsp_state;
    }
}

// This constructs all the callbacks used to integrate the example device-specific underlying implementation
// into the server instance for the NMOS Registry.
nmos::experimental::registry_implementation make_registry_implementation(nmos::registry_model& model, slog::base_gate& gate);

// This constructs all the callbacks including OCSP stapling used to integrate the example device-specific underlying implementation
// into the server instance for the NMOS Registry.
nmos::experimental::registry_implementation make_registry_implementation(nmos::registry_model& model, nmos::experimental::ocsp_state& ocsp_state, slog::base_gate& gate);

#endif
