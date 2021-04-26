#ifndef CPPREST_JSON_VALIDATOR_H
#define CPPREST_JSON_VALIDATOR_H

#include "cpprest/base_uri.h"

namespace web
{
    namespace json
    {
        class value;

        namespace experimental
        {
            // json validator implementation
            namespace details
            {
                class json_validator_impl;
            }

            class json_validator
            {
            public:
                // initialize for the specified base URIs using the specified loader
                explicit json_validator(std::function<web::json::value(const web::uri&)> load_schema, const std::vector<web::uri>& ids = {});

                // validate the specified instance with the schema identified by the specified base URI
                void validate(const web::json::value& value, const web::uri& id = {}) const;

            private:
                std::shared_ptr<details::json_validator_impl> impl;
            };
        }
    }
}

#endif
