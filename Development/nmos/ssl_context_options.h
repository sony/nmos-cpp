#ifndef NMOS_SSL_CONTEXT_OPTIONS_H
#define NMOS_SSL_CONTEXT_OPTIONS_H

#if !defined(_WIN32) || !defined(__cplusplus_winrt) || defined(CPPREST_FORCE_HTTP_CLIENT_ASIO)
#include "boost/asio/ssl.hpp"

namespace nmos
{
    namespace details
    {
        // AMWA BCP-003-01 states that "implementations SHOULD support TLS 1.3 and SHALL support TLS 1.2."
        // "Implementations SHALL NOT use TLS 1.0 or 1.1. These are deprecated."
        // "Implementations SHALL NOT use SSL. Although the SSL protocol has previously,
        // been used to secure HTTP traffic no version of SSL is now considered secure."
        // See https://github.com/AMWA-TV/nmos-secure-communication/blob/v1.0.x/docs/1.0.%20Secure%20Communication.md#tls
        const auto ssl_context_options =
            ( boost::asio::ssl::context::no_sslv2
            | boost::asio::ssl::context::no_sslv3
            | boost::asio::ssl::context::no_tlsv1
// TLS v1.2 support was added in Boost 1.54.0
// but the option to disable TLS v1.1 didn't arrive until Boost 1.58.0
#if BOOST_VERSION >= 105800
            | boost::asio::ssl::context::no_tlsv1_1
#endif
            );

        const auto ssl_cipher_list = "HIGH:!kRSA";
    }
}

#endif

#endif
