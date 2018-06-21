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
                        // see https://stackoverflow.com/a/3824105
                        bst::regex re(R"((([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])\.){3}([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5]))");
                        if (!bst::regex_match(value, re))
                            throw std::invalid_argument(value + " is not a valid ipv4");

                    }
                    else if (format == "ipv6")
                    {
                        // see https://stackoverflow.com/a/17871737
                        bst::regex re(R"((([0-9a-fA-F]{1,4}:){7,7}[0-9a-fA-F]{1,4}|([0-9a-fA-F]{1,4}:){1,7}:|([0-9a-fA-F]{1,4}:){1,6}:[0-9a-fA-F]{1,4}|([0-9a-fA-F]{1,4}:){1,5}(:[0-9a-fA-F]{1,4}){1,2}|([0-9a-fA-F]{1,4}:){1,4}(:[0-9a-fA-F]{1,4}){1,3}|([0-9a-fA-F]{1,4}:){1,3}(:[0-9a-fA-F]{1,4}){1,4}|([0-9a-fA-F]{1,4}:){1,2}(:[0-9a-fA-F]{1,4}){1,5}|[0-9a-fA-F]{1,4}:((:[0-9a-fA-F]{1,4}){1,6})|:((:[0-9a-fA-F]{1,4}){1,7}|:)|fe80:(:[0-9a-fA-F]{0,4}){0,4}%[0-9a-zA-Z]{1,}|::(ffff(:0{1,4}){0,1}:){0,1}((25[0-5]|(2[0-4]|1{0,1}[0-9]){0,1}[0-9])\.){3,3}(25[0-5]|(2[0-4]|1{0,1}[0-9]){0,1}[0-9])|([0-9a-fA-F]{1,4}:){1,4}:((25[0-5]|(2[0-4]|1{0,1}[0-9]){0,1}[0-9])\.){3,3}(25[0-5]|(2[0-4]|1{0,1}[0-9]){0,1}[0-9])))");
                        if (!bst::regex_match(value, re))
                            throw std::invalid_argument(value + " is not a valid ipv6");
                    }
                    else if (format == "hostname")
                    {
                        // see https://stackoverflow.com/a/106223
                        bst::regex re(R"(([a-zA-Z0-9]|[a-zA-Z0-9][a-zA-Z0-9\-]{0,61}[a-zA-Z0-9])(\.([a-zA-Z0-9]|[a-zA-Z0-9][a-zA-Z0-9\-]{0,61}[a-zA-Z0-9]))*)");
                        if (!bst::regex_match(value, re))
                            throw std::invalid_argument(value + " is not a valid hostname");
                    }
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
                    json_validator_impl(std::function<web::json::value(const web::uri&)> load_schema, std::vector<web::uri> ids)
                    {
                        for (auto id : ids)
                        {
                            nlohmann::json_schema_draft4::json_validator validator
                            {
                                [load_schema](const nlohmann::json_uri& id_impl, nlohmann::json& value_impl)
                                {
                                    const auto id = web::uri(utility::s2us(id_impl.to_string()));
                                    const auto value = load_schema(id);
                                    value_impl = nlohmann::json::parse(utility::us2s(value.serialize()));
                                },
                                check_format
                            };

                            validator.set_root_schema(
                            {
                                { "$ref", utility::us2s(id.to_string()) }
                            });

                            validators[id] = validator;
                        }
                    }

                    void validate(const web::json::value& value, const web::uri& id) const
                    {
                        auto validator = validators.find(id);
                        if (validators.end() == validator)
                        {
                            throw web::json::json_exception((_XPLATSTR("schema not found for ") + id.to_string()).c_str());
                        }

                        try
                        {
                            auto instance = nlohmann::json::parse(utility::us2s(value.serialize()));
                            const_cast<nlohmann::json_schema_draft4::json_validator&>(validator->second).validate(instance);
                        }
                        catch (const std::exception& e)
                        {
                            throw web::json::json_exception(utility::s2us(e.what()).c_str());
                        }
                    }

                private:
                    std::map<web::uri, nlohmann::json_schema_draft4::json_validator> validators;
                };
            }

            // initialize for the specified base URIs using the specified loader
            json_validator::json_validator(std::function<web::json::value(const web::uri&)> load_schema, std::vector<web::uri> ids)
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
