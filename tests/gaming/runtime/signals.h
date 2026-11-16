#ifndef NIXOS_TEST_SIGNALS_H
#define NIXOS_TEST_SIGNALS_H
#include <signal.h>
#include <unistd.h>
static inline volatile sig_atomic_t *testStoppedSlot(void) {
    static volatile sig_atomic_t testStopped = 0;
    return &testStopped;
}
static inline void testStop(int number) { *testStoppedSlot() = number; }
static inline int testInstallStop(void) {
    struct sigaction action = { .sa_handler = testStop };
    sigemptyset(&action.sa_mask);
    return sigaction(SIGTERM, &action, NULL) == 0;
}
static inline int testHasStopped(void) { return *testStoppedSlot() != 0; }
static inline int testSendSignal(int pid, int number) { return pid > 0 && kill(pid, number) == 0; }
#endif
