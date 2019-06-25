#include "cpprest/json_storage.h"

#include "detail/private_access.h"

namespace web
{
    namespace json
    {
        namespace details
        {
            struct array_elements { typedef details::array_storage_t(array::*type); };
            struct object_elements { typedef details::object_storage_t(object::*type); };
        }

        // take care, but sometimes the limited interface just isn't enough
        details::array_storage_t& storage_of(web::json::array& array)
        {
            return array.*detail::stowed<details::array_elements>::value;
        }

        // take care, but sometimes the limited interface just isn't enough
        details::object_storage_t& storage_of(web::json::object& object)
        {
            return object.*detail::stowed<details::object_elements>::value;
        }
    }
}

template struct detail::stow_private<web::json::details::array_elements, &web::json::array::m_elements>;
template struct detail::stow_private<web::json::details::object_elements, &web::json::object::m_elements>;
