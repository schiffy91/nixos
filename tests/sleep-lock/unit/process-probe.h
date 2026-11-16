#ifndef NIXOS_SLEEP_PROCESS_PROBE_H
#define NIXOS_SLEEP_PROCESS_PROBE_H
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <unistd.h>

static inline volatile sig_atomic_t *sleepTestTerminatedSlot(void) {
    static volatile sig_atomic_t terminated = 0;
    return &terminated;
}

static inline void sleepTestTerminateHandler(int number) {
    *sleepTestTerminatedSlot() = (sig_atomic_t)number;
}

static inline void sleepTestConfigureTermSignal(bool ignore) {
    *sleepTestTerminatedSlot() = 0;
    signal(SIGTERM, ignore ? SIG_IGN : sleepTestTerminateHandler);
}

static inline bool sleepTestWasTerminated(void) {
    return *sleepTestTerminatedSlot() != 0;
}

static inline int sleepTestOpenHighDescriptor(void) {
    int original = open("/dev/null", O_RDONLY);
    if (original < 0) return -1;
    int descriptor = fcntl(original, F_DUPFD, 100);
    close(original);
    return descriptor;
}

static inline bool sleepTestDescriptorInheritable(int descriptor) {
    return descriptor >= 0 && fcntl(descriptor, F_GETFD, 0) == 0;
}

static inline void sleepTestCloseDescriptor(int descriptor) {
    if (descriptor >= 0) close(descriptor);
}

static inline int sleepTestVerifyChildDescriptors(int closedDescriptor) {
    if (fcntl(closedDescriptor, F_GETFD, 0) >= 0 || errno != EBADF) return 1;
    for (int standard = 0; standard < 3; standard++) {
        if (fcntl(standard, F_GETFD, 0) < 0) return 2;
    }
    return 0;
}

static inline bool sleepTestChildReaped(int pid) {
    int status = 0;
    return waitpid((pid_t)pid, &status, WNOHANG) == (pid_t)(-1) && errno == ECHILD;
}
#endif
