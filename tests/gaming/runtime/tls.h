#ifndef NIXOS_TEST_TLS_H
#define NIXOS_TEST_TLS_H
#include <openssl/ssl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <unistd.h>
#include <poll.h>
#include <signal.h>
#include <stdlib.h>
/* Transport only: HTTP, challenge policy and assertions live in BTRC. */
typedef struct TestTLSListener { SSL_CTX *context; int fd; int port; } TestTLSListener;
typedef struct TestTLSConnection { SSL *ssl; int fd; unsigned char bytes[8192]; int length; } TestTLSConnection;
static void testTLSListenerClose(TestTLSListener *listener) {
    if (!listener) return;
    if (listener->fd >= 0) close(listener->fd);
    SSL_CTX_free(listener->context); free(listener);
}
static TestTLSListener *testTLSListen(const char *certificate, const char *key) {
    TestTLSListener *listener = calloc(1, sizeof(*listener));
    if (!listener) return NULL;
    listener->fd = -1; listener->context = SSL_CTX_new(TLS_server_method());
    if (!listener->context || SSL_CTX_use_certificate_file(listener->context, certificate, SSL_FILETYPE_PEM) != 1 || SSL_CTX_use_PrivateKey_file(listener->context, key, SSL_FILETYPE_PEM) != 1 || SSL_CTX_check_private_key(listener->context) != 1) goto fail;
    listener->fd = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
    struct sockaddr_in address = { .sin_family = AF_INET, .sin_addr.s_addr = htonl(INADDR_LOOPBACK) };
    socklen_t length = sizeof(address);
    if (listener->fd < 0 || bind(listener->fd, (struct sockaddr *)&address, length) || listen(listener->fd, 16) || getsockname(listener->fd, (struct sockaddr *)&address, &length)) goto fail;
    listener->port = ntohs(address.sin_port);
    signal(SIGPIPE, SIG_IGN);
    return listener;
fail: testTLSListenerClose(listener); return NULL;
}
static int testTLSPort(TestTLSListener *listener) { return listener->port; }
static void testTLSConnectionClose(TestTLSConnection *connection) {
    if (!connection) return;
    SSL_free(connection->ssl); close(connection->fd); free(connection);
}
static TestTLSConnection *testTLSAccept(TestTLSListener *listener) {
    struct pollfd wait = { .fd = listener->fd, .events = POLLIN };
    if (poll(&wait, 1, 1000) <= 0) return NULL;
    int fd = accept4(listener->fd, NULL, NULL, SOCK_CLOEXEC);
    if (fd < 0) return NULL;
    struct timeval limit = { .tv_sec = 5 };
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &limit, sizeof(limit));
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &limit, sizeof(limit));
    TestTLSConnection *connection = calloc(1, sizeof(*connection));
    if (!connection) { close(fd); return NULL; }
    connection->fd = fd; connection->ssl = SSL_new(listener->context);
    if (!connection->ssl || SSL_set_fd(connection->ssl, fd) != 1 || SSL_accept(connection->ssl) != 1) { testTLSConnectionClose(connection); return NULL; }
    return connection;
}
static int testTLSRead(TestTLSConnection *connection) {
    connection->length = SSL_read(connection->ssl, connection->bytes, sizeof(connection->bytes));
    return connection->length;
}
static const unsigned char *testTLSBytes(TestTLSConnection *connection) { return connection->bytes; }
static int testTLSLength(TestTLSConnection *connection) { return connection->length > 0 ? connection->length : 0; }
static int testTLSWrite(TestTLSConnection *connection, const char *bytes, int length, int offset) {
    if (offset < 0 || offset >= length) return -1;
    return SSL_write(connection->ssl, bytes + offset, length - offset);
}
#endif
