#include "nmos/est_behaviour.h"

#include <boost/algorithm/string.hpp> // for boost::to_upper_copy
#include <time.h> // for tm and strptime
#if (defined(_WIN32) || defined(__cplusplus_winrt)) && !defined(CPPREST_FORCE_HTTP_CLIENT_ASIO)
#include <wincrypt.h>
#include <winhttp.h>
#endif
#include "pplx/pplx_utils.h" // for pplx::complete_at
#include "cpprest/http_client.h"
#include "cpprest/json_validator.h"
#include "mdns/service_discovery.h"
#include "nmos/api_utils.h"
#include "nmos/client_utils.h"
#include "nmos/est_versions.h"
#include "nmos/est_utils.h"
#include "nmos/json_schema.h"
#include "nmos/mdns.h"
#include "nmos/model.h"
#include "nmos/random.h"
#include "nmos/slog.h"
#include "nmos/thread_utils.h" // for wait_until, reverse_lock_guard

namespace nmos
{
    namespace experimental
    {
        namespace fields
        {
            const web::json::field_as_string_or ver{ U("ver"), {} };
            //const web::json::field_as_integer_or pri{ U("pri"), nmos::service_priorities::no_priority }; already defined in settings.h
            const web::json::field_as_string_or uri{ U("uri"), {} };
        }

        namespace details
        {
            struct est_shared_state
            {
                load_ca_certificates_handler load_ca_certificates;
                load_client_certificate_handler load_client_certificate;

                receive_ca_certificate_handler receive_ca_certificate;
                // cacerts data (ca certificate chain)
                utility::string_t cacerts;
                // is cacerts expired (the shortest expiry CA has reached the renewal time)
                bool renewal;

                bool est_service_error;

                nmos::details::seed_generator seeder;
                std::default_random_engine engine;
                std::unique_ptr<web::http::client::http_client> client;

                explicit est_shared_state(load_ca_certificates_handler load_ca_certificates, load_client_certificate_handler load_client_certificate, receive_ca_certificate_handler receive_ca_certificate)
                    : load_ca_certificates(std::move(load_ca_certificates)), load_client_certificate(std::move(load_client_certificate)), receive_ca_certificate(std::move(receive_ca_certificate)), cacerts({}), renewal(false), est_service_error(false), engine(seeder)
                {}
            };

            typedef std::function<void(const utility::string_t& cert)> verify_cert_handler;

            struct cert_shared_state
            {
                receive_server_certificate_handler received;
                // private key data
                utility::string_t key;
                // certificate signing request data
                utility::string_t csr;
                // certificate data
                utility::string_t cert;

                std::unique_ptr<web::http::client::http_client> client;

                // how many seconds before next certificate fetch
                std::chrono::seconds delay;
                // is certificate expired
                bool expired;

                // verify certificate
                verify_cert_handler verify;

                // mutex to be used to protect the crl_urls from simultaneous access by multiple threads
                mutable nmos::mutex mutex;

                // certificate revocation URLs
                std::vector<std::string> crl_urls;

                nmos::read_lock read_lock() const { return nmos::read_lock{ mutex }; }
                nmos::write_lock write_lock() const { return nmos::write_lock{ mutex }; }

                explicit cert_shared_state(receive_server_certificate_handler received, utility::string_t private_key = {}, utility::string_t csr = {}) : received(std::move(received)), key(std::move(private_key)), csr(std::move(csr)), cert({}), delay(std::chrono::seconds(0)), expired(false)
                {}
            };
        }

        namespace details
        {
            void est_behaviour_thread(nmos::model& model, load_ca_certificates_handler load_ca_certificates, load_client_certificate_handler load_client_certificate, receive_ca_certificate_handler ca_certificate_received, receive_server_certificate_handler rsa_server_certificate_received, receive_server_certificate_handler ecdsa_server_certificate_received, mdns::service_discovery& discovery, slog::base_gate& gate);

            bool do_ca_certificates_requests(nmos::model& model, est_shared_state& est_state, slog::base_gate& gate);
            bool do_certificates_requests(nmos::model& model, est_shared_state& est_state, cert_shared_state& rsa_state, cert_shared_state& ecdsa_state, slog::base_gate& gate);
            void do_renewal_certificates_and_certificates_revocation_requests(nmos::model& model, est_shared_state& est_state, cert_shared_state& rsa_state, cert_shared_state& ecdsa_state, slog::base_gate& gate);

            // background service discovery
            void est_services_background_discovery(nmos::model& model, mdns::service_discovery& discovery, slog::base_gate& gate);

            // service discovery
            bool discover_est_services(nmos::base_model& model, mdns::service_discovery& discovery, slog::base_gate& gate, const pplx::cancellation_token& token = pplx::cancellation_token::none());
            bool has_discovered_est_services(const nmos::model& model);
        }

        // uses the default DNS-SD implementation
        // callbacks from this function are called with the model locked, and may read or write directly to the model
        void est_behaviour_thread(nmos::model& model, load_ca_certificates_handler load_ca_certificates, load_client_certificate_handler load_client_certificate, receive_ca_certificate_handler ca_certificate_received, receive_server_certificate_handler rsa_server_certificate_received, receive_server_certificate_handler ecdsa_server_certificate_received, slog::base_gate& gate_)
        {
            nmos::details::omanip_gate gate(gate_, nmos::stash_category(nmos::categories::est_behaviour));

            mdns::service_discovery discovery(gate);

            details::est_behaviour_thread(model, std::move(load_ca_certificates), std::move(load_client_certificate), std::move(ca_certificate_received), std::move(rsa_server_certificate_received), std::move(ecdsa_server_certificate_received), discovery, gate);
        }

        // uses the specified DNS-SD implementation
        // callbacks from this function are called with the model locked, and may read or write directly to the model
        void est_behaviour_thread(nmos::model& model, load_ca_certificates_handler load_ca_certificates, load_client_certificate_handler load_client_certificate, receive_ca_certificate_handler ca_certificate_received, receive_server_certificate_handler rsa_server_certificate_received, receive_server_certificate_handler ecdsa_server_certificate_received, mdns::service_discovery& discovery, slog::base_gate& gate_)
        {
            nmos::details::omanip_gate gate(gate_, nmos::stash_category(nmos::categories::est_behaviour));

            details::est_behaviour_thread(model, std::move(load_ca_certificates), std::move(load_client_certificate), std::move(ca_certificate_received), std::move(rsa_server_certificate_received), std::move(ecdsa_server_certificate_received), discovery, gate);
        }

