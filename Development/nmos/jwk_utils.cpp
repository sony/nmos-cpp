#include "nmos/jwk_utils.h"

#include <openssl/ossl_typ.h>
#include <openssl/pem.h>

#if OPENSSL_VERSION_NUMBER < 0x30000000L
#include <openssl/rsa.h>
#else
#include <openssl/core_names.h>
#include <openssl/param_build.h>
#endif

#include "cpprest/basic_utils.h"
#include "ssl/ssl_utils.h"

namespace nmos
{
    namespace experimental
    {
        typedef std::unique_ptr<BIGNUM, decltype(&BN_free)> BIGNUM_ptr;
        typedef std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)> EVP_PKEY_ptr;
        typedef std::unique_ptr<EVP_PKEY_CTX, decltype(&EVP_PKEY_CTX_free)> EVP_PKEY_CTX_ptr;
#if OPENSSL_VERSION_NUMBER < 0x30000000L
        typedef std::unique_ptr<RSA, decltype(&RSA_free)> RSA_ptr;
#else
        typedef std::unique_ptr<OSSL_PARAM, decltype(&OSSL_PARAM_free)> OSSL_PARAM_ptr;
        typedef std::unique_ptr<OSSL_PARAM_BLD, decltype(&OSSL_PARAM_BLD_free)> OSSL_PARAM_BLD_ptr;
#endif

        namespace details
        {
#if OPENSSL_VERSION_NUMBER < 0x10100000L
            int RSA_set0_key(RSA* r, BIGNUM* n, BIGNUM* e, BIGNUM* d)
            {
                /* If the fields n and e in r are NULL, the corresponding input
                 * parameters MUST be non-NULL for n and e.  d may be
                 * left NULL (in case only the public key is used).
                 */
                if ((r->n == NULL && n == NULL)
                    || (r->e == NULL && e == NULL))
                    return 0;

                if (n != NULL) {
                    BN_free(r->n);
                    r->n = n;
                }
                if (e != NULL) {
                    BN_free(r->e);
                    r->e = e;
                }
                if (d != NULL) {
                    BN_free(r->d);
                    r->d = d;
                }

                return 1;
            }

            void RSA_get0_key(const RSA* r, const BIGNUM** n, const BIGNUM** e, const BIGNUM** d)
            {
                if (n != NULL)
                    *n = r->n;
                if (e != NULL)
                    *e = r->e;
                if (d != NULL)
                    *d = r->d;
            }
#endif
            // convert JSON Web Key to RSA Public Key
            // The "n" (modulus) parameter contains the modulus value for the RSA public key
            // It is represented as a Base64urlUInt - encoded value
            // The "e" (exponent) parameter contains the exponent value for the RSA public key
            // It is represented as a Base64urlUInt - encoded value
            // see https://tools.ietf.org/html/rfc7518#section-6.3.1
            // this function is based on https://stackoverflow.com/questions/57217529/how-to-convert-jwk-public-key-to-pem-format-in-c
            utility::string_t jwk_to_rsa_public_key(const utility::string_t& base64_n, const utility::string_t& base64_e)
            {
#if OPENSSL_VERSION_NUMBER < 0x30000000L
                using ssl::experimental::BIO_ptr;

                auto n = utility::conversions::from_base64url(base64_n);
                auto e = utility::conversions::from_base64url(base64_e);

                BIGNUM_ptr modulus(BN_bin2bn(n.data(), (int)n.size(), NULL), &BN_free);
                BIGNUM_ptr exponent(BN_bin2bn(e.data(), (int)e.size(), NULL), &BN_free);

                RSA_ptr rsa(RSA_new(), &RSA_free);
                if (!rsa)
                {
                    throw jwk_exception("convert jwk to pem error: failed to create RSA");
                }

                // "Calling this function transfers the memory management of the values to the RSA object,
                // and therefore the values that have been passed in should not be freed by the caller after
                // this function has been called."
                // see https://www.openssl.org/docs/man1.1.1/man3/RSA_set0_key.html
                if (RSA_set0_key(rsa.get(), modulus.get(), exponent.get(), NULL))
                {
                    modulus.release();
                    exponent.release();
                }
                else
                {
                    throw jwk_exception("convert jwk to pem error: failed to initialise RSA");
                }

                BIO_ptr bio(BIO_new(BIO_s_mem()), &BIO_free);
                if (!bio)
                {
                    throw jwk_exception("convert jwk to pem error: failed to create BIO memory");
                }
                if (PEM_write_bio_RSA_PUBKEY(bio.get(), rsa.get()))
                {
                    BUF_MEM* buf;
                    BIO_get_mem_ptr(bio.get(), &buf);
                    std::string pem(size_t(buf->length), 0);
                    BIO_read(bio.get(), (void*)pem.data(), (int)pem.length());
                    return utility::s2us(pem);
                }
                else
                {
                    throw jwk_exception("convert jwk to pem error: failed to write RSA public key to BIO memory");
                }
#else
                using ssl::experimental::BIO_ptr;

                auto n = utility::conversions::from_base64url(base64_n);
                auto e = utility::conversions::from_base64url(base64_e);

                BIGNUM_ptr modulus(BN_bin2bn(n.data(), (int)n.size(), NULL), &BN_free);
                BIGNUM_ptr exponent(BN_bin2bn(e.data(), (int)e.size(), NULL), &BN_free);

                OSSL_PARAM_BLD_ptr param_bld(OSSL_PARAM_BLD_new(), &OSSL_PARAM_BLD_free);
                if (OSSL_PARAM_BLD_push_BN(param_bld.get(), OSSL_PKEY_PARAM_RSA_N, modulus.get()))
                {
                    modulus.release();
                }
                if (OSSL_PARAM_BLD_push_BN(param_bld.get(), OSSL_PKEY_PARAM_RSA_E, exponent.get()))
                {
                    exponent.release();
                }

                OSSL_PARAM_ptr params(OSSL_PARAM_BLD_to_param(param_bld.get()), &OSSL_PARAM_free);
                EVP_PKEY_CTX_ptr ctx(EVP_PKEY_CTX_new_from_name(NULL, "RSA", NULL), &EVP_PKEY_CTX_free);

                struct evp_pkey_cleanup
                {
                    EVP_PKEY* p;
                    ~evp_pkey_cleanup() { if (p) { EVP_PKEY_free(p); } }
                };

                evp_pkey_cleanup pkey = { 0 };
                if ((1 != EVP_PKEY_fromdata_init(ctx.get())) || (1 != EVP_PKEY_fromdata(ctx.get(), &pkey.p, EVP_PKEY_PUBLIC_KEY, params.get())))
                {
                    throw jwk_exception("convert jwk to pem error: failed to create EVP_PKEY-RSA public key from OSSL parameters");
                }

                BIO_ptr bio(BIO_new(BIO_s_mem()), &BIO_free);
                if (!bio)
                {
                    throw jwk_exception("convert jwk to pem error: failed to create BIO memory");
                }
                if (PEM_write_bio_PUBKEY(bio.get(), pkey.p))
                {
                    BUF_MEM* buf;
                    BIO_get_mem_ptr(bio.get(), &buf);
                    std::string pem(size_t(buf->length), 0);
                    BIO_read(bio.get(), (void*)pem.data(), (int)pem.length());
                    return utility::s2us(pem);
                }
                else
                {
                    throw jwk_exception("convert jwk to pem error: failed to write RSA public key to BIO memory");
                }
#endif
            }

