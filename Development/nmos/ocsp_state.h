#ifndef NMOS_OCSP_STATE_H
#define NMOS_OCSP_STATE_H

#include <vector>
#include "nmos/mutex.h"

namespace nmos
{
    namespace experimental
    {
        struct ocsp_state
        {
            // mutex to be used to protect the members from simultaneous access by multiple threads
            mutable nmos::mutex mutex;

            std::vector<uint8_t> ocsp_response;

            nmos::read_lock read_lock() const { return nmos::read_lock{ mutex }; }
            nmos::write_lock write_lock() const { return nmos::write_lock{ mutex }; }
        };
    }
}

#endif
