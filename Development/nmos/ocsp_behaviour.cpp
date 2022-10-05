#include "nmos/ocsp_behaviour.h"

#if !defined(_WIN32) || defined(CPPREST_FORCE_HTTP_CLIENT_ASIO)
#include "pplx/pplx_utils.h" // for pplx::complete_at
#include "nmos/client_utils.h"
#include "nmos/model.h"
#include "nmos/ocsp_settings.h"
#include "nmos/ocsp_utils.h"
#include "nmos/random.h"
#include "nmos/slog.h"
#include "ssl/ssl_utils.h"

namespace nmos
{
    namespace details
    {
        struct ocsp_shared_state
        {
            load_ca_certificates_handler load_ca_certificates;
            web::uri base_uri;
            bool ocsp_service_error;
            std::vector<uint8_t> ocsp_request;

            // how many seconds before next certificate status request
            int next_request;

            nmos::details::seed_generator seeder;
            std::default_random_engine engine;
            std::unique_ptr<web::http::client::http_client> client;

            explicit ocsp_shared_state(load_ca_certificates_handler load_ca_certificates)
                : load_ca_certificates(std::move(load_ca_certificates))
                , ocsp_service_error(false)
                , next_request(0)
                , engine(seeder)
            {}
        };

        void ocsp_behaviour_thread(nmos::model& model, nmos::experimental::ocsp_settings& ocsp_settings, load_ca_certificates_handler load_ca_certificates, load_server_certificates_handler load_client_certificate, slog::base_gate& gate);

        int half_certificate_expiry_from_now(const std::vector<utility::string_t>& certificate_chains_data, slog::base_gate& gate);
        std::vector<web::uri> get_ocsp_uris(const std::vector<utility::string_t>& certificates_chain_data, slog::base_gate& gate);
        std::vector<uint8_t> make_ocsp_request(const std::vector<utility::string_t>& certificate_chains_data, slog::base_gate& gate);
        void ocsp_behaviour(nmos::model& model, nmos::experimental::ocsp_settings& ocsp_settings, std::vector<web::uri>& ocsp_uris, ocsp_shared_state& state, slog::base_gate& gate);
    }

    void ocsp_behaviour_thread(nmos::model& model, nmos::experimental::ocsp_settings& ocsp_settings, load_ca_certificates_handler load_ca_certificates, load_server_certificates_handler load_server_certificate, slog::base_gate& gate_)
    {
        nmos::details::omanip_gate gate(gate_, nmos::stash_category(nmos::categories::ocsp_behaviour));

        details::ocsp_behaviour_thread(model, ocsp_settings, load_ca_certificates, load_server_certificate, gate);
    }

