#include "registry_implementation.h"

#include "nmos/model.h"
#include "nmos/registry_server.h"
#include "nmos/slog.h"

// example registry implementation details
namespace impl
{
    // custom logging category for the example registry implementation thread
    namespace categories
    {
        const nmos::category registry_implementation{ "registry_implementation" };
    }
}

// This constructs all the callbacks used to integrate the example device-specific underlying implementation
// into the server instance for the NMOS Registry.
nmos::experimental::registry_implementation make_registry_implementation(nmos::registry_model& model, slog::base_gate& gate)
{
    return nmos::experimental::registry_implementation()
        .on_load_tls(nmos::make_load_tls_handler(model.settings, gate))
        .on_load_dh_param(nmos::make_load_dh_param_handler(model.settings, gate))
        .on_load_cacerts(nmos::make_load_cacerts_handler(model.settings, gate));
}
