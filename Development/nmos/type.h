#ifndef NMOS_TYPE_H
#define NMOS_TYPE_H

#include <vector>
#include "nmos/string_enum.h"

namespace nmos
{
    // Resources have a type
    DEFINE_STRING_ENUM(type)
    namespace types
    {
        const type node{ U("node") };
        const type device{ U("device") };
        const type source{ U("source") };
        const type flow{ U("flow") };
        const type sender{ U("sender") };
        const type receiver{ U("receiver") };

        // a subscription isn't strictly a resource but has many of the same behaviours (it is
        // exposed from the Query API in the same way), and can largely be managed identically
        const type subscription{ U("subscription") };

        // similarly, the information about the next grain for each specific websocket connection
        // to a subscription is managed as a sub-resource of the subscription
        const type grain{ U("grain") };

        // all types ordered so that sub-resource types appear after super-resource types
        const std::vector<type> all{ nmos::types::node, nmos::types::device, nmos::types::source, nmos::types::flow, nmos::types::sender, nmos::types::receiver, nmos::types::subscription, nmos::types::grain };
    }
}

#endif
