#ifndef NMOS_CALLBACKS_H
#define NMOS_CALLBACKS_H

#include <map>

#include "nmos/id.h"
#include "nmos/type.h"
#include "nmos/mutex.h"

namespace nmos
{
    struct callbacks
    {
        virtual void activate(const nmos::id&, const nmos::type&, nmos::write_lock&, nmos::condition_variable&) = 0;
    };
}

#endif
