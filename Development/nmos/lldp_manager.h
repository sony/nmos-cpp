#ifndef NMOS_LLDP_MANAGER_H
#define NMOS_LLDP_MANAGER_H

#include <map>
#include "cpprest/details/basic_types.h"

namespace lldp
{
    class lldp_manager;
}

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
        // make an LLDP manager configured to receive LLDP frames on the specified interfaces
        // and update the node interfaces JSON data with attached_network_device details
        // and optionally, transmit LLDP frames for these interfaces
        lldp::lldp_manager make_lldp_manager(nmos::model& model, const std::map<utility::string_t, node_interface>& interfaces, bool transmit, slog::base_gate& gate);
    }
}

#endif
