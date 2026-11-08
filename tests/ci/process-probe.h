#ifndef NIXOS_TEST_PROCESS_PROBE_H
#define NIXOS_TEST_PROCESS_PROBE_H
#include <errno.h>
#include <poll.h>
#include <signal.h>
#include <unistd.h>

static int testReadInputByte(void) {
    unsigned char byte;
    ssize_t count;
    do { count = read(STDIN_FILENO, &byte, 1); } while (count < 0 && errno == EINTR);
    return count == 1 ? byte : count == 0 ? -1 : -2;
}

static int testInputRemainsOpen(int milliseconds) {
    struct pollfd input = { .fd = STDIN_FILENO, .events = POLLHUP };
    /* An interrupted observation is a failed test, never evidence of openness. */
    return poll(&input, 1, milliseconds) == 0;
}

static int testProcessExists(int pid) {
    return kill((pid_t)pid, 0) == 0 || errno != ESRCH;
}
#endif
