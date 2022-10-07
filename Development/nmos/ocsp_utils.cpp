#include "nmos/ocsp_utils.h"

#include <boost/asio/ssl.hpp>
#include <openssl/ocsp.h>
#include "cpprest/basic_utils.h"
#include "nmos/ocsp_settings.h"
#include "ssl/ssl_utils.h"

namespace nmos
{
    namespace experimental
    {
        typedef std::unique_ptr<OCSP_REQUEST, decltype(&OCSP_REQUEST_free)> OCSP_REQUEST_ptr;
        typedef std::unique_ptr<OCSP_RESPONSE, decltype(&OCSP_RESPONSE_free)> OCSP_RESPONSE_ptr;

        namespace details
        {
            // create OCSP request from list of server certificate chains data
            // This is based on the example given at https://stackoverflow.com/questions/56253312/how-to-create-ocsp-request-using-openssl-in-c
            std::vector<uint8_t> make_ocsp_request(const std::string& issuer_cert_data, const std::vector<std::string>& server_certs_data)
            {
                using ssl::experimental::BIO_ptr;
                using ssl::experimental::X509_ptr;

                // load issuer certificate
                BIO_ptr issuer_bio(BIO_new(BIO_s_mem()), &BIO_free);
                if ((size_t)BIO_write(issuer_bio.get(), issuer_cert_data.data(), (int)issuer_cert_data.size()) != issuer_cert_data.size())
                {
                    throw ocsp_exception("failed to make_ocsp_request while loading issuer certificate: BIO_write failure: " + ssl::experimental::last_openssl_error());
                }
                X509_ptr issuer_x509(PEM_read_bio_X509_AUX(issuer_bio.get(), NULL, NULL, NULL), &X509_free);
                if (!issuer_x509)
                {
                    throw ocsp_exception("failed to make_ocsp_request while loading issuer certificate: PEM_read_bio_X509_AUX failure: " + ssl::experimental::last_openssl_error());
                }

                // setup OCSP request to load certificates
                OCSP_REQUEST_ptr ocsp_req(OCSP_REQUEST_new(), &OCSP_REQUEST_free);
                if (!ocsp_req)
                {
                    throw ocsp_exception("failed to make_ocsp_request while setup OCSP request: OCSP_REQUEST_new failure: " + ssl::experimental::last_openssl_error());
                }

                // load server certificate then add to OCSP request
                for (const auto& server_cert_data : server_certs_data)
                {
                    // load server certificate
                    BIO_ptr server_bio(BIO_new(BIO_s_mem()), &BIO_free);
                    if ((size_t)BIO_write(server_bio.get(), server_cert_data.data(), (int)server_cert_data.size()) != server_cert_data.size())
                    {
                        throw ocsp_exception("failed to make_ocsp_request while loading server certificate: BIO_write failure: " + ssl::experimental::last_openssl_error());
                    }
                    X509_ptr server_x509(PEM_read_bio_X509_AUX(server_bio.get(), NULL, NULL, NULL), &X509_free);
                    if (!server_x509)
                    {
                        throw ocsp_exception("failed to make_ocsp_request while loading server certificate: PEM_read_bio_X509_AUX failure: " + ssl::experimental::last_openssl_error());
                    }

                    auto const cert_id_md = EVP_sha1();

                    // creates and returns a new OCSP_CERTID structure using message digest dgst for certificate subject with issuer
                    auto id = OCSP_cert_to_id(cert_id_md, server_x509.get(), issuer_x509.get());
                    if (!id)
                    {
                        throw ocsp_exception("failed to make_ocsp_request while creating new OCSP_CERTID: OCSP_cert_to_id failure: " + ssl::experimental::last_openssl_error());
                    }
                    if (!OCSP_request_add0_id(ocsp_req.get(), id))
                    {
                        throw ocsp_exception("failed to make_ocsp_request while adding certificate to OCSP request: OCSP_request_add0_id failure: " + ssl::experimental::last_openssl_error());
                    }
                }

                // write the OCSP request out in DER format
                BIO_ptr bio(BIO_new(BIO_s_mem()), &BIO_free);
                if (!bio)
                {
                    throw ocsp_exception("failed to make_ocsp_request while creating BIO to load OCSP request: BIO_new failure: " + ssl::experimental::last_openssl_error());
                }
                if (ASN1_i2d_bio(reinterpret_cast<i2d_of_void*>(i2d_OCSP_REQUEST), bio.get(), reinterpret_cast<uint8_t*>(ocsp_req.get())) == 0)
                {
                    throw ocsp_exception("failed to make_ocsp_request while converting OCSP REQUEST in DER format: ASN1_i2d_bio failed: " + ssl::experimental::last_openssl_error());
                }

                BUF_MEM* buf;
                BIO_get_mem_ptr(bio.get(), &buf);
                std::vector<uint8_t> ocsp_req_data(size_t(buf->length), 0);
                if ((size_t)BIO_read(bio.get(), (void*)ocsp_req_data.data(), (int)ocsp_req_data.size()) != ocsp_req_data.size())
                {
                    throw ocsp_exception("failed to make_ocsp_request while converting OCSP REQUEST to byte array: BIO_read failed: " + ssl::experimental::last_openssl_error());
                }

                return ocsp_req_data;
            }

#if !defined(_WIN32) || defined(CPPREST_FORCE_HTTP_CLIENT_ASIO)
            // This callback is called when client includes a certificate status request extension in the TLS handshake
            int server_certificate_status_request(SSL* s, void* arg)
            {
                nmos::experimental::ocsp_settings* settings = (nmos::experimental::ocsp_settings*)arg;
                nmos::experimental::send_ocsp_response(s, nmos::with_read_lock(*settings->mutex, [&] { return settings->ocsp_response; }));
                return SSL_TLSEXT_ERR_OK;
            }
#endif
        }