            // convert Bignum to base64url string
            utility::string_t to_base64url(const BIGNUM* bignum)
            {
                if (bignum)
                {
                    const auto size = BN_num_bytes(bignum);
                    std::vector<uint8_t> data(size);
                    if (BN_bn2bin(bignum, data.data()))
                    {
                        return utility::conversions::to_base64url(data);
                    }
                }
                return utility::string_t{};
            }

            // convert RSA to JSON Web Key
            web::json::value rsa_to_jwk(const EVP_PKEY_ptr& pkey, const utility::string_t& keyid, const jwk::public_key_use& pubkey_use, const jwk::algorithm& alg)
            {
#if OPENSSL_VERSION_NUMBER < 0x30000000L
                RSA_ptr rsa(EVP_PKEY_get1_RSA(pkey.get()), &RSA_free);

                // The n, e and d parameters can be obtained by calling RSA_get0_key().
                // If they have not been set yet, then *n, *e and *d will be set to NULL.
                // Otherwise, they are set to pointers to their respective values.
                // These point directly to the internal representations of the values and
                // therefore should not be freed by the caller.
                // see https://manpages.debian.org/unstable/libssl-doc/RSA_get0_key.3ssl.en.html#DESCRIPTION
                const BIGNUM* modulus = nullptr;
                const BIGNUM* exponent = nullptr;
                RSA_get0_key(rsa.get(), &modulus, &exponent, nullptr);

                const auto base64_n = to_base64url(modulus);
                const auto base64_e = to_base64url(exponent);
#else
                BIGNUM* modulus = nullptr;
                BIGNUM* exponent = nullptr;

                utility::string_t base64_n;
                if (EVP_PKEY_get_bn_param(pkey.get(), OSSL_PKEY_PARAM_RSA_N, &modulus))
                {
                    base64_n = to_base64url(modulus);
                    BN_clear_free(modulus);
                }

                utility::string_t base64_e;
                if (EVP_PKEY_get_bn_param(pkey.get(), OSSL_PKEY_PARAM_RSA_E, &exponent))
                {
                    base64_e = to_base64url(exponent);
                    BN_clear_free(exponent);
                }
#endif
                // construct jwk
                return web::json::value_of({
                    { U("kid"), keyid },
                    { U("kty"), U("RSA") },
                    { U("n"), base64_n },
                    { U("e"), base64_e },
                    { U("alg"), alg.name },
                    { U("use"), pubkey_use.name }
                });
            }
        }

