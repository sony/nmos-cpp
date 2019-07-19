#ifndef NMOS_CPP_NODE_NODE_IMPLEMENTATION_H
#define NMOS_CPP_NODE_NODE_IMPLEMENTATION_H

#include "nmos/connection_activation.h"
#include "nmos/resources.h" // just a forward declaration of nmos::resources
#include "nmos/settings.h" // just a forward declaration of nmos::settings
#include "nmos/connection_api.h"

namespace slog
{
    class base_gate;
}

namespace nmos
{
    struct node_model;
}

// When NMOS_IS_MAIN is defined as 1, then main.cpp will define the "main" function
// as main. This is good for the example.
// When integrating into an application which has its own main() and is able to 
// use all the initialization in the nmos "main" function as is, then define
// NMOS_IS_MAIN=0 to change the function name to "nmos_main" instead.
#if !defined(NMOS_IS_MAIN)
#define NMOS_IS_MAIN 1
#endif
#if (NMOS_IS_MAIN==0)
int nmos_main(int argc, char* argv[]);
#endif

// This is an example of how to integrate the nmos-cpp library with a device-specific underlying implementation.
// It constructs and inserts a node resource and some sub-resources into the model, based on the model settings,
// starts background tasks to emit regular events from the temperature event source and then waits for shutdown.
void node_implementation_thread(nmos::node_model& model, slog::base_gate& gate);

// Example transport file parser
// The parse_transport_file hook allows the sender/receiver to reject PATCH /staged requests 
// with either 400 Bad Request or 500 Internal Error and a "debug" message in the response body 
// from the exception.
//
// This must always return a callable function. {} is not allowed. The default function is
// nmos::parse_rtp_transport_file.
nmos::transport_file_parser make_node_implementation_transport_file_parser();

// Example patch validator
// the validate_merged hook allows the sender/receiver to reject PATCH /staged requests 
// with either 400 Bad Request or 500 Internal Error and a "debug" message in the response body
// from the exception.
nmos::details::connection_resource_patch_validator make_node_implementation_patch_validator();

// Example connection activation auto_resolver callback
nmos::connection_resource_auto_resolver make_node_implementation_auto_resolver(const nmos::settings& settings);

// Example connection activation transportfile_setter callback - captures node_resources by reference!
nmos::connection_sender_transportfile_setter make_node_implementation_transportfile_setter(const nmos::resources& node_resources, const nmos::settings& settings);

// Example connection activation handler callback
// This callback lets the application perform application specific actions to complete an
// activated connection.
nmos::connection_activation_handler make_node_implementation_connection_activated();

#endif
