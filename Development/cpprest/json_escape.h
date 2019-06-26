#ifndef CPPREST_JSON_ESCAPE_H
#define CPPREST_JSON_ESCAPE_H

#include "cpprest/details/basic_types.h" // for utility::string_t

namespace web
{
    namespace json
    {
        class value;

        namespace details
        {
            bool has_escape_chars(const web::json::value& string_value);

            // escape characters inside a string for serialization
            utility::string_t escape_characters(const utility::string_t& unescaped);
#ifdef _UTF16_STRINGS
            std::string escape_characters(const std::string& unescaped);
#endif
        }
    }
}

#endif
