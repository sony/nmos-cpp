#include "nmos/est_utils.h"

#include <memory>
#include <sstream>
#include <string.h>
#include <boost/algorithm/string.hpp> // for boost::split
#include <openssl/asn1.h>
#include <openssl/err.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h> // for X509V3_EXT_conf_nid
#include "ssl/ssl_utils.h"

namespace nmos
{
    namespace experimental
    {
        namespace details
        {
            typedef std::unique_ptr<BIGNUM, decltype(&BN_free)> BIGNUM_ptr;
            typedef std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)> EVP_PKEY_ptr;
            typedef std::unique_ptr<X509_REQ, decltype(&X509_REQ_free)> X509_REQ_ptr;
            typedef std::unique_ptr<EVP_PKEY_CTX, decltype(&EVP_PKEY_CTX_free)> EVP_PKEY_CTX_ptr;

            std::shared_ptr<EVP_PKEY> make_rsa_key(const std::string& private_key_data = {}, const std::string& password = {})
            {
                if (private_key_data.empty())
                {
                    // create the context for the key generation
                    EVP_PKEY_CTX_ptr ctx(EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, NULL), &EVP_PKEY_CTX_free);
                    if (!ctx)
                    {
                        throw est_exception("failed to generate RSA key: EVP_PKEY_CTX_new_id failure: " + ssl::experimental::last_openssl_error());
                    }

                    // generate the RSA key
                    if (0 >= EVP_PKEY_keygen_init(ctx.get()))
                    {
                        throw est_exception("failed to generate RSA key: EVP_PKEY_keygen_init failure: " + ssl::experimental::last_openssl_error());
                    }

                    if (0 >= EVP_PKEY_CTX_set_rsa_keygen_bits(ctx.get(), 2048))
                    {
                        throw est_exception("failed to generate RSA key: EVP_PKEY_CTX_set_rsa_keygen_bits failure: " + ssl::experimental::last_openssl_error());
                    }

                    EVP_PKEY* pkey_temp = NULL;
                    if (0 >= EVP_PKEY_keygen(ctx.get(), &pkey_temp))
                    {
                        throw est_exception("failed to generate RSA key: EVP_PKEY_keygen failure: " + ssl::experimental::last_openssl_error());
                    }

                    // create a EVP_PKEY to store key
                    std::shared_ptr<EVP_PKEY> pkey(EVP_PKEY_new(), &EVP_PKEY_free);

                    return pkey;
                }
                else
                {
                    ssl::experimental::BIO_ptr bio(BIO_new(BIO_s_mem()), &BIO_free);
                    if ((size_t)BIO_write(bio.get(), private_key_data.data(), (int)private_key_data.size()) != private_key_data.size())
                    {
                        throw est_exception("failed to load RSA key: BIO_write failure: " + ssl::experimental::last_openssl_error());
                    }
                    std::shared_ptr<EVP_PKEY> pkey(PEM_read_bio_PrivateKey(bio.get(), NULL, NULL, const_cast<char*>(password.c_str())), &EVP_PKEY_free);
                    if (!pkey)
                    {
                        throw est_exception("failed to load RSA key: PEM_read_bio_PrivateKey failure: " + ssl::experimental::last_openssl_error());
                    }
                    return pkey;
                }
            }

