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
    }
}
