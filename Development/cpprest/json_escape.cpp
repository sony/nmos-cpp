#include "cpprest/json_escape.h"

namespace web
{
    namespace json
    {
        namespace details
        {
            // escape characters inside a JSON string
            template <typename CharType>
            inline std::basic_string<CharType> escape_characters(const std::basic_string<CharType>& unescaped)
            {
                std::basic_string<CharType> escaped;
                escaped.reserve(unescaped.size());
                for (const auto& c : unescaped)
                {
                    // Grammar for characters in JSON strings:
                    // character
                    //     '0020' . '10FFFF' - '"' - '\'
                    //     '\' escape
                    // 
                    // escape
                    //     '"' (0022 - quotation mark)
                    //     '\' (005C - reverse solidus)
                    //     '/' (002F - solidus - not required)
                    //     'b' (0008 - backspace)
                    //     'f' (000C - formfeed)
                    //     'n' (000A - newline)
                    //     'r' (000D - carriage return)
                    //     't' (0009 - horizontal tab)
                    //     'u' hex hex hex hex (Unicode code point up to FFFF)
                    // See https://www.json.org/
                    switch (c)
                    {
                    case '\"': escaped += '\\'; escaped += '\"'; break;
                    case '\\': escaped += '\\'; escaped += '\\'; break;
                    case '\b': escaped += '\\'; escaped += 'b'; break;
                    case '\f': escaped += '\\'; escaped += 'f'; break;
                    case '\n': escaped += '\\'; escaped += 'n'; break;
                    case '\r': escaped += '\\'; escaped += 'r'; break;
                    case '\t': escaped += '\\'; escaped += 't'; break;
                    default:
                        // when working with UTF-8 byte-by-byte (or UTF-16 on Windows) multi-byte encoded code points must be
                        // left as-is, so only use the Unicode escape form for the control characters not covered above!
                        if (0 <= c && c < 0x20)
                        {
                            static const CharType hex[] = {
                                '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
                            };
                            escaped += '\\'; escaped += 'u';
                            escaped += '0';
                            escaped += '0';
                            escaped += hex[(c & 0xF0) >> 4];
                            escaped += hex[(c & 0x0F)];
                        }
                        else
                        {
                            escaped += c;
                        }
                        break;
                    }
                }
                return escaped;
            }

            utility::string_t escape_characters(const utility::string_t& unescaped)
            {
                return escape_characters<>(unescaped);
            }
#ifdef _UTF16_STRINGS
            std::string escape_characters(const std::string& unescaped)
            {
                return escape_characters<>(unescaped);
            }
#endif
        }
    }
}