            std::shared_ptr<EVP_PKEY> make_ecdsa_key(const std::string& private_key_data = {}, const std::string& password = {})
            {
                if (private_key_data.empty())
                {
                    // create the context for the key generation
                    EVP_PKEY_CTX_ptr ctx(EVP_PKEY_CTX_new_id(EVP_PKEY_EC, NULL), &EVP_PKEY_CTX_free);
                    if (!ctx)
                    {
                        throw est_exception("failed to generate ECDSA key: EVP_PKEY_CTX_new_id failure: " + ssl::experimental::last_openssl_error());
                    }

                    // generate the ECDSA key
                    if (0 >= EVP_PKEY_keygen_init(ctx.get()))
                    {
                        throw est_exception("failed to generate ECDSA key: EVP_PKEY_keygen_init failure: " + ssl::experimental::last_openssl_error());
                    }

                    // use the ANSI X9.62 Prime 256v1 curve
                    // NIST P-256 is refered to as secp256r1 and prime256v1. Different names, but they are all the same.
                    // See https://tools.ietf.org/search/rfc4492#appendix-A
                    if (0 >= EVP_PKEY_CTX_set_ec_paramgen_curve_nid(ctx.get(), NID_X9_62_prime256v1))
                    {
                        throw est_exception("failed to generate ECDSA key: EVP_PKEY_CTX_set_ec_paramgen_curve_nid failure: " + ssl::experimental::last_openssl_error());
                    }

                    EVP_PKEY* pkey_temp = NULL;
                    if (0 >= EVP_PKEY_keygen(ctx.get(), &pkey_temp))
                    {
                        throw est_exception("failed to generate ECDSA key: EVP_PKEY_keygen failure: " + ssl::experimental::last_openssl_error());
                    }

                    // create a EVP_PKEY to store key
                    std::shared_ptr<EVP_PKEY> pkey(pkey_temp, EVP_PKEY_free);

                    return pkey;
                }
                else
                {
                    ssl::experimental::BIO_ptr bio(BIO_new(BIO_s_mem()), &BIO_free);
                    if ((size_t)BIO_write(bio.get(), private_key_data.data(), (int)private_key_data.size()) != private_key_data.size())
                    {
                        throw est_exception("failed to load ECDSA key: BIO_write failure: " + ssl::experimental::last_openssl_error());
                    }
                    std::shared_ptr<EVP_PKEY> pkey(PEM_read_bio_PrivateKey(bio.get(), NULL, NULL, const_cast<char*>(password.c_str())), EVP_PKEY_free);
                    if (!pkey)
                    {
                        throw est_exception("failed to load ECDSA key: PEM_read_bio_PrivateKey failure: " + ssl::experimental::last_openssl_error());
                    }
                    return pkey;
                }
            }

            // convert PKCS7 to pem format
            // it is based on openssl example
            // See https://github.com/openssl/openssl/blob/master/apps/pkcs7.c
            std::string to_pem(std::shared_ptr<PKCS7> p7)
            {
                if (!p7)
                {
                    throw est_exception("failed to convert PKCS7 to pem: no PKCS7");
                }

                if (!p7->d.sign)
                {
                    throw est_exception("failed to convert PKCS7 to pem: no NID_pkcs7_signed");
                }

                auto certs = p7->d.sign->cert;
                if (certs)
                {
                    ssl::experimental::BIO_ptr bio(BIO_new(BIO_s_mem()), &BIO_free);

                    for (auto idx = 0; idx < sk_X509_num(certs); idx++)
                    {
                        auto x509 = sk_X509_value(certs, idx);
                        auto cert_info = [](BIO* out, X509* x)
                        {
                            auto p = X509_NAME_oneline(X509_get_subject_name(x), NULL, 0);
                            BIO_puts(out, "subject=");
                            BIO_puts(out, p);
                            OPENSSL_free(p);

                            p = X509_NAME_oneline(X509_get_issuer_name(x), NULL, 0);
                            BIO_puts(out, "\n\nissuer=");
                            BIO_puts(out, p);
                            OPENSSL_free(p);

                            BIO_puts(out, "\n\nnotBefore=");
#if (OPENSSL_VERSION_NUMBER >= 0x1010100fL)
                            ASN1_TIME_print(out, X509_get0_notBefore(x));
#else
                            ASN1_TIME_print(out, X509_get_notBefore(x));
#endif
                            BIO_puts(out, "\n\nnotAfter=");
#if (OPENSSL_VERSION_NUMBER >= 0x1010100fL)
                            ASN1_TIME_print(out, X509_get0_notAfter(x));
#else
                            ASN1_TIME_print(out, X509_get_notAfter(x));
#endif
                            BIO_puts(out, "\n\n");
                        };
                        cert_info(bio.get(), x509);
                        PEM_write_bio_X509(bio.get(), x509);
                        BIO_puts(bio.get(), "\n");
                    }

                    BUF_MEM* buf;
                    BIO_get_mem_ptr(bio.get(), &buf);
                    std::string pem(size_t(buf->length), 0);
                    if ((size_t)BIO_read(bio.get(), (void*)pem.data(), (int)pem.length()) != pem.length())
                    {
                        throw est_exception("failed to convert PKCS7 to pem: BIO_read failure: " + ssl::experimental::last_openssl_error());
                    }
                    return pem;
                }
                else
                {
                    throw est_exception("failed to convert PKCS7 to pem: no certificate found");
                }
            }

