#ifndef NMOS_SDP_UTILS_H
#define NMOS_SDP_UTILS_H

#include "cpprest/json.h"

namespace nmos
{
    // Get transport parameters from the parsed SDP file
    web::json::value get_session_description_transport_params(const web::json::value& session_description, bool smpte2022_7);
}

#endif
