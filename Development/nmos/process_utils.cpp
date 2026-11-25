#include "nmos/process_utils.h"

#include <boost/version.hpp>
#if BOOST_VERSION >= 106600
#include <boost/asio/io_context.hpp>
#else
#include <boost/asio/io_service.hpp>
#endif
#include <boost/asio/signal_set.hpp>

// boost::this_process::get_id() would be perfect, except that Boost.Process was only introduced in Boost 1.64.0
// and VS2013 chokes on boost/process/detail/config.hpp (tested at Boost.1.65.1)
#if defined(_WIN32)
extern "C" __declspec(dllimport) unsigned long __stdcall GetCurrentProcessId();
#else
#include <unistd.h>
#endif

namespace nmos
{
    namespace details
    {
        // Get the current process ID
        int get_process_id()
        {
#if defined(_WIN32)
            return GetCurrentProcessId();
#else
            return getpid();
#endif
        }

        // Wait for a process termination signal
        void wait_term_signal()
        {
#if BOOST_VERSION >= 106600
            boost::asio::io_context service;
#else
            boost::asio::io_service service;
#endif
            // Ctrl+C generates SIGINT on both Linux and Windows
            // On Windows, closing the console window generates SIGBREAK
            // On Linux, the kill command generates SIGTERM by default
            boost::asio::signal_set signals(service,
                SIGINT,
#ifdef _WIN32
                SIGBREAK,
#endif
                SIGTERM);
            signals.async_wait([](const boost::system::error_code&, int){});
            service.run();
        }
    }
}
