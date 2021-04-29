#ifndef NMOS_CERTIFICATE_HANDLERS_H
#define NMOS_CERTIFICATE_HANDLERS_H

#include <functional>
#include <vector>
#include "cpprest/details/basic_types.h"
#include "cpprest/json_utils.h"
#include "nmos/settings.h"
#include "nmos/string_enum.h"

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
            // (deprecated) private_key_files [registry, node]: full paths of private key files in PEM format
            const web::json::field_as_value_or private_key_files{ U("private_key_files"), web::json::value::array() };

            // (deprecated) certificate_chain_files [registry, node]: full paths of server certificate chain files which must be in PEM format and must be sorted
            // starting with the server's certificate, followed by any intermediate CA certificates, and ending with the highest level (root) CA
            // on Windows, if C++ REST SDK is built with CPPREST_HTTP_LISTENER_IMPL=httpsys (reported as "listener=httpsys" by nmos::get_build_settings_info)
            // one of the certificates must also be bound to each port e.g. using 'netsh add sslcert'
            const web::json::field_as_value_or certificate_chain_files{ U("certificate_chain_files"), web::json::value::array() };

            // ca_certificate_file: full path of certification authorities file in PEM format
            // on Windows, if C++ REST SDK is built with CPPREST_HTTP_CLIENT_IMPL=winhttp (reported as "client=winhttp" by nmos::get_build_settings_info)
            // the trusted root CA certificates must also be imported into the certificate store
            const web::json::field_as_string_or ca_certificate_file{ U("ca_certificate_file"), U("") };

            // type: (attribute of "tls" object) type of TLS, which can only be "ECDSA" or "RSA"
            const web::json::field_as_string_or type{ U("type"), U("") };

            // private_key_file: (attribute of "tls" object) full path of private key file in PEM format
            const web::json::field_as_string_or private_key_file{ U("private_key_file"), U("") };

            // certificate_chain_file: (attribute of "tls" object) full path of server certificate chain file in PEM format, which must be sorted
            // starting with the server's certificate, followed by any intermediate CA certificates, and ending with the highest level (root) CA
            const web::json::field_as_string_or certificate_chain_file{ U("certificate_chain_file"), U("") };

            // tls [registry, node]: an array of TLS objects, each has the full paths of private key file and server certificate chain file
            // each value must be an object like { "type": "ECDSA/RSA", "private_key_file": "server-key.pem, "certificate_chain_file": "server-chain.pem"}
            // see private_key_file and certificate_chain_file above
            const web::json::field_as_value_or tls{ U("tls"), web::json::value::array() };

            // dh_param_file [registry, node]: Diffie-Hellman parameters file in PEM format for ephemeral key exchange support, or empty string for no support
            const web::json::field_as_string_or dh_param_file{ U("dh_param_file"), U("") };
        }
    }

    // callback to supply trusted root CA certificate(s), in PEM format
    // on Windows, if C++ REST SDK is built with CPPREST_HTTP_CLIENT_IMPL=winhttp (reported as "client=winhttp" by nmos::get_build_settings_info)
    // the trusted root CA certificates must also be imported into the certificate store
    // this callback should not throw exceptions
    typedef std::function<utility::string_t()> load_cacerts_handler;

    // callback to supply a list of TLS private key and certificate chain, in PEM format
    // the chain should be sorted starting with the server's certificate, followed by
    // any intermediate CA certificates, and ending with the highest level (root) CA
    // on Windows, if C++ REST SDK is built with CPPREST_HTTP_LISTENER_IMPL=httpsys (reported as "listener=httpsys" by nmos::get_build_settings_info)
    // one of the certificates must also be bound to each port e.g. using 'netsh add sslcert'
    // this callback should not throw exceptions
    DEFINE_STRING_ENUM(tls_type)
    namespace tls_types
    {
        const tls_type ECDSA{ U("ECDSA") };
        const tls_type RSA{ U("RSA") };
    }
    typedef std::tuple<tls_type, utility::string_t, utility::string_t> tls;
    typedef std::function<std::vector<tls>()> load_tls_handler;

    // callback to supply Diffie-Hellman parameters for ephemeral key exchange support, in PEM format or empty string for no support
    // see e.g. https://wiki.openssl.org/index.php/Diffie-Hellman_parameters
    // this callback should not throw exceptions
    typedef std::function<utility::string_t()> load_dh_param_handler;

    // implement callback to load TLS keys and certificate chains
    load_tls_handler make_load_tls_handler(const nmos::settings& settings, slog::base_gate& gate);

    // implement callback to load Diffie-Hellman parameters for ephemeral key exchange support
    load_dh_param_handler make_load_dh_param_handler(const nmos::settings& settings, slog::base_gate& gate);

    // implement callback to load certification authorities
    load_cacerts_handler make_load_cacerts_handler(const nmos::settings& settings, slog::base_gate& gate);
}

#endif
