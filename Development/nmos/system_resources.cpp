#include "nmos/system_resources.h"

#include "cpprest/json_validator.h"
#include "nmos/is09_versions.h"
#include "nmos/json_fields.h"
#include "nmos/json_schema.h"
#include "nmos/resource.h"

namespace nmos
{
    namespace details
    {
        static const web::json::experimental::json_validator& system_validator()
        {
            // hmm, could be based on supported API versions from settings, like other APIs' validators?
            static const web::json::experimental::json_validator validator
            {
                nmos::experimental::load_json_schema,
                boost::copy_range<std::vector<web::uri>>(is09_versions::all | boost::adaptors::transformed(experimental::make_systemapi_global_schema_uri))
            };
            return validator;
        }
    }

    nmos::resource make_system_global(const nmos::id& id, const nmos::settings& settings)
    {
        const auto version = is09_versions::v1_0;
        const auto data = make_system_global_data(id, settings);

        details::system_validator().validate(data, experimental::make_systemapi_global_schema_uri(version));

        return{ version, types::global, std::move(data), true };
    }

    web::json::value make_system_global_data(const nmos::id& id, const nmos::settings& settings)
    {
        using web::json::value_of;

        const auto& label = nmos::experimental::fields::system_label(settings);
        const auto& description = nmos::experimental::fields::system_description(settings);
        const auto& tags = nmos::experimental::fields::system_tags(settings);
        auto data = details::make_resource_core(id, label, description, tags);

        data[nmos::fields::is04] = value_of({
            { nmos::fields::heartbeat_interval, nmos::fields::registration_heartbeat_interval(settings) }
        });
        data[nmos::fields::ptp] = value_of({
            { nmos::fields::announce_receipt_timeout, nmos::fields::ptp_announce_receipt_timeout(settings) },
            { nmos::fields::domain_number, nmos::fields::ptp_domain_number(settings) }
        });

        const auto& system_syslog_host_name = nmos::experimental::fields::system_syslog_host_name(settings);
        if (!system_syslog_host_name.empty())
        {
            data[nmos::fields::syslog] = value_of({
                { nmos::fields::hostname, system_syslog_host_name },
                { nmos::fields::port, nmos::experimental::fields::system_syslog_port(settings) }
            });
        }

        const auto& system_syslogv2_host_name = nmos::experimental::fields::system_syslogv2_host_name(settings);
        if (!system_syslogv2_host_name.empty())
        {
            data[nmos::fields::syslogv2] = value_of({
                { nmos::fields::hostname, system_syslogv2_host_name },
                { nmos::fields::port, nmos::experimental::fields::system_syslogv2_port(settings) }
            });
        }

        return data;
    }

    std::pair<nmos::id, nmos::settings> parse_system_global_data(const web::json::value& data)
    {
        using web::json::value;
        using web::json::value_of;

        const auto& is04 = nmos::fields::is04(data);
        const auto& ptp = nmos::fields::ptp(data);

        auto settings = value_of({
            { nmos::experimental::fields::system_label, nmos::fields::label(data) },
            { nmos::experimental::fields::system_description, nmos::fields::description(data) },
            { nmos::fields::registration_heartbeat_interval, nmos::fields::heartbeat_interval(is04) },
            { nmos::fields::ptp_announce_receipt_timeout, nmos::fields::announce_receipt_timeout(ptp) },
            { nmos::fields::ptp_domain_number, nmos::fields::domain_number(ptp) },
            { nmos::experimental::fields::system_tags, data.at(nmos::fields::tags) }
        });

        const auto& syslog = nmos::fields::syslog(data);
        if (!syslog.is_null())
        {
            settings[nmos::experimental::fields::system_syslog_host_name] = value::string(nmos::fields::hostname(syslog));
            settings[nmos::experimental::fields::system_syslog_port] = nmos::fields::port(syslog);
        }

        const auto& syslogv2 = nmos::fields::syslogv2(data);
        if (!syslogv2.is_null())
        {
            settings[nmos::experimental::fields::system_syslogv2_host_name] = value::string(nmos::fields::hostname(syslogv2));
            settings[nmos::experimental::fields::system_syslogv2_port] = nmos::fields::port(syslogv2);
        }

        return{ nmos::fields::id(data), std::move(settings) };
    }

    namespace experimental
    {
        // assign a system global configuration resource according to the settings
        void assign_system_global_resource(nmos::resource& resource, const nmos::settings& settings)
        {
            resource = nmos::make_system_global(nmos::make_repeatable_id(nmos::experimental::fields::seed_id(settings), U("/x-nmos/system/global")), settings);
        }
    }
}
