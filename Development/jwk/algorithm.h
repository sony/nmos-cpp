#ifndef JWK_ALGORITHM_H
#define JWK_ALGORITHM_H

#include "nmos/string_enum.h"

namespace jwk
{
    DEFINE_STRING_ENUM(algorithm)
    namespace algorithms
    {
		// RS256/RS384/RS512
        const algorithm RS256{ U("RS256") };
        const algorithm RS384{ U("RS384") };
        const algorithm RS512{ U("RS512") };
    }
}

#endif
