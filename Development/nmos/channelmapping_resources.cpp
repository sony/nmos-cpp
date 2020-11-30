#include "nmos/channelmapping_resources.h"

#include <boost/range/adaptor/transformed.hpp>
#include "nmos/activation_utils.h"
#include "nmos/resource.h"
#include "nmos/is08_versions.h"

namespace nmos
{
    nmos::id make_channelmapping_resource_id(const std::pair<channelmapping_id, type>& channelmapping_id_type)
    {
        // neither id nor type can contain ':'
        // cf. nmos::patterns::inputOutputId
        return channelmapping_id_type.second.name + U(':') + channelmapping_id_type.first;
    }

    std::pair<channelmapping_id, type> parse_channelmapping_resource_id(const nmos::id& id)
    {
        const auto pos = id.find(U(':'));
        if (nmos::id::npos == pos) return{};
        return{ id.substr(pos + 1), nmos::type{ id.substr(0, pos) } };
    }

    // construct input resource
    nmos::resource make_channelmapping_input(const nmos::channelmapping_id& channelmapping_id, const utility::string_t& name, const utility::string_t& description, const std::pair<nmos::id, nmos::type>& parent, const std::vector<utility::string_t>& channel_labels, bool reordering, unsigned int block_size)
    {
        return make_channelmapping_input(
            channelmapping_id,
            make_channelmapping_properties(name, description),
            make_channelmapping_input_parent(parent.first, parent.second),
            make_channelmapping_channels(channel_labels),
            make_channelmapping_input_caps(reordering, block_size));
    }

    // construct input resource
    // hm, be very careful with the order of the arguments!
    nmos::resource make_channelmapping_input(const nmos::channelmapping_id& channelmapping_id, web::json::value properties, web::json::value parent, web::json::value channels, web::json::value caps)
    {
        using web::json::value_of;

        auto data = value_of({
            { nmos::fields::channelmapping_id, channelmapping_id },
            { nmos::fields::endpoint_io, value_of({
                { U("properties"), std::move(properties) },
                { U("parent"), std::move(parent) },
                { U("channels"), std::move(channels) },
                { U("caps"), std::move(caps) }
            })}
        });

        return{ is08_versions::v1_0, types::input, std::move(data), nmos::make_channelmapping_resource_id({ channelmapping_id, types::input }), true };
    }

    // construct output resource with entirely unrouted /map/active output endpoint map value
    nmos::resource make_channelmapping_output(const nmos::channelmapping_id& channelmapping_id, const utility::string_t& name, const utility::string_t& description, const nmos::id& source_id, const std::vector<utility::string_t>& channel_labels, const std::vector<nmos::channelmapping_id>& routable_inputs)
    {
        return make_channelmapping_output(
            channelmapping_id,
            name,
            description,
            source_id,
            channel_labels,
            routable_inputs,
            std::vector<std::pair<nmos::channelmapping_id, uint32_t>>(channel_labels.size()));
    }

    // construct output resource
    nmos::resource make_channelmapping_output(const nmos::channelmapping_id& channelmapping_id, const utility::string_t& name, const utility::string_t& description, const nmos::id& source_id, const std::vector<utility::string_t>& channel_labels, const std::vector<nmos::channelmapping_id>& routable_inputs, const std::vector<std::pair<nmos::channelmapping_id, uint32_t>>& active_map)
    {
        return make_channelmapping_output(
            channelmapping_id,
            make_channelmapping_properties(name, description),
            make_channelmapping_output_source_id(source_id),
            make_channelmapping_channels(channel_labels),
            make_channelmapping_output_caps(routable_inputs),
            make_channelmapping_active_map(active_map));
    }

