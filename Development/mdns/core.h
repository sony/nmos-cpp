#ifndef MDNS_CORE_H
#define MDNS_CORE_H

#include <string>
#include <vector>

namespace mdns
{
    const unsigned int default_timeout_seconds = 5;

    typedef std::vector<std::string> txt_records;
}

#endif