            // convert Private key to pem format
            std::string to_pem(std::shared_ptr<EVP_PKEY> pkey)
            {
                ssl::experimental::BIO_ptr bio(BIO_new(BIO_s_mem()), &BIO_free);
                if (PEM_write_bio_PrivateKey(bio.get(), pkey.get(), NULL, NULL, 0, NULL, NULL))
                {
                    BUF_MEM* buf;
                    BIO_get_mem_ptr(bio.get(), &buf);
                    std::string pem(size_t(buf->length), 0);
                    if ((size_t)BIO_read(bio.get(), (void*)pem.data(), (int)pem.length()) != pem.length())
                    {
                        throw est_exception("failed to convert private key to pem: BIO_read failure: " + ssl::experimental::last_openssl_error());
                    }
                    return pem;
                }
                else
                {
                    throw est_exception("failed to convert private key to pem: PEM_write_bio_PrivateKey failure: " + ssl::experimental::last_openssl_error());
                }
            }

            // convert PKCS7 to pem format
            std::string make_pem_from_pkcs7(const std::string& pkcs7_data)
            {
                if (pkcs7_data.empty())
                {
                    throw est_exception("no pkcs7 to convert to pem");
                }

                const std::string prefix{ "-----BEGIN PKCS7-----\n" };
                const std::string suffix{ '\n' == pkcs7_data.back() ? "-----END PKCS7-----\n" : "\n-----END PKCS7-----\n" };

                // insert PKCS7 prefix & suffix if missing
                auto pkcs7 = (prefix != pkcs7_data.substr(0, prefix.length())) ? prefix + pkcs7_data + suffix : pkcs7_data;

                // convert PKCS7 to pem format
                // it is based on openssl example
                // See https://github.com/openssl/openssl/blob/master/apps/pkcs7.c
                ssl::experimental::BIO_ptr bio(BIO_new(BIO_s_mem()), &BIO_free); // BIO_free_all
                if ((size_t)BIO_write(bio.get(), pkcs7.data(), (int)pkcs7.size()) != pkcs7.size())
                {
                    throw est_exception("failed to load PKCS7: BIO_write failure: " + ssl::experimental::last_openssl_error());
                }
                std::shared_ptr<PKCS7> p7(PEM_read_bio_PKCS7(bio.get(), NULL, NULL, NULL), PKCS7_free);
                if (p7)
                {
                    return to_pem(p7);
                }
                else
                {
                    throw est_exception("failed to load PKCS7: PEM_read_bio_PKCS7 failure: " + ssl::experimental::last_openssl_error());
                }
            }

