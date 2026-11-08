#pragma once
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/x509.h>

/* The pinned native importer cannot lower PEM's optional password callback.
 * Key policy remains in BTRC; these adapters only fix that callback to NULL. */
static inline EVP_PKEY *bundle_private_key(BIO *input) {
    return PEM_read_bio_PrivateKey(input, NULL, NULL, (void *)"");
}
static inline X509 *bundle_certificate(BIO *input) {
    return PEM_read_bio_X509(input, NULL, NULL, NULL);
}
