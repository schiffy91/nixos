#define _GNU_SOURCE
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

/* Test-only interposition in plymouthd, never in the pre-blanking helper. */
static bool fault(const char *kind) {
    char requested[32] = {0};
    int fd = syscall(SYS_openat, AT_FDCWD, "/run/sleep-renderer-fault", O_RDONLY);
    if (fd < 0) return false;
    ssize_t length = syscall(SYS_read, fd, requested, sizeof(requested) - 1);
    syscall(SYS_close, fd);
    if (length <= 0 || strcmp(requested, kind)) return false;
    fd = syscall(SYS_openat, AT_FDCWD, "/run/sleep-renderer-injected", O_WRONLY | O_CREAT | O_APPEND, 0600);
    if (fd >= 0) {
        syscall(SYS_write, fd, kind, strlen(kind));
        syscall(SYS_write, fd, "\n", 1);
        syscall(SYS_close, fd);
    }
    errno = EIO;
    return true;
}

static bool target(int fd) {
    char path[64], destination[128] = {0};
    snprintf(path, sizeof(path), "/proc/self/fd/%d", fd);
    ssize_t length = readlink(path, destination, sizeof(destination) - 1);
    return length > 0 && !strcmp(destination, "/dev/dri/card1");
}

int open(const char *path, int flags, ...) {
    mode_t mode = 0;
    if ((flags & O_CREAT) || (flags & O_TMPFILE) == O_TMPFILE) {
        va_list arguments;
        va_start(arguments, flags);
        mode = va_arg(arguments, mode_t);
        va_end(arguments);
    }
    if (!strcmp(path, "/dev/dri/card1") && fault("open")) return -1;
    return syscall(SYS_openat, AT_FDCWD, path, flags, mode);
}

int open64(const char *path, int flags, ...) {
    mode_t mode = 0;
    if ((flags & O_CREAT) || (flags & O_TMPFILE) == O_TMPFILE) {
        va_list arguments;
        va_start(arguments, flags);
        mode = va_arg(arguments, mode_t);
        va_end(arguments);
    }
    return open(path, flags, mode);
}

int drmSetMaster(int fd) {
    int (*real)(int) = dlsym(RTLD_NEXT, "drmSetMaster");
    if (target(fd) && fault("master")) return -1;
    return real(fd);
}

int drmIoctl(int fd, unsigned long request, void *argument) {
    int (*real)(int, unsigned long, void *) = dlsym(RTLD_NEXT, "drmIoctl");
    if (request == DRM_IOCTL_MODE_MAP_DUMB && target(fd) && fault("map")) return -1;
    return real(fd, request, argument);
}

int drmModeSetCrtc(int fd, uint32_t crtc, uint32_t buffer, uint32_t x, uint32_t y,
                   uint32_t *connectors, int count, drmModeModeInfoPtr mode) {
    int (*real)(int, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t *, int, drmModeModeInfoPtr) =
        dlsym(RTLD_NEXT, "drmModeSetCrtc");
    if (buffer && target(fd) && fault("modeset")) return -1;
    return real(fd, crtc, buffer, x, y, connectors, count, mode);
}