            // generate X509 certificate request
            std::string make_X509_req(const std::shared_ptr<EVP_PKEY>& pkey, const std::string& common_name, const std::string& country, const std::string& state, const std::string& city, const std::string& organization, const std::string& organizational_unit, const std::string& email_address)
            {
                const long version = 0;

                // generate x509 request
                X509_REQ_ptr x509_req(X509_REQ_new(), &X509_REQ_free);
                if (!x509_req)
                {
                    throw est_exception("failed to create x509 req: X509_REQ_new failure: " + ssl::experimental::last_openssl_error());
                }
                // set version of x509 request
                if (!X509_REQ_set_version(x509_req.get(), version))
                {
                    throw est_exception("failed to set version of x509 req: X509_REQ_set_version failure: " + ssl::experimental::last_openssl_error());
                }

                // set subject of x509 request
                auto x509_name = X509_REQ_get_subject_name(x509_req.get());
                auto add_X509_REQ_subject = [&x509_name](const std::string& subject, const std::string& value)
                {
                    if (!value.empty())
                    {
                        if (!X509_NAME_add_entry_by_txt(x509_name, subject.c_str(), MBSTRING_ASC, (const unsigned char*)value.c_str(), -1, -1, 0))
                        {
                            std::stringstream ss;
                            ss << "failed to set '" << subject << "' of x509 req: X509_NAME_add_entry_by_txt failure: " << ssl::experimental::last_openssl_error();
                            throw est_exception(ss.str());
                        }
                    }
                };
                // set common name of x509 request, common name MUST be presented
                if (common_name.empty())
                {
                    throw est_exception("missing common name for x509 req");
                }
                add_X509_REQ_subject("CN", common_name);
                // set country of x509 request
                add_X509_REQ_subject("C", country);
                // set state of x509 request
                add_X509_REQ_subject("ST", state);
                // set city of x509 request
                add_X509_REQ_subject("L", city);
                // set organization of x509 request
                add_X509_REQ_subject("O", organization);
                // set organizational unit of x509 request
                add_X509_REQ_subject("OU", organizational_unit);
                // set email address of x509 request
                add_X509_REQ_subject("emailAddress", email_address);

                // set x509 extensions
                auto extensions = sk_X509_EXTENSION_new_null();
                auto add_extension = [&extensions](int nid, const std::string& value)
                {
#if OPENSSL_VERSION_NUMBER < 0x30000000L
                    auto extension = X509V3_EXT_conf_nid(NULL, NULL, nid, const_cast<char*>(value.c_str()));
#else
                    auto extension = X509V3_EXT_conf_nid(NULL, NULL, nid, value.c_str());
#endif
                    if (!extension)
                    {
                        std::stringstream ss;
                        ss << "failed to create '" << nid << "' extension: X509V3_EXT_conf_nid failure: " << ssl::experimental::last_openssl_error();

                        // release all previous added extension
                        sk_X509_EXTENSION_pop_free(extensions, X509_EXTENSION_free);

                        throw est_exception(ss.str());
                    }
                    sk_X509_EXTENSION_push(extensions, extension);
                };
                // set subjectAltName of x509 request
                add_extension(NID_subject_alt_name, "DNS:" + common_name);

                // add extensions to x509 request
                if (!X509_REQ_add_extensions(x509_req.get(), extensions))
                {
                    throw est_exception("failed to add extnsions to x509 req: X509_REQ_add_extensions failure: " + ssl::experimental::last_openssl_error());
                }
                // release x509 extensions
                sk_X509_EXTENSION_pop_free(extensions, X509_EXTENSION_free);

                // set public key of x509 req
                if (!X509_REQ_set_pubkey(x509_req.get(), pkey.get()))
                {
                    throw est_exception("failed to set public key of x509 req: X509_REQ_set_pubkey failure: " + ssl::experimental::last_openssl_error());
                }

                // set sign key of x509 req
                if (0 >= X509_REQ_sign(x509_req.get(), pkey.get(), EVP_sha256()))
                {
                    throw est_exception("failed to set sign key of x509 req: X509_REQ_sign failure: " + ssl::experimental::last_openssl_error());
                }

                // generate x509 req in pem format
                ssl::experimental::BIO_ptr bio(BIO_new(BIO_s_mem()), &BIO_free);
                if (PEM_write_bio_X509_REQ(bio.get(), x509_req.get()))
                {
                    BUF_MEM* buf;
                    BIO_get_mem_ptr(bio.get(), &buf);
                    std::string pem(size_t(buf->length), 0);
                    if ((size_t)BIO_read(bio.get(), (void*)pem.data(), (int)pem.length()) != pem.length())
                    {
                        throw est_exception("failed to generate CSR: BIO_read failure: " + ssl::experimental::last_openssl_error());
                    }
                    return pem;
                }
                else
                {
                    throw est_exception("failed to generate CSR: PEM_write_bio_X509_REQ failure: " + ssl::experimental::last_openssl_error());
                }
            }

            // generate RSA certificate request
            csr make_rsa_csr(const std::string& common_name, const std::string& country, const std::string& state, const std::string& city, const std::string& organization, const std::string& organizational_unit, const std::string& email_address, const std::string& private_key_data, const std::string& password)
            {
                auto pkey = make_rsa_key(private_key_data, password);
                return{ to_pem(pkey), make_X509_req(pkey, common_name, country, state, city, organization, organizational_unit, email_address) };
            }

            // generate ECDSA certificate request
            csr make_ecdsa_csr(const std::string& common_name, const std::string& country, const std::string& state, const std::string& city, const std::string& organization, const std::string& organizational_unit, const std::string& email_address, const std::string& private_key_data, const std::string& password)
            {
                auto pkey = make_ecdsa_key(private_key_data, password);
                return{ to_pem(pkey), make_X509_req(pkey, common_name, country, state, city, organization, organizational_unit, email_address) };
            }

