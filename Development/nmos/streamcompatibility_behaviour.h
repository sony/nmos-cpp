#ifndef NMOS_STREAMCOMPATIBILITY_BEHAVIOUR_H
#define NMOS_STREAMCOMPATIBILITY_BEHAVIOUR_H

#include <functional>

#include "nmos/streamcompatibility_state.h"

namespace slog
{
    class base_gate;
}

namespace web
{
    namespace json
    {
        class value;
        class array;
    }
}

namespace nmos
{
    struct node_model;

    namespace experimental
    {
        namespace details
        {
            typedef std::function<std::pair<nmos::sender_state, utility::string_t>(const web::json::value& transport_file, const web::json::value& sender, const web::json::value& flow, const web::json::value& source, const web::json::array& constraint_sets)> streamcompatibility_sender_validator;
            typedef std::function<std::pair<nmos::receiver_state, utility::string_t>(const web::json::value& transport_file, const web::json::value& receiver)> streamcompatibility_receiver_validator;
        }

        std::pair<nmos::sender_state, utility::string_t> validate_sender_resources(const web::json::value& transport_file, const web::json::value& sender, const web::json::value& flow, const web::json::value& source, const web::json::array& constraint_sets);
        std::pair<nmos::receiver_state, utility::string_t> validate_receiver_resources(const web::json::value& transport_file, const web::json::value& receiver);
        void streamcompatibility_behaviour_thread(nmos::node_model& model, details::streamcompatibility_sender_validator validate_sender, details::streamcompatibility_receiver_validator validate_receiver, slog::base_gate& gate);
    }
}

#endif
