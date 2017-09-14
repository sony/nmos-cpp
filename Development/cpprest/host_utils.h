#ifndef CPPREST_HOST_UTILS_H
#define CPPREST_HOST_UTILS_H

#include <vector>
#include "cpprest/details/basic_types.h"

// Extensions to provide the current host name and name resolution services
namespace web
{
    namespace http
    {
        namespace experimental
        {
            utility::string_t host_name();
            std::vector<utility::string_t> host_addresses(const utility::string_t& host_name);
        }
    }
}

#endif
