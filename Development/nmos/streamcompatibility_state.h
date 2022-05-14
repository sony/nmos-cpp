#ifndef NMOS_SENDER_STATE_H
#define NMOS_SENDER_STATE_H

#include "nmos/string_enum.h"

namespace nmos
{
    // Flow Compatibility Sender states
    // See https://specs.amwa.tv/is-11/branches/v1.0-dev/docs/Behaviour_-_Server_Side.html#state-of-sender
    // and https://specs.amwa.tv/is-11/branches/v1.0-dev/APIs/schemas/with-refs/sender-status.html
    DEFINE_STRING_ENUM(sender_state)
    namespace sender_states
    {
        const sender_state no_signal{ U("No Signal") };
        const sender_state awaiting_signal{ U("Awaiting Signal") };
        const sender_state unconstrained{ U("Unconstrained") };
        const sender_state constrained{ U("Constrained") };
        const sender_state active_constraints_violation{ U("Active Constraints Violation") };
    }

    // State of the signal coming into a Sender from its Inputs. It is used to set Sender Status properly.
    DEFINE_STRING_ENUM(signal_state)
    namespace signal_states
    {
        const signal_state no_signal{ U("No Signal") };
        const signal_state awaiting_signal{ U("Awaiting Signal") };
        const signal_state signal_is_present{ U("Signal is Present") };
    }

    // Flow Compatibility Receiver states
    // See https://specs.amwa.tv/is-11/branches/v1.0-dev/docs/Behaviour_-_Server_Side.html#state-of-receiver
    // and https://specs.amwa.tv/is-11/branches/v1.0-dev/APIs/schemas/with-refs/receiver-status.html
    DEFINE_STRING_ENUM(receiver_state)
    namespace receiver_states
    {
        const receiver_state no_transport_file{ U("No Transport File") };
        const receiver_state ok{ U("OK") };
        const receiver_state receiver_capabilities_violation{ U("Receiver Capabilities Violation") };
    }
}

#endif
