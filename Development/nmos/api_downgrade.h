#ifndef NMOS_API_DOWNGRADE_H
#define NMOS_API_DOWNGRADE_H

#include "cpprest/json.h"

// "Downgrade queries permit old-versioned responses to be provided to clients which are confident
// that they can handle any missing attributes between the specified API versions."
// See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/docs/2.5.%20APIs%20-%20Query%20Parameters.md#downgrade-queries
namespace nmos
{
    struct api_version;
    struct resource;
    struct type;

    bool is_permitted_downgrade(const nmos::resource& resource, const nmos::api_version& version);
    bool is_permitted_downgrade(const nmos::resource& resource, const nmos::api_version& version, const nmos::api_version& downgrade_version);
    bool is_permitted_downgrade(const nmos::api_version& resource_version, const nmos::type& resource_type, const nmos::api_version& version, const nmos::api_version& downgrade_version);

    web::json::value downgrade(const nmos::resource& resource, const nmos::api_version& version);
    web::json::value downgrade(const nmos::resource& resource, const nmos::api_version& version, const nmos::api_version& downgrade_version);
    web::json::value downgrade(const nmos::api_version& resource_version, const nmos::type& resource_type, const web::json::value& resource_data, const nmos::api_version& version, const nmos::api_version& downgrade_version);
}

#endif