        void details::est_behaviour_thread(nmos::model& model, load_ca_certificates_handler load_ca_certificates, load_client_certificate_handler load_client_certificate, receive_ca_certificate_handler ca_certificate_received, receive_server_certificate_handler rsa_server_certificate_received, receive_server_certificate_handler ecdsa_server_certificate_received, mdns::service_discovery& discovery, slog::base_gate& gate)
        {
            enum
            {
                initial_discovery,
                fetch_cacerts,
                fetch_certificates,
                renewal_certificates_and_certificates_revocation,
                rediscovery,
                background_discovery
            } mode = initial_discovery;

            // If the chosen EST API does not respond correctly at any time, another EST API should be selected from the discovered list.

            with_write_lock(model.mutex, [&model] { model.settings[nmos::experimental::fields::est_services] = web::json::value::array(); });

            nmos::details::seed_generator discovery_backoff_seeder;
            std::default_random_engine discovery_backoff_engine(discovery_backoff_seeder);
            double discovery_backoff = 0;

            est_shared_state est_state{ load_ca_certificates, load_client_certificate, ca_certificate_received };
            cert_shared_state rsa_state{ rsa_server_certificate_received };
            cert_shared_state ecdsa_state{ ecdsa_server_certificate_received };

            // continue until the server is being shut down
            for (;;)
            {
                if (with_read_lock(model.mutex, [&] { return model.shutdown; })) break;

                switch (mode)
                {
                case initial_discovery:
                case rediscovery:
                    if (0 != discovery_backoff)
                    {
                        auto lock = model.read_lock();
                        const auto random_backoff = std::uniform_real_distribution<>(0, discovery_backoff)(discovery_backoff_engine);
                        slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "Waiting to retry EST API discovery for about " << std::fixed << std::setprecision(3) << random_backoff << " seconds (current backoff limit: " << discovery_backoff << " seconds)";
                        model.wait_for(lock, std::chrono::milliseconds(std::chrono::milliseconds::rep(1000 * random_backoff)), [&] { return model.shutdown; });
                        if (model.shutdown) break;
                    }

                    // The Node performs a DNS-SD browse for services of type '_nmos-certs._tcp' as specified.
                    if (details::discover_est_services(model, discovery, gate))
                    {
                        mode = fetch_cacerts;

                        // If the Node is unable to contact the EST API, the Node implements an exponential backoff algorithm
                        // to avoid overloading the EST API in the event of a system restart.
                        auto lock = model.read_lock();
                        discovery_backoff = (std::min)((std::max)((double)nmos::fields::discovery_backoff_min(model.settings), discovery_backoff * nmos::fields::discovery_backoff_factor(model.settings)), (double)nmos::fields::discovery_backoff_max(model.settings));
                    }
                    else
                    {
                        mode = background_discovery;
                    }
                    break;

                case fetch_cacerts:
                    // fetch Root CA certificates
                    mode = do_ca_certificates_requests(model, est_state, gate) ? fetch_certificates : rediscovery;
                    break;

                case fetch_certificates:
                    // generate CSRs then request to sign them
                    mode = do_certificates_requests(model, est_state, rsa_state, ecdsa_state, gate) ? renewal_certificates_and_certificates_revocation : rediscovery;
                    break;

                case renewal_certificates_and_certificates_revocation:
                    // start a background task to renewal CA certificates
                    // start background tasks to renewal server certificates
                    // start a background task to do certificates revocation
                    do_renewal_certificates_and_certificates_revocation_requests(model, est_state, rsa_state, ecdsa_state, gate);

                    // reaching here, either Root CA certificate(s) need to be renrewed, server certificate(s) has expired,
                    // or no further EST APIs be available or TTLs on advertised services expired.
                    mode = (est_state.renewal || rsa_state.expired || ecdsa_state.expired) ? fetch_cacerts : rediscovery;
                    break;

                case background_discovery:
                    details::est_services_background_discovery(model, discovery, gate);

                    if (details::has_discovered_est_services(model))
                    {
                        mode = fetch_cacerts;
                    }

                    break;
                }
            }
        }

        // service discovery
        namespace details
        {
            static web::json::value make_service(const resolved_service& service)
            {
                using  web::json::value;

                return web::json::value_of({
                    { nmos::experimental::fields::ver, value::string(make_api_version(service.first.first)) },
                    { nmos::fields::pri, service.first.second },
                    { nmos::experimental::fields::uri, value::string(service.second.to_string()) }
                });
            }

            static resolved_service parse_service(const web::json::value& data)
            {
                return {
                    { parse_api_version(nmos::experimental::fields::ver(data)), nmos::fields::pri(data) },
                    web::uri(nmos::experimental::fields::uri(data))
                };
            }

            // get the fallback EST service from settings (if present)
            resolved_service get_est_service(const nmos::settings& settings)
            {
                if (settings.has_field(nmos::experimental::fields::est_address))
                {
                    const auto api_selector = nmos::experimental::fields::est_selector(settings);

                    return { { {}, 0 },
                        web::uri_builder()
                        .set_scheme(nmos::http_scheme(settings))
                        .set_host(nmos::experimental::fields::est_address(settings))
                        .set_port(nmos::experimental::fields::est_port(settings))
                        .set_path(U("/.well-known/est")).append_path(!api_selector.empty() ? U("/") + api_selector : U(""))
                        .to_uri() };
                }
                return {};
            }

            // query DNS Service Discovery for any EST API based on settings
            bool discover_est_services(nmos::base_model& model, mdns::service_discovery& discovery, slog::base_gate& gate, const pplx::cancellation_token& token)
            {
                slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "Trying EST API discovery";

                // lock to read settings, then unlock to wait for the discovery task to complete
                auto est_services = with_read_lock(model.mutex, [&]
                {
                    auto& settings = model.settings;

                    if (nmos::service_priorities::no_priority != nmos::fields::highest_pri(settings))
                    {
                        slog::log<slog::severities::info>(gate, SLOG_FLF) << "Attempting discovery of a EST API in domain: " << nmos::get_domain(settings);

                        return nmos::experimental::resolve_service_(discovery, nmos::service_types::est, settings, token);
                    }
                    else
                    {
                        return pplx::task_from_result(std::list<resolved_service>{});
                    }
                }).get();

                with_write_lock(model.mutex, [&]
                {
                    if (!est_services.empty())
                    {
                        slog::log<slog::severities::info>(gate, SLOG_FLF) << "Discovered " << est_services.size() << " EST API(s)";
                    }
                    else
                    {
                        slog::log<slog::severities::warning>(gate, SLOG_FLF) << "Did not discover a suitable EST API via DNS-SD";

                        auto fallback_authorization_service = get_est_service(model.settings);
                        if (!fallback_authorization_service.second.is_empty())
                        {
                            est_services.push_back(fallback_authorization_service);
                        }
                    }

                    if (!est_services.empty()) slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "Using the EST API(s):" << slog::log_manip([&](slog::log_statement& s)
                        {
                            for (auto& est_service : est_services)
                            {
                                s << '\n' << est_service.second.to_string();
                            }
                        });

                    model.settings[nmos::experimental::fields::est_services] = web::json::value_from_elements(est_services | boost::adaptors::transformed([](const resolved_service& est_service) { return make_service(est_service); }));

                    model.notify();
                });

                return !est_services.empty();
            }

            bool empty_est_services(const nmos::settings& settings)
            {
                return web::json::empty(nmos::experimental::fields::est_services(settings));
            }

            bool has_discovered_est_services(const nmos::model& model)
            {
                return with_read_lock(model.mutex, [&] { return !empty_est_services(model.settings); });
            }

            // "The Node selects a EST API to use based on the priority"
            resolved_service top_est_service(const nmos::settings& settings)
            {
                const auto value = web::json::front(nmos::experimental::fields::est_services(settings));
                return parse_service(value);
            }

            // If the chosen EST API does not respond correctly at any time,
            // another EST API should be selected from the discovered list.
            void pop_est_service(nmos::settings& settings)
            {
                web::json::pop_front(nmos::experimental::fields::est_services(settings));
            }
        }

        // EST operation
        namespace details
        {
#if defined(_WIN32)
            // convert a string representation of time to a time tm structure
            // See https://stackoverflow.com/questions/321849/strptime-equivalent-on-windows
            char* strptime(const char* s, const char* f, tm* tm)
            {
                std::istringstream input(s);
                input.imbue(std::locale(setlocale(LC_ALL, nullptr)));
                input >> std::get_time(tm, f);
                if (input.fail()) {
                    return nullptr;
                }
                return (char*)(s + input.tellg());
            }
#endif
            std::chrono::seconds parse_retry_after(const std::string& retry_after)
            {
                const bst::regex http_date_regex(R"([a-z|A-Z]{3}, \d{2} [a-z|A-Z]{3} \d{4} \d{2}:\d{2}:\d{2} [a-z|A-Z]{3})");

                // parse the retry-after header to obtain the retry next value
                // See https://tools.ietf.org/html/rfc7231#section-7.1.3
                // e.g. Retry-After: Fri, 31 Dec 1999 23:59:59 GMT
                //      Retry-After: 120
                if (bst::regex_match(retry_after, http_date_regex))
                {
                    tm when;
                    strptime(retry_after.c_str(), "%a, %d %b %Y %H:%M:%S %Z", &when);

                    time_t now = time(0);
                    const auto diff = difftime(mktime(&when), now);
                    return std::chrono::seconds(diff > 0 ? (int)diff : 0);
                }
                else
                {
                    int delay{ 0 };
                    std::istringstream(retry_after) >> delay;
                    return std::chrono::seconds(delay);
                }
            }

