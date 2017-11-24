#include "cpprest/host_utils.h"

#include <boost/asio/ip/host_name.hpp>
#include <boost/asio/ip/tcp.hpp>
#include "cpprest/basic_utils.h" // for utility::s2us, etc.

namespace web
{
    namespace http
    {
        namespace experimental
        {
            utility::string_t host_name()
            {
                return utility::s2us(boost::asio::ip::host_name());
            }

            std::vector<utility::string_t> host_addresses(const utility::string_t& host_name)
            {
                boost::asio::io_service service;
                boost::asio::ip::tcp::resolver resolver(service);
                std::vector<utility::string_t> addresses;
                boost::system::error_code ec;
                std::for_each(resolver.resolve({ boost::asio::ip::tcp::v4(), utility::us2s(host_name), "" }, ec), boost::asio::ip::tcp::resolver::iterator{}, [&](const boost::asio::ip::tcp::resolver::iterator::value_type& re)
                {
                    addresses.push_back(utility::s2us(re.endpoint().address().to_string()));
                });
                return addresses; // empty if host_name cannot be resolved
            }
        }
    }
}
