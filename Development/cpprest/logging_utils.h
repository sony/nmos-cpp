#ifndef CPPREST_LOGGING_UTILS_H
#define CPPREST_LOGGING_UTILS_H

#include <functional>
#include "cpprest/details/basic_types.h"

// Experimental extension to cpprestsdk framework to be able to add logging callbacks
// to e.g. http_listener, websocket_listener
namespace web
{
    namespace logging
    {
        namespace experimental
        {
            typedef uint32_t level;

            namespace levels
            {
                // Low level debugging information (warning: very chatty)
                const level devel = 0x1;
                // Information about unusual system states or other minor internal library
                // problems, less chatty than devel.
                const level verbose = 0x2;
                // Information about minor configuration problems or additional information
                // about other warnings.
                const level info = 0x4;
                // Information about important problems not severe enough to terminate
                // connections.
                const level warning = 0x8;
                // Recoverable error. Recovery may mean cleanly closing the connection with
                // an appropriate error code to the remote endpoint.
                const level error = 0x10;
                // Unrecoverable error. This error will trigger immediate unclean
                // termination of the connection or endpoint.
                const level severe = 0x20;
            }

            // A logging callback includes the level, a message and optionally (i.e. maybe empty) category identifier
            // (similar to http_exception, use std::string rather than utility::string_t to simplify integration with <system_error>, etc.)
            typedef void (callback_signature)(level level, const std::string& message, const std::string& category);
            typedef callback_signature* callback_ptr;
            typedef std::function<callback_signature> callback_function;
        }
    }
}

#endif
