#ifndef NMOS_FLOWCOMPATIBILITY_API_H
#define NMOS_FLOWCOMPATIBILITY_API_H

#include <boost/variant.hpp>
#include "bst/optional.h"
#include "nmos/api_utils.h"
#include "nmos/slog.h"

// Flow Compatibility Management API implementation
// See https://github.com/AMWA-TV/is-11/blob/v1.0-dev/APIs/FlowCompatibilityManagementAPI.raml
namespace nmos
{
    struct node_model;

    namespace experimental
    {
        namespace details
        {
            // a flowcompatibility_base_edid_put_handler is a notification that the Base EDID for the specified IS-11 input has changed
            // it can be used to perform any final validation of the specified Base EDID and set parsed Base EDID properties
            // it may throw web::json::json_exception, which will be mapped to a 400 Bad Request status code with NMOS error "debug" information including the exception message
            // or std::runtime_error, which will be mapped to a 500 Internal Error status code with NMOS error "debug" information including the exception message
            // if base_edid has no value, the handler should not throw any exceptions
            typedef std::function<void(const nmos::id& input_id, const utility::string_t& base_edid, bst::optional<web::json::value>& base_edid_properties)> flowcompatibility_base_edid_put_handler;

            // a flowcompatibility_base_edid_delete_handler is a notification that the Base EDID for the specified IS-11 input has been deleted
            // this callback should not throw exceptions, as the Base EDID will already have been changed and those changes will not be rolled back
            typedef std::function<void(const nmos::id& input_id)> flowcompatibility_base_edid_delete_handler;

            // a flowcompatibility_effective_edid_setter updates the specified Effective EDID for the specified IS-11 input
            // effective EDID of the input is updated when either Active Constraints of a Sender associated with this input are updated
            // or base EDID of the input is updated or deleted
            // this callback should not throw exceptions, as it's called after the mentioned changes which will not be rolled back
            typedef std::function<void(const nmos::id& input_id, boost::variant<utility::string_t, web::uri>& effective_edid, bst::optional<web::json::value>& effective_edid_properties)> flowcompatibility_effective_edid_setter;
        }

        web::http::experimental::listener::api_router make_flowcompatibility_api(nmos::node_model& model, details::flowcompatibility_base_edid_put_handler base_edid_put_handler, details::flowcompatibility_base_edid_delete_handler base_edid_delete_handler, details::flowcompatibility_effective_edid_setter effective_edid_setter, slog::base_gate& gate);
    }
}

#endif
