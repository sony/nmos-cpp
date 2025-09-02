#ifndef NMOS_RWNODE_API_H
#define NMOS_RWNODE_API_H

#include "cpprest/api_router.h"

namespace slog
{
    class base_gate;
}

// Annotation API implementation
// See https://specs.amwa.tv/is-13/branches/v1.0-dev/APIs/AnnotationAPI.html
namespace nmos
{
    struct model;
    struct resource;

    // Annotation API callbacks

    // an annotation_patch_merger validates the specified patch data for the specified IS-04 resource and updates the object to be merged
    // or may throw std::runtime_error, which will be mapped to a 500 Internal Error status code with NMOS error "debug" information including the exception message
    // (the default patch merger, nmos::merge_annotation_patch, implements the minimum requirements)
    typedef std::function<void(const nmos::resource& resource, web::json::value& value, const web::json::value& patch)> annotation_patch_merger;

    // Annotation API factory functions

    // callbacks from this function are called with the model locked, and may read but should not write directly to the model
    web::http::experimental::listener::api_router make_annotation_api(nmos::model& model, annotation_patch_merger merge_patch, slog::base_gate& gate);

    // Helper functions for the Annotation API callbacks

    namespace details
    {
        typedef std::function<bool(const utility::string_t& name)> annotation_tag_predicate;

        // BCP-002-01 Group Hint tag and BCP-002-02 Asset Distinguishing Information tags are read-only
        // all other tags are read/write
        bool is_read_only_tag(const utility::string_t& name);

        // this function merges the patch into the value with few additional constraints
        // when any fields are reset using null, default values are applied if specified or
        // read-write tags are removed, and label and description are set to the empty string
        void merge_annotation_patch(web::json::value& value, const web::json::value& patch, annotation_tag_predicate is_read_only_tag = &nmos::details::is_read_only_tag, const web::json::value& default_value = {});
    }

    // this is the default patch merger
    inline void merge_annotation_patch(const nmos::resource& resource, web::json::value& value, const web::json::value& patch)
    {
        details::merge_annotation_patch(value, patch);
    }
}

#endif
