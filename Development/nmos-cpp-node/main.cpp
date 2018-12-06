
#include "node_implementation.h"

// The sample implementation entry point. This just calls the node_main_thread function.
// An implementation integrated with an existing app would not include this file and would 
// likely just call the node_main_thread() function directly.
int main(int argc, char* argv[])
{
    return node_main_thread (argc, argv);
}
