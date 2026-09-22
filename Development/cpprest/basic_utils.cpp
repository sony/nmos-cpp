#include "cpprest/basic_utils.h"

#ifdef _WIN32
#include <climits> // for INT_MAX
#include <windows.h>
#endif

namespace utility
{
    namespace details
    {
        string_t system_narrow_to_us(const std::string& s)
        {
            if (s.empty()) return{};

#ifdef _WIN32
            // narrow platform strings, e.g. std::system_error::what(), are in the ANSI code page,
            // so try that first, unless the ANSI code page is itself UTF-8, which Windows 10 and
            // later allow - in that case it would just repeat the decode that has already failed,
            // and would substitute U+FFFD for each bad byte, losing the very characters at issue
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
            // otherwise fall through to Latin-1
#endif

            // Latin-1 maps each byte to the code point of the same value, so it cannot fail and
            // cannot discard anything, which keeps the conversion lossless and reversible
            // it also agrees with the Windows Western code page (1252) over 0xA0-0xFF,
            // i.e. for the accented letters, so the common case matches on every platform
            return conversions::to_string_t(conversions::latin1_to_utf16(s));
        }
    }
}
