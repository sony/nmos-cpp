#ifndef NMOS_CERTIFICATE_SETTINGS_H
#define NMOS_CERTIFICATE_SETTINGS_H

#include "cpprest/json_utils.h"

namespace slog
{
    class base_gate;
}

namespace nmos
{
    namespace experimental
    {
        namespace fields
        {
            // ca_certificate_file: full path of certification authorities file in PEM format
            // on Windows, if C++ REST SDK is built with CPPREST_HTTP_CLIENT_IMPL=winhttp (reported as "client=winhttp" by nmos::get_build_settings_info)
            // the trusted root CA certificates must also be imported into the certificate store
            const web::json::field_as_string_or ca_certificate_file{ U("ca_certificate_file"), U("") };

            // server_certificates [registry, node]: an array of server certificate objects, each has the name of the key algorithm, the full paths of private key file and certificate chain file
            // each value must be an object like { "key_algorithm": "ECDSA", "private_key_file": "server-key.pem, "certificate_chain_file": "server-chain.pem" }
            // see key_algorithm, private_key_file and certificate_chain_file below
            const web::json::field_as_value_or server_certificates{ U("server_certificates"), web::json::value::array() };

            // key_algorithm (attribute of server_certificates objects): name of the key algorithm for the certificate, see nmos::key_algorithm
            const web::json::field_as_string_or key_algorithm{ U("key_algorithm"), U("") };

            // private_key_file (attribute of server_certificates objects): full path of private key file in PEM format
            const web::json::field_as_string_or private_key_file{ U("private_key_file"), U("") };

            // certificate_chain_file (attribute of server_certificates objects): full path of certificate chain file in PEM format, which must be sorted
            // starting with the server's certificate, followed by any intermediate CA certificates, and ending with the highest level (root) CA
            // on Windows, if C++ REST SDK is built with CPPREST_HTTP_LISTENER_IMPL=httpsys (reported as "listener=httpsys" by nmos::get_build_settings_info)
            // one of the certificates must also be bound to each port e.g. using 'netsh add sslcert'
            const web::json::field_as_string_or certificate_chain_file{ U("certificate_chain_file"), U("") };

            // dh_param_file [registry, node]: Diffie-Hellman parameters file in PEM format for ephemeral key exchange support, or empty string for no support
            const web::json::field_as_string_or dh_param_file{ U("dh_param_file"), U("") };

            // (deprecated, replaced by server_certificates)
            // private_key_files [registry, node]: full paths of private key files in PEM format
            const web::json::field_as_value_or private_key_files{ U("private_key_files"), web::json::value::array() };

            // (deprecated, replaced by server_certificates)
            // certificate_chain_files [registry, node]: full paths of server certificate chain files which must be in PEM format and must be sorted
            // starting with the server's certificate, followed by any intermediate CA certificates, and ending with the highest level (root) CA
            // on Windows, if C++ REST SDK is built with CPPREST_HTTP_LISTENER_IMPL=httpsys (reported as "listener=httpsys" by nmos::get_build_settings_info)
            // one of the certificates must also be bound to each port e.g. using 'netsh add sslcert'
            const web::json::field_as_value_or certificate_chain_files{ U("certificate_chain_files"), web::json::value::array() };
        }
    }
}

#endif
