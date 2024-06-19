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
        // see https://specs.amwa.tv/is-04/releases/v1.2.1/docs/4.1._Behaviour_-_Registration.html#referential-integrity
        const std::vector<type> all{ nmos::types::node, nmos::types::device, nmos::types::source, nmos::types::flow, nmos::types::sender, nmos::types::receiver, nmos::types::subscription, nmos::types::grain };

        // the Channel Mapping API resource types, see nmos/channelmapping_resources.h
        const type input{ U("input") };
        const type output{ U("output") };

        // the System API global configuration resource type, see nmos/system_resources.h
        const type global{ U("global") };

        // the Control Protocol API resource type, see nmos/control_protcol_resources.h
        const type nc_block{ U("nc_block") };
        const type nc_worker{ U("nc_worker") };
        const type nc_manager{ U("nc_manager") };
        const type nc_device_manager{ U("nc_device_manager") };
        const type nc_class_manager{ U("nc_class_manager") };
        const type nc_receiver_monitor{ U("nc_receiver_monitor") };
        const type nc_receiver_monitor_protected{ U("nc_receiver_monitor_protected") };
        const type nc_ident_beacon{ U("nc_ident_beacon") };
        const type nc_bulk_properties_manager{ U("nc_bulk_properties_manager") };
    }
}

#endif
