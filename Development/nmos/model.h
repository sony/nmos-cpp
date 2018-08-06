#ifndef NMOS_MODEL_H
#define NMOS_MODEL_H

#include "nmos/resources.h"
#include "nmos/settings.h"

// This is the model of an NMOS node or nodes, i.e. a registry.
namespace nmos
{
    struct model
    {
        nmos::resources resources;
        nmos::settings settings;
        nmos::resources active;
        nmos::resources constraints;
        nmos::resources staged;

        // These are resources that have been modified by an IS-05
        // PATCH request but have not yet been activated. It is
        // separate from staged (above) because of the requirement
        // that an updated transport parameter should override an
        // updated SDP entry, but the SDP entry should take priority
        // if the transport paramateres have not been changed.
        nmos::resources patched;

        struct
        {
            std::map<nmos::id, std::string> redirects;
            // Consider adding more here, like the actual SDP string.
        } transportfile;
    };
}

#endif
