#include "nmos/ocsp_utils.h"

#include <boost/asio/ssl.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <openssl/ocsp.h>
#include "cpprest/basic_utils.h"
#include "nmos/ocsp_state.h"
#include "ssl/ssl_utils.h"

namespace nmos
{
    namespace experimental
    {
        typedef std::unique_ptr<OCSP_REQUEST, decltype(&OCSP_REQUEST_free)> OCSP_REQUEST_ptr;
        typedef std::unique_ptr<OCSP_RESPONSE, decltype(&OCSP_RESPONSE_free)> OCSP_RESPONSE_ptr;

        namespace details
        {
            typedef std::map<std::string, std::vector<std::string>> issuer_certificate_vs_server_certificates_map;

            // construct OCSP request using an issuer certificate to server certificates lookup map
            // see https://stackoverflow.com/questions/56253312/how-to-create-ocsp-request-using-openssl-in-c
            std::vector<uint8_t> make_ocsp_request(const issuer_certificate_vs_server_certificates_map& issuer_certificate_vs_server_certificates)
            {
                using ssl::experimental::BIO_ptr;
                using ssl::experimental::X509_ptr;

                // set up OCSP request to load certificates
                OCSP_REQUEST_ptr ocsp_request(OCSP_REQUEST_new(), &OCSP_REQUEST_free);
                if (!ocsp_request)
                {
                    throw ocsp_exception("failed to make_ocsp_request while setting up OCSP request: OCSP_REQUEST_new failure: " + ssl::experimental::last_openssl_error());
                }

                for (const auto& issuer_certificate_vs_server_certificates_item : issuer_certificate_vs_server_certificates)
                {
                    // load issuer certificate
                    BIO_ptr issuer_bio(BIO_new(BIO_s_mem()), &BIO_free);
                    if (!issuer_bio)
                    {
                        throw ocsp_exception("failed to make_ocsp_request while creating BIO memory: BIO_new failure: " + ssl::experimental::last_openssl_error());
                    }
                    const auto& issuer_certificate = issuer_certificate_vs_server_certificates_item.first;
                    if ((size_t)BIO_write(issuer_bio.get(), issuer_certificate.data(), (int)issuer_certificate.size()) != issuer_certificate.size())
                    {
                        throw ocsp_exception("failed to make_ocsp_request while loading issuer certificate: BIO_write failure: " + ssl::experimental::last_openssl_error());
                    }
                    X509_ptr issuer_x509(PEM_read_bio_X509_AUX(issuer_bio.get(), NULL, NULL, NULL), &X509_free);
                    if (!issuer_x509)
                    {
                        throw ocsp_exception("failed to make_ocsp_request while loading issuer certificate: PEM_read_bio_X509_AUX failure: " + ssl::experimental::last_openssl_error());
                    }

                    const auto& server_certificates = issuer_certificate_vs_server_certificates_item.second;
                    // load server certificate then add to OCSP request
                    for (const auto& server_certificate : server_certificates)
                    {
                        // load server certificate
                        BIO_ptr server_bio(BIO_new(BIO_s_mem()), &BIO_free);
                        if (!server_bio)
                        {
                            throw ocsp_exception("failed to make_ocsp_request while creating BIO memory: BIO_new failure: " + ssl::experimental::last_openssl_error());
                        }
                        if ((size_t)BIO_write(server_bio.get(), server_certificate.data(), (int)server_certificate.size()) != server_certificate.size())
                        {
                            throw ocsp_exception("failed to make_ocsp_request while loading server certificate: BIO_write failure: " + ssl::experimental::last_openssl_error());
                        }
                        X509_ptr server_x509(PEM_read_bio_X509_AUX(server_bio.get(), NULL, NULL, NULL), &X509_free);
                        if (!server_x509)
                        {
                            throw ocsp_exception("failed to make_ocsp_request while loading server certificate: PEM_read_bio_X509_AUX failure: " + ssl::experimental::last_openssl_error());
                        }

                        auto const cert_id_md = EVP_sha1();

                        // creates and returns a new OCSP_CERTID structure using SHA1 message digest for the server certificate with the issuer
                        auto id = OCSP_cert_to_id(cert_id_md, server_x509.get(), issuer_x509.get());
                        if (!id)
                        {
                            throw ocsp_exception("failed to make_ocsp_request while creating new OCSP_CERTID: OCSP_cert_to_id failure: " + ssl::experimental::last_openssl_error());
                        }
                        if (!OCSP_request_add0_id(ocsp_request.get(), id))
                        {
                            throw ocsp_exception("failed to make_ocsp_request while adding certificate to OCSP request: OCSP_request_add0_id failure: " + ssl::experimental::last_openssl_error());
                        }
                    }
                }

                // write the OCSP request out in DER format
                BIO_ptr bio(BIO_new(BIO_s_mem()), &BIO_free);
                if (!bio)
                {
                    throw ocsp_exception("failed to make_ocsp_request while creating BIO to load OCSP request: BIO_new failure: " + ssl::experimental::last_openssl_error());
                }
                if (ASN1_i2d_bio(reinterpret_cast<i2d_of_void*>(i2d_OCSP_REQUEST), bio.get(), reinterpret_cast<uint8_t*>(ocsp_request.get())) == 0)
                {
                    throw ocsp_exception("failed to make_ocsp_request while converting OCSP REQUEST in DER format: ASN1_i2d_bio failed: " + ssl::experimental::last_openssl_error());
                }

                BUF_MEM* buf;
                BIO_get_mem_ptr(bio.get(), &buf);
                std::vector<uint8_t> ocsp_request_der(size_t(buf->length), 0);
                if ((size_t)BIO_read(bio.get(), (void*)ocsp_request_der.data(), (int)ocsp_request_der.size()) != ocsp_request_der.size())
                {
                    throw ocsp_exception("failed to make_ocsp_request while converting OCSP REQUEST to byte array: BIO_read failed: " + ssl::experimental::last_openssl_error());
                }

                return ocsp_request_der;
            }

#if !defined(_WIN32) || !defined(__cplusplus_winrt) || defined(CPPREST_FORCE_HTTP_CLIENT_ASIO)
            // this callback is called when client includes a certificate status request extension in the TLS handshake
            int server_certificate_status_request(SSL* ssl, void* arg)
            {
                auto ocsp_response = *(std::vector<uint8_t>*)arg;
                return nmos::experimental::set_ocsp_response(ssl, ocsp_response) ? SSL_TLSEXT_ERR_OK : SSL_TLSEXT_ERR_NOACK;
            }
#endif

