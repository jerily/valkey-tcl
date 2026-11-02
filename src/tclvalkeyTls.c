#include "tclvalkeyCtx.h"

#ifdef ENABLE_SSL

SSL_CTX *vktcl_CreateTLSContext(Tcl_Interp *interp, const char *ca_file,
    const char *cert_file, const char *key_file)
{
    SSL_CTX *ctx = SSL_CTX_new(TLS_client_method());
    if (ctx == NULL) {
        SetResult("failed to initialize SSL context");
        return NULL;
    }
    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, NULL);
    if (!SSL_CTX_set_min_proto_version(ctx, TLS1_2_VERSION)) {
        SetResult("failed to set minimum TLS version");
        goto error;
    }
    int trusted = ca_file != NULL
        ? SSL_CTX_load_verify_locations(ctx, ca_file, NULL)
        : SSL_CTX_set_default_verify_paths(ctx);
    if (!trusted) {
        SetResult("failed to load TLS CA certificates");
        goto error;
    }
    if (cert_file != NULL &&
        (!SSL_CTX_use_certificate_chain_file(ctx, cert_file) ||
         !SSL_CTX_use_PrivateKey_file(ctx, key_file, SSL_FILETYPE_PEM) ||
         !SSL_CTX_check_private_key(ctx))) {
        SetResult("failed to load TLS client certificate and key");
        goto error;
    }
    return ctx;

error:
    SSL_CTX_free(ctx);
    return NULL;
}

int vktcl_StartTLS(Tcl_Interp *interp, valkeyContext *vk_ctx, SSL_CTX *ssl_ctx) {
    SSL *ssl = SSL_new(ssl_ctx);
    if (ssl == NULL) {
        SetResult("failed to create SSL instance");
        return TCL_ERROR;
    }

    const char *host = vk_ctx->tcp.host;
    ASN1_OCTET_STRING *ip = a2i_IPADDRESS(host);
    int verified;
    if (ip != NULL) {
        verified = X509_VERIFY_PARAM_set1_ip_asc(SSL_get0_param(ssl), host);
        ASN1_OCTET_STRING_free(ip);
    } else {
        verified = SSL_set1_host(ssl, host) && SSL_set_tlsext_host_name(ssl, host);
    }
    if (!verified) {
        SetResult("failed to configure TLS server identity verification");
        SSL_free(ssl);
        return TCL_ERROR;
    }
#ifdef LIBVALKEY_TLS
    int result = valkeyInitiateTLS(vk_ctx, ssl);
#else
    int result = valkeyInitiateSSL(vk_ctx, ssl);
#endif
    if (result != VALKEY_OK) {
        Tcl_SetObjResult(interp, Tcl_ObjPrintf("failed to create SSL channel: %s",
            vk_ctx->errstr));
        SSL_free(ssl);
        return TCL_ERROR;
    }
    // libvalkey owns the SSL object after a successful handshake.
    return TCL_OK;
}

#endif
