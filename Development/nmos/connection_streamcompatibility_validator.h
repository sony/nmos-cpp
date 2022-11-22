#ifndef NMOS_CONNECTION_STREAMCOMPATIBILITY_VALIDATOR_H
#define NMOS_CONNECTION_STREAMCOMPATIBILITY_VALIDATOR_H

#include "nmos/connection_api.h"

namespace nmos
{
    struct node_model;

    namespace experimental
    {
        // "At any time if State of an active Sender becomes active_constraints_violation, the Sender MUST become inactive.
        // An inactive Sender in this state MUST NOT allow activations.
        // At any time if State of an active Receiver becomes non_compliant_stream, the Receiver SHOULD become inactive.
        // An inactive Receiver in this state SHOULD NOT allow activations."
        // See https://specs.amwa.tv/is-11/branches/v1.0-dev/docs/Behaviour_-_Server_Side.html#preventing-restrictions-violation
        nmos::details::connection_resource_patch_validator make_connection_streamcompatibility_validator(nmos::node_model& model);
    }
}

#endif
