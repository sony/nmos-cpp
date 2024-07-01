// The first "test" is of course whether the header compiles standalone
#include "nmos/jwt_validator.h"

#include <boost/range/join.hpp>
#include <cpprest/http_msg.h>
#include <jwt-cpp/jwt.h>
#include "bst/test/test.h"
#include "cpprest/basic_utils.h" // for utility::us2s, utility::s2us
#include "cpprest/json_utils.h"
#include "cpprest/json_validator.h"
#include "nmos/is10_schemas/is10_schemas.h"
#include "nmos/jwk_utils.h"
#include "nmos/scope.h"

namespace
{
    using web::json::value_of;

    // this is the private key (rsa.mocks.testsuite.nmos.tv.key.pem) from the nmos-testing
    // https://github.com/AMWA-TV/nmos-testing/blob/master/test_data/BCP00301/ca/intermediate/private/rsa.mocks.testsuite.nmos.tv.key.pem
    const auto test_private_key = utility::s2us(R"(-----BEGIN RSA PRIVATE KEY-----
MIIEpAIBAAKCAQEA6F36+0gzW9XURoRGzRfFmIZQnCQJS7+sQrUmhPjm+X/gsNAm
zcGlgpG37jr4YbabBotlpOyRmpYfsts/9Ts1UFdqGx/aaONAmldc16arEYwSLNFW
KQ18rac0qGKAiUn2swpDIje8tTKHBE7XVLd5XVtcWrhGqX9liUFSKCb+TBuraMne
na5gA3ruXZZcn+wdHM777QQq6D1Hf9g62stePPzUeNPX+Ttulr+ju8OvQy6MK2Ij
6IHNzPT7McHUE0Z7sk7czWG60kCthRqv16OwuzR3Bhn5blPwRGXjbJJfkN6a7ck+
4bt3LetDw/1idPArF2ONFgMO21ZJp1qm7ZnbcQIDAQABAoIBAQCKdW2XC7emsixx
9GHn1ZFlSCuCTqrHWyf++8g/Fb0z0DIHyZBFrGy996xcpQDZ4KBRbwCbHGfKcEfl
IGXk72nePKg6D2nqc/dLwGDPEz3+D7PIxtgLUEEJjIeBCmjC5bK9jpDgM8wbQEdZ
ls0Sat1Ddqv6VrGsUAAloCmfSVTf7b402o2JpJMlcqjGpZKMVZbSKlTavi6wDm9B
9BMTEER6EypK+B+jtp2CK4Jw8Tnwx7RkVVS731N0vmMqInQCzMhvO1wyeDWZkN3u
7Pzan7xiL9cL0K6icnsgHWsyC9Vq8vxMkQWuqnJ2t8u8FZik4OjD0K4uvtRuSQxU
/Exf0qPRAoGBAPPrKCtb/EMNvUdV2e3d82NsZmaQnOgoqkrpjPol8I/2YJeb7Imi
bx1Ky2qA9opydnm8br3WKGNBJGlPLhenlekaVcIOC+897ijqvy0x7ZhvUVam7N97
Q9iYbI5iArPlDixBUnNDfsQh8ZRJy5NQz2Z6pNILToQMx83A0tKO4WbNAoGBAPPg
WlqUJNfEIuu67DHo7iFulV04CiL43LVGA0QsZiw75qJQvlsusj6pJbUN29rtihp/
F6F09rbHqvEUv8MSp2SywqNpYgcDZxlcxnwj5RQONhkoChXYSGM6FnTVIHxa0Wai
C5JOHwIwy8mHn+roQLIe9g8vkSDKQnLrUYwj7981AoGBAOyKuOrLiqhwM4VxUSUn
H7fkUK3YQgG2Jeb99LRFhLPnpyZ/lHSo7H6IoRnItM3wUMqfnPlGLOaMLsZdfgJ8
h5mF63KD8rjw4vwVIo6uo443LbcNrBrRzCrJLkUp8RsJ36O1OUMESnPjwwYeRmi3
blogR6RWSK8wQbdb7lc5LodlAoGAMuQidrxrY8s+Lkr3dwLQjpFxAd7r3phoFjvh
+pv5RknJux12W7jG4WSSxdF6i5j+NMFIwRyTT1kjRuO5kI+X9t+G1mrrVeNT5GsD
0Gv9Jc5BY8aDNEPJ90rr3L2M5eZdxDkUiRdcSSy9mfR/XpnQxlrHpiua8WjDrQ+G
GOR27fECgYAQkxp8abfj4q57nWHt4Nmr5WDXrCIPNNBQvd596DGOFiSp7IsyPuzt
rKZp5TDgbxdcDIN0Jag78tzY5Ms6SHXpNe648tJBmnSHCFrx8dL95sdGKf4/DtOv
LWWWvv8Ld9XO7GPVLVFgg9wCgkIF9lUgjfhzoalCA1i1L90jcy8WDQ==
-----END RSA PRIVATE KEY-----
)");

    // using openssl to extract the public key of the rsa.mocks.testsuite.nmos.tv.key.pem via private key
    // $ openssl pkey -in rsa.mocks.testsuite.nmos.tv.key.pem -pubout
    const auto test_public_key = utility::s2us(R"(-----BEGIN PUBLIC KEY-----
MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA6F36+0gzW9XURoRGzRfF
mIZQnCQJS7+sQrUmhPjm+X/gsNAmzcGlgpG37jr4YbabBotlpOyRmpYfsts/9Ts1
UFdqGx/aaONAmldc16arEYwSLNFWKQ18rac0qGKAiUn2swpDIje8tTKHBE7XVLd5
XVtcWrhGqX9liUFSKCb+TBuraMnena5gA3ruXZZcn+wdHM777QQq6D1Hf9g62ste
PPzUeNPX+Ttulr+ju8OvQy6MK2Ij6IHNzPT7McHUE0Z7sk7czWG60kCthRqv16Ow
uzR3Bhn5blPwRGXjbJJfkN6a7ck+4bt3LetDw/1idPArF2ONFgMO21ZJp1qm7Znb
cQIDAQAB
-----END PUBLIC KEY-----
)");

    const auto id = web::uri{ U("/test") };
    const auto audience = U("https://api-nmos.testsuite.nmos.tv");
    const utility::string_t key_id{ U("test_key") };

    const auto jwk1 = nmos::experimental::rsa_private_key_to_jwk(test_private_key, key_id, jwk::public_key_uses::signing, jwk::algorithms::RS512);
    const auto pems = value_of({
        value_of({
            { U("jwk"), jwk1 },
            { U("pem"), test_public_key }
        })
    });

    web::json::value make_schema(const char* schema)
    {
        return web::json::value::parse(utility::s2us(schema));
    }

