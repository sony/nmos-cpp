#include "nmos/control_protocol_state.h"

#include "nmos/control_protocol_resource.h"

namespace nmos
{
    namespace experimental
    {
        control_protocol_state::control_protocol_state()
        {
            // setup the core control classes (properties/methods/events)
            control_classes =
            {
                { details::make_nc_class_id(details::nc_object_class_id), { details::make_nc_object_properties(), details::make_nc_object_methods(), details::make_nc_object_events() } },
                { details::make_nc_class_id(details::nc_block_class_id), { details::make_nc_block_properties(), details::make_nc_block_methods(), details::make_nc_block_events() } },
                { details::make_nc_class_id(details::nc_worker_class_id), { details::make_nc_worker_properties(), details::make_nc_worker_methods(), details::make_nc_worker_events() } },
                { details::make_nc_class_id(details::nc_manager_class_id), { details::make_nc_manager_properties(), details::make_nc_manager_methods(), details::make_nc_manager_events() } },
                { details::make_nc_class_id(details::nc_device_manager_class_id), { details::make_nc_device_manager_properties(), details::make_nc_device_manager_methods(), details::make_nc_device_manager_events() } },
                { details::make_nc_class_id(details::nc_class_manager_class_id), { details::make_nc_class_manager_properties(), details::make_nc_class_manager_methods(), details::make_nc_class_manager_events() } }
            };
        }
    }
}