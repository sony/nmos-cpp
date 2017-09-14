#ifndef NMOS_QUERY_UTILS_H
#define NMOS_QUERY_UTILS_H

#include "cpprest/basic_utils.h" // for utility::ostringstreamed, etc.
#include "cpprest/json_utils.h" // for web::json::field_as_string_or, etc.
#include "nmos/resources.h" // for nmos::resources

namespace nmos
{
    // Helpers for paged APIs

    // Offset-limit paging is the traditional kind, which works well for relatively static data
    template <typename Predicate, typename ArgT = typename Predicate::argument_type>
    struct offset_limit_paged
    {
        typedef ArgT argument_type;
        typedef bool result_type;

        Predicate pred;
        size_t offset;
        size_t limit;
        size_t count;

        offset_limit_paged(const Predicate& pred, size_t offset, size_t limit)
            : pred(pred)
            , offset(offset)
            , limit(limit)
            , count(0)
        {}

        bool operator()(argument_type arg)
        {
            const bool in_page = offset <= count && count - offset < limit;
            const bool match = pred(arg);
            if (match) ++count;
            return in_page && match;
        }
    };

    template <typename ArgT, typename Predicate>
    inline offset_limit_paged<Predicate, ArgT> paged(Predicate pred, size_t offset = 0, size_t limit = (std::numeric_limits<size_t>::max)())
    {
        return{ pred, offset, limit };
    }

    namespace fields
    {
        // why not use web::json::field_as_integer_or?
        struct paging_integer_field
        {
            utility::string_t field;
            int or;
            int operator()(const web::json::value& value) const
            {
                return utility::istringstreamed<int>(web::json::field_as_string_or{ field, utility::ostringstreamed(or) }(value));
            }
        };
        const paging_integer_field offset{ U("offset"), 0 };
        const paging_integer_field limit{ U("limit"), 30 };
    }

    // Helpers for advanced query options

    namespace experimental
    {
        web::json::match_flag_type parse_match_type(const utility::string_t& match_type);
    }

    // Predicate to match resources against a query
    struct resource_query
    {
        typedef const nmos::resource& argument_type;
        typedef bool result_type;

        // resource_path may be empty (matching all resource types) or e.g. "/nodes"
        resource_query(const nmos::api_version& version, const utility::string_t& resource_path, const web::json::value& flat_query_params);

        result_type operator()(argument_type resource) const { return (*this)(resource.version, resource.type, resource.data); }

        result_type operator()(const nmos::api_version& resource_version, const nmos::type& resource_type, const web::json::value& resource_data) const;

        nmos::api_version version;
        utility::string_t resource_path;
        web::json::value basic_query;
        nmos::api_version downgrade_version;
        web::json::value rql_query;
        web::json::match_flag_type match_flags;
    };

    // Helpers for constructing /subscriptions websocket grains

    // resource_path may be empty (matching all resource types) or e.g. "/nodes"
    web::json::value make_resource_event(const utility::string_t& resource_path, const nmos::type& type, const web::json::value& pre, const web::json::value& post);

    void insert_resource_events(nmos::resources& resources, const nmos::api_version& version, const nmos::type& type, const web::json::value& pre, const web::json::value& post);

    inline web::json::value& websocket_message(nmos::resource& websocket)
    {
        return websocket.data[U("message")];
    }

    inline const web::json::value& websocket_message(const nmos::resource& websocket)
    {
        return websocket.data.at(U("message"));
    }

    inline web::json::value& websocket_resource_events(nmos::resource& websocket)
    {
        return websocket_message(websocket)[U("grain")][U("data")];
    }

    inline const web::json::value& websocket_resource_events(const nmos::resource& websocket)
    {
        return websocket_message(websocket).at(U("grain")).at(U("data"));
    }
}

#endif
