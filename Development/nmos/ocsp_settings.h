#ifndef NMOS_OCSP_SETTINGS_H
#define NMOS_OCSP_SETTINGS_H

#include <vector>
#include "nmos/mutex.h"

namespace nmos
{
    namespace experimental
    {
        struct ocsp_settings
        {
            // mutex to be used to protect the members of the settings from simultaneous access by multiple threads
            std::shared_ptr<nmos::mutex> mutex;

            std::vector<uint8_t> ocsp_response;

            nmos::read_lock read_lock() const { return nmos::read_lock{ *mutex }; }
            nmos::write_lock write_lock() const { return nmos::write_lock{ *mutex }; }

            ocsp_settings()
                : mutex(std::make_shared<nmos::mutex>())
            {}

            ocsp_settings(const ocsp_settings& other)
            {
                mutex = other.mutex;
                ocsp_response = other.ocsp_response;
            }
        };
    }
}

#endif
