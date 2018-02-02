#ifndef NMOS_RESOURCE_H
#define NMOS_RESOURCE_H

#include <set>
#include "nmos/api_version.h"
#include "nmos/copyable_atomic.h"
#include "nmos/json_fields.h"
#include "nmos/health.h"
#include "nmos/id.h"
#include "nmos/tai.h"
#include "nmos/type.h"

namespace nmos
{
    // Resources have an API version, resource type and representation as json data
    // Everything else is (internal) registry information: their id, references to their sub-resources, creation and update timestamps,
    // and health which is usually propagated from a node, because only nodes get heartbeats and keep all their sub-resources alive

    struct resource_core
    {
        // the API version, type, id and creation timestamp are logically const after construction*, other data may be modified
        // when any data is modified, the update timestamp must be set, and resource events should be generated
        // *or more accurately, after insertion into the registry

        resource_core(api_version version, type type, web::json::value data)
            : version(version)
            , type(type)
            , data(data)
            , id(fields::id(data))
            , created(tai_now())
            , updated(created)
        {}

        // the API version of the Node API or Registration API exposing this resource
        api_version version;

        // see nmos/type.h
        nmos::type type;

        // resource data is stored directly as json rather than e.g. being deserialized to a class hierarchy to allow quick
        // prototyping; json validation at the API boundary would ensure the data met the schema for the specified version
        web::json::value data;

        // could use fields::id(data) but the id is such an important index...
        nmos::id id;

        // sub-resources are tracked in order to optimise resource expiry and deletion
        std::set<nmos::id> sub_resources;

        // see https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/docs/2.5.%20APIs%20-%20Query%20Parameters.md#pagination
        tai created;
        tai updated;
    };

    struct resource : resource_core
    {
        resource(api_version version, nmos::type type, web::json::value data, bool never_expire)
            : resource_core(version, type, data)
            , health(never_expire ? health_forever : created.seconds)
        {}

        // see https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/docs/4.1.%20Behaviour%20-%20Registration.md#heartbeating
        mutable details::copyable_atomic<nmos::health> health;
    };
}

#endif
