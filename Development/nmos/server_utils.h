#ifndef NMOS_SERVER_UTILS_H
#define NMOS_SERVER_UTILS_H

#include "nmos/settings.h"

// Utility types, constants and functions for implementing NMOS REST API servers
namespace nmos
{
    namespace experimental
    {
        // map the configured client port to the server port on which to listen
        int server_port(int client_port, const nmos::settings& settings);
    }
}

#endif
