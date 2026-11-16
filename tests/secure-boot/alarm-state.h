#ifndef NIXOS_TEST_ALARM_STATE_H
#define NIXOS_TEST_ALARM_STATE_H
#include <signal.h>
#include <unistd.h>

static inline int testIgnoreAndBlockAlarm(void) {
    sigset_t mask;
    if (sigemptyset(&mask) != 0 || sigaddset(&mask, SIGALRM) != 0) return 0;
    return signal(SIGALRM, SIG_IGN) != SIG_ERR && sigprocmask(SIG_BLOCK, &mask, NULL) == 0;
}
static inline void testWaitForSignal(void) { pause(); }
static inline int testAlarmSignal(void) { return SIGALRM; }
static inline int testRemainingAlarmSeconds(void) { return (int)alarm(0); }
#endif
