#include "nmos/est_certificate_handlers.h"

namespace nmos
{
    namespace experimental
    {
        // construct callback to save certification authorities to file based on settings, see nmos/certificate_settings.h
        receive_ca_certificate_handler make_ca_certificate_received_handler(const nmos::settings& settings, slog::base_gate& gate)
        {
            auto save_ca_certificates = nmos::make_save_ca_certificates_handler(settings, gate);

            return[save_ca_certificates](const utility::string_t& ca_certificate)
            {
                save_ca_certificates(ca_certificate);
            };
        }

        // construct callback to save ECDSA server certificate to file based on settings, see nmos/certificate_settings.h
        receive_server_certificate_handler make_ecdsa_server_certificate_received_handler(const nmos::settings& settings, slog::base_gate& gate)
        {
            auto save_server_certificate = nmos::make_save_ecdsa_server_certificate_handler(settings, gate);

            return[save_server_certificate](const nmos::certificate& server_certificate)
            {
                if (save_server_certificate)
                {
                    save_server_certificate(server_certificate);
                }
            };
        }

        // construct callback to save RSA server certificate to file based on settings, see nmos/certificate_settings.h
        receive_server_certificate_handler make_rsa_server_certificate_received_handler(const nmos::settings& settings, slog::base_gate& gate)
        {
            auto save_server_certificate = nmos::make_save_rsa_server_certificate_handler(settings, gate);

            return[save_server_certificate](const nmos::certificate& server_certificate)
            {
                if (save_server_certificate)
                {
                    save_server_certificate(server_certificate);
                }
            };
        }
    }
}
