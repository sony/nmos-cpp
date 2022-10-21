#include "nmos/ocsp_response_handler.h"

#include "cpprest/basic_utils.h"
#include "nmos/ocsp_state.h"
#include "nmos/slog.h"

namespace nmos
{
    ocsp_response_handler make_ocsp_response_handler(nmos::experimental::ocsp_state& ocsp_state, slog::base_gate& gate)
    {
        return [&]()
        {
            slog::log<slog::severities::info>(gate, SLOG_FLF) << "Retrieve OCSP response from cache";

            auto lock = ocsp_state.read_lock();
            return ocsp_state.ocsp_response;
        };
    }
}