    // construct output resource
    // hm, be very careful with the order of the arguments!
    nmos::resource make_channelmapping_output(const nmos::channelmapping_id& channelmapping_id, web::json::value properties, web::json::value source_id, web::json::value channels, web::json::value caps, web::json::value active_map)
    {
        using web::json::value;
        using web::json::value_of;

        auto data = value_of({
            { nmos::fields::channelmapping_id, channelmapping_id },
            { nmos::fields::endpoint_io, value_of({
                { U("properties"), std::move(properties) },
                { U("source_id"), std::move(source_id) },
                { U("channels"), std::move(channels) },
                { U("caps"), std::move(caps) }
            }) },
            { nmos::fields::endpoint_active, value_of({
                { nmos::fields::activation, nmos::make_activation() },
                { nmos::fields::map, std::move(active_map) }
            }) },
            { nmos::fields::endpoint_staged, value_of({
                { nmos::fields::activation, nmos::make_activation() }
            }) }
        });

        return{ is08_versions::v1_0, types::output, std::move(data), nmos::make_channelmapping_resource_id({ channelmapping_id, types::output }), true };
    }

    // construct input or output "properties" or /properties endpoint value
    web::json::value make_channelmapping_properties(const utility::string_t& name, const utility::string_t& description)
    {
        using web::json::value_of;

        return value_of({
            { nmos::fields::name, name },
            { nmos::fields::description, description }
        });
    }

    // construct input "parent" or /parent endpoint value
    // type may be source or receiver
    // empty (default) arguments correspond to null values
    web::json::value make_channelmapping_input_parent(const nmos::id& id, const nmos::type& type)
    {
        using web::json::value;
        using web::json::value_of;

        const bool empty = id.empty() || type.name.empty();
        return value_of({
            { nmos::fields::id, empty ? value::null() : value::string(id) },
            { nmos::fields::type, empty ? value::null() : value::string(type.name) }
        });
    }

    // construct output "source_id" or /sourceid endpoint value
    // empty (default) argument corresponds to null value
    web::json::value make_channelmapping_output_source_id(const nmos::id& source_id)
    {
        using web::json::value;

        return source_id.empty() ? value::null() : value::string(source_id);
    }

    // construct input or output "channels" or /channels endpoint values
    // empty vector is invalid
    web::json::value make_channelmapping_channels(const std::vector<utility::string_t>& channel_labels)
    {
        using web::json::value_from_elements;
        using web::json::value_of;

        return value_from_elements(channel_labels | boost::adaptors::transformed([](const utility::string_t& label)
        {
            return value_of({
                { nmos::fields::label, label }
            });
        }));
    }

    // construct input "caps" or /caps endpoint value
    // default arguments correspond to no constraints
    web::json::value make_channelmapping_input_caps(bool reordering, unsigned int block_size)
    {
        using web::json::value_of;

        return value_of({
            { nmos::fields::reordering, reordering },
            { nmos::fields::block_size, block_size }
        });
    }

    // construct output "caps" or /caps endpoint value
    // an empty (default) vector argument corresponds to "routable_inputs": null, i.e. no constraints
    // an empty id in the vector corresponds to null in the "routable_inputs" array, i.e. unrouted outputs are permitted
    web::json::value make_channelmapping_output_caps(const std::vector<nmos::channelmapping_id>& routable_inputs)
    {
        using web::json::value;
        using web::json::value_from_elements;
        using web::json::value_of;

        return value_of({
            {
                nmos::fields::routable_inputs,
                routable_inputs.empty() ? value::null() : value_from_elements(routable_inputs | boost::adaptors::transformed([](const nmos::channelmapping_id& channelmapping_id)
                {
                    return channelmapping_id.empty() ? value::null() : value::string(channelmapping_id);
                }))
            }
        });
    }

    // construct /map/active output endpoint map value
    // empty vector is invalid
    web::json::value make_channelmapping_active_map(const std::vector<std::pair<nmos::channelmapping_id, uint32_t>>& active_map)
    {
        using web::json::value;
        using web::json::value_from_fields;
        using web::json::value_of;

        int index = 0;
        return value_from_fields(active_map | boost::adaptors::transformed([&index](const std::pair<nmos::channelmapping_id, uint32_t>& channel)
        {
            return std::make_pair(utility::conversions::details::to_string_t(index++), value_of({
                {  nmos::fields::input, channel.first.empty() ? value::null() : value::string(channel.first) },
                {  nmos::fields::channel_index, channel.first.empty() ? value::null() : value::number(channel.second) }
            }));
        }));
    }
}
