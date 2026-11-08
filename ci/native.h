#ifndef NIXOS_CI_NATIVE_H
#define NIXOS_CI_NATIVE_H
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static inline void ciConfigureIO(void) {
    signal(SIGPIPE, SIG_IGN);
    setvbuf(stdout, NULL, _IOLBF, 0);
}
static inline bool ciWriteSigningKey(const char *path, const char *text) {
    int descriptor = open(path, O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC, 0600);
    if (descriptor < 0) return false;
    size_t remaining = strlen(text);
    bool valid = true;
    while (remaining) {
        ssize_t count = write(descriptor, text, remaining);
        if (count < 0 && errno == EINTR) continue;
        if (count <= 0) { valid = false; break; }
        text += count; remaining -= (size_t)count;
    }
    if (fsync(descriptor) != 0) valid = false;
    if (close(descriptor) != 0) valid = false;
    if (!valid) unlink(path);
    return valid;
}
static inline const char *ciHostname(void) {
    static _Thread_local char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) != 0) return NULL;
    hostname[sizeof(hostname) - 1] = '\0';
    return hostname;
}
#endif
