#ifndef NMOS_CONSTRAINTS_H
#define NMOS_CONSTRAINTS_H

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
        bool is_subconstraint(const web::json::value& constraint, const web::json::value& subconstraint);
        bool is_constraint_subset(const web::json::value& constraint_set, const web::json::value& constraint_subset);
    }
}

#endif
