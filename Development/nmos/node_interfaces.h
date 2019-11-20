#ifndef NMOS_NODE_INTERFACES_H
#define NMOS_NODE_INTERFACES_H

#include <map>
#include <vector>
#include "cpprest/details/basic_types.h"

namespace web
{
    namespace hosts
    {
        namespace experimental
        {
            struct host_interface;
        }
    }

    namespace json
    {
        class value;
    }
}

namespace nmos
{
    struct model;

    struct node_interface
    {
        // Chassis ID may be a MAC address (recommended) or IPv4 or IPv6 address; empty string indicates null
        utility::string_t chassis_id;
        // Port ID must be a MAC address
        utility::string_t port_id;
        // Interface name (recommended to be the local interface_id)
        utility::string_t name;
    };

    // make node interfaces JSON data, for the specified map from local interface_id
    // no attached_network_device details are included
    web::json::value make_node_interfaces(const std::map<utility::string_t, node_interface>& interfaces);

    namespace experimental
    {
        // make a map from local interface_id to the (recommended) node interface details for the specified host interfaces
        std::map<utility::string_t, node_interface> node_interfaces(const std::vector<web::hosts::experimental::host_interface>& host_interfaces);

        // make a map from local interface_id to the (recommended) node interface details
        std::map<utility::string_t, node_interface> node_interfaces();
    }
}

#endif
