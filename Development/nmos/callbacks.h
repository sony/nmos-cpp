#ifndef NMOS_CALLBACKS_H
#define NMOS_CALLBACKS_H

#include <map>

#include "nmos/id.h"
#include "nmos/type.h"

namespace nmos
{
    struct callbacks
    {
        virtual void activate(const std::string& resourceId, const nmos::type& resourceType) = 0;
    };
}

#endif
