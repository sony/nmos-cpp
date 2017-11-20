#ifndef NMOS_PROCESS_UTILS_H
#define NMOS_PROCESS_UTILS_H

namespace nmos
{
    namespace details
    {
        // Get the current process ID
        int get_process_id();

        // Wait for a process termination signal
        void wait_term_signal();
    }
}

#endif
