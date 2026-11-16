#ifndef NIXOS_SLOPSMITH_SIGNALS_H
#define NIXOS_SLOPSMITH_SIGNALS_H

#include <errno.h>
#include <signal.h>
#include <stdbool.h>

static volatile sig_atomic_t slopsmithRecordedSignal = 0;

static inline void slopsmithHandleSignal(int number) {
    slopsmithRecordedSignal = (sig_atomic_t)number;
}

static inline void slopsmithInstallSignals(void) {
    slopsmithRecordedSignal = 0;
    signal(SIGINT, slopsmithHandleSignal);
    signal(SIGTERM, slopsmithHandleSignal);
    signal(SIGHUP, slopsmithHandleSignal);
}

static inline bool slopsmithStopRequested(void) {
    return slopsmithRecordedSignal != 0;
}

static inline int slopsmithSignalExitCode(void) {
    if (slopsmithRecordedSignal == 0) return 0;
    return slopsmithRecordedSignal == (sig_atomic_t)SIGINT ? 130 : 143;
}

static inline bool slopsmithLockContended(int nativeCode) {
    return nativeCode == EWOULDBLOCK || nativeCode == EAGAIN;
}

#endif
