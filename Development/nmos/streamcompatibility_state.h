#ifndef NMOS_SENDER_STATE_H
#define NMOS_SENDER_STATE_H

#include "nmos/string_enum.h"

namespace nmos
{
    // Stream Compatibility Input states
    // See https://specs.amwa.tv/is-11/branches/v1.0-dev/docs/Behaviour_-_Server_Side.html#status-of-input
    // and https://specs.amwa.tv/is-11/branches/v1.0-dev/APIs/schemas/with-refs/input.html
    DEFINE_STRING_ENUM(input_state)
    namespace input_states
    {
        const input_state no_signal{ U("no_signal") };
        const input_state awaiting_signal{ U("awaiting_signal") };
        const input_state signal_present{ U("signal_present") };
    }

    // Stream Compatibility Output states
    // See https://specs.amwa.tv/is-11/branches/v1.0-dev/docs/Behaviour_-_Server_Side.html#status-of-output
    // and https://specs.amwa.tv/is-11/branches/v1.0-dev/APIs/schemas/with-refs/output.html
    DEFINE_STRING_ENUM(output_state)
    namespace output_states
    {
        const output_state no_signal{ U("no_signal") };
        const output_state signal_present{ U("signal_present") };
    }

    // Stream Compatibility Sender states
    // See https://specs.amwa.tv/is-11/branches/v1.0-dev/docs/Behaviour_-_Server_Side.html#status-of-sender
    // and https://specs.amwa.tv/is-11/branches/v1.0-dev/APIs/schemas/with-refs/sender-status.html
    DEFINE_STRING_ENUM(sender_state)
    namespace sender_states
    {
        const sender_state no_essence{ U("no_essence") };
        const sender_state awaiting_essence{ U("awaiting_essence") };
        const sender_state unconstrained{ U("unconstrained") };
        const sender_state constrained{ U("constrained") };
        const sender_state active_constraints_violation{ U("active_constraints_violation") };
    }

    // Stream Compatibility Receiver states
    // See https://specs.amwa.tv/is-11/branches/v1.0-dev/docs/Behaviour_-_Server_Side.html#status-of-receiver
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
