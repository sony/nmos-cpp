#ifndef NMOS_NODE_REGISTRATION_H
#define NMOS_NODE_REGISTRATION_H

#include "nmos/mutex.h"
#include "nmos/resources.h"

namespace slog
{
    class base_gate;
}

// Node Registration behaviour
// See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/docs/4.1.%20Behaviour%20-%20Registration.md
namespace nmos
{
    struct model;

    namespace details
    {
        nmos::resource make_node_registration_subscription(const nmos::id& id);

        nmos::resource make_node_registration_grain(const nmos::id& id, const nmos::id& subscription_id);
    }

    void node_registration_thread(nmos::model& model, nmos::mutex& mutex, nmos::condition_variable& condition, bool& shutdown, slog::base_gate& gate);
    void node_registration_heartbeat_thread(const nmos::model& model, nmos::mutex& mutex, nmos::condition_variable& condition, bool& shutdown, slog::base_gate& gate);
}

#endif
