#ifndef NIXOS_TEST_ALARM_STATE_H
#define NIXOS_TEST_ALARM_STATE_H
#include <signal.h>
#include <unistd.h>

static int testIgnoreAndBlockAlarm(void) {
    sigset_t mask;
    if (sigemptyset(&mask) != 0 || sigaddset(&mask, SIGALRM) != 0) return 0;
    return signal(SIGALRM, SIG_IGN) != SIG_ERR && sigprocmask(SIG_BLOCK, &mask, NULL) == 0;
}
static void testWaitForSignal(void) { pause(); }
static int testAlarmSignal(void) { return SIGALRM; }
#endif
