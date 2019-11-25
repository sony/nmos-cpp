#include "nmos/components.h"

#include "cpprest/json_ops.h"

namespace nmos
{
    web::json::value make_component(const nmos::component_name& name, unsigned int width, unsigned int height, unsigned int bit_depth)
    {
        return web::json::value_of({
            { U("name"), web::json::value::string(name.name) },
            { U("width"), width },
            { U("height"), height },
            { U("bit_depth"), bit_depth }
        });
    }

    web::json::value make_components(chroma_subsampling chroma_subsampling, unsigned int frame_width, unsigned int frame_height, unsigned int bit_depth)
    {
        using web::json::value;
        using web::json::value_of;

        switch (chroma_subsampling)
        {
        case RGB444:
            return value_of({
                make_component(component_names::R, frame_width, frame_height, bit_depth),
                make_component(component_names::G, frame_width, frame_height, bit_depth),
                make_component(component_names::B, frame_width, frame_height, bit_depth)
            });
        case YCbCr422:
            return  value_of({
                make_component(component_names::Y, frame_width, frame_height, bit_depth),
                make_component(component_names::Cb, frame_width / 2, frame_height, bit_depth),
                make_component(component_names::Cr, frame_width / 2, frame_height, bit_depth)
            });
        default:
            return web::json::value::null();
        }
    }
}
