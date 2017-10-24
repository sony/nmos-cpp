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
    };
}

#endif
