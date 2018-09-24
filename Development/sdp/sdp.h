#ifndef SDP_SDP_H
#define SDP_SDP_H

#include "cpprest/json.h"

// SDP: Session Description Protocol
// See https://tools.ietf.org/html/rfc4566
namespace sdp
{
    // make a complete SDP session description out of its json representation, using the default attribute formatters
    std::string make_session_description(const web::json::value& session_description);

    // parse a complete SDP session description into its json representation, using the default attribute parsers
    web::json::value parse_session_description(const std::string& session_description);
}

#endif
