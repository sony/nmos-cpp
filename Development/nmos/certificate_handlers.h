#ifndef NMOS_CERTIFICATE_HANDLERS_H
#define NMOS_CERTIFICATE_HANDLERS_H

#include <functional>
#include "cpprest/details/basic_types.h"

namespace nmos
{
    // callback to supply trusted root CA certificate(s), in PEM format
    // this callback should not throw exceptions
    typedef std::function<utility::string_t()> load_cacert_handler;

    // callback to supply private key and certificate chain, in PEM format
    // the chain should be sorted starting with the server's certificate, followed by
    // any intermediate CA certificates, and ending with the highest level (root) CA
    // this callback should not throw exceptions
    typedef std::function<std::pair<utility::string_t, utility::string_t>()> load_cert_handler;

    // callback to supply Diffie-Hellman parameters for ephemeral key exchange support, in PEM format
    // see e.g. https://wiki.openssl.org/index.php/Diffie-Hellman_parameters
    // this callback should not throw exceptions
    typedef std::function<utility::string_t()> load_dh_param_handler;
}

#endif