        // get a list of OCSP URIs from certificate
        std::vector<web::uri> get_ocsp_uris(const std::string& cert_data)
        {
            using ssl::experimental::BIO_ptr;
            using ssl::experimental::X509_ptr;

            BIO_ptr bio(BIO_new(BIO_s_mem()), &BIO_free);
            if ((size_t)BIO_write(bio.get(), cert_data.data(), (int)cert_data.size()) != cert_data.size())
            {
                throw ocsp_exception("failed to get_ocsp_uris while loading server certificate: BIO_new failure: " + ssl::experimental::last_openssl_error());
            }
            X509_ptr x509(PEM_read_bio_X509_AUX(bio.get(), NULL, NULL, NULL), &X509_free);
            if (!x509)
            {
                throw ocsp_exception("failed to get_ocsp_uris while loading server certificate: PEM_read_bio_X509_AUX failure: " + ssl::experimental::last_openssl_error());
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

        // create OCSP request from list of server certificate chains data
        std::vector<uint8_t> make_ocsp_request(const std::vector<std::string>& cert_chains_data)
        {
            // extract issuer certificate from the 1st server certificate
            if (cert_chains_data.empty())
            {
                throw ocsp_exception("no server certificate");
            }
            // split issuer certificate from server certificate chain
            const auto certs = ssl::experimental::split_certificate_chain(cert_chains_data[0]);

            if (certs.size() < 2)
            {
                throw ocsp_exception("missing issuer certificate");
            }
            const auto issuer_data = certs[1];

            return details::make_ocsp_request(issuer_data, cert_chains_data);
        }

        // send OCSP response in TLS handshake
        bool send_ocsp_response(SSL* s, const std::vector<uint8_t>& ocsp_resp_data)
        {
            using ssl::experimental::BIO_ptr;

            if (ocsp_resp_data.size() <= 0) { return false; }

            // load OCSP response cache to BIO
            BIO_ptr bio(BIO_new(BIO_s_mem()), &BIO_free);
            if (BIO_write(bio.get(), ocsp_resp_data.data(), (int)ocsp_resp_data.size()) != (int)ocsp_resp_data.size()) { return false; }

            // BIO to OCSP response structure
            OCSP_RESPONSE_ptr ocsp_resp(d2i_OCSP_RESPONSE_bio(bio.get(), NULL), &OCSP_RESPONSE_free);

            // encode OCSP response structure
            unsigned char* buffer = NULL;
            const auto buffer_len = i2d_OCSP_RESPONSE(ocsp_resp.get(), &buffer);
            if (buffer_len <= 0) { return false; }

            // send OCSP response
            SSL_set_tlsext_status_ocsp_resp(s, buffer, buffer_len);
            return true;
        }


#if !defined(_WIN32) || defined(CPPREST_FORCE_HTTP_CLIENT_ASIO)
        // setup server certificate status callback when client includes a certificate status request extension in the TLS handshake
        void set_server_certificate_status_handler(boost::asio::ssl::context& ctx, const nmos::experimental::ocsp_settings& settings)
        {
            SSL_CTX_set_tlsext_status_cb(ctx.native_handle(), details::server_certificate_status_request);
            SSL_CTX_set_tlsext_status_arg(ctx.native_handle(), (void*)&settings);
        }
#endif
    }
}
