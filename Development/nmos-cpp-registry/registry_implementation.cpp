#include "registry_implementation.h"

#include "nmos/model.h"
#include "nmos/registry_server.h"
#include "nmos/slog.h"

// example registry implementation details
namespace impl
{
    // custom logging category for the example node implementation thread
    namespace categories
    {
        const nmos::category registry_implementation{ "registry_implementation" };
    }

    // custom settings for the example node implementation
    namespace fields
    {
        // rsa: full path of private key file in PEM format and full path of server chain certificate in PEM format and must be sorted
        // starting with the server's certificate, followed by any intermediate CA certificates, and ending with the highest level (root) CA
        const web::json::field_as_value_or rsa{ U("rsa"), web::json::value_of({ { U("private_key_file"), U("") }, { U("certificate_chain_file"), U("") } }) };

        // ecdsa: full path of private key file in PEM format and full path of server chain certificate file in PEM format and must be sorted
        // starting with the server's certificate, followed by any intermediate CA certificates, and ending with the highest level (root) CA
        const web::json::field_as_value_or ecdsa{ U("ecdsa"), web::json::value_of({ { U("private_key_file"), U("") }, { U("certificate_chain_file"), U("") } }) };
    }
}

// Example callback to load RSA key and chain certificate
nmos::load_cert_handler make_registry_implementation_load_rsa_handler(nmos::registry_model& model, slog::base_gate& gate)
{
    // this example load the RSA key and chain certificate from files for the caller
    if (model.settings.has_field(impl::fields::rsa))
    {
        const auto& rsa = impl::fields::rsa(model.settings);
        const auto private_key_file = utility::us2s(rsa.at(U("private_key_file")).as_string());
        const auto certificate_chain_file = utility::us2s(rsa.at(U("certificate_chain_file")).as_string());

        return[&, private_key_file, certificate_chain_file]()
        {
            slog::log<slog::severities::info>(gate, SLOG_FLF) << nmos::stash_category(impl::categories::registry_implementation) << "Load RSA key and chain certificate";

            std::ifstream pkey_file(private_key_file);
            std::stringstream pkey;
            pkey << pkey_file.rdbuf();

            std::ifstream cert_chain_file(certificate_chain_file);
            std::stringstream cert;
            cert << cert_chain_file.rdbuf();

            return std::pair<utility::string_t, utility::string_t>(utility::s2us(pkey.str()), utility::s2us(cert.str()));
        };
    }
    else
    {
        return nullptr;
    }
}

// Example callback to load ECDSA key and chain certificate
nmos::load_cert_handler make_registry_implementation_load_ecdsa_handler(nmos::registry_model& model, slog::base_gate& gate)
{
    // this example load the ECDSA key and chain certificate from files for the caller
    if (model.settings.has_field(impl::fields::ecdsa))
    {
        const auto& ecdsa = impl::fields::ecdsa(model.settings);
        const auto private_key_file = utility::us2s(ecdsa.at(U("private_key_file")).as_string());
        const auto certificate_chain_file = utility::us2s(ecdsa.at(U("certificate_chain_file")).as_string());

        return[&, private_key_file, certificate_chain_file]()
        {
            slog::log<slog::severities::info>(gate, SLOG_FLF) << nmos::stash_category(impl::categories::registry_implementation) << "Load ECDSA key and chain certificate";

            std::ifstream pkey_file(private_key_file);
            std::stringstream pkey;
            pkey << pkey_file.rdbuf();

            std::ifstream cert_chain_file(certificate_chain_file);
            std::stringstream cert;
            cert << cert_chain_file.rdbuf();

            return std::pair<utility::string_t, utility::string_t>(utility::s2us(pkey.str()), utility::s2us(cert.str()));
        };
    }
    else
    {
        return nullptr;
    }
}

// Example callback to load dh parameters
nmos::load_dh_param_handler make_registry_implementation_load_dh_param_handler(nmos::registry_model& model, slog::base_gate& gate)
{
    // this example load the dh parameters from file for the caller
    if (model.settings.has_field(nmos::experimental::fields::dh_param_file))
    {
        const auto dh_param_file = utility::us2s(nmos::experimental::fields::dh_param_file(model.settings));

        return[&, dh_param_file]()
        {
            slog::log<slog::severities::info>(gate, SLOG_FLF) << nmos::stash_category(impl::categories::registry_implementation) << "Load dh parameters";

            std::ifstream dh_file(dh_param_file);
            std::stringstream dh_param;
            dh_param << dh_file.rdbuf();
            return utility::s2us(dh_param.str());
        };
    }
    else
    {
        return nullptr;
    }
}

// Example callback to load Root CA certificate
nmos::load_cacert_handler make_registry_implementation_load_cacert_handler(nmos::registry_model& model, slog::base_gate& gate)
{
    // this example load the Root CA certificate from file for the caller
    if (model.settings.has_field(nmos::experimental::fields::ca_certificate_file))
    {
        const auto ca_certificate_file = utility::us2s(nmos::experimental::fields::ca_certificate_file(model.settings));

        return [&, ca_certificate_file]()
        {
            slog::log<slog::severities::info>(gate, SLOG_FLF) << nmos::stash_category(impl::categories::registry_implementation) << "Load root cacert";

            std::ifstream ca_file(ca_certificate_file);
            std::stringstream cacerts;
            cacerts << ca_file.rdbuf();
            return utility::s2us(cacerts.str());
        };
    }
    else
    {
        return nullptr;
    }
}

// This constructs all the callbacks used to integrate the example device-specific underlying implementation
// into the server instance for the NMOS Registry.
nmos::experimental::registry_implementation make_registry_implementation(nmos::registry_model& model, slog::base_gate& gate)
{
    return nmos::experimental::registry_implementation()
        .on_load_rsa(make_registry_implementation_load_rsa_handler(model, gate))
        .on_load_ecdsa(make_registry_implementation_load_ecdsa_handler(model, gate))
        .on_load_dh_param(make_registry_implementation_load_dh_param_handler(model, gate))
        .on_load_cacert(make_registry_implementation_load_cacert_handler(model, gate));
}
