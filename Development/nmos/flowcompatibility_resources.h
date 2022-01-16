#ifndef NMOS_FLOWCOMPATIBILITY_RESOURCES_H
#define NMOS_FLOWCOMPATIBILITY_RESOURCES_H

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
        web::json::value make_flowcompatibility_active_constraints_endpoint(const web::json::value& constraint_sets, bool locked = false);

        nmos::resource make_flowcompatibility_sender(const nmos::id& id, const std::vector<nmos::id>& inputs, const std::vector<utility::string_t>& param_constraints);
        nmos::resource make_flowcompatibility_receiver(const nmos::id& id, const std::vector<nmos::id>& outputs);

        // Makes a dummy EDID endpoint to show that an input supports EDID of this type but it currently has no value
        web::json::value make_flowcompatibility_dummy_edid_endpoint(bool locked = false);
        web::json::value make_flowcompatibility_edid_endpoint(const web::uri& edid_file, bool locked = false);
        web::json::value make_flowcompatibility_edid_endpoint(const utility::string_t& edid_file, bool locked = false);

        // See https://github.com/AMWA-TV/is-11/blob/v1.0-dev/APIs/schemas/input.json
        // Makes an input without EDID support
        nmos::resource make_flowcompatibility_input(const nmos::id& id, bool connected, const std::vector<nmos::id>& senders, const nmos::settings& settings);
        // Makes an input with EDID support
        nmos::resource make_flowcompatibility_input(const nmos::id& id, bool connected, bool base_edid_changeable, const boost::variant<utility::string_t, web::uri>& effective_edid, const bst::optional<web::json::value>& effective_edid_properties, const std::vector<nmos::id>& senders, const nmos::settings& settings);

        // See https://github.com/AMWA-TV/is-11/blob/v1.0-dev/APIs/schemas/output.json
        // Makes an output without EDID support
        nmos::resource make_flowcompatibility_output(const nmos::id& id, bool connected, const std::vector<nmos::id>& receivers, const nmos::settings& settings);
        // Makes an output with EDID support
        nmos::resource make_flowcompatibility_output(const nmos::id& id, bool connected, const boost::variant<utility::string_t, web::uri>& edid, const bst::optional<web::json::value>& edid_properties, const std::vector<nmos::id>& receivers, const nmos::settings& settings);

        struct edid_file_visitor : public boost::static_visitor<web::json::value>
        {
            web::json::value operator()(utility::string_t edid_file) const
            {
                return make_flowcompatibility_edid_endpoint(edid_file);
            }

            web::json::value operator()(web::uri edid_file) const
            {
                return make_flowcompatibility_edid_endpoint(edid_file);
            }
        };
    }
}

#endif
