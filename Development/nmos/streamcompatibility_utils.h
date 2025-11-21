#include "nmos/id.h"
#include "nmos/resources.h"
#include "nmos/streamcompatibility_state.h"

namespace nmos
{
    namespace experimental
    {
        // Update version of the given resource Id
        void update_version(nmos::resources& resources, const nmos::id& resource_id, const utility::string_t& version);

        // Update version of all the given resource Ids
        void update_version(nmos::resources& resources, const std::set<nmos::id>& resource_ids, const utility::string_t& version);

        // Set sender status
        // `When State of Sender is changed, then the version attribute of the relevant IS-04 Sender MUST be incremented.`
        // See https://specs.amwa.tv/is-11/branches/v1.0.x/docs/Interoperability.html#version-increments
        bool set_sender_status(resources& node_resources, resources& streamcompatibility_resources, const nmos::id& sender_id, const nmos::sender_state& state, const utility::string_t& state_debug, slog::base_gate& gate);

        // Set receiver status
        // `When State of Receiver is changed, then the version attribute of the relevant IS-04 Receiver MUST be incremented.`
        // See https://specs.amwa.tv/is-11/branches/v1.0.x/docs/Interoperability.html#version-increments
        bool set_receiver_status(resources& node_resources, resources& streamcompatibility_resources, const nmos::id& receiver_id, const nmos::receiver_state& state, const utility::string_t& state_debug, slog::base_gate& gate);

        // Set sender inputs
        // `When the set of Inputs associated with the Sender is changed, then the version attribute of the relevant IS-04 Sender MUST be incremented.`
        // See https://specs.amwa.tv/is-11/branches/v1.0.x/docs/Interoperability.html#version-increments
        bool set_sender_inputs(resources& node_resources, resources& streamcompatibility_resources, const nmos::id& sender_id, const std::vector<nmos::id>& input_ids, slog::base_gate& gate);

        // Set receiver outputs
        // `When the set of Outputs associated with the Receiver is changed, then the version attribute of the relevant IS-04 Receiver MUST be incremented.`
        // See https://specs.amwa.tv/is-11/branches/v1.0.x/docs/Interoperability.html#version-increments
        bool set_receiver_outputs(resources& node_resources, resources& streamcompatibility_resources, const nmos::id& receiver_id, const std::vector<nmos::id>& output_ids, slog::base_gate& gate);

        // hmm, add other helper functions, such as set_input_properties and set_output_properties in future
    }
}
