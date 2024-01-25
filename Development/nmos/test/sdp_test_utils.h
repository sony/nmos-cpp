#ifndef NMOS_SDP_TEST_UTILS_H
#define NMOS_SDP_TEST_UTILS_H

namespace nmos
{
    struct sdp_parameters;
}

namespace sdp_test
{
    void check_sdp_parameters(const nmos::sdp_parameters& lhs, const nmos::sdp_parameters& rhs);
}

#endif
