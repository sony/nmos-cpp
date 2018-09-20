#ifndef NMOS_TRANSFER_CHARACTERISTIC_H
#define NMOS_TRANSFER_CHARACTERISTIC_H

#include "nmos/string_enum.h"

namespace nmos
{
    // Transfer characteristic (used in video flows)
    // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/APIs/schemas/flow_video.json
    DEFINE_STRING_ENUM(transfer_characteristic)
    namespace transfer_characteristics
    {
        const transfer_characteristic none{};

        const transfer_characteristic SDR{ U("SDR") };
        const transfer_characteristic HLG{ U("HLG") };
        const transfer_characteristic PQ{ U("PQ") };
    }
}

#endif
