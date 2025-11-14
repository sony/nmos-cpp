#include "nmos/id.h"
#include "nmos/resources.h"
#include "nmos/streamcompatibility_state.h"

namespace nmos
{
    namespace experimental
    {
        // Update version of the given resource Id
        bool update_version(nmos::resources& resources, const nmos::id& resource_id, const utility::string_t& new_version);

        // Update version of all the given resource Ids
        bool update_version(nmos::resources& resources, const web::json::array& resource_ids, const utility::string_t& new_version);

        // Set sender state
        // `When State of Sender is changed, then the version attribute of the relevant IS-04 Sender MUST be incremented.`
        // See https://specs.amwa.tv/is-11/branches/v1.0.x/docs/Interoperability.html#version-increments
        bool set_sender_status(resources& streamcompatibility_resources, resources& node_resources, const nmos::id& sender_id, const nmos::sender_state& state, slog::base_gate& gate);

        // Set receiver state
        // `When State of Receiver is changed, then the version attribute of the relevant IS-04 Receiver MUST be incremented.`
        // See https://specs.amwa.tv/is-11/branches/v1.0.x/docs/Interoperability.html#version-increments
        bool set_receiver_status(resources& streamcompatibility_resources, resources& node_resources, const nmos::id& receiver_id, const nmos::receiver_state& state, slog::base_gate& gate);

        // Set sender inputs
        // `When the set of Inputs associated with the Sender is changed, then the version attribute of the relevant IS-04 Sender MUST be incremented.`
        // See https://specs.amwa.tv/is-11/branches/v1.0.x/docs/Interoperability.html#version-increments
        bool set_sender_inputs(resources& streamcompatibility_resources, resources& node_resources, const nmos::id& sender_id, const std::vector<nmos::id>& input_ids, slog::base_gate& gate);

        // Set receiver outputs
        // `When the set of Outputs associated with the Receiver is changed, then the version attribute of the relevant IS-04 Receiver MUST be incremented.`
        // See https://specs.amwa.tv/is-11/branches/v1.0.x/docs/Interoperability.html#version-increments
        bool set_receiver_outputs(resources& streamcompatibility_resources, resources& node_resources, const nmos::id& receiver_id, const std::vector<nmos::id>& output_ids, slog::base_gate& gate);
/*
        // Set input properties
        // `When properties of any Input are changed, then the version attribute of the relevant IS-04 Device MUST be incremented. Inputs identify the corresponding Devices via the device_id property.`
        // See https://specs.amwa.tv/is-11/branches/v1.0.x/docs/Interoperability.html#version-increments
        bool set_input_properties(resources& streamcompatibility_resources, resources& node_resources, const nmos::id& input_id, slog::base_gate& gate);

        // Set output properties
        // `When properties of any Output are changed, then the version attribute of the relevant IS-04 Device MUST be incremented. Outputs identify the corresponding Devices via the device_id property.`
        // See https://specs.amwa.tv/is-11/branches/v1.0.x/docs/Interoperability.html#version-increments
        bool set_output_properties(resources& streamcompatibility_resources, resources& node_resources, const nmos::id& output_id, slog::base_gate& gate);

        // Set sender active constraints
        // `When Active Constraints of Sender are modified, then the version attribute of the relevant IS-04 Sender MUST be incremented.`
        // See https://specs.amwa.tv/is-11/branches/v1.0.x/docs/Interoperability.html#version-increments
        bool set_sender_active_constraints(resources& streamcompatibility_resources, resources& node_resources, const nmos::id& sender_id, slog::base_gate& gate);
*/
    }
}
