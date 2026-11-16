#ifndef NIXOS_DECKPAD_POSIX_H
#define NIXOS_DECKPAD_POSIX_H
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

typedef struct DeckHidrawReportStorage {
    unsigned char bytes[128];
    int length;
} *DeckHidrawReportHandle;

static inline void deckHidrawReportRelease(DeckHidrawReportHandle handle) {
    free(handle);
}

static inline int deckHidrawReportLength(DeckHidrawReportHandle handle) {
    return handle ? handle->length : -1;
}

static inline const unsigned char *deckHidrawReportBytes(DeckHidrawReportHandle handle) {
    static const unsigned char empty = 0;
    if (!handle || handle->length <= 0) return &empty;
    return handle->bytes;
}

static inline size_t deckHidrawReportSize(DeckHidrawReportHandle handle) {
    return handle && handle->length > 0 ? (size_t)handle->length : 0;
}

static inline int deckOpenHidraw(const char *path) {
    if (!path) return -1;
    return open(path, O_RDONLY | O_CLOEXEC);
}

static inline DeckHidrawReportHandle deckReadHidrawReport(int descriptor) {
    DeckHidrawReportHandle handle = calloc(1, sizeof(*handle));
    if (!handle) return NULL;
    if (descriptor < 0) {
        handle->length = -1;
        return handle;
    }
    ssize_t count = read(descriptor, handle->bytes, sizeof(handle->bytes));
    handle->length = count < 0 ? -1 : (int)count;
    return handle;
}

static inline bool deckBlockStopSignals(bool includeHup) {
    sigset_t stopping;
    sigemptyset(&stopping);
    sigaddset(&stopping, SIGINT);
    sigaddset(&stopping, SIGTERM);
    if (includeHup) sigaddset(&stopping, SIGHUP);
    return sigprocmask(SIG_BLOCK, &stopping, NULL) == 0;
}

static inline bool deckStopSignalled(bool includeHup) {
    sigset_t pending;
    sigemptyset(&pending);
    if (sigpending(&pending) != 0) return false;
    if (sigismember(&pending, SIGINT) == 1 || sigismember(&pending, SIGTERM) == 1) return true;
    return includeHup && sigismember(&pending, SIGHUP) == 1;
}

static inline int deckTunnelParentPid(void) {
    return (int)getpid();
}

static inline int deckTunnelFork(void) {
    fflush(stdout);
    fflush(stderr);
    return (int)fork();
}

static inline bool deckTunnelParentAlive(int parentPid) {
    return (int)getppid() == parentPid;
}

static inline void deckTunnelExit(int code) {
    _exit(code);
}

static inline void deckTunnelWait(int childPid) {
    if (childPid <= 0) return;
    int status = 0;
    while (waitpid((pid_t)childPid, &status, 0) < 0 && errno == EINTR) {}
}

static inline void deckTunnelTerminate(int childPid) {
    if (childPid > 0) kill((pid_t)childPid, SIGTERM);
}

static inline void deckCloseDescriptor(int descriptor) {
    if (descriptor >= 0) close(descriptor);
}

static inline bool deckConfirmStdinYes(void) {
    char line[16];
    memset(line, 0, sizeof(line));
    fflush(stdout);
    return fgets(line, (int)sizeof(line), stdin) != NULL && (line[0] == 'y' || line[0] == 'Y');
}

static inline bool deckTransferTimedOut(int received) {
    return received == -ETIMEDOUT || received == -EINTR;
}

static inline bool deckReadableFile(const char *path) {
    return path != NULL && access(path, R_OK) == 0;
}
#endif
