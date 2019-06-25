#ifndef CPPREST_JSON_ESCAPE_H
#define CPPREST_JSON_ESCAPE_H

#include "cpprest/details/basic_types.h" // for utility::string_t

namespace web
{
    namespace json
    {
        namespace details
        {
            // escape characters inside a string for serialization
            utility::string_t escape_characters(const utility::string_t& unescaped);
#ifdef _UTF16_STRINGS
            std::string escape_characters(const std::string& unescaped);
#endif
        }
    }
}

#endif
