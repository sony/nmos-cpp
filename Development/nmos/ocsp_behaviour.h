#ifndef NMOS_OCSP_BEHAVIOUR_H
#define NMOS_OCSP_BEHAVIOUR_H

#include <functional>
#include "nmos/certificate_handlers.h"

namespace slog
{
    class base_gate;
}

namespace nmos
{
    struct model;

    namespace experimental
    {
        struct ocsp_state;
    }

    // callbacks from this function are called with the model locked, and may read or write directly to the model
    void ocsp_behaviour_thread(nmos::model& model, nmos::experimental::ocsp_state& ocsp_state, load_ca_certificates_handler load_ca_certificates, load_server_certificates_handler load_server_certificates, slog::base_gate& gate);
}

#endif
