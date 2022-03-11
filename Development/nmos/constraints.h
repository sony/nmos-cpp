#ifndef NMOS_CONSTRAINTS_H
#define NMOS_CONSTRAINTS_H

#include "nmos/capabilities.h"
#include "nmos/json_fields.h"

namespace web
{
    namespace json
    {
        class value;
    }
}

namespace nmos
{
    namespace experimental
    {
        bool match_source_parameters_constraint_set(const web::json::value& source, const web::json::value& constraint_set);
        bool match_flow_parameters_constraint_set(const web::json::value& flow, const web::json::value& constraint_set);
        bool is_constraint_subset(const web::json::value& constraint_set, const web::json::value& constraint_subset);
    }
}

#endif