            std::vector<std::string> x509_crl_urls(const std::string& certificate)
            {
                ssl::experimental::BIO_ptr bio(BIO_new(BIO_s_mem()), &BIO_free);
                if ((size_t)BIO_write(bio.get(), certificate.data(), (int)certificate.size()) != certificate.size())
                {
                    throw est_exception("failed to load cert to bio: BIO_write failure: " + ssl::experimental::last_openssl_error());
                }

                ssl::experimental::X509_ptr x509(PEM_read_bio_X509_AUX(bio.get(), NULL, NULL, NULL), &X509_free);
                if (!x509)
                {
                    throw est_exception("failed to load cert: PEM_read_bio_X509_AUX failure: " + ssl::experimental::last_openssl_error());
                }

                std::vector<std::string> list;
                auto dist_points = (STACK_OF(DIST_POINT)*)X509_get_ext_d2i(x509.get(), NID_crl_distribution_points, NULL, NULL);
                for (auto idx = 0; idx < sk_DIST_POINT_num(dist_points); idx++)
                {
                    auto dp = sk_DIST_POINT_value(dist_points, idx);
                    auto distpoint = dp->distpoint;
                    if (distpoint->type == 0) //fullname GENERALIZEDNAME
                    {
                        for (int i = 0; i < sk_GENERAL_NAME_num(distpoint->name.fullname); i++)
                        {
                            auto gen = sk_GENERAL_NAME_value(distpoint->name.fullname, i);
                            auto asn1_str = gen->d.uniformResourceIdentifier;
#if (OPENSSL_VERSION_NUMBER >= 0x1010100fL)
                            list.push_back(std::string((const char*)ASN1_STRING_get0_data(asn1_str), ASN1_STRING_length(asn1_str)));
#else
                            list.push_back(std::string((const char*)ASN1_STRING_data(asn1_str), ASN1_STRING_length(asn1_str)));
#endif
                        }
                    }
                    else if (distpoint->type == 1) //relativename X509NAME
                    {
                        auto sk_relname = distpoint->name.relativename;
                        for (int i = 0; i < sk_X509_NAME_ENTRY_num(sk_relname); i++)
                        {
                            auto e = sk_X509_NAME_ENTRY_value(sk_relname, i);
                            auto d = X509_NAME_ENTRY_get_data(e);
#if (OPENSSL_VERSION_NUMBER >= 0x1010100fL)
                            list.push_back(std::string((const char*)ASN1_STRING_get0_data(d), ASN1_STRING_length(d)));
#else
                            list.push_back(std::string((const char*)ASN1_STRING_data(d), ASN1_STRING_length(d)));
#endif
                        }
                    }
                }
                return list;
            }

            // revocation check
            // it is based on zedwood.com example
            // See http://www.zedwood.com/article/cpp-check-crl-for-revocation
            bool is_revoked_by_crl(X509* x509, X509* issuer, X509_CRL* crl)
            {
                if (!x509) { throw est_exception("invalid cert"); }
                if (!issuer) { throw est_exception("invalid cert issuer"); }
                if (!crl) { throw est_exception("invalid CRL"); }

                auto ikey = X509_get_pubkey(issuer);
                auto cert_serial_number = X509_get_serialNumber(x509);

                if (!ikey) { throw est_exception("invalid cert public key"); }

                if (!X509_CRL_verify(crl, ikey)) { throw est_exception("failed to verify CRL signature"); }

                auto revoked_list = X509_CRL_get_REVOKED(crl);
                for (auto idx = 0; idx < sk_X509_REVOKED_num(revoked_list); idx++)
                {
                    auto entry = sk_X509_REVOKED_value(revoked_list, idx);
#if (OPENSSL_VERSION_NUMBER >= 0x1010100fL)
                    auto serial_number = X509_REVOKED_get0_serialNumber(entry);
#else
                    auto serial_number = entry->serialNumber;
#endif
                    if (serial_number->length == cert_serial_number->length)
                    {
                        if (memcmp(serial_number->data, cert_serial_number->data, cert_serial_number->length) == 0)
                        {
                            return true;
                        }
                    }
                }
                return false;
            }

            bool is_revoked_by_crl(const std::string& certificate, const std::string& cert_issuer_data, const std::string& crl_data)
            {
                ssl::experimental::BIO_ptr bio_cert(BIO_new(BIO_s_mem()), &BIO_free);
                BIO_puts(bio_cert.get(), certificate.c_str());
                ssl::experimental::BIO_ptr bio_cert_issuer(BIO_new(BIO_s_mem()), &BIO_free);
                BIO_puts(bio_cert_issuer.get(), cert_issuer_data.c_str());
                ssl::experimental::BIO_ptr bio_crl(BIO_new(BIO_s_mem()), &BIO_free);
                BIO_puts(bio_crl.get(), crl_data.c_str());

                return is_revoked_by_crl(
                    PEM_read_bio_X509(bio_cert.get(), NULL, NULL, NULL),
                    PEM_read_bio_X509(bio_cert_issuer.get(), NULL, NULL, NULL),
                    PEM_read_bio_X509_CRL(bio_crl.get(), NULL, NULL, NULL));
            }
        }
    }
}
