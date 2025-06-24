#ifndef NMOS_NODE_SERVER_H
#define NMOS_NODE_SERVER_H

#include "nmos/authorization_handlers.h"
#include "nmos/certificate_handlers.h"
#include "nmos/channelmapping_api.h"
#include "nmos/channelmapping_activation.h"
#include "nmos/connection_api.h"
#include "nmos/connection_activation.h"
#include "nmos/control_protocol_handlers.h"
#include "nmos/node_behaviour.h"
#include "nmos/node_system_behaviour.h"
#include "nmos/ocsp_response_handler.h"
#include "nmos/ws_api_utils.h"

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
            node_implementation(nmos::load_server_certificates_handler load_server_certificates, nmos::load_dh_param_handler load_dh_param, nmos::load_ca_certificates_handler load_ca_certificates, nmos::system_global_handler system_changed, nmos::registration_handler registration_changed, nmos::transport_file_parser parse_transport_file, nmos::details::connection_resource_patch_validator validate_staged, nmos::connection_resource_auto_resolver resolve_auto, nmos::connection_sender_transportfile_setter set_transportfile, nmos::connection_activation_handler connection_activated, nmos::ocsp_response_handler get_ocsp_response, get_authorization_bearer_token_handler get_authorization_bearer_token, validate_authorization_handler validate_authorization, ws_validate_authorization_handler ws_validate_authorization, nmos::load_rsa_private_keys_handler load_rsa_private_keys, load_authorization_clients_handler load_authorization_clients, save_authorization_client_handler save_authorization_client, request_authorization_code_handler request_authorization_code, nmos::get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, nmos::get_control_protocol_datatype_descriptor_handler get_control_protocol_datatype_descriptor, nmos::get_control_protocol_method_descriptor_handler get_control_protocol_method_descriptor, nmos::control_protocol_property_changed_handler control_protocol_property_changed, nmos::control_protocol_connection_activation_handler receiver_monitor_connection_activated, nmos::get_packet_counters_handler get_lost_packet_counters, nmos::get_packet_counters_handler get_late_packet_counters, nmos::reset_monitor_handler reset_monitor)
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
                , get_ocsp_response(std::move(get_ocsp_response))
                , get_authorization_bearer_token(std::move(get_authorization_bearer_token))
                , validate_authorization(std::move(validate_authorization))
                , ws_validate_authorization(std::move(ws_validate_authorization))
                , load_rsa_private_keys(std::move(load_rsa_private_keys))
                , load_authorization_clients(std::move(load_authorization_clients))
                , save_authorization_client(std::move(save_authorization_client))
                , request_authorization_code(std::move(request_authorization_code))
                , get_control_protocol_class_descriptor(std::move(get_control_protocol_class_descriptor))
                , get_control_protocol_datatype_descriptor(std::move(get_control_protocol_datatype_descriptor))
                , get_control_protocol_method_descriptor(std::move(get_control_protocol_method_descriptor))
                , control_protocol_property_changed(std::move(control_protocol_property_changed))
                , receiver_monitor_connection_activated(std::move(receiver_monitor_connection_activated))
                , get_lost_packet_counters(std::move(get_lost_packet_counters))
                , get_late_packet_counters(std::move(get_late_packet_counters))
                , reset_monitor(std::move(reset_monitor))
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
            node_implementation& on_get_ocsp_response(nmos::ocsp_response_handler get_ocsp_response) { this->get_ocsp_response = std::move(get_ocsp_response); return *this; }
            node_implementation& on_get_authorization_bearer_token(get_authorization_bearer_token_handler get_authorization_bearer_token) { this->get_authorization_bearer_token = std::move(get_authorization_bearer_token); return *this; }
            node_implementation& on_validate_authorization(validate_authorization_handler validate_authorization) { this->validate_authorization = std::move(validate_authorization); return *this; }
            node_implementation& on_ws_validate_authorization(ws_validate_authorization_handler ws_validate_authorization) { this->ws_validate_authorization = std::move(ws_validate_authorization); return *this; }
            node_implementation& on_load_rsa_private_keys(nmos::load_rsa_private_keys_handler load_rsa_private_keys) { this->load_rsa_private_keys = std::move(load_rsa_private_keys); return *this; }
            node_implementation& on_load_authorization_clients(load_authorization_clients_handler load_authorization_clients) { this->load_authorization_clients = std::move(load_authorization_clients); return *this; }
            node_implementation& on_save_authorization_client(save_authorization_client_handler save_authorization_client) { this->save_authorization_client = std::move(save_authorization_client); return *this; }
            node_implementation& on_request_authorization_code(request_authorization_code_handler request_authorization_code) { this->request_authorization_code = std::move(request_authorization_code); return *this; }
            node_implementation& on_get_control_class_descriptor(nmos::get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor) { this->get_control_protocol_class_descriptor = std::move(get_control_protocol_class_descriptor); return *this; }
            node_implementation& on_get_control_datatype_descriptor(nmos::get_control_protocol_datatype_descriptor_handler get_control_protocol_datatype_descriptor) { this->get_control_protocol_datatype_descriptor = std::move(get_control_protocol_datatype_descriptor); return *this; }
            node_implementation& on_get_control_protocol_method_descriptor(nmos::get_control_protocol_method_descriptor_handler get_control_protocol_method_descriptor) { this->get_control_protocol_method_descriptor = std::move(get_control_protocol_method_descriptor); return *this; }
            node_implementation& on_control_protocol_property_changed(nmos::control_protocol_property_changed_handler control_protocol_property_changed) { this->control_protocol_property_changed = std::move(control_protocol_property_changed); return *this; }
            node_implementation& on_receiver_monitor_activated(nmos::control_protocol_connection_activation_handler receiver_monitor_connection_activated) { this->receiver_monitor_connection_activated = std::move(receiver_monitor_connection_activated); return *this; }
            node_implementation& on_get_lost_packet_counters(nmos::get_packet_counters_handler get_lost_packet_counters) { this->get_lost_packet_counters = std::move(get_lost_packet_counters); return *this; }
            node_implementation& on_get_late_packet_counters(nmos::get_packet_counters_handler get_late_packet_counters) { this->get_late_packet_counters = std::move(get_late_packet_counters); return *this; }
            node_implementation& on_reset_monitor(nmos::reset_monitor_handler reset_monitor) { this->reset_monitor = std::move(reset_monitor); return *this; }

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

            nmos::ocsp_response_handler get_ocsp_response;

            get_authorization_bearer_token_handler get_authorization_bearer_token;
            validate_authorization_handler validate_authorization;
            ws_validate_authorization_handler ws_validate_authorization;
            nmos::load_rsa_private_keys_handler load_rsa_private_keys;
            load_authorization_clients_handler load_authorization_clients;
            save_authorization_client_handler save_authorization_client;
            request_authorization_code_handler request_authorization_code;

            nmos::get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor;
            nmos::get_control_protocol_datatype_descriptor_handler get_control_protocol_datatype_descriptor;
            nmos::get_control_protocol_method_descriptor_handler get_control_protocol_method_descriptor;
            nmos::control_protocol_property_changed_handler control_protocol_property_changed;
            nmos::control_protocol_connection_activation_handler receiver_monitor_connection_activated;
            nmos::get_packet_counters_handler get_lost_packet_counters;
            nmos::get_packet_counters_handler get_late_packet_counters;
            nmos::reset_monitor_handler reset_monitor;
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
