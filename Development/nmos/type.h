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
        // the Node API and Query API resource types, see nmos/resources.h
        // sender and receiver are also used in the Connection API, see nmos/connection_resources.h
        // source is also used in the Events API, see nmos/events_resources.h
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
        // according to the guidelines on referential integrity
        // see https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2.1/docs/4.1.%20Behaviour%20-%20Registration.md#referential-integrity
        const std::vector<type> all{ nmos::types::node, nmos::types::device, nmos::types::source, nmos::types::flow, nmos::types::sender, nmos::types::receiver, nmos::types::subscription, nmos::types::grain };

        // the Channel Mapping API resource types, see nmos/channelmapping_resources.h
        const type input{ U("input") };
        const type output{ U("output") };

        // the System API global configuration resource type, see nmos/system_resources.h
        const type global{ U("global") };
    }
}

#endif
