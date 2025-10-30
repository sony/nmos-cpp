#ifndef NMOS_SCOPE_H
#define NMOS_SCOPE_H

#include "nmos/string_enum.h"

namespace nmos
{
    // experimental extension, for BCP-003-02 Authorization
    namespace experimental
    {
        // scope (used in JWT)
        DEFINE_STRING_ENUM(scope)
        namespace scopes
        {
            // IS-04
            const scope registration{ U("registration") };
            const scope query{ U("query") };
            const scope node{ U("node") };
            // IS-05
            const scope connection{ U("connection") };
            // IS-06
            const scope netctrl{ U("netctrl") };
            // IS-07
            const scope events{ U("events") };
            // IS-08
            const scope channelmapping{ U("channelmapping") };
            // IS-11
            const scope streamcompatibility{ U("streamcompatibility") };
            // IS-12
            const scope ncp{ U("ncp") };
            // IS-14
            const scope configuration{ U("configuration") };
        }

        inline utility::string_t make_scope(const scope& scope)
        {
            return scope.name;
        }

        inline scope parse_scope(const utility::string_t& scope)
        {
            if (scopes::registration.name == scope) { return scopes::registration; }
            if (scopes::query.name == scope) { return scopes::query; }
            if (scopes::node.name == scope) { return scopes::node; }
            if (scopes::connection.name == scope) { return scopes::connection; }
            if (scopes::netctrl.name == scope) { return scopes::netctrl; }
            if (scopes::events.name == scope) { return scopes::events; }
            if (scopes::channelmapping.name == scope) { return scopes::channelmapping; }
            if (scopes::streamcompatibility.name == scope) { return scopes::streamcompatibility; }
            if (scopes::ncp.name == scope) { return scopes::ncp; }
            if (scopes::configuration.name == scope) { return scopes::configuration; }
            return{};
        }
    }
}

#endif
