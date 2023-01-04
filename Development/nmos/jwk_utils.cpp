#include "nmos/jwk_utils.h"

#include <map>
#include <openssl/ec.h>
#include <openssl/ossl_typ.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include "cpprest/basic_utils.h"
#include "ssl/ssl_utils.h"

namespace nmos
{
    namespace experimental
    {
        namespace details
        {
            typedef std::unique_ptr<BIGNUM, decltype(&BN_free)> BIGNUM_ptr;
            typedef std::unique_ptr<RSA, decltype(&RSA_free)> RSA_ptr;
            typedef std::unique_ptr<EC_KEY, decltype(&EC_KEY_free)> EC_KEY_ptr;
            typedef std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)> EVP_PKEY_ptr;

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
            // The "e" (exponent)parameter contains the exponent value for the RSA public key
            // It is represented as a Base64urlUInt - encoded value
            // see https://tools.ietf.org/html/rfc7518#section-6.3.1
            // this function is based on https://stackoverflow.com/questions/57217529/how-to-convert-jwk-public-key-to-pem-format-in-c
            utility::string_t jwk_to_public_key(const utility::string_t& base64_n, const utility::string_t& base64_e)
            {
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
                    throw jwk_exception("convert jwk to pem error: failed to write BIO to pem");
                }
            }

            // convert JSON Web Key to EC Public Key
            // The "crv" (curve) parameter identifies the cryptographic curve used with the EC public key
            // The supported curves are "P-256", "P-384" and "P-521"
            // The "x" (x coordinate) parameter contains the x coordinate for the Elliptic Curve point
            // It is represented as the base64url encoding of the octet string representation of the coordinate
            // The "y" (y coordinate) parameter contains the y coordinate for the Elliptic Curve point
            // It is represented as the base64url encoding of the octet string representation of the coordinate
            // see https://tools.ietf.org/html/rfc7518#section-6.2.1
            utility::string_t jwk_to_public_key(const utility::string_t& curve_type, const utility::string_t& base64_x, const utility::string_t& base64_y)
            {
                using ssl::experimental::BIO_ptr;

                // supported Elliptic-Curve types
                // see https://tools.ietf.org/search/rfc4492#appendix-A
                const std::map<utility::string_t, int> curve =
                {
                    { U("P-256"), NID_X9_62_prime256v1 },
                    { U("P-384"), NID_secp384r1 },
                    { U("P-521"), NID_secp521r1 }
                };

                auto found = curve.find(curve_type);
                if (curve.end() == found)
                {
                    throw jwk_exception("convert jwk to pem error: EC type not supported");
                }
                EC_KEY_ptr ec_key(EC_KEY_new_by_curve_name(found->second), &EC_KEY_free);
                if (!ec_key)
                {
                    throw jwk_exception("convert jwk to pem error: failed to create EC key with named curve");
                }

                auto x = utility::conversions::from_base64url(base64_x);
                auto y = utility::conversions::from_base64url(base64_y);

                BIGNUM_ptr x_coordinate(BN_bin2bn(x.data(), (int)x.size(), NULL), &BN_free);
                BIGNUM_ptr y_coordinate(BN_bin2bn(y.data(), (int)y.size(), NULL), &BN_free);

                if (EC_KEY_set_public_key_affine_coordinates(ec_key.get(), x_coordinate.get(), y_coordinate.get()))
                {
                    x_coordinate.release();
                    y_coordinate.release();
                }
                else
                {
                    throw jwk_exception("convert jwk to pem error: failed to initialise EC");
                }

                BIO_ptr bio(BIO_new(BIO_s_mem()), &BIO_free);
                if (!bio)
                {
                    throw jwk_exception("convert jwk to pem error: failed to create BIO memory");
                }
                if (PEM_write_bio_EC_PUBKEY(bio.get(), ec_key.get()))
                {
                    BUF_MEM* buf;
                    BIO_get_mem_ptr(bio.get(), &buf);
                    std::string pem(size_t(buf->length), 0);
                    BIO_read(bio.get(), (void*)pem.data(), (int)pem.length());
                    return utility::s2us(pem);
                }
                else
                {
                    throw jwk_exception("convert jwk to pem error: failed to write BIO to pem");
                }
            }

