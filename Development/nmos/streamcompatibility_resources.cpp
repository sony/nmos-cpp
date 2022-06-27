#include "nmos/streamcompatibility_resources.h"

#include <unordered_set>
#include "nmos/capabilities.h" // for nmos::fields::constraint_sets
#include "nmos/is11_versions.h"
#include "nmos/resource.h"
#include "nmos/streamcompatibility_state.h"

namespace nmos
{
    namespace experimental
    {
        web::json::value make_streamcompatibility_active_constraints_endpoint(const web::json::value& constraint_sets, bool locked)
        {
            using web::json::value_of;

            auto active_constraint_sets = value_of({
                { nmos::fields::constraint_sets, constraint_sets },
            });

            return value_of({
                { nmos::fields::active_constraint_sets, active_constraint_sets },
                { nmos::fields::temporarily_locked, locked },
            });
        }

        nmos::resource make_streamcompatibility_sender(const nmos::id& id, const std::vector<nmos::id>& inputs, const std::vector<utility::string_t>& param_constraints)
        {
            using web::json::value;
            using web::json::value_of;
            using web::json::value_from_elements;

            std::unordered_set<utility::string_t> parameter_constraints{
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

            auto supported_param_constraints = value_of({
                { nmos::fields::parameter_constraints, value_from_elements(parameter_constraints) },
            });

            auto data = value_of({
                { nmos::fields::id, id },
                { nmos::fields::device_id, U("these are not the droids you are looking for") },
                { nmos::fields::endpoint_active_constraints, make_streamcompatibility_active_constraints_endpoint(value::array()) },
                { nmos::fields::inputs, value_from_elements(inputs) },
                { nmos::fields::supported_param_constraints, supported_param_constraints },
                { nmos::fields::status, value_of({ { nmos::fields::state, nmos::sender_states::unconstrained.name } }) },
            });

            return{ is11_versions::v1_0, types::sender, std::move(data), id, false };
        }

        nmos::resource make_streamcompatibility_receiver(const nmos::id& id, const std::vector<nmos::id>& outputs)
        {
            using web::json::value_of;
            using web::json::value_from_elements;

            auto data = value_of({
                { nmos::fields::id, id },
                { nmos::fields::device_id, U("these are not the droids you are looking for") },
                { nmos::fields::outputs, value_from_elements(outputs) },
                { nmos::fields::status, value_of({ { nmos::fields::state, nmos::receiver_states::unknown.name } }) },
            });

            return{ is11_versions::v1_0, types::receiver, std::move(data), id, false };
        }

        web::json::value make_streamcompatibility_dummy_edid_endpoint(bool locked)
        {
            using web::json::value_of;

            return value_of({
                { nmos::fields::temporarily_locked, locked },
            });
        }

        web::json::value make_streamcompatibility_edid_endpoint(const web::uri& edid_file, bool locked)
        {
            using web::json::value_of;

            return value_of({
                { nmos::fields::edid_href, edid_file.to_string() },
                { nmos::fields::temporarily_locked, locked },
            });
        }

        web::json::value make_streamcompatibility_edid_endpoint(const utility::string_t& edid_file, bool locked)
        {
            using web::json::value_of;

            return value_of({
                { nmos::fields::edid_binary, edid_file },
                { nmos::fields::temporarily_locked, locked },
            });
        }

        web::json::value make_streamcompatibility_input_output_base(const nmos::id& id, bool connected, bool edid_support, const nmos::settings& settings)
        {
            using web::json::value;

            auto data = details::make_resource_core(id, settings);

            data[nmos::fields::connected] = value::boolean(connected);
            data[nmos::fields::edid_support] = value::boolean(edid_support);

            return data;
        }

        nmos::resource make_streamcompatibility_input(const nmos::id& id, bool connected, const std::vector<nmos::id>& senders, const nmos::settings& settings)
        {
            using web::json::value_from_elements;
            using web::json::value_of;

            auto data = make_streamcompatibility_input_output_base(id, connected, false, settings);
            data[nmos::fields::senders] = value_from_elements(senders);
            data[nmos::fields::status] = value_of({ { nmos::fields::state, nmos::input_states::signal_present.name } });

            return{ is11_versions::v1_0, types::input, std::move(data), id, false };
        }

        nmos::resource make_streamcompatibility_input(const nmos::id& id, bool connected, bool base_edid_changeable, const boost::variant<utility::string_t, web::uri>& effective_edid, const bst::optional<web::json::value>& effective_edid_properties, const std::vector<nmos::id>& senders, const nmos::settings& settings)
        {
            using web::json::value_from_elements;
            using web::json::value_of;

            auto data = make_streamcompatibility_input_output_base(id, connected, true, settings);

            if (base_edid_changeable)
            {
                data[nmos::fields::endpoint_base_edid] = make_streamcompatibility_dummy_edid_endpoint(false);
            }

            data[nmos::fields::endpoint_effective_edid] = boost::apply_visitor(edid_file_visitor(), effective_edid);

            if (effective_edid_properties)
            {
                data[nmos::fields::effective_edid_properties] = *effective_edid_properties;
            }

            data[nmos::fields::senders] = value_from_elements(senders);
            data[nmos::fields::status] = value_of({ { nmos::fields::state, nmos::input_states::signal_present.name } });

            return{ is11_versions::v1_0, types::input, std::move(data), id, false };
        }

        nmos::resource make_streamcompatibility_output(const nmos::id& id, bool connected, const std::vector<nmos::id>& receivers, const nmos::settings& settings)
        {
            using web::json::value_from_elements;
            using web::json::value_of;

            auto data = make_streamcompatibility_input_output_base(id, connected, false, settings);
            data[nmos::fields::receivers] = value_from_elements(receivers);
            data[nmos::fields::status] = value_of({ { nmos::fields::state, nmos::output_states::signal_present.name } });

            return{ is11_versions::v1_0, types::output, std::move(data), id, false };
        }

        nmos::resource make_streamcompatibility_output(const nmos::id& id, bool connected, const bst::optional<boost::variant<utility::string_t, web::uri>>& edid, const bst::optional<web::json::value>& edid_properties, const std::vector<nmos::id>& receivers, const nmos::settings& settings)
        {
            using web::json::value_from_elements;
            using web::json::value_of;

            auto data = make_streamcompatibility_input_output_base(id, connected, true, settings);
            data[nmos::fields::receivers] = value_from_elements(receivers);
            data[nmos::fields::status] = value_of({ { nmos::fields::state, nmos::output_states::signal_present.name } });

            if (edid)
            {
                data[nmos::fields::endpoint_edid] = boost::apply_visitor(edid_file_visitor(), *edid);
            }

            if (edid_properties)
            {
                data[nmos::fields::edid_properties] = *edid_properties;
            }

            return{ is11_versions::v1_0, types::output, std::move(data), id, false };
        }
    }
}
