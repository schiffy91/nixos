#ifndef NIXOS_TEST_SIGNALS_H
#define NIXOS_TEST_SIGNALS_H
#include <signal.h>
#include <unistd.h>
static volatile sig_atomic_t testStopped = 0;
static void testStop(int number) { testStopped = number; }
static int testInstallStop(void) {
    struct sigaction action = { .sa_handler = testStop };
    sigemptyset(&action.sa_mask);
    return sigaction(SIGTERM, &action, NULL) == 0;
}
static int testHasStopped(void) { return testStopped != 0; }
static int testSendSignal(int pid, int number) { return pid > 0 && kill(pid, number) == 0; }
#endif