            // convert JSON Web Key to public key in pem format
            utility::string_t jwk_to_public_key(const web::json::value& jwk)
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
                    return jwk_to_public_key(jwk.at(U("n")).as_string(), jwk.at(U("e")).as_string()); // may throw jwk_exception
                }
                // Elliptic Curve Public Keys
                // see https://tools.ietf.org/html/rfc7518#section-6.2
                else if (U("EC") == jwk.at(U("kty")).as_string())
                {
                    // Public Key Use (use), optional!
                    // see https://tools.ietf.org/html/rfc7517#section-4.2
                    if (jwk.has_field(U("use")))
                    {
                        if (U("sig") != jwk.at(U("use")).as_string()) throw jwk_exception("jwk contains invalid 'use': " + utility::us2s(jwk.serialize()));
                    }

                    // is crv presented?
                    // The "crv" (curve) parameter identifies the cryptographic curve used with the EC public Key
                    // see https://tools.ietf.org/html/rfc7518#section-6.2.1.1
                    if (!jwk.has_field(U("crv"))) throw jwk_exception("jwk does not contain 'crv': " + utility::us2s(jwk.serialize()));

                    // is x presented?
                    // Base64 URL encoded string representation of the x coordinate of the EC public Key
                    // see https://tools.ietf.org/html/rfc7518#section-6.2.1.2
                    if (!jwk.has_field(U("x"))) throw jwk_exception("jwk does not contains 'x': " + utility::us2s(jwk.serialize()));

                    // is y presented?
                    // Base64 URL encoded string representation of the y coordinate of the EC public Key
                    // see https://tools.ietf.org/html/rfc7518#section-6.2.1.3
                    if (!jwk.has_field(U("y"))) throw jwk_exception("jwk does not contain 'y': " + utility::us2s(jwk.serialize()));

                    // using crv, x & y to convert Json Web Key to EC Public Key
                    return jwk_to_public_key(jwk.at(U("crv")).as_string(), jwk.at(U("x")).as_string(), jwk.at(U("y")).as_string()); // may throw jwk_exception
                }
                else
                {
                    throw jwk_exception("jwk contains invalid 'kty': " + utility::us2s(jwk.serialize()));
                }
            }

            // convert RSA to JSON Web Key
            web::json::value rsa_to_jwk(const RSA_ptr& rsa, const utility::string_t& keyid, const jwk::public_key_use& pubkey_use, const jwk::algorithm& alg)
            {
                const BIGNUM* modulus = NULL;
                const BIGNUM* exponent = NULL;

                // The n, e and d parameters can be obtained by calling RSA_get0_key().
                // If they have not been set yet, then *n, *e and *d will be set to NULL.
                // Otherwise, they are set to pointers to their respective values.
                //These point directly to the internal representations of the values and
                // therefore should not be freed by the caller.
                // see https://manpages.debian.org/unstable/libssl-doc/RSA_get0_key.3ssl.en.html#DESCRIPTION
                RSA_get0_key(rsa.get(), &modulus, &exponent, NULL);

                const auto modulus_bytes = BN_num_bytes(modulus);
                std::vector<uint8_t> n(modulus_bytes);
                BN_bn2bin(modulus, n.data());
                const auto base64_n = utility::conversions::to_base64url(n);

                const auto exponent_bytes = BN_num_bytes(exponent);
                std::vector<uint8_t> e(exponent_bytes);
                BN_bn2bin(exponent, e.data());
                const auto base64_e = utility::conversions::to_base64url(e);

                // construct jwk
                return  web::json::value_of({
                    { U("kid"), keyid },
                    { U("kty"), U("RSA") },
                    { U("n"), base64_n },
                    { U("e"), base64_e },
                    { U("alg"), alg.name },
                    { U("use"), pubkey_use.name }
                });
            }

            // convert RSA public key to JSON Web Key
            web::json::value public_key_to_jwk(const utility::string_t& pubkey, const utility::string_t& keyid, const jwk::public_key_use& pubkey_use, const jwk::algorithm& alg)
            {
                using ssl::experimental::BIO_ptr;

                const std::string public_key{ utility::us2s(pubkey) };
                BIO_ptr bio(BIO_new_mem_buf((void*)public_key.c_str(), (int)public_key.length()), &BIO_free);
                if (!bio)
                {
                    throw jwk_exception("convert pem to jwk error: failed to create BIO memory from public key");
                }

                RSA* rsa_ = NULL;
                RSA_ptr rsa(PEM_read_bio_RSA_PUBKEY(bio.get(), &rsa_, NULL, NULL), &RSA_free);
                if(!rsa)
                {
                    throw jwk_exception("convert pem to jwk error: failed to load RSA");
                }

                return rsa_to_jwk(rsa, keyid, pubkey_use, alg);
            }

            // convert RSA private key to JSON Web Key
            web::json::value private_key_to_jwk(const utility::string_t& private_key_, const utility::string_t& keyid, const jwk::public_key_use& pubkey_use, const jwk::algorithm& alg)
            {
                using ssl::experimental::BIO_ptr;

                const std::string buffer{ utility::us2s(private_key_) };
                BIO_ptr bio(BIO_new_mem_buf((void*)buffer.c_str(), (int)buffer.length()), &BIO_free);
                EVP_PKEY_ptr private_key(PEM_read_bio_PrivateKey(bio.get(), NULL, NULL, NULL), &EVP_PKEY_free);

                if (private_key)
                {
                    RSA_ptr rsa(EVP_PKEY_get1_RSA(private_key.get()), &RSA_free);
                    if (rsa)
                    {
                        return rsa_to_jwk(rsa, keyid, pubkey_use, alg);
                    }
                }
                return{};
            }

            // find the RSA private key from private key list
            utility::string_t found_rsa_key(const std::vector<utility::string_t>& private_keys)
            {
                using ssl::experimental::BIO_ptr;

                for (const auto private_key_ : private_keys)
                {
                    const std::string buffer{ utility::us2s(private_key_) };
                    BIO_ptr bio(BIO_new_mem_buf((void*)buffer.c_str(), (int)buffer.length()), &BIO_free);
                    EVP_PKEY_ptr private_key(PEM_read_bio_PrivateKey(bio.get(), NULL, NULL, NULL), &EVP_PKEY_free);

                    if (private_key)
                    {
                        RSA_ptr rsa(EVP_PKEY_get1_RSA(private_key.get()), &RSA_free);
                        if (rsa)
                        {
                            return private_key_;
                        }
                    }
                }
                return{};
            }

            // extract RSA public key from RSA private key
            utility::string_t rsa_public_key(const utility::string_t& private_key_)
            {
                using ssl::experimental::BIO_ptr;

                const std::string private_key_buffer{ utility::us2s(private_key_) };
                BIO_ptr private_key_bio(BIO_new_mem_buf((void*)private_key_buffer.c_str(), (int)private_key_buffer.length()), &BIO_free);
                if (!private_key_bio)
                {
                    throw jwk_exception("extract public key error: failed to create BIO memory from private key");
                }

                EVP_PKEY_ptr private_key(PEM_read_bio_PrivateKey(private_key_bio.get(), NULL, NULL, NULL), &EVP_PKEY_free);

                if (!private_key)
                {
                    throw jwk_exception("extract public key error: failed to read BIO private key");
                }

                RSA_ptr rsa(EVP_PKEY_get1_RSA(private_key.get()), &RSA_free);
                if (!rsa)
                {
                    throw jwk_exception("extract public key error: failed to load RSA key");
                }

                BIO_ptr bio(BIO_new(BIO_s_mem()), &BIO_free);
                if (bio && PEM_write_bio_RSA_PUBKEY(bio.get(), rsa.get()))
                {
                    BUF_MEM* buf;
                    BIO_get_mem_ptr(bio.get(), &buf);
                    std::string public_key(size_t(buf->length), 0);
                    BIO_read(bio.get(), (void*)public_key.data(), (int)public_key.length());
                    return utility::s2us(public_key);
                }
                else
                {
                    throw jwk_exception("extract public key error: failed to read RSA public key");
                }
            }
        }
    }
}
