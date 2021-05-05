#ifndef NMOS_NODE_SERVER_H
#define NMOS_NODE_SERVER_H

#include "nmos/certificate_handlers.h"
#include "nmos/channelmapping_api.h"
#include "nmos/channelmapping_activation.h"
#include "nmos/connection_api.h"
#include "nmos/connection_activation.h"
#include "nmos/node_behaviour.h"
#include "nmos/node_system_behaviour.h"

namespace nmos
{
    struct node_model;

    struct server;

    namespace experimental
    {
        struct log_model;

        // a type to simplify passing around the application callbacks necessary to integrate the device-specific
        // underlying implementation into the server instance for the NMOS Node
        struct node_implementation
        {
            node_implementation(nmos::load_server_certificates_handler load_server_certificates, nmos::load_dh_param_handler load_dh_param, nmos::load_ca_certificates_handler load_ca_certificates, nmos::system_global_handler system_changed, nmos::registration_handler registration_changed, nmos::transport_file_parser parse_transport_file, nmos::details::connection_resource_patch_validator validate_merged, nmos::connection_resource_auto_resolver resolve_auto, nmos::connection_sender_transportfile_setter set_transportfile, nmos::connection_activation_handler connection_activated)
                : load_server_certificates(std::move(load_server_certificates))
                , load_dh_param(std::move(load_dh_param))
                , load_ca_certificates(std::move(load_ca_certificates))
                , system_changed(std::move(system_changed))
                , registration_changed(std::move(registration_changed))
                , parse_transport_file(std::move(parse_transport_file))
                , validate_staged(std::move(validate_staged))
                , resolve_auto(std::move(resolve_auto))
                , set_transportfile(std::move(set_transportfile))
                , connection_activated(std::move(connection_activated))
            {}

            // use the default constructor and chaining member functions for fluent initialization
            // (by itself, the default constructor does not construct a valid instance)
            node_implementation()
                : parse_transport_file(&nmos::parse_rtp_transport_file)
            {}

            node_implementation& on_load_server_certificates(nmos::load_server_certificates_handler load_server_certificates) { this->load_server_certificates = std::move(load_server_certificates); return *this; }
            node_implementation& on_load_dh_param(nmos::load_dh_param_handler load_dh_param) { this->load_dh_param = std::move(load_dh_param); return *this; }
            node_implementation& on_load_ca_certificates(nmos::load_ca_certificates_handler load_ca_certificates) { this->load_ca_certificates = std::move(load_ca_certificates); return *this; }
            node_implementation& on_system_changed(nmos::system_global_handler system_changed) { this->system_changed = std::move(system_changed); return *this; }
            node_implementation& on_registration_changed(nmos::registration_handler registration_changed) { this->registration_changed = std::move(registration_changed); return *this; }
            node_implementation& on_parse_transport_file(nmos::transport_file_parser parse_transport_file) { this->parse_transport_file = std::move(parse_transport_file); return *this; }
            node_implementation& on_validate_connection_resource_patch(nmos::details::connection_resource_patch_validator validate_staged) { this->validate_staged = std::move(validate_staged); return *this; }
            node_implementation& on_resolve_auto(nmos::connection_resource_auto_resolver resolve_auto) { this->resolve_auto = std::move(resolve_auto); return *this; }
            node_implementation& on_set_transportfile(nmos::connection_sender_transportfile_setter set_transportfile) { this->set_transportfile = std::move(set_transportfile); return *this; }
            node_implementation& on_connection_activated(nmos::connection_activation_handler connection_activated) { this->connection_activated = std::move(connection_activated); return *this; }
            node_implementation& on_validate_channelmapping_output_map(nmos::details::channelmapping_output_map_validator validate_map) { this->validate_map = std::move(validate_map); return *this; }
            node_implementation& on_channelmapping_activated(nmos::channelmapping_activation_handler channelmapping_activated) { this->channelmapping_activated = std::move(channelmapping_activated); return *this; }

            // deprecated, use on_validate_connection_resource_patch
            node_implementation& on_validate_merged(nmos::details::connection_resource_patch_validator validate_merged) { return on_validate_connection_resource_patch(std::move(validate_merged)); }

            // determine if the required callbacks have been specified
            bool valid() const
            {
                return parse_transport_file && resolve_auto && set_transportfile && connection_activated;
            }

            nmos::load_server_certificates_handler load_server_certificates;
            nmos::load_dh_param_handler load_dh_param;
            nmos::load_ca_certificates_handler load_ca_certificates;

            nmos::system_global_handler system_changed;
            nmos::registration_handler registration_changed;

            nmos::transport_file_parser parse_transport_file;
            nmos::details::connection_resource_patch_validator validate_staged;
            nmos::connection_resource_auto_resolver resolve_auto;
            nmos::connection_sender_transportfile_setter set_transportfile;

            nmos::connection_activation_handler connection_activated;

            nmos::details::channelmapping_output_map_validator validate_map;

            nmos::channelmapping_activation_handler channelmapping_activated;
        };

        // Construct a server instance for an NMOS Node, implementing the IS-04 Node API, IS-05 Connection API, IS-07 Events API
        // and the experimental Logging API and Settings API, according to the specified data models and callbacks
        nmos::server make_node_server(nmos::node_model& node_model, nmos::experimental::node_implementation node_implementation, nmos::experimental::log_model& log_model, slog::base_gate& gate);

        // deprecated
        nmos::server make_node_server(nmos::node_model& node_model, nmos::transport_file_parser parse_transport_file, nmos::details::connection_resource_patch_validator validate_merged, nmos::connection_resource_auto_resolver resolve_auto, nmos::connection_sender_transportfile_setter set_transportfile, nmos::connection_activation_handler connection_activated, nmos::experimental::log_model& log_model, slog::base_gate& gate);
        nmos::server make_node_server(nmos::node_model& node_model, nmos::transport_file_parser parse_transport_file, nmos::connection_resource_auto_resolver resolve_auto, nmos::connection_sender_transportfile_setter set_transportfile, nmos::connection_activation_handler connection_activated, nmos::experimental::log_model& log_model, slog::base_gate& gate);
        nmos::server make_node_server(nmos::node_model& node_model, nmos::connection_resource_auto_resolver resolve_auto, nmos::connection_sender_transportfile_setter set_transportfile, nmos::connection_activation_handler connection_activated, nmos::experimental::log_model& log_model, slog::base_gate& gate);
    }
}

#endif
