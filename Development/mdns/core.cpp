#include "mdns/core.h"

#include <boost/range/adaptor/filtered.hpp>
#include <boost/range/adaptor/transformed.hpp>

namespace mdns
{
    structured_txt_records parse_txt_records(const txt_records& records)
    {
        return boost::copy_range<structured_txt_records>(records
            | boost::adaptors::filtered([](const std::string& record)
        {
            const auto eq = record.find('=');
            // For now, discard "boolean attributes"...
            return std::string::npos != eq && 0 != eq;
        })
            | boost::adaptors::transformed([](const std::string& record) -> structured_txt_record
        {
            const auto eq = record.find('=');
            return{ record.substr(0, eq), record.substr(std::string::npos == eq ? eq : eq + 1) };
        }));
    }

    txt_records make_txt_records(const structured_txt_records& records)
    {
        return boost::copy_range<txt_records>(records
            | boost::adaptors::filtered([](const structured_txt_record& record)
        {
            return !record.first.empty();
        })
            | boost::adaptors::transformed([](const structured_txt_record& record)
        {
            return record.first + '=' + record.second;
        }));
    }
}
