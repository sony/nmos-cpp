#include "node_application_hooks.h"
#include "node_implementation.h"

// The sample implementation entry point. This just calls the node_main_thread function.
// An implementation integrated with an existing app would  
// likely just call the node_main_thread() function directly. This function
// is useful for the sample implementation which otherwise needs a "main" function.
int main(int argc, char* argv[])
{
    return node_main_thread (argc, argv);
}

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
