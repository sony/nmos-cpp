#include <iostream>
#include <thread>
#include "nmos/id.h"
#include "nmos/log_gate.h"
#include "nmos/model.h"
#include "nmos/node_resource.h"
#include "nmos/node_server.h"
#include "nmos/server.h"

int main()
{
    nmos::node_model node_model;
    nmos::experimental::log_model log_model;
    nmos::experimental::log_gate gate(std::cerr, std::cout, log_model);
    nmos::experimental::node_implementation node_implementation;
    auto node_server = nmos::experimental::make_node_server(node_model, node_implementation, log_model, gate);
    nmos::insert_resource(node_model.node_resources, nmos::make_node(nmos::make_id(), node_model.settings));

    nmos::server_guard node_server_guard(node_server);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    return 0;
}
