#ifndef NMOS_CONFIGURATION_API_H
#define NMOS_CONFIGURATION_API_H

#include "cpprest/api_router.h"
#include "nmos/control_protocol_handlers.h"
#include "nmos/configuration_handlers.h"

namespace slog
{
    class base_gate;
}

// Configuration API implementation
// See https://specs.amwa.tv/is-device-configuration/branches/publish-CR/docs/API_requests.html
namespace nmos
{
    struct node_model;

    web::http::experimental::listener::api_router make_configuration_api(nmos::node_model& model, web::http::experimental::listener::route_handler validate_authorization, get_control_protocol_class_descriptor_handler get_control_protocol_class_descriptor, get_control_protocol_datatype_descriptor_handler get_control_protocol_datatype_descriptor, get_control_protocol_method_descriptor_handler get_control_protocol_method_descriptor, create_validation_fingerprint_handler create_validation_fingerprint, validate_validation_fingerprint_handler validate_validation_fingerprint, get_read_only_modification_allow_list_handler get_read_only_modification_allow_list, nmos::remove_device_model_object_handler remove_device_model_object, nmos::create_device_model_object_handler create_device_model_object, control_protocol_property_changed_handler property_changed, slog::base_gate& gate);
}

#endif