    // callbacks from this function are called with the model locked, and may read or write directly to the model
    void details::ocsp_behaviour_thread(nmos::model& model, nmos::experimental::ocsp_settings& ocsp_settings, load_ca_certificates_handler load_ca_certificates, load_server_certificates_handler load_server_certificate, slog::base_gate& gate)
    {
        enum
        {
            get_ocsp_uris,
            ocsp_behaviour
        } mode = get_ocsp_uris;

        std::vector<web::uri> ocsp_uris;

        nmos::details::seed_generator backoff_seeder;
        std::default_random_engine backoff_engine(backoff_seeder);
        double backoff = 0;

        ocsp_shared_state state{ load_ca_certificates };

        // continue until the server is being shut down
        for (;;)
        {
            if (with_read_lock(model.mutex, [&] { return model.shutdown; })) break;

            switch (mode)
            {
                case get_ocsp_uris:
                {
                    // Note: use the same discovery backoff settings for OCSP API retry
                    if (0 != backoff)
                    {
                        auto lock = model.read_lock();
                        const auto random_backoff = std::uniform_real_distribution<>(0, backoff)(backoff_engine);
                        slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "Waiting to retry OCSP API for about " << std::fixed << std::setprecision(3) << random_backoff << " seconds (current backoff limit: " << backoff << " seconds)";
                        model.wait_for(lock, std::chrono::milliseconds(std::chrono::milliseconds::rep(1000 * random_backoff)), [&] { return model.shutdown; });
                        if (model.shutdown) break;
                    }

                    // get the list of server certificates chain data
                    const auto server_certificates = load_server_certificate();
                    const auto server_certificate_chains = boost::copy_range<std::vector<utility::string_t>>(server_certificates | boost::adaptors::transformed([](const nmos::certificate& cert) { return cert.certificate_chain; }));

                    // get OCSP URIs
                    ocsp_uris = details::get_ocsp_uris(server_certificate_chains, gate);
                    if (ocsp_uris.size())
                    {
                        mode = ocsp_behaviour;

                        // If the Node is unable to contact the OCSP API, the Node implements an exponential backoff algorithm
                        // to avoid overloading the OCSP API in the event of a system restart.
                        auto lock = model.read_lock();
                        backoff = (std::min)((std::max)((double)nmos::fields::discovery_backoff_min(model.settings), backoff * nmos::fields::discovery_backoff_factor(model.settings)), (double)nmos::fields::discovery_backoff_max(model.settings));

                        // extract the shortest half certificate expiry time from all server certificates
                        state.next_request = details::half_certificate_expiry_from_now(server_certificate_chains, gate);

                        // make ocsp request with the server certificates
                        state.ocsp_request = details::make_ocsp_request(server_certificate_chains, gate);
                        if (state.ocsp_request.size())
                        {
                            slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "Using the OCSP API(s):" << slog::log_manip([&](slog::log_statement& s)
                            {
                                for (auto& ocsp_service : ocsp_uris)
                                {
                                    s << '\n' << ocsp_service.to_string();
                                }
                            });
                        }
                    }
                }
                break;

            case ocsp_behaviour:
                details::ocsp_behaviour(model, ocsp_settings, ocsp_uris, state, gate);

                // Should no further OCSP APIs be available, a re-query may be performed.
                mode = get_ocsp_uris;
                break;
            }
        }
    }

    namespace details
    {
        web::http::client::http_client_config make_ocsp_client_config(const nmos::settings& settings, load_ca_certificates_handler load_ca_certificates, slog::base_gate& gate)
        {
            auto config = nmos::make_http_client_config(settings, std::move(load_ca_certificates), gate);
            config.set_timeout(std::chrono::seconds(nmos::experimental::fields::ocsp_request_max(settings)));
            return config;
        }

        struct ocsp_service_exception {};

        // make an asynchronously POST request on the OCSP API to get certificate status
        // see https://specs.amwa.tv/bcp-003-03/releases/v1.0.0/docs/1.0._Certificate_Provisioning.html#certificate-request
        pplx::task<std::vector<uint8_t>> request_certificate_status(web::http::client::http_client client, std::vector<uint8_t>& ocsp_request, slog::base_gate& gate, const pplx::cancellation_token& token = pplx::cancellation_token::none())
        {
            slog::log<slog::severities::too_much_info>(gate, SLOG_FLF) << "Requesting certificate status";

            using namespace web::http;

            http_request req(methods::POST);
            req.headers().add(header_names::content_type, U("application/ocsp-request"));
            req.set_body(ocsp_request);

            return nmos::api_request(client, req, gate, token).then([=, &gate](pplx::task<http_response> response_task)
            {
                auto response = response_task.get(); // may throw http_exception

                if (status_codes::OK == response.status_code())
                {
                    if (response.body())
                    {
                        return response.extract_vector().then([&gate](std::vector<uint8_t> body)
                        {
                            slog::log<slog::severities::too_much_info>(gate, SLOG_FLF) << "Received OCSP response";
                            return body;

                        }, token);
                    }
                    else
                    {
                        slog::log<slog::severities::error>(gate, SLOG_FLF) << "OCSP certificate status request error: missing OCSP response";
                        throw ocsp_service_exception();
                    }
                }
                else
                {
                    slog::log<slog::severities::error>(gate, SLOG_FLF) << "OCSP certificate status request error: " << response.status_code() << " " << response.reason_phrase();
                    throw ocsp_service_exception();
                }
            }, token);
        }

