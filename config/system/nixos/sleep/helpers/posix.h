#ifndef NIXOS_SLEEP_POSIX_H
#define NIXOS_SLEEP_POSIX_H
#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <limits.h>
#include <linux/vt.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

static inline int sleepConsoleOpen(void) {
    return open("/dev/tty63", O_RDWR | O_CLOEXEC | O_NOCTTY);
}

static inline void sleepConsoleClose(int descriptor) {
    if (descriptor >= 0) close(descriptor);
}

static inline int sleepConsoleActiveVt(int descriptor) {
    struct vt_stat state;
    if (descriptor < 0 || ioctl(descriptor, VT_GETSTATE, &state) < 0) return -1;
    return (int)state.v_active;
}

static inline int sleepConsoleLockSwitch(int descriptor) {
    return descriptor < 0 ? -1 : ioctl(descriptor, VT_LOCKSWITCH);
}

static inline int sleepConsoleUnlockSwitch(int descriptor) {
    return descriptor < 0 ? -1 : ioctl(descriptor, VT_UNLOCKSWITCH);
}

static inline int sleepConsoleActivate(int descriptor, int vt) {
    return descriptor < 0 ? -1 : ioctl(descriptor, VT_ACTIVATE, vt);
}

static inline int sleepConsoleWaitActive(int descriptor, int vt) {
    return descriptor < 0 ? -1 : ioctl(descriptor, VT_WAITACTIVE, vt);
}

static inline int sleepConsoleSecureInput(int descriptor) {
    struct termios mode;
    if (descriptor < 0 || tcgetattr(descriptor, &mode) < 0) return -1;
    mode.c_lflag &= ~((tcflag_t)ECHO | (tcflag_t)ECHONL | (tcflag_t)ISIG);
    mode.c_iflag &= ~(tcflag_t)IXON;
    return tcsetattr(descriptor, TCSAFLUSH, &mode);
}

static inline int sleepConsoleDiscardInput(int descriptor) {
    return descriptor < 0 ? -1 : tcflush(descriptor, TCIFLUSH);
}

static inline int sleepDisableCoreDumps(void) {
    struct rlimit core = { 0, 0 };
    return setrlimit(RLIMIT_CORE, &core);
}

static inline int sleepDisableProcessDumps(void) {
    return prctl(PR_SET_DUMPABLE, 0);
}
#endif
