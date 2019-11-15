#ifndef NMOS_LLDP_HANDLER_H
#define NMOS_LLDP_HANDLER_H

#include <map>
#include "cpprest/details/basic_types.h"
#include "lldp/lldp.h" // forward declaration of lldp::lldp_handler

namespace slog
{
    class base_gate;
}

namespace nmos
{
    struct model;

    struct node_interface;

    namespace experimental
    {
        // make an LLDP handler to update the node interfaces JSON data with attached_network_device details
        lldp::lldp_handler make_lldp_handler(nmos::model& model, const std::map<utility::string_t, node_interface>& interfaces, slog::base_gate& gate);
    }
}

#endif
