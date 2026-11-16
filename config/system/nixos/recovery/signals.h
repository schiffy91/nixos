#ifndef NIXOS_RECOVERY_SIGNALS_H
#define NIXOS_RECOVERY_SIGNALS_H
#include <signal.h>

static inline volatile sig_atomic_t *recoveryArtifactSignalSlot(void) {
    static volatile sig_atomic_t caught = 0;
    return &caught;
}

static inline void interruptRecoveryArtifacts(int number) {
    *recoveryArtifactSignalSlot() = (sig_atomic_t)number;
}

static inline void recoveryArtifactsHandleInterruptions(void) {
    *recoveryArtifactSignalSlot() = 0;
    signal(SIGINT, interruptRecoveryArtifacts);
    signal(SIGTERM, interruptRecoveryArtifacts);
}

static inline int recoveryArtifactsCaughtSignal(void) {
    return (int)(*recoveryArtifactSignalSlot());
}
#endif
