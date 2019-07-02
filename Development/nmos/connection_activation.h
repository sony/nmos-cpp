#ifndef NMOS_CONNECTION_ACTIVATION_H
#define NMOS_CONNECTION_ACTIVATION_H

#include <functional>

namespace web
{
    namespace json
    {
        class value;
    }
}

namespace slog
{
    class base_gate;
}

namespace nmos
{
    struct node_model;
    struct resource;

    // a connection_resource_auto_resolver overwrites every instance of "auto" in the specified transport_params array for the specified (IS-04/IS-05) sender/connection_sender or receiver/connection_receiver
    // it may throw e.g. web::json::json_exception or std::runtime_error, which will prevent activation and for immediate activations cause a 500 Internal Error status code to be returned
    typedef std::function<void(const nmos::resource&, const nmos::resource&, web::json::value&)> connection_resource_auto_resolver;

    // a connection_sender_transportfile_setter updates the specified /transportfile endpoint for the specified (IS-04/IS-05) sender/connection_sender
    // it may throw e.g. web::json::json_exception or std::runtime_error, which will prevent activation and for immediate activations cause a 500 Internal Error status code to be returned
    typedef std::function<void(const nmos::resource&, const nmos::resource&, web::json::value&)> connection_sender_transportfile_setter;

    void connection_activation_thread(nmos::node_model& model, connection_resource_auto_resolver resolve_auto, connection_sender_transportfile_setter set_transportfile, slog::base_gate& gate);
}

#endif
