#ifndef NMOS_SENDER_STATE_H
#define NMOS_SENDER_STATE_H

#include "nmos/string_enum.h"

namespace nmos
{
    // Stream Compatibility Sender states
    // See https://specs.amwa.tv/is-11/branches/v1.0-dev/docs/Behaviour_-_Server_Side.html#state-of-sender
    // and https://specs.amwa.tv/is-11/branches/v1.0-dev/APIs/schemas/with-refs/sender-status.html
    DEFINE_STRING_ENUM(sender_state)
    namespace sender_states
    {
        const sender_state no_signal{ U("no_signal") };
        const sender_state awaiting_signal{ U("awaiting_signal") };
        const sender_state unconstrained{ U("unconstrained") };
        const sender_state constrained{ U("constrained") };
        const sender_state active_constraints_violation{ U("active_constraints_violation") };
    }

    // State of the signal coming into a Sender from its Inputs. It is used to set Sender Status properly.
    DEFINE_STRING_ENUM(signal_state)
    namespace signal_states
    {
        const sender_state no_signal{ U("no_signal") };
        const sender_state awaiting_signal{ U("awaiting_signal") };
        const signal_state signal_is_present{ U("signal_is_present") };
    }

    // Stream Compatibility Receiver states
    // See https://specs.amwa.tv/is-11/branches/v1.0-dev/docs/Behaviour_-_Server_Side.html#state-of-receiver
    // and https://specs.amwa.tv/is-11/branches/v1.0-dev/APIs/schemas/with-refs/receiver-status.html
    DEFINE_STRING_ENUM(receiver_state)
    namespace receiver_states
    {
        const receiver_state unknown{ U("unknown") };
        const receiver_state compliant_stream{ U("compliant_stream") };
        const receiver_state non_compliant_stream{ U("non_compliant_stream") };
    }
}

#endif