            // get the certificate at position index in the server certificate chain
            std::string get_certificate_at(const std::string& certificate_chain, size_t index)
            {
                // split the server certificate chain to a list of individual certificates
                const auto certificates = ssl::experimental::split_certificate_chain(certificate_chain);

                // get the certificate at position index
                if (index >= certificates.size())
                {
                    throw ocsp_exception("required certificate not found");
                }
                return certificates[index];
            }
        }

        // get a list of OCSP URIs from server certificate
        std::vector<web::uri> get_ocsp_uris(const std::string& certificate)
        {
            using ssl::experimental::BIO_ptr;
            using ssl::experimental::X509_ptr;

            BIO_ptr bio(BIO_new(BIO_s_mem()), &BIO_free);
            if (!bio)
            {
                throw ocsp_exception("failed to get_ocsp_uris while creating BIO memory: BIO_new failure: " + ssl::experimental::last_openssl_error());
            }
            if ((size_t)BIO_write(bio.get(), certificate.data(), (int)certificate.size()) != certificate.size())
            {
                throw ocsp_exception("failed to get_ocsp_uris while loading server certificate to BIO: BIO_new failure: " + ssl::experimental::last_openssl_error());
            }
            X509_ptr x509(PEM_read_bio_X509_AUX(bio.get(), NULL, NULL, NULL), &X509_free);
            if (!x509)
            {
                throw ocsp_exception("failed to get_ocsp_uris while converting server certificate BIO to X509: PEM_read_bio_X509_AUX failure: " + ssl::experimental::last_openssl_error());
            }
            auto ocsp_uris_ = X509_get1_ocsp(x509.get());

            std::vector<web::uri> ocsp_uris;
            for (int idx = 0; idx < sk_OPENSSL_STRING_num(ocsp_uris_); idx++)
            {
                web::uri ocsp_uri(utility::s2us(sk_OPENSSL_STRING_value(ocsp_uris_, idx)));
                if (ocsp_uris.end() == std::find_if(ocsp_uris.begin(), ocsp_uris.end(), [&](const web::uri& uri) { return uri == ocsp_uri; }))
                {
                    ocsp_uris.push_back(ocsp_uri);
                }
            }
            return ocsp_uris;
        }

