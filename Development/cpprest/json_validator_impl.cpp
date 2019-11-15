#include "cpprest/json_validator.h"

#include "bst/regex.h"
#include "cpprest/basic_utils.h"
#include "cpprest/json.h"
// use of nlohmann/json and pboettch/json-schema-validator should be an implementation detail, i.e. not in a public header file
#include "detail/pragma_warnings.h"
PRAGMA_WARNING_PUSH
PRAGMA_WARNING_DISABLE_CONDITIONAL_EXPRESSION_IS_CONSTANT
#include "nlohmann/json-schema.hpp"
PRAGMA_WARNING_POP

namespace web
{
    namespace json
    {
        namespace experimental
        {
            namespace details
            {
                // see https://stackoverflow.com/a/3824105
                static const bst::regex ipv4_regex(R"((([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])\.){3}([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5]))");
                // see https://stackoverflow.com/a/17871737
                static const bst::regex ipv6_regex(R"((([0-9a-fA-F]{1,4}:){7,7}[0-9a-fA-F]{1,4}|([0-9a-fA-F]{1,4}:){1,7}:|([0-9a-fA-F]{1,4}:){1,6}:[0-9a-fA-F]{1,4}|([0-9a-fA-F]{1,4}:){1,5}(:[0-9a-fA-F]{1,4}){1,2}|([0-9a-fA-F]{1,4}:){1,4}(:[0-9a-fA-F]{1,4}){1,3}|([0-9a-fA-F]{1,4}:){1,3}(:[0-9a-fA-F]{1,4}){1,4}|([0-9a-fA-F]{1,4}:){1,2}(:[0-9a-fA-F]{1,4}){1,5}|[0-9a-fA-F]{1,4}:((:[0-9a-fA-F]{1,4}){1,6})|:((:[0-9a-fA-F]{1,4}){1,7}|:)|fe80:(:[0-9a-fA-F]{0,4}){0,4}%[0-9a-zA-Z]{1,}|::(ffff(:0{1,4}){0,1}:){0,1}((25[0-5]|(2[0-4]|1{0,1}[0-9]){0,1}[0-9])\.){3,3}(25[0-5]|(2[0-4]|1{0,1}[0-9]){0,1}[0-9])|([0-9a-fA-F]{1,4}:){1,4}:((25[0-5]|(2[0-4]|1{0,1}[0-9]){0,1}[0-9])\.){3,3}(25[0-5]|(2[0-4]|1{0,1}[0-9]){0,1}[0-9])))");
#ifdef JSON_VALIDATOR_CHECK_HOSTNAME
                // see https://stackoverflow.com/a/3824105
                static const bst::regex hostname_regex(R"(([a-zA-Z0-9]|[a-zA-Z0-9][a-zA-Z0-9\-]{0,61}[a-zA-Z0-9])(\.([a-zA-Z0-9]|[a-zA-Z0-9][a-zA-Z0-9\-]{0,61}[a-zA-Z0-9]))*\.?)");
#endif

                // string format checking function for use with pboettch/json_schema_validator
                // with some of the defined formats in the JSON Schema specification (draft 4)
                // see https://tools.ietf.org/html/draft-fge-json-schema-validation-00#section-7
                void check_format(const std::string& format, const std::string& value)
                {
                    if (format == "uri")
                    {
                        if (!web::uri::validate(utility::s2us(value)))
                            throw std::invalid_argument(value + " is not a valid uri");
                    }
                    else if (format == "ipv4")
                    {
                        if (!bst::regex_match(value, ipv4_regex))
                            throw std::invalid_argument(value + " is not a valid ipv4");
                    }
                    else if (format == "ipv6")
                    {
                        if (!bst::regex_match(value, ipv6_regex))
                            throw std::invalid_argument(value + " is not a valid ipv6");
                    }
#ifdef JSON_VALIDATOR_CHECK_HOSTNAME
                    // validation of hostnames is disabled due to the unfortunate lack of consistency
                    // between implementations and the bewildering number of possibly relevant RFCs
                    // see https://github.com/sony/nmos-cpp/issues/11
                    else if (format == "hostname")
                    {
                        if (!bst::regex_match(value, hostname_regex))
                            throw std::invalid_argument(value + " is not a valid hostname");
                    }
#endif
                    else if (format == "date-time")
                    {
                        if (utility::datetime() == utility::datetime::from_string(utility::s2us(value), utility::datetime::ISO_8601))
                            throw std::invalid_argument(value + " is not a valid date-time");
                    }
                    else if (format == "regex")
                    {
                        try
                        {
                            bst::regex re(value, bst::regex::ECMAScript);
                        }
                        catch (const bst::regex_error& e)
                        {
                            throw std::invalid_argument(value + " is not a valid regex: " + e.what());
                        }
                    }
                }

                // json validator implementation that uses pboettch/json_schema_validator
                class json_validator_impl
                {
                public:
                    json_validator_impl(std::function<web::json::value(const web::uri&)> load_schema, const std::vector<web::uri>& ids)
                    {
                        for (const auto& id : ids)
                        {
                            nlohmann::json_schema::json_validator validator
                            {
                                [load_schema](const nlohmann::json_uri& id_impl, nlohmann::json& value_impl)
                                {
                                    const auto id = web::uri(utility::s2us(id_impl.url()));
                                    const auto value = load_schema(id);
                                    value_impl = nlohmann::json::parse(utility::us2s(value.serialize()));
                                },
                                check_format
                            };

                            validator.set_root_schema(
                            {
                                { "$ref", utility::us2s(id.to_string()) }
                            });

                            validators.insert(std::make_pair(id, std::move(validator)));
                        }
                    }

                    void validate(const web::json::value& value, const web::uri& id) const
                    {
                        auto validator = validators.find(id);
                        if (validators.end() == validator)
                        {
                            throw web::json::json_exception("schema not found for " + utility::us2s(id.to_string()));
                        }

                        struct error_handler : nlohmann::json_schema::error_handler
                        {
                            virtual void error(const nlohmann::json::json_pointer& ptr, const nlohmann::json& instance, const std::string& message)
                            {
                                throw web::json::json_exception("schema validation failed at " + (nlohmann::json::json_pointer() == ptr ? "root" : ptr.to_string()) + " - " + message + " JSON - " + instance.dump());
                            }
                        } error_handler;

                        try
                        {
                            auto instance = nlohmann::json::parse(utility::us2s(value.serialize()));
                            validator->second.validate(instance, error_handler);
                        }
                        catch (const web::json::json_exception&)
                        {
                            throw;
                        }
                        catch (const std::exception& e)
                        {
                            throw web::json::json_exception(e.what());
                        }
                    }

                private:
                    std::map<web::uri, nlohmann::json_schema::json_validator> validators;
                };
            }

            // initialize for the specified base URIs using the specified loader
            json_validator::json_validator(std::function<web::json::value(const web::uri&)> load_schema, const std::vector<web::uri>& ids)
                : impl(new details::json_validator_impl(load_schema, ids))
            {
            }

            // validate the specified instance with the schema identified by the specified base URI
            void json_validator::validate(const web::json::value& value, const web::uri& id) const
            {
                impl->validate(value, id);
            }
        }
    }
}
