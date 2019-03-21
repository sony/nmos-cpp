#ifndef CPPREST_URI_SCHEMES_H
#define CPPREST_URI_SCHEMES_H

#include "cpprest/details/basic_types.h"

namespace web
{
    // Uniform Resource Identifier (URI) Schemes
    // See https://tools.ietf.org/html/rfc7595
    // and https://www.iana.org/assignments/uri-schemes/uri-schemes.xhtml
    namespace uri_schemes
    {
        // Hypertext Transfer Protocol
        const utility::string_t http{ U("http") };
        // Hypertext Transfer Protocol Secure
        const utility::string_t https{ U("https") };
        // WebSocket
        const utility::string_t ws{ U("ws") };
        // WebSocket Secure
        const utility::string_t wss{ U("wss") };
    }

    inline utility::string_t http_scheme(bool secure) { return secure ? uri_schemes::https : uri_schemes::http; }
    inline utility::string_t ws_scheme(bool secure) { return secure ? uri_schemes::wss : uri_schemes::ws; }
}

#endif
