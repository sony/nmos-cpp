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
        nmos::resource make_flowcompatibility_sender(const nmos::id& id, const std::vector<nmos::id>& inputs, const std::vector<utility::string_t>& param_constraints);
        nmos::resource make_flowcompatibility_receiver(const nmos::id& id, const std::vector<nmos::id>& outputs);

        // See https://github.com/AMWA-TV/is-11/blob/v1.0-dev/APIs/schemas/input.json
        // Makes an input without EDID support
        nmos::resource make_flowcompatibility_input(const nmos::id& id, bool connected, const nmos::settings& settings);
        // Makes an input with EDID support
        nmos::resource make_flowcompatibility_input(const nmos::id& id, bool connected, const bst::optional<web::json::value>& effective_edid_properties, const boost::variant<utility::string_t, web::uri>& effective_edid, const nmos::settings& settings);

        // See https://github.com/AMWA-TV/is-11/blob/v1.0-dev/APIs/schemas/output.json
        // Makes an output without EDID support
        nmos::resource make_flowcompatibility_output(const nmos::id& id, bool connected, const nmos::settings& settings);
        // Makes an output with EDID support
        nmos::resource make_flowcompatibility_output(const nmos::id& id, bool connected, const bst::optional<web::json::value>& edid_properties, const boost::variant<utility::string_t, web::uri>& edid, const nmos::settings& settings);
    }
}

#endif
