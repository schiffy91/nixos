#ifndef NIXOS_SUNSHINE_SSH_LOGIN_H
#define NIXOS_SUNSHINE_SSH_LOGIN_H
#include <curl/curl.h>
#include <openssl/crypto.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

typedef struct SunshineHttpResponseStorage {
    unsigned char bytes[20001];
    size_t count;
    long httpCode;
    int status;
} *SunshineHttpResponseHandle;

static inline void sunshineHttpResponseRelease(SunshineHttpResponseHandle response) {
    free(response);
}

static inline int sunshineHttpResponseStatus(SunshineHttpResponseHandle response) {
    return response ? response->status : -3;
}

static inline long sunshineHttpResponseCode(SunshineHttpResponseHandle response) {
    return response ? response->httpCode : 0;
}

static inline const unsigned char *sunshineHttpResponseBytes(SunshineHttpResponseHandle response) {
    static const unsigned char empty = 0;
    if (!response) return &empty;
    return response->bytes;
}

static inline int sunshineHttpResponseLength(SunshineHttpResponseHandle response) {
    return response ? (int)response->count : 0;
}

static inline size_t sunshineCaptureLoginResponse(char *data, size_t size, size_t count, void *context) {
    SunshineHttpResponseHandle response = (SunshineHttpResponseHandle)context;
    if (!response) return 0;
    if (size != 0 && count > 20000u / size) return 0;
    size_t length = size * count;
    if (length > 20000u - response->count) return 0;
    if (length > 0) {
        memcpy(response->bytes + response->count, data, length);
        response->count += length;
        response->bytes[response->count] = '\0';
    }
    return length;
}

static inline int sunshineVerifyLoginCertificate(X509_STORE_CTX *store, void *expected) {
    unsigned char digest[32];
    unsigned int size = 0;
    X509 *certificate = X509_STORE_CTX_get0_cert(store);
    bool valid = certificate != NULL &&
        X509_digest(certificate, EVP_sha256(), digest, &size) == 1 &&
        size == 32u &&
        CRYPTO_memcmp(digest, expected, 32) == 0;
    X509_STORE_CTX_set_error(store, valid ? X509_V_OK : X509_V_ERR_CERT_REJECTED);
    return valid ? 1 : 0;
}

static inline CURLcode sunshineConfigureLoginTls(CURL *connection, void *context, void *pin) {
    (void)connection;
    SSL_CTX_set_cert_verify_callback((SSL_CTX *)context, sunshineVerifyLoginCertificate, pin);
    return CURLE_OK;
}

static inline bool sunshineCertificatePin(const char *file, unsigned char digest[32]) {
    if (!file) return false;
    BIO *input = BIO_new_file(file, "r");
    if (!input) return false;
    X509 *certificate = PEM_read_bio_X509(input, NULL, NULL, NULL);
    BIO_free(input);
    if (!certificate) return false;
    unsigned int length = 0;
    bool valid = X509_digest(certificate, EVP_sha256(), digest, &length) == 1 && length == 32u;
    X509_free(certificate);
    return valid;
}

static inline SunshineHttpResponseHandle sunshineHttpsPost(const char *url, const char *body, const char *certificate) {
    SunshineHttpResponseHandle response = calloc(1, sizeof(*response));
    if (!response) return NULL;
    unsigned char pin[32];
    bool usePin = certificate != NULL && certificate[0] != '\0';
    if (usePin && !sunshineCertificatePin(certificate, pin)) {
        response->status = -1;
        return response;
    }
    CURL *connection = curl_easy_init();
    if (!connection) {
        response->status = -2;
        return response;
    }
    struct curl_slist *headers = curl_slist_append(NULL, "Content-Type: application/json");
    if (!url || !body || !headers) {
        curl_easy_cleanup(connection);
        curl_slist_free_all(headers);
        response->status = -3;
        return response;
    }
    bool configured = true;
    configured = (curl_easy_setopt(connection, CURLOPT_URL, url) == CURLE_OK) && configured;
    configured = (curl_easy_setopt(connection, CURLOPT_PROXY, "") == CURLE_OK) && configured;
    configured = (curl_easy_setopt(connection, CURLOPT_TIMEOUT, 15L) == CURLE_OK) && configured;
    configured = (curl_easy_setopt(connection, CURLOPT_HTTPHEADER, headers) == CURLE_OK) && configured;
    configured = (curl_easy_setopt(connection, CURLOPT_POSTFIELDS, body) == CURLE_OK) && configured;
    configured = (curl_easy_setopt(connection, CURLOPT_POSTFIELDSIZE, (long)strlen(body)) == CURLE_OK) && configured;
    configured = (curl_easy_setopt(connection, CURLOPT_WRITEFUNCTION, sunshineCaptureLoginResponse) == CURLE_OK) && configured;
    configured = (curl_easy_setopt(connection, CURLOPT_WRITEDATA, response) == CURLE_OK) && configured;
    if (usePin) {
        // Exact leaf pin replaces CA/hostname trust inside the TLS handshake
        // before libcurl sends any HTTP login payload.
        configured = (curl_easy_setopt(connection, CURLOPT_SSL_VERIFYHOST, 0L) == CURLE_OK) && configured;
        configured = (curl_easy_setopt(connection, CURLOPT_SSL_VERIFYPEER, 1L) == CURLE_OK) && configured;
        configured = (curl_easy_setopt(connection, CURLOPT_SSL_CTX_FUNCTION, sunshineConfigureLoginTls) == CURLE_OK) && configured;
        configured = (curl_easy_setopt(connection, CURLOPT_SSL_CTX_DATA, (void *)pin) == CURLE_OK) && configured;
    }
    CURLcode status = configured ? curl_easy_perform(connection) : CURLE_FAILED_INIT;
    curl_easy_getinfo(connection, CURLINFO_RESPONSE_CODE, &response->httpCode);
    curl_easy_cleanup(connection);
    curl_slist_free_all(headers);
    response->status = (status == CURLE_OK) ? 0 : -4;
    return response;
}
#endif
