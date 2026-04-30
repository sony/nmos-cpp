#ifndef NMOS_STREAMCOMPATIBILITY_API_H
#define NMOS_STREAMCOMPATIBILITY_API_H

#include <boost/variant.hpp>
#include "bst/optional.h"
#include "nmos/api_utils.h"
#include "nmos/slog.h"

// Stream Compatibility Management API implementation
// See https://specs.amwa.tv/is-11/branches/v1.0.x/APIs/StreamCompatibilityManagementAPI.html
namespace nmos
{
    struct node_model;
    struct resource;

    namespace experimental
    {
        namespace details
        {
            // a streamcompatibility_base_edid_handler is a notification that the Base EDID for the specified IS-11 input has received the modification request (PUT or DELETEd)
            // it can be used to perform any final validation of the specified Base EDID
            // the validation result is returned along with the error string
            // this callback should not throw exceptions
            typedef std::function<std::pair<bool, utility::string_t>(const nmos::id& input_id, const bst::optional<utility::string_t>& base_edid)> streamcompatibility_base_edid_handler;

            // a streamcompatibility_active_constraints_handler is a notification that the Active Constraints for the specified IS-11 sender has received the modification request (PUT or DELETEd)
            // it can be used to perform any final validation of the specified Active Constraints
            // the validation result is returned along with the error string
            // this callback should not throw exceptions
            typedef std::function<std::pair<bool, utility::string_t>(const nmos::resource& streamcompatibility_sender, const web::json::value& constraint_sets, web::json::value& intersection)> streamcompatibility_active_constraints_handler;

            // a streamcompatibility_effective_edid_setter updates the specified Effective EDID for the specified IS-11 input
            // effective EDID of the input is updated when either Active Constraints of a Sender associated with this input are updated
            // or base EDID of the input is updated or deleted
            // this callback should not throw exceptions, as it's called after the mentioned changes which will not be rolled back
            typedef std::function<void(const nmos::id& input_id, boost::variant<utility::string_t, web::uri>& effective_edid)> streamcompatibility_effective_edid_setter;
        }

        web::http::experimental::listener::api_router make_streamcompatibility_api(nmos::node_model& model, details::streamcompatibility_base_edid_handler validate_base_edid, details::streamcompatibility_effective_edid_setter effective_edid_setter, details::streamcompatibility_active_constraints_handler active_constraints_handler, web::http::experimental::listener::route_handler validate_authorization, slog::base_gate& gate);
    }
}

#endif
