#ifndef CPPREST_HOST_UTILS_H
#define CPPREST_HOST_UTILS_H

#include <vector>
#include "cpprest/details/basic_types.h"

// Extensions to provide the current host name, network interface details, and name resolution services
namespace web
{
    namespace hosts
    {
        namespace experimental
        {
            utility::string_t host_name();

            struct host_interface
            {
                uint32_t index;
                utility::string_t name;
                utility::string_t physical_address;
                std::vector<utility::string_t> addresses;
                utility::string_t domain;

                host_interface(uint32_t index = {}, const utility::string_t& name = {}, const utility::string_t& physical_address = {}, const std::vector<utility::string_t>& addresses = {}, const utility::string_t& domain = {})
                    : index(index), name(name), physical_address(physical_address), addresses(addresses), domain(domain)
                {}
            };

            std::vector<host_interface> host_interfaces();

            std::vector<utility::string_t> host_names(const utility::string_t& address);
            std::vector<utility::string_t> host_addresses(const utility::string_t& host_name);
        }
    }
}

#endif