        // task to continuously fetch the certificate status (OCSP response) on a time interval until failure or cancellation
        pplx::task<void> do_certificate_status_requests(nmos::model& model, nmos::experimental::ocsp_settings& ocsp_settings, ocsp_shared_state& state, slog::base_gate& gate, const pplx::cancellation_token& token = pplx::cancellation_token::none())
        {
            const auto& ocsp_interval_min(nmos::experimental::fields::ocsp_interval_min(model.settings));
            const auto& ocsp_interval_max(nmos::experimental::fields::ocsp_interval_max(model.settings));

            // start a background task to continously request certificate status on a given interval
            return pplx::do_while([=, &model, &ocsp_settings, &state, &gate]
            {
                auto request_interval = std::chrono::seconds(0);
                if (state.base_uri == state.client->base_uri())
                {
                    auto interval = std::uniform_int_distribution<>(
                        std::min(ocsp_interval_min, (state.next_request > 0 ? state.next_request : ocsp_interval_min)),
                        std::min(ocsp_interval_max, (state.next_request > 0 ? state.next_request : ocsp_interval_max)))(state.engine);
                    request_interval = std::chrono::seconds(interval);

                    slog::log<slog::severities::more_info>(gate, SLOG_FLF) << "Waiting to request certificate status for about " << interval << " seconds";
                }
                else
                {
                    state.base_uri = state.client->base_uri();
                }

                auto time_now = std::chrono::steady_clock::now();
                return pplx::complete_at(time_now + request_interval, token).then([=, &state, &gate]()
                {
                    return request_certificate_status(*state.client, state.ocsp_request, gate, token);
                }).then([&model, &ocsp_settings, &state, &gate](std::vector<uint8_t> ocsp_resp)
                {
                    // cache the OCSP response

                    auto lock = model.write_lock(); // in order to update local state

                    slog::log<slog::severities::info>(gate, SLOG_FLF) << "cache the OCSP response";

                    nmos::with_write_lock(*ocsp_settings.mutex, [&] { ocsp_settings.ocsp_response = ocsp_resp; });

                    model.notify();

                    return true;
                });
            }).then([&](pplx::task<void> finally)
            {
                auto lock = model.write_lock(); // in order to update local state

                try
                {
                    finally.get();
                }
                catch (const web::http::http_exception& e)
                {
                    slog::log<slog::severities::error>(gate, SLOG_FLF) << "Certificate status request HTTP error: " << e.what() << " [" << e.error_code() << "]";
                }
                catch (const ocsp_service_exception&)
                {
                    slog::log<slog::severities::error>(gate, SLOG_FLF) << "Certificate status request error";
                }

                // reaching here, there must be something has gone wrong with the OCSP API
                // let's select the next available OCSP API
                state.ocsp_service_error = true;

                model.notify();
            });
        }

        int half_certificate_expiry_from_now(const std::vector<utility::string_t>& certificate_chains_data, slog::base_gate& gate)
        {
            int expiry_time = -1;
            try
            {
                // get the shortest expiry time from all the certificates
                for (const auto& cert_data : certificate_chains_data)
                {
                    const auto expiry_time_ = ssl::experimental::certificate_expiry_from_now(utility::us2s(cert_data), 0.5);
                    expiry_time = expiry_time < 0 ? expiry_time_ : std::min(expiry_time, expiry_time_);
                }
            }
            catch (const ssl::experimental::ssl_exception& e)
            {
                slog::log<slog::severities::error>(gate, SLOG_FLF) << "SSL error while getting certificate expiry time: " << e.what();
            }
            catch (const std::exception& e)
            {
                slog::log<slog::severities::error>(gate, SLOG_FLF) << "Unexpected exception while getting certificate expiry time: " << e.what();
            }
            catch (...)
            {
                slog::log<slog::severities::severe>(gate, SLOG_FLF) << "Unexpected unknown exception while getting certificate expiry time";
            }
            return (expiry_time < 0 ? 0 : expiry_time);
        }

