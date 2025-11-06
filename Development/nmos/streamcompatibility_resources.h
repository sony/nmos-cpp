#ifndef NMOS_STREAMCOMPATIBILITY_RESOURCES_H
#define NMOS_STREAMCOMPATIBILITY_RESOURCES_H

#include <vector>
#include <boost/variant.hpp>
#include "bst/optional.h"
#include "cpprest/base_uri.h"
#include "nmos/id.h"
#include "nmos/settings.h"

namespace nmos
{
    struct resource;

    namespace experimental
    {
        web::json::value make_streamcompatibility_active_constraints_endpoint(const web::json::value& constraint_sets, bool locked = false);

        nmos::resource make_streamcompatibility_sender(const nmos::id& id, const std::vector<nmos::id>& inputs, const std::vector<utility::string_t>& param_constraints);
        nmos::resource make_streamcompatibility_receiver(const nmos::id& id, const std::vector<nmos::id>& outputs);

        // Makes a dummy EDID endpoint to show that an input supports EDID of this type but it currently has no value
        web::json::value make_streamcompatibility_dummy_edid_endpoint(bool locked = false);
        web::json::value make_streamcompatibility_edid_endpoint(const web::uri& edid_file, bool locked = false);
        web::json::value make_streamcompatibility_edid_endpoint(const utility::string_t& edid_file, bool locked = false);

        // See https://specs.amwa.tv/is-11/branches/v1.0.x/APIs/schemas/with-refs/input.html
        // Makes an input without EDID support
        nmos::resource make_streamcompatibility_input(const nmos::id& id, const nmos::id& device_id, bool connected, const std::vector<nmos::id>& senders, const nmos::settings& settings);
        // Makes an input with EDID support
        nmos::resource make_streamcompatibility_input(const nmos::id& id, const nmos::id& device_id, bool connected, bool base_edid_support, const boost::variant<utility::string_t, web::uri>& effective_edid, const std::vector<nmos::id>& senders, const nmos::settings& settings);

        // See https://specs.amwa.tv/is-11/branches/v1.0.x/APIs/schemas/with-refs/output.html
        // Makes an output without EDID support
        nmos::resource make_streamcompatibility_output(const nmos::id& id, const nmos::id& device_id, bool connected, const std::vector<nmos::id>& receivers, const nmos::settings& settings);
        // Makes an output with EDID support
        nmos::resource make_streamcompatibility_output(const nmos::id& id, const nmos::id& device_id, bool connected, const bst::optional<boost::variant<utility::string_t, web::uri>>& edid, const std::vector<nmos::id>& receivers, const nmos::settings& settings);

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
