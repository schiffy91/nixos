#pragma once
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>

typedef struct SecureBootPrivateKeyStorage { EVP_PKEY *key; } *SecureBootPrivateKey;
typedef struct SecureBootCertificateStorage { X509 *certificate; char fingerprint[96]; } *SecureBootCertificate;

static inline bool secureBootVerificationDeadline(int seconds) {
    if (signal(SIGALRM, SIG_DFL) == SIG_ERR) return false;
    sigset_t deadline;
    sigemptyset(&deadline);
    sigaddset(&deadline, SIGALRM);
    if (sigprocmask(SIG_UNBLOCK, &deadline, NULL) != 0) return false;
    alarm((unsigned int)seconds);
    return true;
}
static inline void secureBootVerificationFinished(void) { alarm(0); }

static inline void secureBootPrivateKeyRelease(SecureBootPrivateKey key) {
    if (!key) return;
    EVP_PKEY_free(key->key);
    free(key);
}
static inline SecureBootPrivateKey secureBootPrivateKeyRead(const char *bytes, int length) {
    BIO *input = BIO_new_mem_buf(bytes, length);
    if (!input) return NULL;
    // An explicit empty password prevents OpenSSL from prompting on encrypted keys.
    EVP_PKEY *parsed = PEM_read_bio_PrivateKey(input, NULL, NULL, (void *)"");
    BIO_free(input);
    if (!parsed) {
        input = BIO_new_mem_buf(bytes, length);
        if (!input) return NULL;
        parsed = d2i_PrivateKey_bio(input, NULL);
        BIO_free(input);
    }
    if (!parsed) return NULL;
    SecureBootPrivateKey key = malloc(sizeof(*key));
    if (!key) { EVP_PKEY_free(parsed); return NULL; }
    key->key = parsed;
    return key;
}
static inline bool secureBootPrivateKeyValid(SecureBootPrivateKey key) {
    EVP_PKEY_CTX *context = EVP_PKEY_CTX_new(key->key, NULL);
    if (!context) return false;
    bool valid = EVP_PKEY_check(context) == 1;
    EVP_PKEY_CTX_free(context);
    return valid;
}
static inline void secureBootCertificateRelease(SecureBootCertificate certificate) {
    if (!certificate) return;
    X509_free(certificate->certificate);
    free(certificate);
}
static inline SecureBootCertificate secureBootCertificateRead(const char *bytes, int length) {
    BIO *input = BIO_new_mem_buf(bytes, length);
    if (!input) return NULL;
    X509 *parsed = PEM_read_bio_X509(input, NULL, NULL, NULL);
    BIO_free(input);
    if (!parsed) {
        input = BIO_new_mem_buf(bytes, length);
        if (!input) return NULL;
        parsed = d2i_X509_bio(input, NULL);
        BIO_free(input);
    }
    if (!parsed) return NULL;
    SecureBootCertificate certificate = malloc(sizeof(*certificate));
    if (!certificate) { X509_free(parsed); return NULL; }
    certificate->certificate = parsed;
    return certificate;
}
static inline bool secureBootKeyMatches(SecureBootPrivateKey key, SecureBootCertificate certificate) {
    EVP_PKEY *publicKey = X509_get_pubkey(certificate->certificate);
    if (!publicKey) return false;
    bool matches = EVP_PKEY_eq(key->key, publicKey) == 1;
    EVP_PKEY_free(publicKey);
    return matches;
}
static inline const char *secureBootCertificateFingerprint(SecureBootCertificate certificate) {
    unsigned char digest[32];
    unsigned int length = 0;
    if (X509_digest(certificate->certificate, EVP_sha256(), digest, &length) != 1 || length != sizeof(digest)) return NULL;
    if (!OPENSSL_buf2hexstr_ex(certificate->fingerprint, sizeof(certificate->fingerprint), NULL,
            digest, sizeof(digest), ':')) return NULL;
    return certificate->fingerprint;
}