            web::http::client::http_client_config make_est_client_config(const nmos::settings& settings_, load_ca_certificates_handler load_ca_certificates, load_client_certificate_handler load_client_certificate, bool disable_validate_server_certificates, slog::base_gate& gate)
            {
                nmos::settings settings{ settings_ };
                // overrule server certificates validation
                if (disable_validate_server_certificates)
                {
                    settings[nmos::experimental::fields::validate_certificates] = web::json::value::boolean(!disable_validate_server_certificates);
                }

                auto config = nmos::make_http_client_config(settings, std::move(load_ca_certificates), std::move(load_client_certificate), gate);
                config.set_timeout(std::chrono::seconds(nmos::experimental::fields::est_request_max(settings)));

#if (defined(_WIN32) || defined(__cplusplus_winrt)) && !defined(CPPREST_FORCE_HTTP_CLIENT_ASIO)
                config.set_nativehandle_options([=](web::http::client::native_handle hRequest)
                {
                    // the client_certificate_file must be in PKCS #12 format
                    // hmm, while executing WinHttpSendRequest, it failed with ERROR_WINHTTP_CLIENT_CERT_NO_PRIVATE_KEY(12185) : No credentials were available in the client certificate.
                    const auto& client_certificate_file = nmos::experimental::fields::client_certificate_file(settings);
                    if (!client_certificate_file.empty())
                    {
                        std::ifstream stream(client_certificate_file.c_str(), std::ios::in | std::ios::binary);
                        std::vector<uint8_t> pkcs12_data((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());

                        CRYPT_DATA_BLOB data;
                        data.cbData = (DWORD)pkcs12_data.size();
                        data.pbData = reinterpret_cast<BYTE*>(pkcs12_data.data());

                        auto hCertStore = PFXImportCertStore(&data, {}, 0);
                        if (hCertStore)
                        {
                            PCCERT_CONTEXT pCertContext = NULL;
                            pCertContext = CertFindCertificateInStore(hCertStore, X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, 0, CERT_FIND_ANY, NULL, NULL);
                            if (pCertContext)
                            {
                                WinHttpSetOption(hRequest, WINHTTP_OPTION_CLIENT_CERT_CONTEXT, (LPVOID)pCertContext, sizeof(CERT_CONTEXT));

                                CertFreeCertificateContext(pCertContext);
                            }
                            CertCloseStore(hCertStore, 0);
                        }
                    }
                });
#endif //#if (defined(_WIN32) || defined(__cplusplus_winrt)) && !defined(CPPREST_FORCE_HTTP_CLIENT_ASIO)
                return config;
            }

            struct est_service_exception {};

            bool verify_est_response_header(const web::http::http_headers& headers)
            {
                const auto content_type = headers.find(web::http::header_names::content_type);
                const auto content_type_or_empty = headers.end() != content_type ? content_type->second : utility::string_t{};
                const auto content_transfer_encoding = headers.find(U("Content-Transfer-Encoding"));
                const auto content_transfer_encoding_or_empty = headers.end() != content_transfer_encoding ? content_transfer_encoding->second : utility::string_t{};
                const utility::string_t application_pkc7{ U("application/pkcs7-mime") };
                const utility::string_t base64{ U("base64") };

                return (!content_type_or_empty.empty() && application_pkc7 == web::http::details::get_mime_type(content_type_or_empty) && base64 == content_transfer_encoding_or_empty);
            }

            bool verify_crl_response_header(const web::http::http_headers& headers)
            {
                const auto content_type = headers.find(web::http::header_names::content_type);
                const auto content_type_or_empty = headers.end() != content_type ? content_type->second : utility::string_t{};
                const utility::string_t text_plain{ U("text/plain") };

                return (!content_type_or_empty.empty() && text_plain == web::http::details::get_mime_type(content_type_or_empty));
            }

            // extract and verify Certificate Response
            // See https://tools.ietf.org/html/rfc7030#section-4.2.3
            pplx::task<std::pair<web::http::http_response, utility::string_t>> extract_and_verify_certificate(const web::http::http_response& response, verify_cert_handler verify_certificate, slog::base_gate& gate, const pplx::cancellation_token& token = pplx::cancellation_token::none())
            {
                // verify Enroll/Re-enroll response header
                if (!verify_est_response_header(response.headers()))
                {
                    throw est_exception("invalid Enroll/Re-enroll response header");
                }

                if (response.body())
                {
                    return response.extract_string(true).then([=, &gate](utility::string_t body)
                    {
                        slog::log<slog::severities::too_much_info>(gate, SLOG_FLF) << "Received certificate: " << utility::us2s(body);

                        // convert pkcs7 to pem format
                        auto pem = utility::s2us(make_pem_from_pkcs7(utility::us2s(body)));

                        // verify certificate
                        verify_certificate(pem);

                        return std::pair<web::http::http_response, utility::string_t>(response, pem);
                    }, token);
                }
                else
                {
                    throw est_exception("missing certificate");
                }
            }

            // extract and verify CA Certificates Response
            // See https://tools.ietf.org/html/rfc7030#section-4.1.3
            pplx::task<utility::string_t> extract_and_verify_cacerts(const web::http::http_response& response, verify_cert_handler verify_cacerts, slog::base_gate& gate, const pplx::cancellation_token& token = pplx::cancellation_token::none())
            {
                // verify CA response header
                if (!verify_est_response_header(response.headers()))
                {
                    throw est_exception("invalid CA certificates response header");
                }

                // verify CA
                if (response.body())
                {
                    return response.extract_string(true).then([=, &gate](utility::string_t body)
                    {
                        slog::log<slog::severities::too_much_info>(gate, SLOG_FLF) << "Received CA certificates: " << utility::us2s(body);

                        // convert pkcs7 to pem format
                        auto pem = utility::s2us(make_pem_from_pkcs7(utility::us2s(body)));

                        // verify CA certs
                        verify_cacerts(pem);

                        return pem;
                    }, token);
                }
                else
                {
                    throw est_exception("missing CA certificates");
                }
            }

            // extract and verify Certificate Revocation List Response
            pplx::task<utility::string_t> extract_certificate_revocation_list(const web::http::http_response& response, slog::base_gate& gate, const pplx::cancellation_token& token = pplx::cancellation_token::none())
            {
                // verify certificate revocation list response header
                if (!verify_crl_response_header(response.headers()))
                {
                    throw est_exception("invalid CRL response header");
                }

                // hmm, verify certificate revocation list
                if (response.body())
                {
                    return response.extract_string(true).then([=, &gate](utility::string_t body)
                    {
                        slog::log<slog::severities::too_much_info>(gate, SLOG_FLF) << "Received CRL: " << utility::us2s(body);

                        return body;
                    }, token);
                }
                else
                {
                    throw est_exception("missing CRL");
                }
            }

            // make an asynchronously GET request on the EST API to fetch Root CA certificates
            // see https://specs.amwa.tv/bcp-003-03/releases/v1.0.0/docs/1.0._Certificate_Provisioning.html#getting-the-root-ca
            pplx::task<utility::string_t> request_ca_certificates(web::http::client::http_client client, verify_cert_handler verify_cacerts, slog::base_gate& gate, const pplx::cancellation_token& token = pplx::cancellation_token::none())
            {
                slog::log<slog::severities::too_much_info>(gate, SLOG_FLF) << "Requesting CA certificates";

                using namespace web::http;

                // <api_proto>://<hostname>:<port>/.well-known/est[/<api_selector>]/cacerts
                // see https://specs.amwa.tv/bcp-003-03/releases/v1.0.0/docs/1.0._Certificate_Provisioning.html#est-server-api
                // see https://tools.ietf.org/html/rfc7030#section-4.1.2
                return nmos::api_request(client, methods::GET, U("cacerts"), gate, token).then([=, &gate](pplx::task<http_response> response_task)
                {
                    auto response = response_task.get(); // may throw http_exception

                    if (status_codes::OK == response.status_code())
                    {
                        // extract CA from the response body
                        return extract_and_verify_cacerts(response, verify_cacerts, gate, token);
                    }
                    else
                    {
                        slog::log<slog::severities::error>(gate, SLOG_FLF) << "Request CA certificates error: " << response.status_code() << " " << response.reason_phrase();
                    }
                    throw est_service_exception();

                }, token);
            }

            // make an asynchronously POST request on the EST API to fetch certificate
            // see https://specs.amwa.tv/bcp-003-03/releases/v1.0.0/docs/1.0._Certificate_Provisioning.html#certificate-request
            pplx::task<std::pair<web::http::http_response, utility::string_t>> request_certificate(web::http::client::http_client client, const utility::string_t& key, const utility::string_t& csr, verify_cert_handler verify_certificate, slog::base_gate& gate, const pplx::cancellation_token& token = pplx::cancellation_token::none())
            {
                slog::log<slog::severities::too_much_info>(gate, SLOG_FLF) << "Requesting certificate";

                using namespace web::http;

                // <api_proto>://<hostname>:<port>/.well-known/est[/<api_selector>]/simpleenroll
                // see https://specs.amwa.tv/bcp-003-03/releases/v1.0.0/docs/1.0._Certificate_Provisioning.html#est-server-api
                // see https://tools.ietf.org/html/rfc7030#section-4.2.1
                http_request req(methods::POST);
                req.headers().add(header_names::content_type, U("application/pkcs10"));
                req.set_request_uri(U("simpleenroll"));
                req.set_body(csr);

                return nmos::api_request(client, req, gate, token).then([=, &gate](pplx::task<http_response> response_task)
                {
                    auto response = response_task.get(); // may throw http_exception

                    if (status_codes::OK == response.status_code())
                    {
                        return extract_and_verify_certificate(response, verify_certificate, gate, token);
                    }
                    return pplx::task_from_result(std::pair<web::http::http_response, utility::string_t>(response, {}));

                }, token);
            }

            // make an asynchronously POST request on the EST API to certificate renewal
            // see https://specs.amwa.tv/bcp-003-03/releases/v1.0.0/docs/1.0._Certificate_Provisioning.html#certificate-renewal
            pplx::task<std::pair<web::http::http_response, utility::string_t>> request_certificate_renewal(web::http::client::http_client client, const utility::string_t& key, const utility::string_t& csr, verify_cert_handler verify_certificate, slog::base_gate& gate, const pplx::cancellation_token& token = pplx::cancellation_token::none())
            {
                slog::log<slog::severities::too_much_info>(gate, SLOG_FLF) << "Requesting certificate renewal";

                using namespace web::http;

                // <api_proto>://<hostname>:<port>/.well-known/est[/<api_selector>]/simplereenroll
                // see https://specs.amwa.tv/bcp-003-03/releases/v1.0.0/docs/1.0._Certificate_Provisioning.html#est-server-api
                // see https://tools.ietf.org/html/rfc7030#section-4.2.2
                http_request req(methods::POST);
                req.headers().add(header_names::content_type, U("application/pkcs10"));
                req.set_request_uri(U("simplereenroll"));
                req.set_body(csr);

                return nmos::api_request(client, req, gate, token).then([=, &gate](pplx::task<http_response> response_task)
                {
                    auto response = response_task.get(); // may throw http_exception

                    if (status_codes::OK == response.status_code())
                    {
                        return extract_and_verify_certificate(response, verify_certificate, gate, token);
                    }
                    return pplx::task_from_result(std::pair<web::http::http_response, utility::string_t>(response, {}));

                }, token);
            }

            // make an asynchronously GET request on CRL endpoint to obtain the Certificate Revocation List, then check is certificate revoked
            pplx::task<bool> certificate_revocation(web::http::client::http_client client, const utility::string_t& cacerts, const utility::string_t& cert, slog::base_gate& gate, const pplx::cancellation_token& token = pplx::cancellation_token::none())
            {
                slog::log<slog::severities::too_much_info>(gate, SLOG_FLF) << "Certificate revocation check at " << client.base_uri().to_string();

                using namespace web::http;

                // fetch certificate revocation list
                // see https://specs.amwa.tv/bcp-003-03/releases/v1.0.0/docs/1.0._Certificate_Provisioning.html#certificate-revocation
                return nmos::api_request(client, methods::GET, gate, token).then([=, &gate](pplx::task<http_response> response_task)
                {
                    auto response = response_task.get(); // may throw http_exception

                    if (status_codes::OK == response.status_code())
                    {
                        // extract certificate revocation list from the response body
                        return extract_certificate_revocation_list(response, gate, token);
                    }
                    return pplx::task_from_result(utility::string_t{});

                }, token).then([=, &gate](utility::string_t crl)
                {
                    return is_revoked_by_crl(utility::us2s(cert), utility::us2s(cacerts), utility::us2s(crl));
                });
            }

            csr make_rsa_csr(const nmos::settings& settings)
            {
                // generate RSA CSR
                const auto common_name = utility::us2s(get_host_name(settings));
                const auto country = utility::us2s(nmos::experimental::fields::country(settings));
                const auto state = utility::us2s(nmos::experimental::fields::state(settings));
                const auto city = utility::us2s(nmos::experimental::fields::city(settings));
                const auto organization = utility::us2s(get_domain(settings));
                const auto organizational_unit = utility::us2s(nmos::experimental::fields::organizational_unit(settings));
                const auto email_address = utility::us2s(nmos::experimental::fields::email_address(settings));

                return make_rsa_csr(common_name, country, state, city, organization, organizational_unit, email_address);
            }

            csr make_ecdsa_csr(const nmos::settings& settings)
            {
                // generate ECDSA CSR
                const auto common_name = utility::us2s(get_host_name(settings));
                const auto country = utility::us2s(nmos::experimental::fields::country(settings));
                const auto state = utility::us2s(nmos::experimental::fields::state(settings));
                const auto city = utility::us2s(nmos::experimental::fields::city(settings));
                const auto organization = utility::us2s(get_domain(settings));
                const auto organizational_unit = utility::us2s(nmos::experimental::fields::organizational_unit(settings));
                const auto email_address = utility::us2s(nmos::experimental::fields::email_address(settings));

                return make_ecdsa_csr(common_name, country, state, city, organization, organizational_unit, email_address);
            }

            // "Renewal of the Root CA SHOULD be attempted no sooner than 50% of the certificate's expiry time. It is RECOMMENDED that certificate renewal is performed after 80% of the expiry time.
            // To renew the Root CA and the EST Client's TLS Certificate, follow the Initial Certificate Provisioning workflow, renewing both the Root CA and server certificates in the process."
            // see https://specs.amwa.tv/bcp-003-03/releases/v1.0.0/docs/1.0._Certificate_Provisioning.html#root-certificate-authority-renewal
            pplx::task<void> do_cacerts_renewal_monitor(nmos::model& model, est_shared_state& est_state, slog::base_gate& gate, const pplx::cancellation_token& token)
            {
                // split cacerts chain to list
                auto ca_certs = split_certificate_chain(utility::us2s(est_state.cacerts));

                // find the nearest to expiry time from the list of CA certs
                auto expiry = std::chrono::seconds{ -1 };
                for (auto& cert : ca_certs)
                {
                    auto tmp = std::chrono::seconds(certificate_expiry_from_now(cert, 0.8));
                    if (std::chrono::seconds(-1) == expiry || tmp < expiry)
                    {
                        expiry = tmp;
                    }
                }

                slog::log<slog::severities::info>(gate, SLOG_FLF) << "Root CA certificates renewal for about " << expiry.count() << " seconds";

                auto expiry_time = std::chrono::steady_clock::now() + expiry;
                return pplx::complete_at(expiry_time, token).then([&]()
                {
                    auto lock = model.write_lock(); // in order to update local state
                    // now to renew Root CA
                    slog::log<slog::severities::info>(gate, SLOG_FLF) << "Now to renew Root CA certificates and the EST Client's TLS Certificates";
                    est_state.renewal = true;

                }).then([&](pplx::task<void> finally)
                {
                    auto lock = model.write_lock(); // in order to update local state

                    try { finally.get(); }
                    catch (...) {}

                    model.notify();
                });
            }

            pplx::task<void> do_certificate_request(const est_shared_state& est_state, cert_shared_state& cert_state, slog::base_gate& gate, const pplx::cancellation_token& token)
            {
                using namespace web::http;

                auto retry = std::make_shared<bool>(false);

                return pplx::do_while([=, &est_state, &cert_state, &gate]
                {
                    auto fetch_time = std::chrono::steady_clock::now();
                    return pplx::complete_at(fetch_time + cert_state.delay, token).then([=, &est_state, &cert_state, &gate]()
                    {
                        return request_certificate(*cert_state.client, cert_state.key, cert_state.csr, cert_state.verify, gate, token).then([=, &est_state, &cert_state, &gate](std::pair<web::http::http_response, utility::string_t> result)
                        {
                            cert_state.delay = std::chrono::seconds(0);

                            if (status_codes::OK == result.first.status_code())
                            {
                                cert_state.cert = result.second;

                                // "Renewal of the TLS Certificate SHOULD be attempted no sooner than 50% of the certificate's expiry time or before the 'Not Before' date on the certificate.
                                // It is RECOMMENDED that certificate renewal is performed after 80% of the expiry time."
                                // see https://specs.amwa.tv/bcp-003-03/releases/v1.0.0/docs/1.0._Certificate_Provisioning.html#certificate-renewal
                                cert_state.delay = std::chrono::seconds(certificate_expiry_from_now(utility::us2s(cert_state.cert), 0.8));

                                // get Certificate Revocation List URLs from certificate
                                with_write_lock(cert_state.mutex, [&cert_state]
                                {
                                    cert_state.crl_urls = x509_crl_urls(utility::us2s(cert_state.cert));
                                });

                                // do callback on certificate received
                                if (cert_state.received)
                                {
                                    // callback with server certificate chain
                                    cert_state.received({ cert_state.key, cert_state.cert + est_state.cacerts });
                                }

                                *retry = false;
                            }
                            else if (status_codes::Accepted == result.first.status_code() || status_codes::ServiceUnavailable == result.first.status_code())
                            {
                                // parse the Retry-After header to obtain the retry value
                                // See https://tools.ietf.org/html/rfc7231#section-7.1.3
                                // Retry-After: Fri, 31 Dec 1999 23:59:59 GMT
                                // Retry-After: 120
                                auto& headers = result.first.headers();
                                if (headers.end() != headers.find(U("Retry-After")))
                                {
                                    *retry = true;
                                    cert_state.delay = parse_retry_after(utility::us2s(headers[U("Retry-After")]));
                                    slog::log<slog::severities::warning>(gate, SLOG_FLF) << "Requesting certificate again for about " << cert_state.delay.count() << " seconds";
                                }
                                else
                                {
                                    slog::log<slog::severities::error>(gate, SLOG_FLF) << "Request certificate error: missing Retry-After header";
                                    throw est_service_exception();
                                }
                            }
                            else
                            {
                                slog::log<slog::severities::error>(gate, SLOG_FLF) << "Request certificate error: " << result.first.status_code() << " " << result.first.reason_phrase();
                                throw est_service_exception();
                            }
                        });
                    }).then([retry]()
                    {
                        // continous to re-fetch certificate if retry is set
                        return pplx::task_from_result(*retry);
                    });
                }, token);
            }

            pplx::task<void> do_certificate_renewal_request(nmos::model& model, est_shared_state& est_state, cert_shared_state& cert_state, slog::base_gate& gate, const pplx::cancellation_token& token)
            {
                using namespace web::http;

                return pplx::do_while([=, &est_state, &cert_state, &gate]
                {
                    slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "Requesting certificate renewal for about " << cert_state.delay.count() << " seconds";

                    auto now = std::chrono::steady_clock::now();
                    return pplx::complete_at(now + cert_state.delay, token).then([=, &est_state, &cert_state, &gate]() mutable
                    {
                        return request_certificate_renewal(*cert_state.client, cert_state.key, cert_state.csr, cert_state.verify, gate, token).then([&est_state, &cert_state, &gate](std::pair<web::http::http_response, utility::string_t> result)
                        {
                            if (status_codes::OK == result.first.status_code())
                            {
                                cert_state.cert = result.second;

                                // do callback on certificate received
                                if (cert_state.received)
                                {
                                    // callback with server certificate chain
                                    cert_state.received({ cert_state.key, cert_state.cert + est_state.cacerts });
                                }

                                // "Renewal of the TLS Certificate SHOULD be attempted no sooner than 50% of the certificate's expiry time or before the 'Not Before' date on the certificate.
                                // It is RECOMMENDED that certificate renewal is performed after 80% of the expiry time."
                                // see https://specs.amwa.tv/bcp-003-03/releases/v1.0.0/docs/1.0._Certificate_Provisioning.html#certificate-renewal
                                cert_state.delay = std::chrono::seconds(certificate_expiry_from_now(utility::us2s(result.second), 0.8));

                                with_write_lock(cert_state.mutex, [&cert_state]
                                {
                                    if (cert_state.delay.count() > 0)
                                    {
                                        // cache Certificate Revocation List URLs from certificate
                                        cert_state.crl_urls = x509_crl_urls(utility::us2s(cert_state.cert));
                                    }
                                    else
                                    {
                                        // certificate has expired
                                        cert_state.expired = true;
                                    }
                                });
                            }
                            else if (status_codes::Accepted == result.first.status_code() || status_codes::ServiceUnavailable == result.first.status_code())
                            {
                                // parse the retry-after header to obtain the retry next value
                                // See https://tools.ietf.org/html/rfc7231#section-7.1.3
                                // Retry-After: Fri, 31 Dec 1999 23:59:59 GMT
                                // Retry-After: 120
                                auto& headers = result.first.headers();
                                if (headers.end() != headers.find(U("Retry-After")))
                                {
                                    cert_state.delay = parse_retry_after(utility::us2s(headers[U("Retry-After")]));
                                    slog::log<slog::severities::warning>(gate, SLOG_FLF) << "Requesting certificate renewal again for about " << cert_state.delay.count() << " seconds";
                                }
                                else
                                {
                                    slog::log<slog::severities::error>(gate, SLOG_FLF) << "Request certificate renewal error: missing Retry-After header";
                                    throw est_service_exception();
                                }
                            }
                            else
                            {
                                slog::log<slog::severities::error>(gate, SLOG_FLF) << "Request certificate renewal error: " << result.first.status_code() << " " << result.first.reason_phrase();
                                throw est_service_exception();
                            }
                        });
                    }).then([=, &cert_state]()
                    {
                        // continous to fetch certificate if retry interval is set
                        return pplx::task_from_result(cert_state.delay.count() > 0);
                    });
                }, token).then([&](pplx::task<void> finally)
                {
                    auto lock = model.write_lock(); // in order to update local state

                    try
                    {
                        finally.get();
                    }
                    catch (const web::http::http_exception& e)
                    {
                        slog::log<slog::severities::error>(gate, SLOG_FLF) << "EST API certificate renewal request HTTP error: " << e.what() << " [" << e.error_code() << "]";
                    }
                    catch (const web::json::json_exception& e)
                    {
                        slog::log<slog::severities::error>(gate, SLOG_FLF) << "EST API certificate renewal request JSON error: " << e.what();
                    }
                    catch (const est_service_exception&)
                    {
                        slog::log<slog::severities::error>(gate, SLOG_FLF) << "EST API certificate renewal request error";
                    }
                    catch (const est_exception& e)
                    {
                        slog::log<slog::severities::error>(gate, SLOG_FLF) << "EST API certificate renewal request EST error: " << e.what();
                    }
                    catch (const std::exception& e)
                    {
                        slog::log<slog::severities::error>(gate, SLOG_FLF) << "EST API certificate renewal request unexpected exception: " << e.what();
                    }
                    catch (...)
                    {
                        slog::log<slog::severities::severe>(gate, SLOG_FLF) << "EST API certificate renewal request unexpected unknown exception";
                    }

                    // reaching here, there must be something has gone wrong with the EST Server
                    // let select the next avaliable Authorization Server
                    if (!est_state.renewal)
                    {
                        est_state.est_service_error = true;
                    }

                    model.notify();
                });
            }

            pplx::task<bool> do_certificate_revocation(std::vector<web::http::client::http_client> clients, const utility::string_t& cacerts, const utility::string_t& cert, slog::base_gate& gate, const pplx::cancellation_token& token)
            {
                slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "Do certificate revocation check";

                std::vector<pplx::task<bool>> tasks;
                for (auto& client : clients)
                {
                    tasks.push_back(certificate_revocation(client, cacerts, cert, gate, token));
                }

                return pplx::when_all(tasks.begin(), tasks.end()).then([&, tasks](pplx::task<std::vector<bool>> finally)
                {
                    // to ensure an exception from one doesn't leave other tasks' exceptions unobserved
                    for (auto& task : tasks)
                    {
                        try
                        {
                            task.wait();
                        }
                        catch (const web::http::http_exception& e)
                        {
                            slog::log<slog::severities::warning>(gate, SLOG_FLF) << "Certificate revocation HTTP error: " << e.what() << " [" << e.error_code() << "]";
                        }
                        catch (const est_exception& e)
                        {
                            slog::log<slog::severities::warning>(gate, SLOG_FLF) << "Certificate revocation error: " << e.what();
                        }
                        catch (const std::exception& e)
                        {
                            slog::log<slog::severities::error>(gate, SLOG_FLF) << "Certificate revocation unexpected exception: " << e.what();
                        }
                        catch (...)
                        {
                            slog::log<slog::severities::severe>(gate, SLOG_FLF) << "Certificate revocation unexpected unknown exception";
                        }
                    }

                    for (const auto& revoked : finally.get())
                    {
                        if (revoked)
                        {
                            return true;
                        }
                    }
                    return false;
                });
            }

            pplx::task<void> do_certificates_revocation(nmos::model& model, est_shared_state& est_state, cert_shared_state& rsa_state, cert_shared_state& ecdsa_state, slog::base_gate& gate, const pplx::cancellation_token& token)
            {
                slog::log<slog::severities::info>(gate, SLOG_FLF) << "Starting certificates revocation";

                const auto certificate_revocation_interval = std::make_shared<std::chrono::seconds>(nmos::experimental::fields::certificate_revocation_interval(model.settings));

                auto cacerts = std::make_shared<utility::string_t>(est_state.cacerts);
                auto rsa_cert = std::make_shared<utility::string_t>(rsa_state.cert);
                auto ecdsa_cert = std::make_shared<utility::string_t>(ecdsa_state.cert);
                auto interval = std::make_shared<std::chrono::seconds>(0);

                auto rsa_crl_urls = std::make_shared<std::vector<std::string>>();
                auto ecdsa_crl_urls = std::make_shared<std::vector<std::string>>();
                auto rsa_crl_clients = std::make_shared<std::vector<web::http::client::http_client>>();
                auto ecdsa_crl_clients = std::make_shared<std::vector<web::http::client::http_client>>();

                return pplx::do_while([=, &model, &est_state, &rsa_state, &ecdsa_state, &gate]
                {
                    slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "Requesting certificate revocation check for about " << interval->count() << " seconds";

                    auto config = with_read_lock(model.mutex, [&model, &est_state, &gate] {return make_http_client_config(model.settings, est_state.load_ca_certificates, gate); });

                    // buildup a list of clients for RSA revolcation check
                    with_read_lock(rsa_state.mutex, [=, &config, &rsa_state] {
                        if (rsa_state.crl_urls != *rsa_crl_urls)
                        {
                            *rsa_crl_urls = rsa_state.crl_urls;
                            rsa_crl_clients->clear();
                            for (const auto& crl_url : rsa_state.crl_urls)
                            {
                                rsa_crl_clients->push_back({ utility::s2us(crl_url), config });
                            }
                        }
                    });

                    // buildup a list of clients for ECSDA revolcation check
                    with_read_lock(ecdsa_state.mutex, [=, &config, &ecdsa_state] {
                        if (ecdsa_state.crl_urls != *ecdsa_crl_urls)
                        {
                            *ecdsa_crl_urls = ecdsa_state.crl_urls;
                            ecdsa_crl_clients->clear();
                            for (const auto& crl_url : ecdsa_state.crl_urls)
                            {
                                ecdsa_crl_clients->push_back({ utility::s2us(crl_url), config });
                            }
                        }
                    });

                    // create tasks for RSA and ECSDA revolcation check
                    auto now = std::chrono::steady_clock::now();
                    return pplx::complete_at(now + *interval, token).then([=, &model, &rsa_state, &ecdsa_state, &gate]()
                    {
                        *interval = *certificate_revocation_interval;

                        const std::vector<pplx::task<bool>> tasks{
                            do_certificate_revocation(*rsa_crl_clients, *cacerts, *rsa_cert, gate, token),
                            do_certificate_revocation(*ecdsa_crl_clients, *cacerts, *ecdsa_cert, gate, token)
                        };

                        return pplx::when_all(tasks.begin(), tasks.end()).then([&model, &rsa_state, &ecdsa_state, tasks](pplx::task<std::vector<bool>> finally)
                        {
                            for (auto& task : tasks) { try { task.wait(); } catch (...) {} }

                            bool revoked{ false };
                            auto results = finally.get();
                            if (results[0])
                            {
                                auto lock = rsa_state.write_lock();
                                rsa_state.expired = true;
                                revoked = true;
                            }
                            if (results[1])
                            {
                                auto lock = ecdsa_state.write_lock();
                                ecdsa_state.expired = true;
                                revoked = true;
                            }
                            return !revoked;
                        });
                    });

                }).then([&](pplx::task<void> finally)
                {
                    auto lock = model.write_lock(); // in order to update local state

                    try { finally.wait(); } catch (...) {}

                    // reaching here, there must be because at least one of the certificates is revoked or shutdown
                    // lets restart initial certificate provisioning
                    slog::log<slog::severities::info>(gate, SLOG_FLF) << "Stopping certificates revocation, it could be due to a certificate(s) has been revoked";

                    model.notify();
                });
            }

            // fetch Root CA certificates
            bool do_ca_certificates_requests(nmos::model& model, est_shared_state& state, slog::base_gate& gate)
            {
                slog::log<slog::severities::info>(gate, SLOG_FLF) << "Attempting Root CA certificates fetch";

                auto lock = model.write_lock();
                auto& condition = model.condition;
                auto& shutdown = model.shutdown;

                pplx::cancellation_token_source cancellation_source;
                auto request = pplx::task_from_result();

                bool cacerts_received(false);
                bool running(false);

                auto verify_cacerts = [](const utility::string_t& cacerts)
                {
                    if (!cacerts.empty())
                    {
                        // split the cacerts chain to list
                        auto ca_certs = split_certificate_chain(utility::us2s(cacerts));

                        std::string issuer_name;
                        for (auto& cert : ca_certs)
                        {
                            // verify certificate's NotBefore, Not After, Common Name, Subject Alternative Name and chain of trust
                            const auto cert_info = cert_information(cert);

                            // verify the chain against the certificate issuer name and the issuer certificate common name
                            if (!issuer_name.empty())
                            {
                                if (boost::to_upper_copy(issuer_name) != boost::to_upper_copy(cert_info.common_name)) { throw est_exception("invalid CA chain of trust"); }
                            }
                            issuer_name = cert_info.issuer_name;

                            // Not Before
                            const auto now = time(NULL);
                            if (cert_info.not_before > now) { throw est_exception("CA certificate is not valid yet"); }

                            // Not After
                            if (cert_info.not_after < now) { throw est_exception("CA certificate has expired"); }
                        }
                    }
                    else
                    {
                        throw est_exception("missing CA certificates to verify");
                    }
                };

                for (;;)
                {
                    // wait for the thread to be interrupted because an error has been encountered with the selected EST service
                    // or because the server is being shut down
                    // (or this is the first time through)
                    condition.wait(lock, [&] { return shutdown || state.est_service_error || cacerts_received || !running; });
                    running = true;
                    if (state.est_service_error)
                    {
                        pop_est_service(model.settings);
                        model.notify();
                        state.est_service_error = false;

                        cancellation_source.cancel();
                        // wait without the lock since it is also used by the background tasks
                        nmos::details::reverse_lock_guard<nmos::write_lock> unlock{ lock };
                        request.wait();

                        state.client.reset();
                        cancellation_source = pplx::cancellation_token_source();
                    }
                    if (shutdown || empty_est_services(model.settings) || cacerts_received) break;

                    // selects a EST API to use based on the priority
                    if (!state.client)
                    {
                        const auto service = top_est_service(model.settings);

                        // "If explicit trust of the EST Server is Enabled, the EST Client SHOULD make a HTTPS request to the /cacerts endpoint of the EST Server for the latest Root CA of the current network.
                        // The EST Client SHOULD explicitly trust the EST Server manually configured or discovered using Unicast DNS and not perform authentication of the EST Server's TLS Certificate during the initial request to the EST Server.
                        //
                        // If explicit trust of the EST Server is Disabled, the EST Client SHOULD make a HTTPS request to the /cacerts endpoint of the EST Server for the latest Root CA of the current network.
                        // The EST Client MUST perform authentication of the EST Server's TLS Certificate, using the list of trusted Certificate Authorities."
                        // see https://specs.amwa.tv/bcp-003-03/releases/v1.0.0/docs/1.0._Certificate_Provisioning.html#getting-the-root-ca
                        const auto trusted = nmos::experimental::fields::explicit_trust_est_enabled(model.settings);
                        const auto est_uri = service.second;
                        state.client.reset(new web::http::client::http_client(est_uri, make_est_client_config(model.settings, state.load_ca_certificates, state.load_client_certificate, trusted, gate)));
                    }

                    auto token = cancellation_source.get_token();

                    request = request_ca_certificates(*state.client, verify_cacerts, gate, token).then([&model, &state, &gate, token](utility::string_t cacerts)
                    {
                        // record the cacerts (CA chain) for later use
                        state.cacerts = cacerts;

                        // do callback allowing user to store the Root CA
                        if (state.receive_ca_certificate)
                        {
                            // extract the Root CA from chain, Root CA is always presented in the end of the CA chain
                            const auto ca_certs = split_certificate_chain(utility::us2s(cacerts));

                            state.receive_ca_certificate(utility::s2us(ca_certs.back()));
                        }

                    }).then([&](pplx::task<void> finally)
                    {
                        auto lock = model.write_lock(); // in order to update local state

                        try
                        {
                            finally.get();
                            cacerts_received = true;
                        }
                        catch (const web::http::http_exception& e)
                        {
                            slog::log<slog::severities::error>(gate, SLOG_FLF) << "EST API CA certificates HTTP error: " << e.what() << " [" << e.error_code() << "]";
                            state.est_service_error = true;
                        }
                        catch (const web::json::json_exception& e)
                        {
                            slog::log<slog::severities::error>(gate, SLOG_FLF) << "EST API CA certificates JSON error: " << e.what();
                            state.est_service_error = true;
                        }
                        catch (const est_service_exception&)
                        {
                            slog::log<slog::severities::error>(gate, SLOG_FLF) << "EST API CA certificates error";
                            state.est_service_error = true;
                        }
                        catch (const est_exception& e)
                        {
                            slog::log<slog::severities::error>(gate, SLOG_FLF) << "EST API CA certificates EST error: " << e.what();
                            state.est_service_error = true;
                        }
                        catch (const std::exception& e)
                        {
                            slog::log<slog::severities::error>(gate, SLOG_FLF) << "EST API CA certificates unexpected exception: " << e.what();
                            state.est_service_error = true;
                        }
                        catch (...)
                        {
                            slog::log<slog::severities::severe>(gate, SLOG_FLF) << "EST API CA certificates unexpected unknown exception";
                            state.est_service_error = true;
                        }

                        model.notify();
                    });

                    // wait for the request because interactions with the EST API endpoint must be sequential
                    condition.wait(lock, [&] { return shutdown || state.est_service_error || cacerts_received; });
                }

                cancellation_source.cancel();
                // wait without the lock since it is also used by the background tasks
                nmos::details::reverse_lock_guard<nmos::write_lock> unlock{ lock };
                request.wait();

                return !state.est_service_error && cacerts_received;
            }

            // fetch RSA and ECDSA server certificates
            bool do_certificates_requests(nmos::model& model, est_shared_state& est_state, cert_shared_state& rsa_state, cert_shared_state& ecdsa_state, slog::base_gate& gate)
            {
                slog::log<slog::severities::info>(gate, SLOG_FLF) << "Attempting certificates fetch";

                auto lock = model.write_lock();

                try
                {
                    const auto service = top_est_service(model.settings);
                    const auto est_uri = service.second;

                    const auto FQDN = nmos::get_host_name(model.settings);
                    const auto& cacerts = est_state.cacerts;
                    auto verify_certificate = [FQDN, &cacerts](const utility::string_t& certificate)
                    {
                        if (!certificate.empty())
                        {
                            // verify certificate's NotBefore, Not After, Common Name, Subject Alternative Name and chain of trust
                            const auto cert_info = nmos::experimental::details::cert_information(utility::us2s(certificate));

                            // Not Before
                            const auto now = time(NULL);
                            if (cert_info.not_before > now) { throw est_exception("certificate is not valid yet"); }

                            // Not After
                            if (cert_info.not_after < now) { throw est_exception("certificate has expired"); }

                            // Common Name
                            if (boost::to_upper_copy(utility::us2s(FQDN)) != boost::to_upper_copy(cert_info.common_name)) { throw est_exception("invalid Common Name"); }

                            // Subject Alternative Name
                            auto found_san = std::find_if(cert_info.sans.begin(), cert_info.sans.end(), [&FQDN](const std::string& san) { return boost::to_upper_copy(utility::us2s(FQDN)) == boost::to_upper_copy(san); });
                            if (cert_info.sans.end() == found_san) { throw est_exception("invalid Subject Alternative Name"); }

                            // chain of trust
                            const auto cacerts_info = nmos::experimental::details::cert_information(utility::us2s(cacerts));
                            if (cacerts_info.common_name != cert_info.issuer_name) { throw est_exception("invalid chain of trust"); }
                        }
                        else
                        {
                            throw est_exception("missing certificate to verify");
                        }
                    };

                    // generate RSA CSR with RSA key
                    const auto rsa_csr = make_rsa_csr(model.settings);
                    with_write_lock(rsa_state.mutex, [&]() {
                        rsa_state.key = utility::s2us(rsa_csr.first);
                        rsa_state.csr = utility::s2us(rsa_csr.second);
                        rsa_state.delay = std::chrono::seconds::zero();
                        rsa_state.client.reset(new web::http::client::http_client(est_uri, make_est_client_config(model.settings, est_state.load_ca_certificates, est_state.load_client_certificate, false, gate)));
                        rsa_state.expired = false;
                        rsa_state.cert = U("");
                        rsa_state.verify = verify_certificate;
                        });
                    // generate ECDSA CSR with ECDSA key
                    const auto ecdsa_csr = make_ecdsa_csr(model.settings);
                    with_write_lock(ecdsa_state.mutex, [&]() {
                        ecdsa_state.key = utility::s2us(ecdsa_csr.first);
                        ecdsa_state.csr = utility::s2us(ecdsa_csr.second);
                        ecdsa_state.delay = std::chrono::seconds::zero();
                        ecdsa_state.client.reset(new web::http::client::http_client(est_uri, make_est_client_config(model.settings, est_state.load_ca_certificates, est_state.load_client_certificate, false, gate)));
                        ecdsa_state.expired = false;
                        ecdsa_state.cert = U("");
                        ecdsa_state.verify = verify_certificate;
                        });
                }
                catch (const est_exception& e)
                {
                    slog::log<slog::severities::error>(gate, SLOG_FLF) << "Logic error to generate CSR: " << e.what();
                    return false;
                }
                catch (const std::exception& e)
                {
                    slog::log<slog::severities::error>(gate, SLOG_FLF) << "Generate CSR unexpected exception: " << e.what();
                    return false;
                }
                catch (...)
                {
                    slog::log<slog::severities::severe>(gate, SLOG_FLF) << "Generate CSR unexpected unknown exception";
                    return false;
                }

                auto& condition = model.condition;
                auto& shutdown = model.shutdown;

                auto& service_error = est_state.est_service_error;

                pplx::cancellation_token_source cancellation_source;
                auto token = cancellation_source.get_token();

                const std::vector<pplx::task<void>> tasks{
                    do_certificate_request(est_state, rsa_state, gate, token),
                    do_certificate_request(est_state, ecdsa_state, gate, token)
                };

                bool all_done(false);
                auto completed = pplx::when_all(tasks.begin(), tasks.end()).then([&, tasks](pplx::task<void> finally)
                {
                    auto lock = model.write_lock(); // in order to update local state

                    // to ensure an exception from one doesn't leave other tasks' exceptions unobserved
                    for (auto& task : tasks)
                    {
                        try
                        {
                            task.wait();
                        }
                        catch (const web::http::http_exception& e)
                        {
                            slog::log<slog::severities::error>(gate, SLOG_FLF) << "EST API certificate fetch HTTP error: " << e.what() << " [" << e.error_code() << "]";
                            service_error = true;
                        }
                        catch (const web::json::json_exception& e)
                        {
                            slog::log<slog::severities::error>(gate, SLOG_FLF) << "EST API certificate fetch JSON error: " << e.what();
                            service_error = true;
                        }
                        catch (const est_service_exception&)
                        {
                            slog::log<slog::severities::error>(gate, SLOG_FLF) << "EST API certificate fetch error";
                            service_error = true;
                        }
                        catch (const est_exception& e)
                        {
                            slog::log<slog::severities::error>(gate, SLOG_FLF) << "EST API certificate fetch EST error: " << e.what();
                            service_error = true;
                        }
                        catch (const std::exception& e)
                        {
                            slog::log<slog::severities::error>(gate, SLOG_FLF) << "EST API certificate fetch unexpected exception: " << e.what();
                            service_error = true;
                        }
                        catch (...)
                        {
                            slog::log<slog::severities::severe>(gate, SLOG_FLF) << "EST API certificate fetch unexpected unknown exception";
                            service_error = true;
                        }
                    }

                    try { finally.wait(); } catch (...) {}

                    all_done = true;

                    model.notify();
                });

                // wait for the request because interactions with the EST API endpoint must be sequential
                condition.wait(lock, [&] { return shutdown || service_error || all_done; });

                cancellation_source.cancel();
                // wait without the lock since it is also used by the background tasks
                nmos::details::reverse_lock_guard<nmos::write_lock> unlock{ lock };
                completed.wait();

                return !service_error;
            }

            // monitor when to renew Root CA certificates, do renewal of RSA and ECDSA server certificates and do certicate revocation check
            void do_renewal_certificates_and_certificates_revocation_requests(nmos::model& model, est_shared_state& est_state, cert_shared_state& rsa_state, cert_shared_state& ecdsa_state, slog::base_gate& gate)
            {
                slog::log<slog::severities::info>(gate, SLOG_FLF) << "Attempting renewal certificates and certificates revocation operation";

                auto lock = model.write_lock();
                auto& condition = model.condition;
                auto& shutdown = model.shutdown;

                pplx::cancellation_token_source cancellation_source;
                auto token = cancellation_source.get_token();

                auto cacerts_renewal_monitor = pplx::task_from_result();
                auto rsa_request = pplx::task_from_result();
                auto ecdsa_request = pplx::task_from_result();
                auto certificates_revocation = pplx::task_from_result();

                bool running(false);

                est_state.renewal = false;

                for (;;)
                {
                    // wait for the thread to be interrupted because an error has been encountered with the selected EST service
                    // or because the server is being shut down
                    // (or this is the first time through)
                    condition.wait(lock, [&] { return shutdown || est_state.est_service_error || !running; });
                    running = true;
                    if (est_state.est_service_error)
                    {
                        pop_est_service(model.settings);
                        model.notify();
                        est_state.est_service_error = false;

                        cancellation_source.cancel();
                        // wait without the lock since it is also used by the background tasks
                        nmos::details::reverse_lock_guard<nmos::write_lock> unlock{ lock };
                        rsa_request.wait();
                        ecdsa_request.wait();

                        est_state.client.reset();
                        rsa_state.client.reset();
                        ecdsa_state.client.reset();
                        cancellation_source = pplx::cancellation_token_source();
                    }
                    if (shutdown || empty_est_services(model.settings)) break;

                    if (!rsa_state.client)
                    {
                        const auto service = top_est_service(model.settings);  // selects a EST API to use based on the priority
                        const auto est_uri = service.second;
                        rsa_state.client.reset(new web::http::client::http_client(est_uri, make_est_client_config(model.settings, est_state.load_ca_certificates, est_state.load_client_certificate, false, gate)));
                    }
                    if (!ecdsa_state.client)
                    {
                        const auto service = top_est_service(model.settings); // selects a EST API to use based on the priority
                        const auto est_uri = service.second;
                        ecdsa_state.client.reset(new web::http::client::http_client(est_uri, make_est_client_config(model.settings, est_state.load_ca_certificates, est_state.load_client_certificate, false, gate)));
                    }

                    auto token = cancellation_source.get_token();

                    // create a background task to monitor when the Root CA certificates are reqiired to renew
                    cacerts_renewal_monitor = do_cacerts_renewal_monitor(model, est_state, gate, token);

                    // create a background task to renew RSA certificate
                    const auto rsa_csr = make_rsa_csr(model.settings);
                    rsa_state.key = utility::s2us(rsa_csr.first);
                    rsa_state.csr = utility::s2us(rsa_csr.second);
                    rsa_request = do_certificate_renewal_request(model, est_state, rsa_state, gate, token);

                    // create a background task to renew ECDSA certificate
                    const auto ecdsa_csr = make_ecdsa_csr(model.settings);
                    ecdsa_state.key = utility::s2us(ecdsa_csr.first);
                    ecdsa_state.csr = utility::s2us(ecdsa_csr.second);
                    ecdsa_request = do_certificate_renewal_request(model, est_state, ecdsa_state, gate, token);

                    // create a background task to check for certificates revocation
                    certificates_revocation = do_certificates_revocation(model, est_state, rsa_state, ecdsa_state, gate, token);

                    // wait for the request because interactions with the EST API endpoint must be sequential
                    condition.wait(lock, [&] { return shutdown || est_state.est_service_error || est_state.renewal || rsa_state.expired || ecdsa_state.expired; });

                    if (est_state.renewal || rsa_state.expired || ecdsa_state.expired) break;
                }

                cancellation_source.cancel();
                nmos::details::reverse_lock_guard<nmos::write_lock> unlock{ lock };
                cacerts_renewal_monitor.wait();
                rsa_request.wait();
                ecdsa_request.wait();
                certificates_revocation.wait();
            }
        }