        // construct a list of OCSP URIs from a list of server certificate chains
        std::vector<web::uri> get_ocsp_uris(const std::vector<utility::string_t>& certificate_chains_data, slog::base_gate& gate)
        {
            std::vector<web::uri> ocsp_uris;

            try
            {
                for (const auto& cert_chain_data : certificate_chains_data)
                {
                    const auto uris = nmos::experimental::get_ocsp_uris(utility::us2s(cert_chain_data));

                    // only add new OCSP URIs to the list
                    for (const auto& uri : uris)
                    {
                        if (ocsp_uris.end() == std::find(ocsp_uris.begin(), ocsp_uris.end(), uri))
                        {
                            ocsp_uris.push_back(uri);
                        }
                    }
                }
                if (ocsp_uris.empty())
                {
                    throw nmos::experimental::ocsp_exception("missing OCSP URIs from server certificate");
                }
            }
            catch (const nmos::experimental::ocsp_exception& e)
            {
                slog::log<slog::severities::error>(gate, SLOG_FLF) << "OCSP error while getting OCSP URIs: " << e.what();
            }
            catch (const std::exception& e)
            {
                slog::log<slog::severities::error>(gate, SLOG_FLF) << "Unexpected exception while getting OCSP URIs: " << e.what();
            }
            catch (...)
            {
                slog::log<slog::severities::severe>(gate, SLOG_FLF) << "Get OCSP URIs with nexpected unknown exception";
            }
            return ocsp_uris;
        }

        // create OCSP request from the list of server certificate chains
        std::vector<uint8_t> make_ocsp_request(const std::vector<utility::string_t>& certificate_chains_data, slog::base_gate& gate)
        {
            try
            {
                // make OCSP request from multi cert chains
                const auto certificate_chains_data_ = boost::copy_range<std::vector<std::string>>(certificate_chains_data | boost::adaptors::transformed([](const utility::string_t& cert) { return utility::us2s(cert); }));
                return nmos::experimental::make_ocsp_request(certificate_chains_data_);
            }
            catch (const nmos::experimental::ocsp_exception& e)
            {
                slog::log<slog::severities::error>(gate, SLOG_FLF) << "Create OCSP request with OCSP error: " << e.what();
            }
            catch (const std::exception& e)
            {
                slog::log<slog::severities::error>(gate, SLOG_FLF) << "Create OCSP request with unexpected exception: " << e.what();
            }
            catch (...)
            {
                slog::log<slog::severities::severe>(gate, SLOG_FLF) << "Create OCSP request with nexpected unknown exception";
            }
            return {};
        }

        void ocsp_behaviour(nmos::model& model, nmos::experimental::ocsp_settings& ocsp_settings, std::vector<web::uri>& ocsp_uris, ocsp_shared_state& state, slog::base_gate& gate)
        {
            slog::log<slog::severities::info>(gate, SLOG_FLF) << "Attempting certificates status requests";

            auto lock = model.write_lock();
            auto& condition = model.condition;
            auto& shutdown = model.shutdown;

            pplx::cancellation_token_source cancellation_source;
            auto requests = pplx::task_from_result();

            state.base_uri = {};

            for (;;)
            {
                // wait for the thread to be interrupted because an error has been encountered with the selected OCSP service
                // or because the server is being shut down
                // (or this is the first time through)
                condition.wait(lock, [&] { return shutdown || state.ocsp_service_error || state.base_uri.is_empty(); });
                if (state.ocsp_service_error)
                {
                    ocsp_uris.erase(ocsp_uris.begin());

                    state.ocsp_service_error = false;

                    cancellation_source.cancel();
                    // wait without the lock since it is also used by the background tasks
                    nmos::details::reverse_lock_guard<nmos::write_lock> unlock{ lock };
                    requests.wait();

                    state.client.reset();
                    cancellation_source = pplx::cancellation_token_source();
                }
                if (shutdown || ocsp_uris.empty()) break;

                // selects the first OCSP API from the OCSP URI list
                if (!state.client)
                {
                    const auto ocsp_uri = ocsp_uris.front();
                    state.client.reset(new web::http::client::http_client(ocsp_uri, make_ocsp_client_config(model.settings, state.load_ca_certificates, gate)));
                }

                auto token = cancellation_source.get_token();

                // start a background task to intermittently request certificate status (OCSP response)
                requests = do_certificate_status_requests(model, ocsp_settings, state, gate, token);

                condition.wait(lock, [&] { return shutdown || state.ocsp_service_error; });
            }

            cancellation_source.cancel();
            // wait without the lock since it is also used by the background tasks
            nmos::details::reverse_lock_guard<nmos::write_lock> unlock{ lock };
            requests.wait();
        }
    }
}

#endif
