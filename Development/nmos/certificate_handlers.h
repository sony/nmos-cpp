#ifndef NMOS_CERTIFICATE_HANDLERS_H
#define NMOS_CERTIFICATE_HANDLERS_H

#include <functional>
#include "cpprest/details/basic_types.h"

namespace nmos
{
    // callback to supply trusted root CA certificate(s), in PEM format
    // on Windows, if C++ REST SDK is built with CPPREST_HTTP_CLIENT_IMPL=winhttp (reported as "client=winhttp" by nmos::get_build_settings_info)
    // the trusted root CA certificates must also be imported into the certificate store
    // this callback should not throw exceptions
    typedef std::function<utility::string_t()> load_cacert_handler;

    // callback to supply private key and certificate chain, in PEM format
    // the chain should be sorted starting with the server's certificate, followed by
    // any intermediate CA certificates, and ending with the highest level (root) CA
    // on Windows, if C++ REST SDK is built with CPPREST_HTTP_LISTENER_IMPL=httpsys (reported as "listener=httpsys" by nmos::get_build_settings_info)
    // one of the certificates must also be bound to each port e.g. using 'netsh add sslcert'
    // this callback should not throw exceptions
    typedef std::function<std::pair<utility::string_t, utility::string_t>()> load_cert_handler;

    // callback to supply Diffie-Hellman parameters for ephemeral key exchange support, in PEM format or empty string for no support
    // see e.g. https://wiki.openssl.org/index.php/Diffie-Hellman_parameters
    // this callback should not throw exceptions
    typedef std::function<utility::string_t()> load_dh_param_handler;
}

#endif
