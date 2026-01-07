#ifndef NMOS_STREAMCOMPATIBILITY_RESOURCE_H
#define NMOS_STREAMCOMPATIBILITY_RESOURCE_H

#include <boost/variant.hpp>
#include "bst/optional.h"
#include "cpprest/base_uri.h"
#include "nmos/id.h"
#include "nmos/settings.h"

namespace nmos
{
    namespace experimental
    {
        web::json::value make_streamcompatibility_active_constraints_endpoint(const web::json::value& constraint_sets, bool locked = false);

        // Makes an EDID endpoint to show that an input supports EDID of this type but it currently has no value
        web::json::value make_streamcompatibility_edid_endpoint(bool locked = false);
        // Makes an EDID endpoint to show that an input supports EDID using EDID reference
        web::json::value make_streamcompatibility_edid_endpoint(const web::uri& edid_uri, bool locked = false);
        // Makes an EDID endpoint to show that an input supports EDID using EDID binary
        web::json::value make_streamcompatibility_edid_endpoint(const utility::string_t& edid_binary, bool locked = false);

        web::json::value make_streamcompatibility_input_output_base(const nmos::id& id, const nmos::id& device_id, bool connected, bool edid_support, const nmos::settings& settings);

        struct edid_file_visitor : public boost::static_visitor<web::json::value>
        {
            web::json::value operator()(const utility::string_t& edid_file_binary) const
            {
                return make_streamcompatibility_edid_endpoint(edid_file_binary);
            }

            web::json::value operator()(const web::uri& edid_file_uri) const
            {
                return make_streamcompatibility_edid_endpoint(edid_file_uri);
            }
        };
    }
}

#endif
