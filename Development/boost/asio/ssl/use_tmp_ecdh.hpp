// based on https://github.com/chriskohlhoff/asio/pull/117

#ifndef BOOST_ASIO_SSL_USE_TMP_ECDH_HPP
#define BOOST_ASIO_SSL_USE_TMP_ECDH_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/ssl/context.hpp>

#include <boost/asio/detail/push_options.hpp>

#ifndef BOOST_ASIO_SYNC_OP_VOID
# define BOOST_ASIO_SYNC_OP_VOID void
# define BOOST_ASIO_SYNC_OP_VOID_RETURN(e) return
#endif

#if OPENSSL_VERSION_NUMBER >= 0x30000000L
#include <openssl/core_names.h>
#include <openssl/evp.h>
#endif

namespace boost {
namespace asio {
namespace ssl {

namespace use_tmp_ecdh_details {

struct bio_cleanup
{
    BIO* p;
    ~bio_cleanup() { if (p) ::BIO_free(p); }
};

struct x509_cleanup
{
    X509* p;
    ~x509_cleanup() { if (p) ::X509_free(p); }
};

struct evp_pkey_cleanup
{
    EVP_PKEY* p;
    ~evp_pkey_cleanup() { if (p) ::EVP_PKEY_free(p); }
};

#if OPENSSL_VERSION_NUMBER < 0x30000000L
struct ec_key_cleanup
{
    EC_KEY *p;
    ~ec_key_cleanup() { if (p) ::EC_KEY_free(p); }
};
#endif

inline
BOOST_ASIO_SYNC_OP_VOID do_use_tmp_ecdh(boost::asio::ssl::context& ctx,
    BIO* bio, boost::system::error_code& ec)
{
#if OPENSSL_VERSION_NUMBER < 0x30000000L
    ::ERR_clear_error();

    int nid = NID_undef;

    x509_cleanup x509 = { ::PEM_read_bio_X509(bio, NULL, 0, NULL) };
    if (x509.p)
    {
        evp_pkey_cleanup pkey = { ::X509_get_pubkey(x509.p) };
        if (pkey.p)
        {
            ec_key_cleanup key = { ::EVP_PKEY_get1_EC_KEY(pkey.p) };
            if (key.p)
            {
                const EC_GROUP* group = EC_KEY_get0_group(key.p);
                nid = EC_GROUP_get_curve_name(group);
            }
        }
    }

    ec_key_cleanup ec_key = { ::EC_KEY_new_by_curve_name(nid) };
    if (ec_key.p)
    {
        if (::SSL_CTX_set_tmp_ecdh(ctx.native_handle(), ec_key.p) == 1)
        {
            ec = boost::system::error_code();
            BOOST_ASIO_SYNC_OP_VOID_RETURN(ec);
        }
    }

    ec = boost::system::error_code(
        static_cast<int>(::ERR_get_error()),
        boost::asio::error::get_ssl_category());
    BOOST_ASIO_SYNC_OP_VOID_RETURN(ec);
#else
    ::ERR_clear_error();

    x509_cleanup x509 = { ::PEM_read_bio_X509(bio, NULL, 0, NULL) };
    if (x509.p)
    {
        evp_pkey_cleanup pkey = { ::X509_get_pubkey(x509.p) };
        if (pkey.p)
        {
            char curve_name[64];
            size_t return_size{ 0 };
            if (::EVP_PKEY_get_utf8_string_param(pkey.p, OSSL_PKEY_PARAM_GROUP_NAME, curve_name, sizeof(curve_name), &return_size))
            {
                if (::SSL_CTX_set1_groups_list(ctx.native_handle(), curve_name) == 1)
                {
                    ec = boost::system::error_code();
                    BOOST_ASIO_SYNC_OP_VOID_RETURN(ec);
                }
            }
        }
    }

    ec = boost::system::error_code(
        static_cast<int>(::ERR_get_error()),
        boost::asio::error::get_ssl_category());
    BOOST_ASIO_SYNC_OP_VOID_RETURN(ec);
#endif
}

inline
BOOST_ASIO_SYNC_OP_VOID use_tmp_ecdh_file(boost::asio::ssl::context& ctx,
    const std::string& certificate, boost::system::error_code& ec)
{
    ::ERR_clear_error();

    bio_cleanup bio = { ::BIO_new_file(certificate.c_str(), "r") };
    if (bio.p)
    {
        return do_use_tmp_ecdh(ctx, bio.p, ec);
    }

    ec = boost::system::error_code(
        static_cast<int>(::ERR_get_error()),
        boost::asio::error::get_ssl_category());
    BOOST_ASIO_SYNC_OP_VOID_RETURN(ec);
}

inline
void use_tmp_ecdh_file(boost::asio::ssl::context& ctx, const std::string& certificate)
{
    boost::system::error_code ec;
    use_tmp_ecdh_file(ctx, certificate, ec);
    boost::asio::detail::throw_error(ec, "use_tmp_ecdh_file");
}

inline
BOOST_ASIO_SYNC_OP_VOID use_tmp_ecdh(boost::asio::ssl::context& ctx,
    const boost::asio::const_buffer& certificate, boost::system::error_code& ec)
{
    ::ERR_clear_error();

    bio_cleanup bio = { ::BIO_new_mem_buf(const_cast<unsigned char*>(boost::asio::buffer_cast<const unsigned char*>(certificate)), static_cast<int>(boost::asio::buffer_size(certificate))) };
    if (bio.p)
    {
        return do_use_tmp_ecdh(ctx, bio.p, ec);
    }

    ec = boost::system::error_code(
        static_cast<int>(::ERR_get_error()),
        boost::asio::error::get_ssl_category());
    BOOST_ASIO_SYNC_OP_VOID_RETURN(ec);
}

inline
void use_tmp_ecdh(boost::asio::ssl::context& ctx, const boost::asio::const_buffer& certificate)
{
    boost::system::error_code ec;
    use_tmp_ecdh(ctx, certificate, ec);
    boost::asio::detail::throw_error(ec, "use_tmp_ecdh");
}

} // namespace use_tmp_ecdh_details

using use_tmp_ecdh_details::use_tmp_ecdh_file;
using use_tmp_ecdh_details::use_tmp_ecdh;

} // namespace ssl
} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif // BOOST_ASIO_SSL_USE_TMP_ECDH_HPP
