#ifndef NMOS_RWNODE_API_H
#define NMOS_RWNODE_API_H

#include "cpprest/api_router.h"

namespace slog
{
    class base_gate;
}

// Read/Write Node API implementation
// See https://specs.amwa.tv/is-13/branches/v1.0-dev/APIs/ReadWriteNodeAPI.html
namespace nmos
{
    struct model;
    struct resource;

    // Read/Write Node API callbacks

    // a rwnode_patch_merger validates the specified patch data for the specified IS-04 resource and updates the object to be merged
    // or may throw std::runtime_error, which will be mapped to a 500 Internal Error status code with NMOS error "debug" information including the exception message
    // (the default patch merger, nmos::merge_rwnode_patch, implements the minimum requirements)
    typedef std::function<void(const nmos::resource& resource, web::json::value& value, const web::json::value& patch, slog::base_gate& gate)> rwnode_patch_merger;

    // Read/Write Node API factory functions

    // callbacks from this function are called with the model locked, and may read but should not write directly to the model
    web::http::experimental::listener::api_router make_rwnode_api(nmos::model& model, rwnode_patch_merger merge_patch, slog::base_gate& gate);

    // Helper functions for the Read/Write Node API callbacks

    namespace details
    {
        void merge_rwnode_patch(web::json::value& value, const web::json::value& patch);
    }

    // this function merges the patch into the value with few additional constraints
    // i.e. label, description and all tags are read/write except Group Hint and Asset Distinguishing Information
    // when reset using null, tags are removed, and label and description are set to the empty string
    // (this is the default patch merger)
    inline void merge_rwnode_patch(const nmos::resource& resource, web::json::value& value, const web::json::value& patch, slog::base_gate& gate)
    {
        details::merge_rwnode_patch(value, patch);
    }
}

#endif