        // extract RSA public key from RSA private key
        utility::string_t rsa_public_key(const utility::string_t& rsa_private_key)
        {
            using ssl::experimental::BIO_ptr;

            const std::string private_key_buffer{ utility::us2s(rsa_private_key) };
            BIO_ptr private_key_bio(BIO_new_mem_buf((void*)private_key_buffer.c_str(), (int)private_key_buffer.length()), &BIO_free);
            if (!private_key_bio)
            {
                throw jwk_exception("extract public key error: failed to create BIO memory from PEM private key");
            }

            EVP_PKEY_ptr private_key(PEM_read_bio_PrivateKey(private_key_bio.get(), NULL, NULL, NULL), &EVP_PKEY_free);

            if (!private_key)
            {
                throw jwk_exception("extract public key error: failed to create EVP_PKEY-RSA from BIO private key");
            }

            BIO_ptr bio(BIO_new(BIO_s_mem()), &BIO_free);

            if (bio && PEM_write_bio_PUBKEY(bio.get(), private_key.get()))
            {
                BUF_MEM* buf;
                BIO_get_mem_ptr(bio.get(), &buf);
                std::string public_key(size_t(buf->length), 0);
                BIO_read(bio.get(), (void*)public_key.data(), (int)public_key.length());
                return utility::s2us(public_key);
            }
            else
            {
                throw jwk_exception("extract public key error: failed to write EVP_PKEY-RSA public key to BIO memory");
            }
        }

        // convert JSON Web Key to RSA public key
        utility::string_t jwk_to_rsa_public_key(const web::json::value& jwk)
        {
            // Key Type (kty)
            // see https://tools.ietf.org/html/rfc7517#section-4.1

            // RSA Public Keys
            // see https://tools.ietf.org/html/rfc7518#section-6.3.1
            if (U("RSA") == jwk.at(U("kty")).as_string())
            {
                // Public Key Use (use), optional!
                // see https://tools.ietf.org/html/rfc7517#section-4.2
                if (jwk.has_field(U("use")))
                {
                    if (U("sig") != jwk.at(U("use")).as_string()) throw jwk_exception("jwk contains invalid 'use': " + utility::us2s(jwk.serialize()));
                }

                // is n presented?
                // Base64 URL encoded string representing the modulus of the RSA Key
                // see https://tools.ietf.org/html/rfc7518#section-6.3.1.1
                if (!jwk.has_field(U("n"))) throw jwk_exception("jwk does not contain 'n': " + utility::us2s(jwk.serialize()));

                // is e presented?
                // Base64 URL encoded string representing the public exponent of the RSA Key
                // see https://tools.ietf.org/html/rfc7518#section-6.3.1.2
                if (!jwk.has_field(U("e"))) throw jwk_exception("jwk does not contain 'e': " + utility::us2s(jwk.serialize()));

                // using n & e to convert Json Web Key to RSA Public Key
                return details::jwk_to_rsa_public_key(jwk.at(U("n")).as_string(), jwk.at(U("e")).as_string()); // may throw jwk_exception
            }
            throw jwk_exception("unsupported non-RSA jwk: " + utility::us2s(jwk.serialize()));
        }

        // convert RSA public key to JSON Web Key
        web::json::value rsa_public_key_to_jwk(const utility::string_t& rsa_public_key, const utility::string_t& keyid, const jwk::public_key_use& pubkey_use, const jwk::algorithm& alg)
        {
            using ssl::experimental::BIO_ptr;

            const std::string public_key{ utility::us2s(rsa_public_key) };
            BIO_ptr bio(BIO_new_mem_buf((void*)public_key.c_str(), (int)public_key.length()), &BIO_free);
            if (!bio)
            {
                throw jwk_exception("convert pem to jwk error: failed to create BIO memory from PEM public key");
            }

            // create EVP_PKEY-RSA from BIO public key
            EVP_PKEY_ptr key(PEM_read_bio_PUBKEY(bio.get(), NULL, NULL, NULL), &EVP_PKEY_free);
            if (key)
            {
                // create JWK
                return details::rsa_to_jwk(key, keyid, pubkey_use, alg);
            }
            throw jwk_exception("convert pem to jwk error: failed to create EVP_PKEY-RSA from BIO public key");
        }

        // convert RSA private key to JSON Web Key
        web::json::value rsa_private_key_to_jwk(const utility::string_t& rsa_private_key, const utility::string_t& keyid, const jwk::public_key_use& pubkey_use, const jwk::algorithm& alg)
        {
            using ssl::experimental::BIO_ptr;

            const std::string buffer{ utility::us2s(rsa_private_key) };
            BIO_ptr bio(BIO_new_mem_buf((void*)buffer.c_str(), (int)buffer.length()), &BIO_free);
            if (!bio)
            {
                throw jwk_exception("convert pem to jwk error: failed to create BIO memory from PEM private key");
            }

            // create EVP_PKEY-RSA from BIO private key
            EVP_PKEY_ptr private_key(PEM_read_bio_PrivateKey(bio.get(), NULL, NULL, NULL), &EVP_PKEY_free);
            if (private_key)
            {
                // create JWK
                return details::rsa_to_jwk(private_key, keyid, pubkey_use, alg);
            }
            throw jwk_exception("convert pem to jwk error: failed to create EVP_PKEY-RSA from BIO private key");
        }
    }
}
