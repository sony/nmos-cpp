// The first "test" is of course whether the header compiles standalone
#include "nmos/jwt_generator.h"

#include "bst/test/test.h"
#include "cpprest/basic_utils.h"
#include "nmos/jwk_utils.h" // for nmos::experimental::jwk_exception

namespace
{
    // this is the private key (rsa.api.testsuite.nmos.tv.key.pem) from the nmos-testing
    // https://https://github.com/AMWA-TV/nmos-testing/blob/master/test_data/BCP00301/ca/intermediate/private/rsa.api.testsuite.nmos.tv.key.pem
    const auto test_private_key = utility::s2us(R"(-----BEGIN RSA PRIVATE KEY-----
MIIEogIBAAKCAQEAyXHgphlqcINx+ZKkBefDo5X5rHUuTpom9OcRKpWQHt7oYUr1
UhoKJ+8SxbsSvtlrvvGa6kiSk/m6i7haU9dGKSDJzndYJSi+Qbc2jfSPfoHtHvsy
PIworhKniDA7YNE+olr23KGYSqdWidp3nzQLdaHuvOqjjjb3Jm2hvdt4Rfyk8r90
5FY1kdZ/rINtvUDNHZnno9xPw9Hk+xc/cfOJyLUxBndy5wSp7Dhl8Wg1tLuK0rIG
JuFBFrZWykGySGP8s3KzeSeugojYa4JWoXFix6+hlTOfyyu5VXtDkTIZotXcAOBl
EEFLNtSko0yzWuSDo1HF0IwwCvgmwFnewdgFGQIDAQABAoIBAGnZ2ebNsh1/JHO0
91VXDHk4BGL3jCanX9MOW/nZb0qZbNg68B99KVsEiAO4okgArVo/UFzNV6BD+B8U
9vnZQ7e2z/QayAl2mEqlwBflq0UZdoTyD9q692FI0hmA5qKgMN5VGCSlEQYhWhrD
3lmcmmzscyt3zAudnE7oCrZdzZxQD1r2dgjuLFiSaueUHPS/7FQavMx5sGGowcmN
ex+nEHEBeRn5Ws7NWKtX6UoFa8btJIapsqkLHgtXgEsrbFDfLXbEgdFgXg36e8Ak
lCuzUsehaM56eesVNyT/4FVN9ilJu0I1OlJs7sNXOIR4v//PqWoEnEGV/Fwb0YPT
YWPr81ECgYEA7jIIQD9TKYdBdR2A5b8AmRAuVpYsC36Cw6n/3Mq/Le1Bf0nj1cVf
JBkmLOYO/41h6Z+kITJfUZMniy6a/D7LXMSd/jwJM7WKLBfM/AIQHsSpIo/bkW/Q
zVa2inDLYnICWAuR2KNWR46CWy6wnjlWdag76YYJxlk91vpy6RqUCtUCgYEA2ICb
fBp5DNayESM7zDjh8PvgMn49tj6BjuO+oJ7vycQzDlgp/NV2kwtRpNQUcT4wAvTJ
W/GHOlUTxIDxhZqJu3Ix5ue/R0YprhlZofSzwXVoqh+NvZGi4UYiJkTr7zXhzoU1
PV5vWb1YMqpiYJxA3BRj3K0YdVmPkdcoLsXgKzUCgYAuBh7QAyxXbtn3/h5kxfYg
nR7G/jc+dVBg7B0TFV3BSwGHzcgnCv7qI63bqQwm1rOfh4gYHfqK8YsHepbZvGxg
3WDFueXxRteO0355BxEEUO15TyCWxmsq8eFNeKPjvrGzP3EL0eue4etQIQJhYCTT
kREaexqyZ5XqTvQbFFacjQKBgEhC1KKda12/ovtZWTIWokL+rpvryskzH6cDmLKf
mcUsOSZGgu0iiksV8hAjwRby/K9f6H1JpirwDoL9zp8bL3Fi8gjxvMQbRPoY9/O4
au7dMyvlEDf/je/Gqss/IchboZx+lYCALoYzTmbKu78nJ/bMz2/uTkWMuQCiYYUL
AoEpAoGAYIG2aCsuLV+1bPHC0vYvDC0V+NMA8e9HrplHdrQ4IBxyZnmHqvrGZ+04
huUpDxAX+hxYap0k0RPJ3HaFmIHz7DpStX/aIjcfucEnoev7OLj4/3j6s2tHfeWL
JP+1+v/YSIKc7WXvz95YsmoJZ02Ikv8zBan9HIzczmkqDe0C1RQ=
-----END RSA PRIVATE KEY-----
)");

}

BST_TEST_CASE(testClientAssertion)
{
    using namespace nmos::experimental;

    const utility::string_t issuer{ U("api.testsuite.nmos.tv") };
    const utility::string_t subject{ U("api.testsuite.nmos.tv") };
    const web::uri audience{ U("https://mocks.testsuite.nmos.tv:5010/testtoken") };
    const std::chrono::seconds token_lifetime{ 100 };
    const utility::string_t private_key{ test_private_key };
    const utility::string_t keyid{ U("key_1") };

    // bad cases
    const utility::string_t bad_issuer{};
    BST_REQUIRE_THROW(jwt_generator::create_client_assertion(bad_issuer, subject, audience, token_lifetime, private_key, keyid), nmos::experimental::jwk_exception);

    const utility::string_t bad_subject{};
    BST_REQUIRE_THROW(jwt_generator::create_client_assertion(issuer, bad_subject, audience, token_lifetime, private_key, keyid), nmos::experimental::jwk_exception);

    const utility::string_t bad_audience{};
    BST_REQUIRE_THROW(jwt_generator::create_client_assertion(issuer, subject, bad_audience, token_lifetime, private_key, keyid), nmos::experimental::jwk_exception);

    const utility::string_t bad_private_key{};
    BST_REQUIRE_THROW(jwt_generator::create_client_assertion(issuer, subject, audience, token_lifetime, bad_private_key, keyid), nmos::experimental::jwk_exception);

    // good case
    BST_REQUIRE_NO_THROW(jwt_generator::create_client_assertion(issuer, subject, audience, token_lifetime, private_key, keyid));
}
