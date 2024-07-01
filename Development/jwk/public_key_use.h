#ifndef JWK_PUBLICKEY_USE_H
#define JWK_PUBLICKEY_USE_H

#include "nmos/string_enum.h"

namespace jwk
{
    DEFINE_STRING_ENUM(public_key_use)
    namespace public_key_uses
    {
        const public_key_use signing{ U("sig") };
        const public_key_use encryption{ U("enc") };
    }
}

#endif
