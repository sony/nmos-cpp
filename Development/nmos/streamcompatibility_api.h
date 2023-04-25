#ifndef NMOS_STREAMCOMPATIBILITY_API_H
#define NMOS_STREAMCOMPATIBILITY_API_H

#include <boost/variant.hpp>
#include "bst/optional.h"
#include "nmos/api_utils.h"
#include "nmos/slog.h"

// Stream Compatibility Management API implementation
// See https://specs.amwa.tv/is-11/branches/v1.0-dev/APIs/StreamCompatibilityManagementAPI.html
namespace nmos
{
    struct node_model;
    struct resource;

    namespace experimental
    {
        namespace details
        {
            // a streamcompatibility_base_edid_handler is a notification that the Base EDID for the specified IS-11 input has changed (PUT or DELETEd)
            // it can be used to perform any final validation of the specified Base EDID and set parsed Base EDID properties
            // when PUT, it may throw web::json::json_exception, which will be mapped to a 400 Bad Request status code with NMOS error "debug" information including the exception message
            // or std::runtime_error, which will be mapped to a 500 Internal Error status code with NMOS error "debug" information including the exception message
            // when DELETE, this callback should not throw exceptions, as the Base EDID will already have been changed and those changes will not be rolled back
            typedef std::function<void(const nmos::id& input_id, const bst::optional<utility::string_t>& base_edid, web::json::value& base_edid_properties)> streamcompatibility_base_edid_handler;

            // a streamcompatibility_active_constraints_handler is a notification that the Active Constraints for the specified IS-11 sender has changed (PUT or DELETEd)
            // it can be used to perform any final validation of the specified Active Constraints
            // it may throw web::json::json_exception, which will be mapped to a 400 Bad Request status code with NMOS error "debug" information including the exception message
            // or std::runtime_error, which will be mapped to a 500 Internal Error status code with NMOS error "debug" information including the exception message
            typedef std::function<void(const nmos::resource& streamcompatibility_sender, const web::json::value& constraint_sets, web::json::value& intersection)> streamcompatibility_active_constraints_handler;

            // a streamcompatibility_effective_edid_setter updates the specified Effective EDID for the specified IS-11 input
            // effective EDID of the input is updated when either Active Constraints of a Sender associated with this input are updated
            // or base EDID of the input is updated or deleted
            // this callback should not throw exceptions, as it's called after the mentioned changes which will not be rolled back
            typedef std::function<void(const nmos::id& input_id, boost::variant<utility::string_t, web::uri>& effective_edid, bst::optional<web::json::value>& effective_edid_properties)> streamcompatibility_effective_edid_setter;
        }

        web::http::experimental::listener::api_router make_streamcompatibility_api(nmos::node_model& model, details::streamcompatibility_base_edid_handler base_edid_handler, details::streamcompatibility_effective_edid_setter effective_edid_setter, details::streamcompatibility_active_constraints_handler active_constraints_handler, slog::base_gate& gate);
    }
}

#endif
