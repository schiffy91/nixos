#ifndef NIXOS_TEST_PROCESS_TREE_H
#define NIXOS_TEST_PROCESS_TREE_H
#include <signal.h>
#include <unistd.h>
static int testFork(void) { return (int)fork(); }
static int testIgnoreTermination(void) { return signal(SIGTERM, SIG_IGN) != SIG_ERR; }
static void testKillProcess(int pid) { if (pid > 1) kill((pid_t)pid, SIGKILL); }
#endif
