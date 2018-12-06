#include "node_application_hooks.h"
#include "node_implementation.h"

void nmos::experimental::app_nmos_setup (nmos::node_model& model, const utility::string_t& seed_id, slog::base_gate& gate)
{
    ::node_initial_resources(model, seed_id, gate);
}
// returns true if there is work to do (i.e. break the wait on the condition variable)
bool nmos::experimental::app_check_work()
{
    return false;
}
// returns true if "notify" should be set to wake others up
bool nmos::experimental::app_process_work()
{
    return false;
}
// returns true if "notify" should be set to wake others up
bool nmos::experimental::app_process_activation(const nmos::id& resource_id)
{
    return false;
}
