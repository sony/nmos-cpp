#ifndef NMOS_CPP_NODE_NODE_APPLICATION_HOOKS_H
#define NMOS_CPP_NODE_NODE_APPLICATION_HOOKS_H

#include "nmos/id.h"

namespace slog
{
    class base_gate;
}

namespace nmos
{
    struct node_model;
    
    namespace experimental
    {
        void app_nmos_setup (nmos::node_model& model, const utility::string_t& seed_id, slog::base_gate& gate);
        bool app_check_work();
        bool app_process_work();
        bool app_process_activation(const nmos::id& resource_id);
    }
}

#endif
