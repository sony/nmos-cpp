#ifndef NMOS_CHANNELMAPPING_RESOURCES_H
#define NMOS_CHANNELMAPPING_RESOURCES_H

#include "cpprest/json.h"
#include "nmos/id.h"
#include "nmos/tai.h"
#include "nmos/type.h"

namespace nmos
{
    struct resource;

    // IS-08 Channel Mapping API resources
    // See https://github.com/AMWA-TV/nmos-audio-channel-mapping/blob/v1.0.x/docs/1.0.%20Overview.md#api-structure
    // Each IS-08 input and output's data are json objects with an identifier field 
    // and a field for the resource's view in the /io endpoint, also used for
    // the individual endpoints, "properties", "caps" and so on
    // See https://github.com/AMWA-TV/nmos-audio-channel-mapping/blob/v1.0.x/APIs/schemas/io-response-schema.json
    // The output resource type also has a field for that output's /map/active endpoint
    // and a field that represents a 'staged' endpoint which contains the output-specific
    // "action" when there is a scheduled or in-flight immediate activation for that
    // output (an "activation_id" and the "activation" object are also included so that
    // the /map/activations endpoints can be implemented similarly to the /io endpoint)
    // See https://github.com/AMWA-TV/nmos-audio-channel-mapping/blob/v1.0.x/APIs/schemas/map-active-output-response-schema.json
    // and https://github.com/AMWA-TV/nmos-audio-channel-mapping/blob/v1.0.x/APIs/schemas/map-activations-activation-get-response-schema.json

    // Note that the input/output identifiers used in the Channel Mapping API are not universally unique
    // and one input and one output in an API instance may even share the same identifier
    // so these need to be prefixed with the resource type to make the nmos::resource::id locally unique
    // See https://github.com/AMWA-TV/nmos-audio-channel-mapping/blob/v1.0.x/docs/4.0.%20Behaviour.md#identifiers
    typedef utility::string_t channelmapping_id;

    nmos::id make_channelmapping_resource_id(const std::pair<nmos::channelmapping_id, nmos::type>& id_type);
    std::pair<channelmapping_id, type> parse_channelmapping_resource_id(const nmos::id& id);

    // construct input resource
    nmos::resource make_channelmapping_input(const nmos::channelmapping_id& channelmapping_id, const utility::string_t& name, const utility::string_t& description, const std::pair<nmos::id, nmos::type>& parent, const std::vector<utility::string_t>& channel_labels, bool reordering = true, unsigned int block_size = 1);

    // construct input resource
    // hm, be very careful with the order of the arguments!
    nmos::resource make_channelmapping_input(const nmos::channelmapping_id& channelmapping_id, web::json::value properties, web::json::value parent, web::json::value channels, web::json::value caps);

    // construct output resource with entirely unrouted /map/active output endpoint map value
    nmos::resource make_channelmapping_output(const nmos::channelmapping_id& channelmapping_id, const utility::string_t& name, const utility::string_t& description, const nmos::id& source_id, const std::vector<utility::string_t>& channel_labels, const std::vector<nmos::channelmapping_id>& routable_inputs = {});

    // construct output resource
    nmos::resource make_channelmapping_output(const nmos::channelmapping_id& channelmapping_id, const utility::string_t& name, const utility::string_t& description, const nmos::id& source_id, const std::vector<utility::string_t>& channel_labels, const std::vector<nmos::channelmapping_id>& routable_inputs, const std::vector<std::pair<nmos::channelmapping_id, uint32_t>>& active_map);

    // construct output resource
    // hm, be very careful with the order of the arguments!
    nmos::resource make_channelmapping_output(const nmos::channelmapping_id& channelmapping_id, web::json::value properties, web::json::value source_id, web::json::value channels, web::json::value caps, web::json::value active_map);

    // construct input or output "properties" or /properties endpoint value
    web::json::value make_channelmapping_properties(const utility::string_t& name, const utility::string_t& description);

    // construct input "parent" or /parent endpoint value
    // type may be source or receiver
    // empty (default) arguments correspond to null values
    web::json::value make_channelmapping_input_parent(const nmos::id& id = {}, const nmos::type& type = {});

    // construct output "source_id" or /sourceid endpoint value
    // empty (default) argument corresponds to null value
    web::json::value make_channelmapping_output_source_id(const nmos::id& source_id = {});

    // construct input or output "channels" or /channels endpoint values
    // empty vector is invalid
    web::json::value make_channelmapping_channels(const std::vector<utility::string_t>& channel_labels);

    // construct input "caps" or /caps endpoint value
    // default arguments correspond to no constraints
    web::json::value make_channelmapping_input_caps(bool reordering = true, unsigned int block_size = 1);

    // construct output "caps" or /caps endpoint value
    // an empty (default) vector argument corresponds to "routable_inputs": null, i.e. no constraints
    // an empty id in the vector corresponds to null in the "routable_inputs" array, i.e. unrouted outputs are permitted
    web::json::value make_channelmapping_output_caps(const std::vector<nmos::channelmapping_id>& routable_inputs = {});

    // construct /map/active output endpoint map value
    // empty vector is invalid
    web::json::value make_channelmapping_active_map(const std::vector<std::pair<nmos::channelmapping_id, uint32_t>>& active_map);
}

#endif
