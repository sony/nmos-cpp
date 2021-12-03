#ifndef NMOS_TRANSFER_CHARACTERISTIC_H
#define NMOS_TRANSFER_CHARACTERISTIC_H

#include "nmos/string_enum.h"

namespace nmos
{
    // Transfer characteristic (used in video flows)
    // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/APIs/schemas/flow_video.json
    // and https://github.com/AMWA-TV/nmos-parameter-registers/tree/main/flow-attributes#transfer-characteristic
    DEFINE_STRING_ENUM(transfer_characteristic)
    namespace transfer_characteristics
    {
        const transfer_characteristic none{};

        // Standard Dynamic Range
        const transfer_characteristic SDR{ U("SDR") };
        // Perceptual Quantization
        const transfer_characteristic PQ{ U("PQ") };
        // Hybrid Log Gamma
        const transfer_characteristic HLG{ U("HLG") };

        // Since IS-04 v1.3, transfer_characteristic values may be defined in the Flow Attributes register of the NMOS Parameter Registers

        // Video streams of linear encoded floating-point samples (depth=16f), such that all values fall within the range [0..1.0]
        const transfer_characteristic LINEAR{ U("LINEAR") };
        // Video Stream of linear encoded floating-point samples (depth=16f) normalized from PQ as specified in ITU-R BT.2100-0
        const transfer_characteristic BT2100LINPQ{ U("BT2100LINPQ") };
        // Video Stream of linear encoded floating-point samples (depth=16f) normalized from HLG as specified in ITU-R BT.2100-0
        const transfer_characteristic BT2100LINHLG{ U("BT2100LINHLG") };
        // Video stream of linear encoded floating-point samples (depth=16f) as specified in SMPTE ST 2065-1
        const transfer_characteristic ST2065_1{ U("ST2065-1") };
        // Video stream utilizing the transfer characteristic specified in SMPTE ST 428-1 Section 4.3
        const transfer_characteristic ST428_1{ U("ST428-1") };
        // Video streams of density encoded samples, such as those defined in SMPTE ST 2065-3
        const transfer_characteristic DENSITY{ U("DENSITY") };
    }
}

#endif
