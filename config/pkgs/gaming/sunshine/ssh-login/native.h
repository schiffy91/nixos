#include <curl/curl.h>
#include <openssl/ssl.h>
#include <openssl/pem.h>
#include <openssl/crypto.h>


/* The native importer cannot lower OpenSSL's unused password callback or
 * libcurl's variadic options. These declarations name the same ABI symbols;
 * actual TLS/write callbacks retain their full function types in BTRC. */
X509 *sunshine_read_certificate(BIO *, X509 **, void *, void *) __asm__("PEM_read_bio_X509");
CURLcode sunshine_setopt_pointer(CURL *, CURLoption, void *) __asm__("curl_easy_setopt");
CURLcode sunshine_setopt_long(CURL *, CURLoption, long) __asm__("curl_easy_setopt");
CURLcode sunshine_response_code(CURL *, CURLINFO, long *) __asm__("curl_easy_getinfo");