    web::json::experimental::json_validator make_json_validator(const web::json::value& schema, const web::uri& id)
    {
        return web::json::experimental::json_validator
        {
            [&](const web::uri&) { return schema; },
            { id }
        };
    }

    const nmos::experimental::jwt_validator jwt_validator(pems, [](const web::json::value& payload)
    {
        auto token_json_validator = make_json_validator(make_schema(nmos::is10_schemas::v1_0_x::token_schema), id);
        token_json_validator.validate(payload, id);
    });

}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testJWK)
{
    const auto public_key = nmos::experimental::rsa_public_key(test_private_key);
    BST_REQUIRE_EQUAL(test_public_key, public_key);

    const auto jwk2 = nmos::experimental::rsa_public_key_to_jwk(public_key, key_id, jwk::public_key_uses::signing, jwk::algorithms::RS512);
    BST_REQUIRE_EQUAL(jwk1, jwk2);
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testAccessTokenJSON)
{
    // using rsa.mocks.testsuite.nmos.tv.key.pem private key to create an access token via the https://jwt.io/
    // HEADER:
    //{
    //    "typ": "JWT",
    //    "alg" : "RS512"
    //}
    // example PAYLOAD:
    //{
    //    "iss": "https://nmos-mocks.local:5011",
    //    "sub" : "test@testsuite.nmos.tv",
    //    "aud" : [
    //        "https://*.testsuite.nmos.tv",
    //        "https://*.local"
    //    ],
    //    "exp" : 4828204800,
    //    "iat" : 1696868272,
    //    "scope" : "registration",
    //    "client_id" : "458f6d06-46b1-49fd-b778-7c30428889c6",
    //    "x-nmos-registration" : {
    //        "read": [
    //            "*"
    //        ],
    //        "write": [
    //            "*"
    //        ]
    //    }
    //}
    // missing iss(issuer)
    const utility::string_t missing_iss_token = U("eyJ0eXAiOiJKV1QiLCJhbGciOiJSUzUxMiJ9.eyJzdWIiOiJ0ZXN0QHRlc3RzdWl0ZS5ubW9zLnR2IiwiYXVkIjpbImh0dHBzOi8vKi50ZXN0c3VpdGUubm1vcy50diIsImh0dHBzOi8vKi5sb2NhbCJdLCJleHAiOjQ4MjgyMDQ4MDAsImlhdCI6MTY5Njg2ODI3Miwic2NvcGUiOiJyZWdpc3RyYXRpb24iLCJjbGllbnRfaWQiOiI0NThmNmQwNi00NmIxLTQ5ZmQtYjc3OC03YzMwNDI4ODg5YzYiLCJ4LW5tb3MtcmVnaXN0cmF0aW9uIjp7InJlYWQiOlsiKiJdLCJ3cml0ZSI6WyIqIl19fQ.XS9JkK4mmDtjcyrTT0jEWpcssqeU-aZ7xQImL3f-V7KMlOFOQJ5YiT5kXV9Gxb7xYNSJxqn7ym1oL5kNxID_15VZmzsT2h2oVY5x3yOtRcuhLIpD1d4GzXFak5nvR9D6i_fCm5Ov19oF92l7dhMY_DT6HDm89maGJ9DKVxuP1jqVcwFDcXZnGak0MJYETN8nM4xIuRTmmS7W2NpzVKyfw1sCjie2QyptlPoX_KyLaiv2VMkZh-d4Pi9nA9XLjOz-Gyj0-s-NiPx9Qbocpa-eJSqzxz6gtfx8rbSNaqeGV3ehVkGC-0DJq0iIhwxpxp98qYlodz1df8gSDo106OGI_w");
    // missing sub(subject)
    const utility::string_t missing_sub_token = U("eyJ0eXAiOiJKV1QiLCJhbGciOiJSUzUxMiJ9.eyJpc3MiOiJodHRwczovL25tb3MtbW9ja3MubG9jYWw6NTAxMSIsImF1ZCI6WyJodHRwczovLyoudGVzdHN1aXRlLm5tb3MudHYiLCJodHRwczovLyoubG9jYWwiXSwiZXhwIjo0ODI4MjA0ODAwLCJpYXQiOjE2OTY4NjgyNzIsInNjb3BlIjoicmVnaXN0cmF0aW9uIiwiY2xpZW50X2lkIjoiNDU4ZjZkMDYtNDZiMS00OWZkLWI3NzgtN2MzMDQyODg4OWM2IiwieC1ubW9zLXJlZ2lzdHJhdGlvbiI6eyJyZWFkIjpbIioiXSwid3JpdGUiOlsiKiJdfX0.opBKBVFuHXbc6VepFhELRJ7INYWd_W9SBaqfXoe2dMFvCIf0HJNiDnbBrZ9qC3xyyPGR_-Bv7taNTAk67Eirh_P6dv6kPGH-cyTn4G1xCowEiGxFT-nFHyDdV4Ym50avrU6hLRHKGRy5ke0fXXHmcmQETDZpMrQq6wyg0h-kj6KneQAfNCJyqd6-jQu5VuNPsuH54iHiKOLQITOp_WDQ_3-XDQycSdbJJMhdBBnFv-l0qWqDUZAZkkNdJvKxdyhRMB_P7PhhIZck20ylJFbrcjKyMAnUj1O82L9Mriuj23p4jWd0oUiZ9VQBiTtudrrNAON6ZlIjOrBuPWIH7FXQ8w");
    // missing aud(audience)
    const utility::string_t missing_aud_token = U("eyJ0eXAiOiJKV1QiLCJhbGciOiJSUzUxMiJ9.eyJpc3MiOiJodHRwczovL25tb3MtbW9ja3MubG9jYWw6NTAxMSIsInN1YiI6InRlc3RAdGVzdHN1aXRlLm5tb3MudHYiLCJleHAiOjQ4MjgyMDQ4MDAsImlhdCI6MTY5Njg2ODI3Miwic2NvcGUiOiJyZWdpc3RyYXRpb24iLCJjbGllbnRfaWQiOiI0NThmNmQwNi00NmIxLTQ5ZmQtYjc3OC03YzMwNDI4ODg5YzYiLCJ4LW5tb3MtcmVnaXN0cmF0aW9uIjp7InJlYWQiOlsiKiJdLCJ3cml0ZSI6WyIqIl19fQ.Gm4dmsDQOw-e6B5jtBLCKl6LJex41xlMPXaeeKUoZAFj9JUMsv-CpiMIGs9RYvfpPTMJcvrGSJfAHeIkPuUmuYzBkOsD0NFrXqnWg_TmokNZo-FvJ_W3gg2pVVWG4MMTrjs_npdSU6gWBu2GslZraDTphfCo-ooiFJZgR4xPQ5EJiJYHP9m3ZQPLfgIsxX2mvIycFTjuoNuGR-T9lR70vgmfuLacDoZWreKnzSY87Ug_OWanp33kHfuCqhu6X7gTb8DwJDrpEo3Y0b8pNDms9AEDsCyxOnQGdcb4QBvcLciausFov-GLnCS_hJ1F4hpkOIj88RXQCciWpjIyaVwFMQ");
    // missing exp(expiration)
    const utility::string_t missing_exp_token = U("eyJ0eXAiOiJKV1QiLCJhbGciOiJSUzUxMiJ9.eyJpc3MiOiJodHRwczovL25tb3MtbW9ja3MubG9jYWw6NTAxMSIsInN1YiI6InRlc3RAdGVzdHN1aXRlLm5tb3MudHYiLCJhdWQiOlsiaHR0cHM6Ly8qLnRlc3RzdWl0ZS5ubW9zLnR2IiwiaHR0cHM6Ly8qLmxvY2FsIl0sImlhdCI6MTY5Njg2ODI3Miwic2NvcGUiOiJyZWdpc3RyYXRpb24iLCJjbGllbnRfaWQiOiI0NThmNmQwNi00NmIxLTQ5ZmQtYjc3OC03YzMwNDI4ODg5YzYiLCJ4LW5tb3MtcmVnaXN0cmF0aW9uIjp7InJlYWQiOlsiKiJdLCJ3cml0ZSI6WyIqIl19fQ.RQqfKCwOaBXOVH6CvPo13gT5SP8aQAUVorUoe860sSdETor6aXPZyE733OsRjMrspvgV6r6-abW4s1pUDLPcFQBPEU9QhCqGnTmACWkyBDDI2ZFfnC1tqySW7Qd1ZM8oNHNlIJUO7yXtg7YgJyWbr_Nwj-4W_cbhukIeSGBDTjG_Vhcg7O6sRZBVGFni8aqfegHMxnBFGPxfKb70C6sJbXmyb3-ufQYVs-uWbsRJmZyucjdd317lW7OTgi0nn2ZCUzI07EIArfhlJGeK4E0zzROCJbpFJs751IOpte-4lCUeHCJXg9yhS0N_jjIsdKC1G0SEMqAZ-Uo0RJ1FDU5TNg");

    // missing iss(issuer), on GET request
    BST_REQUIRE_THROW(jwt_validator.basic_validation(missing_iss_token), web::json::json_exception);
    // missing sub(subject), on GET request
    BST_REQUIRE_THROW(jwt_validator.basic_validation(missing_sub_token), web::json::json_exception);
    // missing aud(audience), on GET request
    BST_REQUIRE_THROW(jwt_validator.basic_validation(missing_aud_token), web::json::json_exception);
    // missing exp(expiration), on GET request
    BST_REQUIRE_THROW(jwt_validator.basic_validation(missing_exp_token), web::json::json_exception);
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testAccessTokenStandardClaim)
{
    // using rsa.mocks.testsuite.nmos.tv.key.pem private key to create an access token via the https://jwt.io/
    // HEADER:
    //{
    //    "typ": "JWT",
    //    "alg" : "RS512"
    //}
    // example PAYLOAD:
    //{
    //    "iss": "https://nmos-mocks.local:5011",
    //    "sub" : "test@testsuite.nmos.tv",
    //    "aud" : [
    //        "https://*.testsuite.nmos.tv",
    //        "https://*.local"
    //    ],
    //    "exp" : 4828204800,
    //    "iat" : 1696868272,
    //    "scope" : "registration",
    //    "client_id" : "458f6d06-46b1-49fd-b778-7c30428889c6",
    //    "x-nmos-registration" : {
    //        "read": [
    //            "*"
    //        ],
    //        "write": [
    //            "*"
    //        ]
    //    }
    //}
    // invalid iat(00:00:00 1/1/2123)
    const utility::string_t invalid_iat_token = U("eyJ0eXAiOiJKV1QiLCJhbGciOiJSUzUxMiJ9.eyJpc3MiOiJodHRwczovL25tb3MtbW9ja3MubG9jYWw6NTAxMSIsInN1YiI6InRlc3RAdGVzdHN1aXRlLm5tb3MudHYiLCJhdWQiOlsiaHR0cHM6Ly8qLnRlc3RzdWl0ZS5ubW9zLnR2IiwiaHR0cHM6Ly8qLmxvY2FsIl0sImV4cCI6NDgyODIwNDgwMCwiaWF0Ijo0ODI4MjA0ODAwLCJzY29wZSI6InJlZ2lzdHJhdGlvbiIsImNsaWVudF9pZCI6IjQ1OGY2ZDA2LTQ2YjEtNDlmZC1iNzc4LTdjMzA0Mjg4ODljNiIsIngtbm1vcy1yZWdpc3RyYXRpb24iOnsicmVhZCI6WyIqIl0sIndyaXRlIjpbIioiXX19.pZIddJ8wtSXR4KerpmxChWiqPCIrvPj0ZsrpkrBdOvfgP_rDC-Sy7LLnP4hPEMwGqdnZKK9hJGa1uGRz2O971jwbM-n2UPzfbVpyn66A5OLnppizuWcUIij_zS0ZiXG7Lq4jmZ4vd7GnvCtwpxBKZHSXMCBwps_E7xtg6thZKoTXRIAVPu2InlNyRO5g7BmI5eLZ2vyy5WanHkL29b_lMKEzG8nOw45BdNkRq1uLB6c_aOjR1Ln1Jpcd-DIdfSGSGHLAOGg-aM0R3804W7jtNUugmZ1xyybr6g09CQst4u9A8cNdtHyob5oyCPzlGwU5fnpeYnkaKqH7mADdgC5oyA");
    // missing client_id and azp
    const utility::string_t missing_clientid_azp_token = U("eyJ0eXAiOiJKV1QiLCJhbGciOiJSUzUxMiJ9.eyJpc3MiOiJodHRwczovL25tb3MtbW9ja3MubG9jYWw6NTAxMSIsInN1YiI6InRlc3RAdGVzdHN1aXRlLm5tb3MudHYiLCJhdWQiOlsiaHR0cHM6Ly8qLnRlc3RzdWl0ZS5ubW9zLnR2IiwiaHR0cHM6Ly8qLmxvY2FsIl0sImV4cCI6NDgyODIwNDgwMCwiaWF0IjoxNjk2ODY4MjcyLCJzY29wZSI6InJlZ2lzdHJhdGlvbiIsIngtbm1vcy1yZWdpc3RyYXRpb24iOnsicmVhZCI6WyIqIl0sIndyaXRlIjpbIioiXX19.PZlG03pVQMQMTCyOOSfRHQcQxLL5beDSa6J7yMPk80_KHFhUPzBttGu-cc3j6LH4tcjc_tSCbvAZTW4Po9lF4CgZ-K6DqYuCnKT3S-Q2JUSBILRVy8JcogVT12QtwNzECIIHaQsy2M4t4Geyux5lvMRPQwmfx8QOb4ZuM9_ArEDt9vWdmrJ1l81Luj6XoduwoumyivUUE7ZydFXCE1BCIPA79xOMtidPwbiym0AlSQ00lg0TsRpjcmxcy8E_BXnxKiVyRjy6R9e7eEI3ABqvnDL2KbMd4iOYPmO3Gd3r-KMTTXFx3xcQkDmfw0rAqKofp6H4S5Qhzfk-Qq90Hl6yAQ");
    // missing client_id only azp
    const utility::string_t azp_token = U("eyJ0eXAiOiJKV1QiLCJhbGciOiJSUzUxMiJ9.eyJpc3MiOiJodHRwczovL25tb3MtbW9ja3MubG9jYWw6NTAxMSIsInN1YiI6InRlc3RAdGVzdHN1aXRlLm5tb3MudHYiLCJhdWQiOlsiaHR0cHM6Ly8qLnRlc3RzdWl0ZS5ubW9zLnR2IiwiaHR0cHM6Ly8qLmxvY2FsIl0sImV4cCI6NDgyODIwNDgwMCwiaWF0IjoxNjk2ODY4MjcyLCJzY29wZSI6InJlZ2lzdHJhdGlvbiIsImF6cCI6IjQ1OGY2ZDA2LTQ2YjEtNDlmZC1iNzc4LTdjMzA0Mjg4ODljNiIsIngtbm1vcy1yZWdpc3RyYXRpb24iOnsicmVhZCI6WyIqIl0sIndyaXRlIjpbIioiXX19.njx3yJsJVLO6r8P_U7Eutpr7Iygv5D0T0B9h5kPsryZ8JFc1k3OQ5-ZROeKMTl3an2VvaXXVRJrkzn_k-6W-PwbSW7XMKMNpmeXbOGGu81YFL0bLXoaZrF1Tq6_3ZTjmOj0mHV7kxIXrc239lMRPQu5fAOtLUQFVHR-IdmraWI_1kQCh1UijJSuE2wKSr31PyF2BhfQ3w17JIYWy5SHR9psygUlg9e5EgHrMOpr67gOtrsYtJ1G5enbNYQGSXN6Wcy7U35Py_foqTGk8nmExr5MnEYyUTmfisXYIfKqp6nbYyBPE_ybGUNFx8XsyTW8t_Vqa79hOzKwupx2GuqnutQ");
    // mismacthed client_id and azp
    const utility::string_t mismatch_clientid_azp_token = U("eyJ0eXAiOiJKV1QiLCJhbGciOiJSUzUxMiJ9.eyJpc3MiOiJodHRwczovL25tb3MtbW9ja3MubG9jYWw6NTAxMSIsInN1YiI6InRlc3RAdGVzdHN1aXRlLm5tb3MudHYiLCJhdWQiOlsiaHR0cHM6Ly8qLnRlc3RzdWl0ZS5ubW9zLnR2IiwiaHR0cHM6Ly8qLmxvY2FsIl0sImV4cCI6NDgyODIwNDgwMCwiaWF0IjoxNjk2ODY4MjcyLCJzY29wZSI6InJlZ2lzdHJhdGlvbiIsImNsaWVudF9pZCI6IjQ1OGY2ZDA2LTQ2YjEtNDlmZC1iNzc4LTdjMzA0Mjg4ODljNiIsImF6cCI6IjQ1OGY2ZDA2LTQ2YjEtNDlmZC1iNzc4LTdjMzA0Mjg4ODljNyIsIngtbm1vcy1yZWdpc3RyYXRpb24iOnsicmVhZCI6WyIqIl0sIndyaXRlIjpbIioiXX19.hxs8NR2mMgjky0mkrhV487_stKVdbjxIkYq8kTszSdYrTqOPHZ3e9GC6husO0uirLAroD_yXngTfKekS3SDTMVbDyjNdzDQpC3eXVLQSMg5_Fi3dEHXPWRguOuQ4U6LNX6xoNQVNWWotjbHpndXzKnaySrfS7B2tzcj95pb1f64JPzvkGIWKiZ-STw1sej-T4AQpwO3whMe2_9k_ngB6r5Yvwj33nZfF5SWwiIUkQL4YW3HnSJhW2iz85kWoBrwzeSF8DboE_t2blVN16CMZPI9ZitFEFfTnAAfbx_zsV9sktLjsP2Rg659FqOpZNSo60HX4qr0GfTLPOXDhJDH9yg");
    // bad scope
    const utility::string_t bad_scope_token = U("eyJ0eXAiOiJKV1QiLCJhbGciOiJSUzUxMiJ9.eyJpc3MiOiJodHRwczovL25tb3MtbW9ja3MubG9jYWw6NTAxMSIsInN1YiI6InRlc3RAdGVzdHN1aXRlLm5tb3MudHYiLCJhdWQiOlsiaHR0cHM6Ly8qLnRlc3RzdWl0ZS5ubW9zLnR2IiwiaHR0cHM6Ly8qLmxvY2FsIl0sImV4cCI6NDgyODIwNDgwMCwiaWF0IjoxNjk2ODY4MjcyLCJzY29wZSI6ImJhZCIsImNsaWVudF9pZCI6IjQ1OGY2ZDA2LTQ2YjEtNDlmZC1iNzc4LTdjMzA0Mjg4ODljNiIsIngtbm1vcy1yZWdpc3RyYXRpb24iOnsicmVhZCI6WyIqIl0sIndyaXRlIjpbIioiXX19.udlp4ZfONuajmcIYdopQH4CUg-N1d_Ok5cibhhmG5uS3JS2dK3LyYfdra3tKsrRCGuTYvn7UCcd7XfR1fIh_4413CdQ19o5suU04e_zZRy3guDoLsyVuvY1X8_PSCkz00BgoQ1M4kjMc9bDiCzhl-2iTDxAMma34MGTz5_hCgvdPjv5SZ3k2XCQmkC-_ZI3j3WqTkvEV9XvNAUSAgF5Q-zgRJagyqdGvRBz-XMAG0aJVFEcA_X7j8eP3C5RomCPuoBDectcIysOUZGqgqzbKnJf-UjIMFiVGc2t5WntsPrLQj6OJKQgn4FRY65j0QZQFt-Pam8KIaLpmRAtKO5bRUA");
    // missing optional scope
    const utility::string_t missing_scope_token = U("eyJ0eXAiOiJKV1QiLCJhbGciOiJSUzUxMiJ9.eyJpc3MiOiJodHRwczovL25tb3MtbW9ja3MubG9jYWw6NTAxMSIsInN1YiI6InRlc3RAdGVzdHN1aXRlLm5tb3MudHYiLCJhdWQiOlsiaHR0cHM6Ly8qLnRlc3RzdWl0ZS5ubW9zLnR2IiwiaHR0cHM6Ly8qLmxvY2FsIl0sImV4cCI6NDgyODIwNDgwMCwiaWF0IjoxNjk2ODY4MjcyLCJjbGllbnRfaWQiOiI0NThmNmQwNi00NmIxLTQ5ZmQtYjc3OC03YzMwNDI4ODg5YzYiLCJ4LW5tb3MtcmVnaXN0cmF0aW9uIjp7InJlYWQiOlsiKiJdLCJ3cml0ZSI6WyIqIl19fQ.S-Jhf-q7_eNr733TZao5vHAFeYd2e9ZuLm1isF9fXyqvtodPFAQpiZJcUGZyWyeDsfbe56VWaoI0JubEBU8PFDq8IrqY3g6ySVr8jk-kB-4f9szy9-hmCjWCrZLmxJXgRR9xcYhwzgA7U_Enb5rrSO8afrOYxKxZeqySAKQryIqQYU6aOSzAGKGpkhdtZwzyraQb0LJE0nJrWonEST13Ebzg6LyXD72cISNdUN5miWn77kZ5E5fv_zb-AyvcqBAhM2FxYi6gM8L9Bv6nN-dbFxXZgiaoBkPXn--PfYb5jwiis3w3x79ZcSoUIMr3JLiWPR6U4QI2ApU4V2rEgEZzlg");
    // invalid nbf(not before 00:00:00 1/1/2123)
    const utility::string_t invalid_nbf_token = U("eyJ0eXAiOiJKV1QiLCJhbGciOiJSUzUxMiJ9.eyJpc3MiOiJodHRwczovL25tb3MtbW9ja3MubG9jYWw6NTAxMSIsInN1YiI6InRlc3RAdGVzdHN1aXRlLm5tb3MudHYiLCJhdWQiOlsiaHR0cHM6Ly8qLnRlc3RzdWl0ZS5ubW9zLnR2IiwiaHR0cHM6Ly8qLmxvY2FsIl0sImV4cCI6NDgyODIwNDgwMCwiaWF0IjoxNjk2ODY4MjcyLCJuYmYiOjQ4MjgyMDQ4MDAsInNjb3BlIjoicmVnaXN0cmF0aW9uIiwiY2xpZW50X2lkIjoiNDU4ZjZkMDYtNDZiMS00OWZkLWI3NzgtN2MzMDQyODg4OWM2IiwieC1ubW9zLXJlZ2lzdHJhdGlvbiI6eyJyZWFkIjpbIioiXSwid3JpdGUiOlsiKiJdfX0.WL8XBv2IQB-2TZegIgkJ6oqjH0hkYxeAL3vL_eGE2Xy31U7RKWpq9PkmSfvf4wOe9UNkgEjfc8XIXwdQm4YB5aT8WSXkB9DnXRi6Dr8BJ2v_oRNzT8n75UAnbheqdq9CVNFSy7QVNr95oBGpSeeUL4vRCbGOghjOKUOjNjzuksoLB-52-VNoIRA0T5kwSqaRAL-r0Am8v0ucCzJ1OVtdV-WVMqw9-JrLde9oq_dJPAIZ6no4kfvE2ulxKNu8jRni4L6h3ejlrxdExiQFIt-PGHjeJ8ES8WxIYGdUNxulMiP99ta4pWkGwlRMwNvqa5saflvT1uHXKgOgENpZoZSEew");
    // expired token
    const utility::string_t expired_token = U("eyJ0eXAiOiJKV1QiLCJhbGciOiJSUzUxMiJ9.eyJpc3MiOiJodHRwczovL25tb3MtbW9ja3MubG9jYWw6NTAxMSIsInN1YiI6InRlc3RAdGVzdHN1aXRlLm5tb3MudHYiLCJhdWQiOlsiaHR0cHM6Ly8qLnRlc3RzdWl0ZS5ubW9zLnR2IiwiaHR0cHM6Ly8qLmxvY2FsIl0sImV4cCI6MTY5Njg2OTI3MiwiaWF0IjoxNjk2ODY4MjcyLCJzY29wZSI6InJlZ2lzdHJhdGlvbiIsImNsaWVudF9pZCI6IjQ1OGY2ZDA2LTQ2YjEtNDlmZC1iNzc4LTdjMzA0Mjg4ODljNiIsIngtbm1vcy1yZWdpc3RyYXRpb24iOnsicmVhZCI6WyIqIl0sIndyaXRlIjpbIioiXX19.VAzXD5vkrhumbOAtqgeCJL2quZktwySI8AIE9QUOIm_wIUs7ICvd5vzWhxmgie0SGYdDvX3bQxQyFzCHU2Vccp-v3DSzO4vWx7b8zUJpIL8815dtUVpFr81V1Y8G40Ok-QRJhiJWHNHk3y-dh0AEyWtGjiqnfgMThIw3SnbVk0krUb6d-hTHHmyzk5qFkLGPVRWG2d29tTTKH0j4VY4XD_ONp-M6rTO3zGlCMV2wvlJA8jtuScRzfc5gimfNAZVPiIIqKEQHIGXX1ZaI-iJYIHxKFxXkca5K4r1p0FWaXlGgDTFQEmcgCMw9YRSv3Hl83b8ysQqkkdkkBIVugVm3ew");
    // missing private-claim (x-nmos-*)
    const utility::string_t missing_private_claim_token = U("eyJ0eXAiOiJKV1QiLCJhbGciOiJSUzUxMiJ9.eyJpc3MiOiJodHRwczovL25tb3MtbW9ja3MubG9jYWw6NTAxMSIsInN1YiI6InRlc3RAdGVzdHN1aXRlLm5tb3MudHYiLCJhdWQiOlsiaHR0cHM6Ly8qLnRlc3RzdWl0ZS5ubW9zLnR2IiwiaHR0cHM6Ly8qLmxvY2FsIl0sImV4cCI6NDgyODIwNDgwMCwiaWF0IjoxNjk2ODY4MjcyLCJzY29wZSI6InJlZ2lzdHJhdGlvbiIsImNsaWVudF9pZCI6IjQ1OGY2ZDA2LTQ2YjEtNDlmZC1iNzc4LTdjMzA0Mjg4ODljNiJ9.fyRLkLPIS1WIK5z0FCEuTss7SHAKFtsM6EUB_I7yz1YyIRR4UE-DV2V8YPCNF4dFy-4CzsCpQWUsiGvTtnfwzdcygheiyhB6QyloINJXeSCm-0wB95z285KW6AH5vbVadlBRFkMphsxkDeWP1X7lunkqv3oKei1jFGSNnc3QE4gORoGj4YqAVvUA82X3nV8eI5vNE2XHmBG_HkgTjX_JEqVr-9UcQ1EnqVDPuzrCFaQiFirZCpwg0cRHhVrmJCOrfG-bPIcX3KRfWKCaH5O2n736AwOMFqX7f4VdSbSSx7HcO1CxsmVGwQ-i8fab1IBi4KOvRsSHp3Ti3FxQTEnsEw");
    // valid token (expired at 00:00:00 1/1/2123)
    const utility::string_t valid_token = U("eyJ0eXAiOiJKV1QiLCJhbGciOiJSUzUxMiJ9.eyJpc3MiOiJodHRwczovL25tb3MtbW9ja3MubG9jYWw6NTAxMSIsInN1YiI6InRlc3RAdGVzdHN1aXRlLm5tb3MudHYiLCJhdWQiOlsiaHR0cHM6Ly8qLnRlc3RzdWl0ZS5ubW9zLnR2IiwiaHR0cHM6Ly8qLmxvY2FsIl0sImV4cCI6NDgyODIwNDgwMCwiaWF0IjoxNjk2ODY4MjcyLCJzY29wZSI6InJlZ2lzdHJhdGlvbiIsImNsaWVudF9pZCI6IjQ1OGY2ZDA2LTQ2YjEtNDlmZC1iNzc4LTdjMzA0Mjg4ODljNiIsIngtbm1vcy1yZWdpc3RyYXRpb24iOnsicmVhZCI6WyIqIl0sIndyaXRlIjpbIioiXX19.ybx4VU2E6tuFbWFbCUwKyKm_MPmAXZv70x_2eyuS_Z4qF8rgB0M_yXIJMt_5padA-NPRTd8XIvnq7TLJTYMUV9-F45oQLBBWgiBQh2shsmjYg-1fHCHLxXXdlVLzxennbE38Sm60Jo-u3ZC9yFiYBMaOL5ai6f8bhzNdYaz0xbI8XZaki1pICKgVfpq1XKbXBhUD0quRwfl4PjzKfu0rtAxYc_5IxDWkxJx7BYSHR_lkMaOINda8mkSnim9V7wqkGylOc6b38OoXORtfGJCdmhc_oR9n2jwj_42r4HPo6rEul9_yYUwcYOBG65RLEB3-cbwbj8DNPguHu_TnbzBJsA");

    {
        // invalid nbf(not before 00:00:00 1/1/2123), on GET request
        BST_REQUIRE_THROW(jwt_validator.basic_validation(invalid_nbf_token), jwt::error::token_verification_exception);
        // expired token, on GET request
        BST_REQUIRE_THROW(jwt_validator.basic_validation(expired_token), jwt::error::token_verification_exception);
        // invalid audience, on GET request
        BST_REQUIRE_THROW(nmos::experimental::jwt_validator::registered_claims_validation(valid_token, web::http::methods::GET, U("/x-nmos/registration/v1.3"), nmos::experimental::scopes::registration, U("https://api-nmos.bad_audience.com")), nmos::experimental::insufficient_scope_exception);

        // missing optional scope, on GET request
        BST_REQUIRE_NO_THROW(jwt_validator.basic_validation(missing_scope_token));
        BST_REQUIRE_NO_THROW(nmos::experimental::jwt_validator::registered_claims_validation(missing_scope_token, web::http::methods::GET, U("/x-nmos/registration/v1.3"), nmos::experimental::scopes::registration, audience));
        // missing x-nmos-*, on GET request
        BST_REQUIRE_NO_THROW(jwt_validator.basic_validation(missing_private_claim_token));
        BST_REQUIRE_NO_THROW(nmos::experimental::jwt_validator::registered_claims_validation(missing_private_claim_token, web::http::methods::GET, U("/x-nmos/registration/v1.3"), nmos::experimental::scopes::registration, audience));
        // valid token (expired at 00:00:00 1/1/2123), on GET request
        BST_REQUIRE_NO_THROW(jwt_validator.basic_validation(valid_token));
        BST_REQUIRE_NO_THROW(nmos::experimental::jwt_validator::registered_claims_validation(valid_token, web::http::methods::GET, U("/x-nmos/registration/v1.3"), nmos::experimental::scopes::registration, audience));
    }

    {
        // missing optional scope, on POST request
        BST_REQUIRE_THROW(nmos::experimental::jwt_validator::registered_claims_validation(missing_scope_token, web::http::methods::POST, U("/x-nmos/registration/v1.3"), nmos::experimental::scopes::registration, audience), nmos::experimental::insufficient_scope_exception);
        // missing x-nmos-*, on POST request
        BST_REQUIRE_THROW(nmos::experimental::jwt_validator::registered_claims_validation(missing_private_claim_token, web::http::methods::POST, U("/x-nmos/registration/v1.3"), nmos::experimental::scopes::registration, audience), nmos::experimental::insufficient_scope_exception);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testAccessTokenPrivateClaim1)
{
    // using rsa.mocks.testsuite.nmos.tv.key.pem private key to create an access token via the https://jwt.io/
    // HEADER:
    //{
    //    "typ": "JWT",
    //    "alg" : "RS512"
    //}
    // example PAYLOAD:
    //{
    //    "iss": "https://nmos-mocks.local:5011",
    //    "sub" : "test@testsuite.nmos.tv",
    //    "aud" : [
    //        "https://*.testsuite.nmos.tv",
    //        "https://*.local"
    //    ],
    //    "exp" : 4828204800,
    //    "iat" : 1696868272,
    //    "scope" : "registration",
    //    "client_id" : "458f6d06-46b1-49fd-b778-7c30428889c6",
    //    "x-nmos-registration" : {
    //        "read": [
    //            "*"
    //        ]
    //    }
    //}

    // readonly token
    const utility::string_t readonly_token = U("eyJ0eXAiOiJKV1QiLCJhbGciOiJSUzUxMiJ9.eyJpc3MiOiJodHRwczovL25tb3MtbW9ja3MubG9jYWw6NTAxMSIsInN1YiI6InRlc3RAdGVzdHN1aXRlLm5tb3MudHYiLCJhdWQiOlsiaHR0cHM6Ly8qLnRlc3RzdWl0ZS5ubW9zLnR2IiwiaHR0cHM6Ly8qLmxvY2FsIl0sImV4cCI6NDgyODIwNDgwMCwiaWF0IjoxNjk2ODY4MjcyLCJzY29wZSI6InJlZ2lzdHJhdGlvbiIsImNsaWVudF9pZCI6IjQ1OGY2ZDA2LTQ2YjEtNDlmZC1iNzc4LTdjMzA0Mjg4ODljNiIsIngtbm1vcy1yZWdpc3RyYXRpb24iOnsicmVhZCI6WyIqIl19fQ.0offeC5TooP73p2VedN27DeyHdjXIY-RFZzf2NCsyrB03dX89v2i3eHDF3nl-ZNFviNAlTiEMZqA9Sb6kvUI4jsmwpHRQ19nA9QQBKmYCog_uLvxUcGroxTJ7f9Nj8WIaWM1NZ25ZlylyOtz7QHhmkqNSVr8-eXYx8zVUtOurFUXNTN7UnCZ3ZpKoj9sR5O4bRb-11oxEKoOjQadHq22CN9_8AReKl1e3dx5aILYG1Xf_gvYxWpTfzYcgIVYjxKarE7msCUe6PnXBzJMlpu1Abu2llNQz7eCTAbNNA-PPN5cYFYuEdXSIcd8erkXSAK_8VbyizJRU1hE0uFFx0r3Iw");

    // test x-nmos-*
    // valid token with x-nmos-registration read only set
    BST_REQUIRE_THROW(nmos::experimental::jwt_validator::registered_claims_validation(readonly_token, web::http::methods::POST, U("/x-nmos/registration/v1.3/health/nodes/88888888-4444-4444-4444-cccccccccccc"), nmos::experimental::scopes::registration, audience), nmos::experimental::insufficient_scope_exception);
}

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testAccessTokenPrivateClaim2)
{
    // using rsa.mocks.testsuite.nmos.tv.key.pem private key to create an access token via the https://jwt.io/
    // HEADER:
    //{
    //    "typ": "JWT",
    //    "alg" : "RS512"
    //}
    // example PAYLOAD:
    //{
    //    "iss": "https://nmos-mocks.local:5011",
    //    "sub" : "test@testsuite.nmos.tv",
    //    "aud" : [
    //        "https://*.testsuite.nmos.tv",
    //        "https://*.local"
    //    ],
    //    "exp" : 4828204800,
    //    "iat" : 1696868272,
    //    "scope" : "registration",
    //    "client_id" : "458f6d06-46b1-49fd-b778-7c30428889c6",
    //    "x-nmos-registration" : {
    //        "write": [
    //            "*"
    //        ]
    //    }
    //}

    // valid token
    //    "x-nmos-registration" : {
    //        "write": [
    //            "*"
    //        ]
    //    }
    const utility::string_t valid_token1 = U("eyJ0eXAiOiJKV1QiLCJhbGciOiJSUzUxMiJ9.eyJpc3MiOiJodHRwczovL25tb3MtbW9ja3MubG9jYWw6NTAxMSIsInN1YiI6InRlc3RAdGVzdHN1aXRlLm5tb3MudHYiLCJhdWQiOlsiaHR0cHM6Ly8qLnRlc3RzdWl0ZS5ubW9zLnR2IiwiaHR0cHM6Ly8qLmxvY2FsIl0sImV4cCI6NDgyODIwNDgwMCwiaWF0IjoxNjk2ODY4MjcyLCJzY29wZSI6InJlZ2lzdHJhdGlvbiIsImNsaWVudF9pZCI6IjQ1OGY2ZDA2LTQ2YjEtNDlmZC1iNzc4LTdjMzA0Mjg4ODljNiIsIngtbm1vcy1yZWdpc3RyYXRpb24iOnsid3JpdGUiOlsiKiJdfX0.exiZrwWY1nvxnS_LA0R0YMbzkpQNbzUneKO5ruwkSlqW7XdI3TQgRoiGXW1vDbC8XH4RQCD8SPoS2vuX4rLMfLGZGLtpHUFp3khAvs142Oc6K15ldYGfGpjeyDxSw9syRtl37XiG1MPOygYaqjEOXpI9Ljwj8jzGyJXpLGWzLHPnC9SkNCfe7C1ATjz86938qEW-ksxKP7CCQbNVWy13Trti7ow5jiSSd71rqB448tliNi9CDcd_xlx9SvRXZmvomUQOWhJlAQnwKbT7krk1gWqw2JFtOVblP8sKsQHdLX6wxc6F_pHlwJJmWg-cLs0oOV7PKzokIqw7wHN0fnQLtQ");
    //
    // valid token
    //    "x-nmos-registration" : {
    //        "write": [
    //            "health/*"
    //        ]
    //    }
    const utility::string_t valid_token2 = U("eyJ0eXAiOiJKV1QiLCJhbGciOiJSUzUxMiJ9.eyJpc3MiOiJodHRwczovL25tb3MtbW9ja3MubG9jYWw6NTAxMSIsInN1YiI6InRlc3RAdGVzdHN1aXRlLm5tb3MudHYiLCJhdWQiOlsiaHR0cHM6Ly8qLnRlc3RzdWl0ZS5ubW9zLnR2IiwiaHR0cHM6Ly8qLmxvY2FsIl0sImV4cCI6NDgyODIwNDgwMCwiaWF0IjoxNjk2ODY4MjcyLCJzY29wZSI6InJlZ2lzdHJhdGlvbiIsImNsaWVudF9pZCI6IjQ1OGY2ZDA2LTQ2YjEtNDlmZC1iNzc4LTdjMzA0Mjg4ODljNiIsIngtbm1vcy1yZWdpc3RyYXRpb24iOnsid3JpdGUiOlsiaGVhbHRoLyoiXX19.l-dZnLsODuEyGjpT9tWDq5GpzNFtjAIhLDWZI20yfskzKpW0dagNLFp3sKfZOAZspMp3DLb-lCRIp1fXS9rlkBB6mQ-z3XMf4pJXPFaCkxf3EGEsTtpsoYw6jic9Ue8EYPAx7Ma1ersd6TH41HZDi06K9Ko0vwl7qQ4HzctEXMA53afCkc4vIlChWZ8bFAU6gF2avfzU5nAsLPAGrGATFPG4meCmPFtdjnZBLPwyINOP9rCN3Qw6Hwt5f9Y7obAcbuwK9adTYFDqti9j3hzg8p-AGE4Ixo_ItOw0Kg1D1TowlPm7U2pMz-7S4OmwEq8alktufhLPuX_M3m_W5-37Ew");
    //
    // valid token
    //    "x-nmos-registration" : {
    //        "write": [
    //            "health/nodes/*"
    //        ]
    //    }
    const utility::string_t valid_token3 = U("eyJ0eXAiOiJKV1QiLCJhbGciOiJSUzUxMiJ9.eyJpc3MiOiJodHRwczovL25tb3MtbW9ja3MubG9jYWw6NTAxMSIsInN1YiI6InRlc3RAdGVzdHN1aXRlLm5tb3MudHYiLCJhdWQiOlsiaHR0cHM6Ly8qLnRlc3RzdWl0ZS5ubW9zLnR2IiwiaHR0cHM6Ly8qLmxvY2FsIl0sImV4cCI6NDgyODIwNDgwMCwiaWF0IjoxNjk2ODY4MjcyLCJzY29wZSI6InJlZ2lzdHJhdGlvbiIsImNsaWVudF9pZCI6IjQ1OGY2ZDA2LTQ2YjEtNDlmZC1iNzc4LTdjMzA0Mjg4ODljNiIsIngtbm1vcy1yZWdpc3RyYXRpb24iOnsid3JpdGUiOlsiaGVhbHRoL25vZGVzLyoiXX19.UGcQLz47PYhVFulnKgVoFb6V53bvIwjACPHyvm8P8NCkYcnMvjcyDKPCBnIfEoVT8a9LbCK2rFisgo4Rw3NXhDnfbGRoq-4Dad8TbvFpJfEs-Wcb1GDKeaCuS78NvEW8KhbTXoOD04Yj6vRkLSg_Vk-nalNmpjG1vnUPuLO2DZux36l7Ggaq3kDBcIfDCIicrA7V2cu9qL9EqzgEB2DXtrjZ0y219nkGp7UK6wxdI8_-p1LqvpU7vNJmqserri_waEJ-vWhP3JU8b5aeFuQS946Sjr3PHAAraO0RkDAje20dGPpCE5doMmjNZRIEa529MO-g3LQoZABhUCIr57Z0kA");
    //
    // bad token
    //    "x-nmos-registration" : {
    //        "write": [
    //            "bad/*"
    //        ]
    //    }
    const utility::string_t invalid_token1 = U("eyJ0eXAiOiJKV1QiLCJhbGciOiJSUzUxMiJ9.eyJpc3MiOiJodHRwczovL25tb3MtbW9ja3MubG9jYWw6NTAxMSIsInN1YiI6InRlc3RAdGVzdHN1aXRlLm5tb3MudHYiLCJhdWQiOlsiaHR0cHM6Ly8qLnRlc3RzdWl0ZS5ubW9zLnR2IiwiaHR0cHM6Ly8qLmxvY2FsIl0sImV4cCI6NDgyODIwNDgwMCwiaWF0IjoxNjk2ODY4MjcyLCJzY29wZSI6InJlZ2lzdHJhdGlvbiIsImNsaWVudF9pZCI6IjQ1OGY2ZDA2LTQ2YjEtNDlmZC1iNzc4LTdjMzA0Mjg4ODljNiIsIngtbm1vcy1yZWdpc3RyYXRpb24iOnsid3JpdGUiOlsiYmFkLyoiXX19.nZZ3gmmuvEJhF51EJM3OMwsT9_xDwix0U2y8tGQ6G-3nrIMDjM5zAYk8_IeyOXgI063wcQLwY0s83hYZXjKH4ifEb9xDAGBSaF-lijVQAaAbzTX5aIFEngz6pBloUGpWnS7LUJbDDhX8bBO00dH8Umh88GNaxxfBmKTDBb7CAlRpMjRHVid4MPdDAcO0SkeI8K5_71LitDjoXGXkqd1r_AKFh5jRQvdZuNy-6pkg1xSHS8HRsskNIguIYFEpciw22KMDbVZKSBiWUq1tTjGzwv2fDrEEnQZDvyNHqep6DxOOzrJPQtwZoADcq1simZ6IZFKf0ewo6SMMfOmC7JNcuQ");
    //
    // bad token
    //    "x-nmos-registration" : {
    //        "write": [
    //            "health/bad/*"
    //        ]
    //    }
    const utility::string_t invalid_token2 = U("eyJ0eXAiOiJKV1QiLCJhbGciOiJSUzUxMiJ9.eyJpc3MiOiJodHRwczovL25tb3MtbW9ja3MubG9jYWw6NTAxMSIsInN1YiI6InRlc3RAdGVzdHN1aXRlLm5tb3MudHYiLCJhdWQiOlsiaHR0cHM6Ly8qLnRlc3RzdWl0ZS5ubW9zLnR2IiwiaHR0cHM6Ly8qLmxvY2FsIl0sImV4cCI6NDgyODIwNDgwMCwiaWF0IjoxNjk2ODY4MjcyLCJzY29wZSI6InJlZ2lzdHJhdGlvbiIsImNsaWVudF9pZCI6IjQ1OGY2ZDA2LTQ2YjEtNDlmZC1iNzc4LTdjMzA0Mjg4ODljNiIsIngtbm1vcy1yZWdpc3RyYXRpb24iOnsid3JpdGUiOlsiaGVhbHRoL2JhZC8qIl19fQ.o_5XAUKjv4Dyf7cxvuL6bP8GsFhV5IcscndUYenzmGo50sRw0sHvi7eANMTdoh1HAMvTcAYzdpPRPEsIrk2tvKsEVKQzKjCVXw_uKc_Xew00qEF6nUbCPAPd0TotJXTQKtqP_NIcUsRDFWL4X9wpAJQkPdv9xzE_j3RKmbOv3uQq3iRA-TBSOcgJlsCZ37IGNM-_gyOzyRZSKaaY2xAHuPpEt7Gm88sjRmgerIyRLC9zSFt-5jIYAOXlUSMv1tsQK0BQCvqxF_nppHKyfpQacxDTN-UOiD7DvJWhMTpny0mM0mwFnoS-UyQq_cHPA03BDF9-noYeBqo4VMRMx_gnlA");

    BST_REQUIRE_NO_THROW(nmos::experimental::jwt_validator::registered_claims_validation(valid_token1, web::http::methods::POST, U("/x-nmos/registration/v1.3/health/nodes/88888888-4444-4444-4444-cccccccccccc"), nmos::experimental::scopes::registration, audience));
    BST_REQUIRE_NO_THROW(nmos::experimental::jwt_validator::registered_claims_validation(valid_token2, web::http::methods::POST, U("/x-nmos/registration/v1.3/health/nodes/88888888-4444-4444-4444-cccccccccccc"), nmos::experimental::scopes::registration, audience));
    BST_REQUIRE_NO_THROW(nmos::experimental::jwt_validator::registered_claims_validation(valid_token3, web::http::methods::POST, U("/x-nmos/registration/v1.3/health/nodes/88888888-4444-4444-4444-cccccccccccc"), nmos::experimental::scopes::registration, audience));
    BST_REQUIRE_THROW(nmos::experimental::jwt_validator::registered_claims_validation(invalid_token1, web::http::methods::POST, U("/x-nmos/registration/v1.3/health/nodes/88888888-4444-4444-4444-cccccccccccc"), nmos::experimental::scopes::registration, audience), nmos::experimental::insufficient_scope_exception);
    BST_REQUIRE_THROW(nmos::experimental::jwt_validator::registered_claims_validation(invalid_token2, web::http::methods::POST, U("/x-nmos/registration/v1.3/health/nodes/88888888-4444-4444-4444-cccccccccccc"), nmos::experimental::scopes::registration, audience), nmos::experimental::insufficient_scope_exception);
}