        // construct an OCSP request from the specified list of server certificate chains
        std::vector<uint8_t> make_ocsp_request(const std::vector<std::string>& certificate_chains)
        {
            if (certificate_chains.empty())
            {
                throw ocsp_exception("failed to make_ocsp_request: no server certificate chains");
            }

            details::issuer_certificate_vs_server_certificates_map issuer_certificate_vs_server_certificates;
            for (const auto& certificate_chain : certificate_chains)
            {
                // a minimal server certificate chain starts with the server's certificate, followed by the server's issuer certificate.

                // get the issuer certificate from the certificate chain, this should be the 2nd certificate in the chain
                const auto issuer_certificate = details::get_certificate_at(certificate_chain, 1);
                // get the server certificate from the certificate chain, this should be the 1st certificate in the chain
                const auto server_certificate = details::get_certificate_at(certificate_chain, 0);

                // construct the issuer certificate to server certificates lookup map
                const auto found = issuer_certificate_vs_server_certificates.find(issuer_certificate);
                if (issuer_certificate_vs_server_certificates.end() == found)
                {
                    issuer_certificate_vs_server_certificates[issuer_certificate] = { server_certificate };
                }
                else
                {
                    issuer_certificate_vs_server_certificates[issuer_certificate].push_back(server_certificate);
                }
            }

            return details::make_ocsp_request(issuer_certificate_vs_server_certificates);
        }

        // set up OCSP response for the OCSP stapling in the TLS handshake
        bool set_ocsp_response(SSL* ssl, const std::vector<uint8_t>& ocsp_response)
        {
            if (ocsp_response.empty()) return false;

#if (OPENSSL_VERSION_NUMBER < 0x1010100fL)
            auto buffer = OPENSSL_malloc(ocsp_response.size());
#else
            auto buffer = OPENSSL_memdup(ocsp_response.data(), ocsp_response.size());
#endif
            if (0 == buffer) return false;
#if (OPENSSL_VERSION_NUMBER < 0x1010100fL)
            std::memcpy(buffer, ocsp_response.data(), ocsp_response.size());
#endif
            return 0 != SSL_set_tlsext_status_ocsp_resp(ssl, buffer, (int)ocsp_response.size());
        }

#if !defined(_WIN32) || !defined(__cplusplus_winrt) || defined(CPPREST_FORCE_HTTP_CLIENT_ASIO)
        // set up server certificate status callback when client includes a certificate status request extension in the TLS handshake
        void set_server_certificate_status_handler(boost::asio::ssl::context& ctx, const std::vector<uint8_t>& ocsp_response)
        {
            SSL_CTX_set_tlsext_status_cb(ctx.native_handle(), details::server_certificate_status_request);
            SSL_CTX_set_tlsext_status_arg(ctx.native_handle(), (void*)(&ocsp_response));
        }
#endif
    }
}
