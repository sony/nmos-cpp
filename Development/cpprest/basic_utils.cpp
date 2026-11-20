#include "cpprest/basic_utils.h"

#include <iterator> // for std::back_inserter
#include <stdexcept> // for std::range_error
// boost::locale::utf is header-only, so don't auto-link the Boost.Locale library
#ifndef BOOST_LOCALE_NO_LIB
#define BOOST_LOCALE_NO_LIB
#endif
#include <boost/locale/utf.hpp>

#ifdef _WIN32
#include <climits> // for INT_MAX
#include <windows.h>
#endif

namespace utility
{
    namespace details
    {
        // decode a narrow string that is not valid UTF-8, using the Windows ANSI code page if there is one,
        // and otherwise replacing each invalid byte with U+FFFD; never throws
        string_t s2us_fallback(const std::string& s)
        {
            if (s.empty()) return{};

#ifdef _WIN32
            // narrow platform strings, e.g. std::system_error::what(), are in the ANSI code page,
            // so try that first, unless the ANSI code page is itself UTF-8, which Windows 10 and
            // later allow - in that case it would just repeat the decode that has already failed
            // MB_ERR_INVALID_CHARS is deliberately not set, since it would reject the bytes we are
            // here for, and a best-effort transliteration beats losing the message
            if (CP_UTF8 != ::GetACP() && s.size() <= (size_t)INT_MAX)
            {
                const int size = ::MultiByteToWideChar(CP_ACP, 0, s.data(), (int)s.size(), NULL, 0);
                if (0 < size)
                {
                    // utf16string is std::wstring on Windows, so this is the native conversion target
                    utf16string ws(size, L'\0');
                    if (0 < ::MultiByteToWideChar(CP_ACP, 0, s.data(), (int)s.size(), &ws[0], size))
                    {
                        return conversions::to_string_t(std::move(ws));
                    }
                }
            }
#endif

            // otherwise, decode as UTF-8, replacing each byte that does not start a valid sequence
            // with U+FFFD, so that the valid text around it is unchanged
            // decode also consumes the byte that cuts a sequence short, which may be valid by itself,
            // so after a failure, decoding resumes at the byte following the one replaced
            namespace utf = boost::locale::utf;
            string_t result;
            result.reserve(s.size());
            auto it = s.begin();
            while (s.end() != it)
            {
                auto next = it;
                auto c = utf::utf_traits<char>::decode(next, s.end());
                if (utf::illegal == c || utf::incomplete == c)
                {
                    c = 0xFFFD;
                    next = it + 1;
                }
                utf::utf_traits<char_t>::encode(c, std::back_inserter(result));
                it = next;
            }
            return result;
        }
    }

    // convert a narrow string of uncertain encoding to string_t; never throws
    // on a wide build, s2us requires valid UTF-8 and throws std::range_error otherwise, so use
    // this instead at boundaries where the narrow string comes from the platform rather than from
    // this codebase, e.g. std::exception::what(), system error messages, or __FILE__, none of
    // which are guaranteed to be UTF-8 - on Windows they are typically in the ANSI code page
    string_t s2us_lenient(const std::string& s)
    {
        try
        {
            // conversions::to_utf16string is the same decoder that s2us itself uses on a wide
            // build, on every platform, so this accepts exactly what s2us accepts and the
            // fallback handles exactly what it would have rejected
#ifdef _UTF16_STRINGS
            return conversions::to_utf16string(s);
#else
            // on a narrow build, s2us returns s unchanged, so the decoded value is not needed
            (void)conversions::to_utf16string(s);
            return s;
#endif
        }
        catch (const std::range_error&)
        {
            return details::s2us_fallback(s);
        }
    }
}
