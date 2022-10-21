#ifndef NMOS_OCSP_RESPONSE_HANDLER_H
#define NMOS_OCSP_RESPONSE_HANDLER_H

#include <functional>
#include <vector>

namespace slog
{
    class base_gate;
}

namespace nmos
{
    namespace experimental
    {
        struct ocsp_state;
    }

    // callback to return OCSP response in DER format
    // this callback is executed for each new TLS connection
    // this callback should not throw exceptions
    typedef std::function<std::vector<uint8_t>()> ocsp_response_handler;

    // construct callback to receive OCSP response
    ocsp_response_handler make_ocsp_response_handler(nmos::experimental::ocsp_state& ocsp_state, slog::base_gate& gate);
}

#endif
