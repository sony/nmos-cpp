#ifndef NMOS_FLOWCOMPATIBILITY_BEHAVIOUR_H
#define NMOS_FLOWCOMPATIBILITY_BEHAVIOUR_H

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
            typedef std::function<nmos::sender_state(const web::json::value& transport_file, const web::json::value& flow, const web::json::value& source, const web::json::array& constraint_sets)> streamcompatibility_sender_validator;
        }

        nmos::sender_state validate_sender_resources(const web::json::value& transport_file, const web::json::value& flow, const web::json::value& source, const web::json::array& constraint_sets);
        void streamcompatibility_behaviour_thread(nmos::node_model& model, details::streamcompatibility_sender_validator validate_sender, slog::base_gate& gate);
    }
}

#endif
