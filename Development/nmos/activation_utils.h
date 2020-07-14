#ifndef NMOS_ACTIVATION_UTILS_H
#define NMOS_ACTIVATION_UTILS_H

#include "nmos/id.h"
#include "nmos/mutex.h" // forward declarations of nmos::read_lock, nmos::write_lock
#include "nmos/type.h"

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
    struct tai;

    // Construct a 'not pending' activation response object with all null values
    web::json::value make_activation();

    // Helper functions extracted from the Connection API implementation that can also be applied to the Channel Mapping API
    namespace details
    {
        enum activation_state { immediate_activation_pending, scheduled_activation_pending, activation_not_pending, staging_only };

        // Discover which kind of activation this is, or whether it is only a request for staging
        activation_state get_activation_state(const web::json::value& activation);

        // Calculate the absolute TAI from the requested time of a scheduled activation
        nmos::tai get_absolute_requested_time(const web::json::value& activation, const nmos::tai& request_time);

        // Set the appropriate fields of the response/staged activation from the specified request
        void merge_activation(web::json::value& activation, const web::json::value& request_activation, const nmos::tai& request_time);

        bool wait_immediate_activation_not_pending(nmos::node_model& model, nmos::read_lock& lock, const std::pair<nmos::id, nmos::type>& id_type);
        bool wait_immediate_activation_not_pending(nmos::node_model& model, nmos::write_lock& lock, const std::pair<nmos::id, nmos::type>& id_type);

        void handle_immediate_activation_pending(nmos::node_model& model, nmos::write_lock& lock, const std::pair<nmos::id, nmos::type>& id_type, web::json::value& response_activation, slog::base_gate& gate);
    }
}

#endif
