#include "nmos/flowcompatibility_resources.h"

#include <set>
#include "nmos/capabilities.h"
#include "nmos/is11_versions.h"
#include "nmos/resource.h"

namespace nmos
{
    namespace experimental
    {
        nmos::resource make_flowcompatibility_sender(const nmos::id& id, const std::vector<nmos::id>& inputs, const std::vector<utility::string_t>& param_constraints)
        {
            using web::json::value;
            using web::json::value_of;
            using web::json::value_from_elements;

            std::set<utility::string_t> parameter_constraints{
                nmos::caps::meta::label.key,
                nmos::caps::meta::preference.key,
                nmos::caps::meta::enabled.key,
                nmos::caps::format::media_type.key,
                nmos::caps::format::grain_rate.key,
                nmos::caps::format::frame_width.key,
                nmos::caps::format::frame_height.key,
                nmos::caps::format::interlace_mode.key,
                nmos::caps::format::colorspace.key,
                nmos::caps::format::color_sampling.key,
                nmos::caps::format::component_depth.key,
                nmos::caps::format::channel_count.key,
                nmos::caps::format::sample_rate.key,
                nmos::caps::format::sample_depth.key
            };

            parameter_constraints.insert(param_constraints.begin(), param_constraints.end());

            auto status = value_of({
                { nmos::fields::state, U("Unconstrained") },
            });

            auto active_constraint_sets = value_of({
                { nmos::fields::constraint_sets, value::array() },
            });

            auto supported_param_constraints = value_of({
                { nmos::fields::parameter_constraints, value_from_elements(parameter_constraints) },
            });

            auto data = value_of({
                { nmos::fields::id, id },
                { nmos::fields::device_id, U("these are not the droids you are looking for") },
                { nmos::fields::active_constraint_sets, active_constraint_sets },
                { nmos::fields::inputs, value_from_elements(inputs) },
                { nmos::fields::supported_param_constraints, supported_param_constraints },
                { nmos::fields::status, status },
            });

            return{ is11_versions::v1_0, types::sender, std::move(data), id, false };
        }

        nmos::resource make_flowcompatibility_receiver(const nmos::id& id, const std::vector<nmos::id>& outputs)
        {
            using web::json::value_of;
            using web::json::value_from_elements;

            auto status = value_of({
                { nmos::fields::state, U("OK") },
            });

            auto data = value_of({
                { nmos::fields::id, id },
                { nmos::fields::device_id, U("these are not the droids you are looking for") },
                { nmos::fields::outputs, value_from_elements(outputs) },
                { nmos::fields::status, status },
            });

            return{ is11_versions::v1_0, types::receiver, std::move(data), id, false };
        }

        web::json::value make_flowcompatibility_edid_endpoint(const web::uri& edid_file)
        {
            using web::json::value_of;

            return value_of({
                { nmos::fields::edid_href, edid_file.to_string() }
            });
        }

        web::json::value make_flowcompatibility_edid_endpoint(const utility::string_t& edid_file)
        {
            using web::json::value_of;

            return value_of({
                { nmos::fields::edid_binary, edid_file }
            });
        }

        web::json::value make_flowcompatibility_input_output_base(const nmos::id& id, bool connected, bool edid_support, const nmos::settings& settings)
        {
            using web::json::value;

            auto data = details::make_resource_core(id, settings);

            data[nmos::fields::connected] = value::boolean(connected);
            data[nmos::fields::edid_support] = value::boolean(edid_support);

            return data;
        }

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

        nmos::resource make_flowcompatibility_input(const nmos::id& id, bool connected, const nmos::settings& settings)
        {
            auto data = make_flowcompatibility_input_output_base(id, connected, false, settings);

            return{ is11_versions::v1_0, types::input, std::move(data), id, false };
        }

        nmos::resource make_flowcompatibility_input(const nmos::id& id, bool connected, const bst::optional<web::json::value>& effective_edid_properties, const boost::variant<utility::string_t, web::uri>& effective_edid, const nmos::settings& settings)
        {
            auto data = make_flowcompatibility_input_output_base(id, connected, true, settings);

            if (effective_edid_properties.has_value())
            {
                data[nmos::fields::effective_edid_properties] = effective_edid_properties.value();
            }

            data[nmos::fields::endpoint_effective_edid] = boost::apply_visitor(edid_file_visitor(), effective_edid);

            return{ is11_versions::v1_0, types::input, std::move(data), id, false };
        }

        nmos::resource make_flowcompatibility_output(const nmos::id& id, bool connected, const nmos::settings& settings)
        {
            auto data = make_flowcompatibility_input_output_base(id, connected, false, settings);

            return{ is11_versions::v1_0, types::output, std::move(data), id, false };
        }

        nmos::resource make_flowcompatibility_output(const nmos::id& id, bool connected, const bst::optional<web::json::value>& edid_properties, const boost::variant<utility::string_t, web::uri>& edid, const nmos::settings& settings)
        {
            auto data = make_flowcompatibility_input_output_base(id, connected, true, settings);

            if (edid_properties.has_value())
            {
                data[nmos::fields::edid_properties] = edid_properties.value();
            }

            data[nmos::fields::endpoint_edid] = boost::apply_visitor(edid_file_visitor(), edid);

            return{ is11_versions::v1_0, types::output, std::move(data), id, false };
        }
    }
}
