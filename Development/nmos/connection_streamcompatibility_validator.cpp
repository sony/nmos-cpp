#include "nmos/connection_streamcompatibility_validator.h"

#include "nmos/json_fields.h"
#include "nmos/model.h"
#include "nmos/resource.h"
#include "nmos/resources.h"
#include "nmos/streamcompatibility_state.h"

namespace nmos
{
    namespace experimental
    {
        // "At any time if State of an active Sender becomes active_constraints_violation, the Sender MUST become inactive.
        // An inactive Sender in this state MUST NOT allow activations.
        // At any time if State of an active Receiver becomes non_compliant_stream, the Receiver SHOULD become inactive.
        // An inactive Receiver in this state SHOULD NOT allow activations."
        // See https://specs.amwa.tv/is-11/branches/v1.0-dev/docs/Behaviour_-_Server_Side.html#preventing-restrictions-violation
        nmos::details::connection_resource_patch_validator make_connection_streamcompatibility_validator(nmos::node_model& model)
        {
            return [&model] (const nmos::resource& resource, const nmos::resource& connection_resource, const web::json::value& endpoint_staged, slog::base_gate& gate)
            {
                if (nmos::fields::master_enable(endpoint_staged))
                {
                    const auto id = nmos::fields::id(resource.data);
                    const auto type = resource.type;

                    auto streamcompatibility_resource = find_resource(model.streamcompatibility_resources, {id, type});
                    if (model.streamcompatibility_resources.end() != streamcompatibility_resource)
                    {
                        auto resource_state = nmos::fields::state(nmos::fields::status(streamcompatibility_resource->data));

                        if (resource_state == nmos::sender_states::active_constraints_violation.name || resource_state == nmos::receiver_states::non_compliant_stream.name)
                        {
                            throw std::logic_error("activation failed due to Status of the connector");
                        }
                    }
                }
            };
        }
    }
}
