#ifndef CPPREST_JSON_STORAGE_H
#define CPPREST_JSON_STORAGE_H

#include "cpprest/json.h"

namespace web
{
    namespace json
    {
        namespace details
        {
            typedef std::vector<json::value> array_storage_t;
            typedef std::vector<std::pair<utility::string_t, json::value>> object_storage_t;
        }

        // take care, but sometimes the limited interface just isn't enough
        details::array_storage_t& storage_of(web::json::array& array);

        // take care, but sometimes the limited interface just isn't enough
        details::object_storage_t& storage_of(web::json::object& object);
    }
}

#endif
