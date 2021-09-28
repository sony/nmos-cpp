#ifndef NMOS_IS05_SCHEMAS_H
#define NMOS_IS05_SCHEMAS_H

// Extern declarations for auto-generated constants
// could be auto-generated, but isn't currently!
namespace nmos
{
    namespace is05_schemas
    {
        namespace v1_1_x
        {
            extern const char* activation_schema;
            extern const char* sender_stage_schema;
            extern const char* sender_transport_params;
            extern const char* sender_transport_params_rtp;
            extern const char* sender_transport_params_dash;
            extern const char* sender_transport_params_websocket;
            extern const char* sender_transport_params_mqtt;
            extern const char* sender_transport_params_ext;
            extern const char* receiver_stage_schema;
            extern const char* receiver_transport_file;
            extern const char* receiver_transport_params;
            extern const char* receiver_transport_params_rtp;
            extern const char* receiver_transport_params_dash;
            extern const char* receiver_transport_params_websocket;
            extern const char* receiver_transport_params_mqtt;
            extern const char* receiver_transport_params_ext;
        }

        namespace v1_0_x
        {
            extern const char* v1_0_activation_schema;
            extern const char* v1_0_sender_stage_schema;
            extern const char* v1_0_sender_transport_params_rtp;
            extern const char* v1_0_sender_transport_params_dash;
            extern const char* v1_0_receiver_stage_schema;
            extern const char* v1_0_receiver_transport_params_rtp;
            extern const char* v1_0_receiver_transport_params_dash;
        }
    }
}

#endif