        // service discovery operation
        namespace details
        {
            void est_services_background_discovery(nmos::model& model, mdns::service_discovery& discovery, slog::base_gate& gate)
            {
                slog::log<slog::severities::info>(gate, SLOG_FLF) << "Adopting background discovery of a EST API";

                auto lock = model.write_lock();
                auto& condition = model.condition;
                auto& shutdown = model.shutdown;

                bool est_services_discovered(false);

                // background tasks may read/write the above local state by reference
                pplx::cancellation_token_source cancellation_source;
                auto token = cancellation_source.get_token();
                pplx::task<void> background_discovery = pplx::do_while([&]
                {
                    // add a short delay since initial discovery or rediscovery must have only just failed
                    // (this also prevents a tight loop in the case that the underlying DNS-SD implementation is just refusing to co-operate
                    // though that would be better indicated by an exception from discover_est_services)
                    return pplx::complete_after(std::chrono::seconds(1), token).then([&]
                    {
                        return !discover_est_services(model, discovery, gate, token);
                    });
                }, token).then([&]
                {
                    auto lock = model.write_lock(); // in order to update local state

                    est_services_discovered = true; // since discovery must have succeeded

                    model.notify();
                });

                for (;;)
                {
                    // wait for the thread to be interrupted because a EST API has been discovered
                    // or because the server is being shut down
                    condition.wait(lock, [&] { return shutdown || est_services_discovered; });
                    if (shutdown || est_services_discovered) break;
                }

                cancellation_source.cancel();
                // wait without the lock since it is also used by the background tasks
                nmos::details::reverse_lock_guard<nmos::write_lock> unlock{ lock };
                background_discovery.wait();
            }
        }
    }
}
