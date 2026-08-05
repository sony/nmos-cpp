#ifndef NMOS_EST_CERTIFICATE_HANDLERS_H
#define NMOS_EST_CERTIFICATE_HANDLERS_H

#include <functional>
#include "cpprest/details/basic_types.h"
#include "nmos/certificate_handlers.h"
#include "nmos/settings.h"

namespace web
{
    class uri;
}

namespace nmos
{
    namespace experimental
    {
        // an est_handler is a notification that a EST server has been identified
        // est uri should be like http://api.example.com//.well-known/est/{api_selector}
        // or empty if errors have been encountered when interacting with all discoverable EST APIs
        // this callback should not throw exceptions
        typedef std::function<void(const web::uri& est_uri)> est_handler;

        // callback after root ca certificate have been received
        // this callback should not throw exceptions
        typedef std::function<void(const utility::string_t& ca_certificate)> receive_ca_certificate_handler;

        // callback after server certificate has been received
        // this callback should not throw exceptions
        typedef std::function<void(const nmos::certificate& server_certificate)> receive_server_certificate_handler;

        // construct callback to save certification authorities to file based on settings, see nmos/certificate_settings.h
        receive_ca_certificate_handler make_ca_certificate_received_handler(const nmos::settings& settings, slog::base_gate& gate);

        // construct callback to save ECDSA server certificate to file based on settings, see nmos/certificate_settings.h
        receive_server_certificate_handler make_ecdsa_server_certificate_received_handler(const nmos::settings& settings, slog::base_gate& gate);

        // construct callback to save RSA server certificate to file based on settings, see nmos/certificate_settings.h
        receive_server_certificate_handler make_rsa_server_certificate_received_handler(const nmos::settings& settings, slog::base_gate& gate);
    }
}

#endif
