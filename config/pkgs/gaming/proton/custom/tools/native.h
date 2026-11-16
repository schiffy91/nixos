#ifndef NIXOS_PROTON_TOOLS_NATIVE_H
#define NIXOS_PROTON_TOOLS_NATIVE_H

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#if defined(__linux__)
#include <sys/syscall.h>
#endif

extern char **environ;

typedef struct ProtonLaunchStorage {
    char **argv;
    size_t argc;
    char **envp;
    size_t envc;
    char *directory;
    char *log;
} *ProtonLaunchPlan;

static inline void protonFreeStrings(char **items, size_t count) {
    if (!items) return;
    for (size_t i = 0; i < count; i++) free(items[i]);
    free(items);
}

static inline void protonLaunchRelease(ProtonLaunchPlan plan) {
    if (!plan) return;
    protonFreeStrings(plan->argv, plan->argc);
    protonFreeStrings(plan->envp, plan->envc);
    free(plan->directory);
    free(plan->log);
    free(plan);
}

static inline bool protonPushString(char ***items, size_t *count, const char *value) {
    char *copy = strdup(value ? value : "");
    if (!copy) return false;
    char **grown = realloc(*items, (*count + 2) * sizeof(char *));
    if (!grown) {
        free(copy);
        return false;
    }
    grown[*count] = copy;
    grown[*count + 1] = NULL;
    *items = grown;
    *count += 1;
    return true;
}

static inline bool protonLaunchAddArg(ProtonLaunchPlan plan, const char *argument) {
    if (!plan || !argument) return false;
    return protonPushString(&plan->argv, &plan->argc, argument);
}

static inline bool protonLaunchAddEnv(ProtonLaunchPlan plan, const char *entry) {
    if (!plan || !entry) return false;
    const char *equals = strchr(entry, '=');
    if (!equals || equals == entry) return false;
    size_t keyLength = (size_t)(equals - entry);
    for (size_t i = 0; i < plan->envc; i++) {
        if (strncmp(plan->envp[i], entry, keyLength) == 0 && plan->envp[i][keyLength] == '=') {
            char *copy = strdup(entry);
            if (!copy) return false;
            free(plan->envp[i]);
            plan->envp[i] = copy;
            return true;
        }
    }
    return protonPushString(&plan->envp, &plan->envc, entry);
}

static inline ProtonLaunchPlan protonLaunchCreate(const char *executable, const char *directory, const char *log) {
    if (!executable || !directory || !log) return NULL;
    ProtonLaunchPlan plan = calloc(1, sizeof(*plan));
    if (!plan) return NULL;
    plan->directory = strdup(directory);
    plan->log = strdup(log);
    if (!plan->directory || !plan->log || !protonLaunchAddArg(plan, executable)) {
        protonLaunchRelease(plan);
        return NULL;
    }
    if (environ) {
        for (size_t i = 0; environ[i] != NULL; i++) {
            if (!protonPushString(&plan->envp, &plan->envc, environ[i])) {
                protonLaunchRelease(plan);
                return NULL;
            }
        }
    } else {
        plan->envp = calloc(1, sizeof(char *));
        if (!plan->envp) {
            protonLaunchRelease(plan);
            return NULL;
        }
    }
    return plan;
}

static inline int protonMoveAboveStderr(int *fd) {
    if (*fd > STDERR_FILENO) return 0;
    int moved = fcntl(*fd, F_DUPFD_CLOEXEC, STDERR_FILENO + 1);
    if (moved < 0) return -1;
    close(*fd);
    *fd = moved;
    return 0;
}

static inline int protonCloseDescriptorsExcept(int upperBound, int keepFd) {
#if defined(__linux__) && defined(SYS_close_range)
    bool firstOk = keepFd <= 3 || syscall(SYS_close_range, 3U, (unsigned int)(keepFd - 1), 0U) == 0;
    bool secondOk = syscall(SYS_close_range, (unsigned int)(keepFd + 1), ~0U, 0U) == 0;
    if (firstOk && secondOk) return 0;
#endif
    for (int fd = 3; fd < upperBound; fd++) {
        if (fd == keepFd) continue;
        while (close(fd) != 0 && errno == EINTR) {}
    }
    return 0;
}

static inline int protonPollChild(int pid) {
    if (pid <= 0) return 1;
    int status = 0;
    pid_t result = waitpid((pid_t)pid, &status, WNOHANG);
    if (result == (pid_t)pid || (result < 0 && errno == ECHILD)) return 1;
    if (result == 0 || (result < 0 && errno == EINTR)) return 0;
    return -1;
}

static inline void protonKillProcess(int pid, bool force) {
    if (pid > 0) kill((pid_t)pid, force ? SIGKILL : SIGTERM);
}

static inline void protonReapChild(int pid) {
    if (pid <= 0) return;
    int status = 0;
    while (waitpid((pid_t)pid, &status, 0) < 0 && errno == EINTR) {}
}

static inline int protonLaunchStart(ProtonLaunchPlan plan, int closeBound) {
    if (!plan || plan->argc == 0 || !plan->envp) return -1;
    int output = open(plan->log, O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, (mode_t)0600);
    if (output < 0) return -1;
    int errors[2];
    if (pipe(errors) != 0) {
        close(output);
        return -1;
    }
    if (protonMoveAboveStderr(&output) != 0 || protonMoveAboveStderr(&errors[0]) != 0 || protonMoveAboveStderr(&errors[1]) != 0) {
        close(output);
        close(errors[0]);
        close(errors[1]);
        return -1;
    }
    fcntl(errors[1], F_SETFD, FD_CLOEXEC);
    pid_t pid = fork();
    if (pid == 0) {
        close(errors[0]);
        if (setsid() >= 0 && chdir(plan->directory) == 0 && dup2(output, STDOUT_FILENO) >= 0 && dup2(output, STDERR_FILENO) >= 0) {
            close(output);
            if (protonCloseDescriptorsExcept(closeBound, errors[1]) == 0) {
                execve(plan->argv[0], plan->argv, plan->envp);
            }
        }
        int failure = errno;
        ssize_t written = write(errors[1], &failure, sizeof(failure));
        _exit(written < 0 ? 126 : 127);
    }
    close(output);
    close(errors[1]);
    int failure = 0;
    ssize_t count = -1;
    if (pid > 0) {
        do {
            count = read(errors[0], &failure, sizeof(failure));
        } while (count < 0 && errno == EINTR);
    }
    close(errors[0]);
    if (pid <= 0 || count != 0) {
        if (pid > 0) {
            protonKillProcess((int)pid, true);
            protonReapChild((int)pid);
        }
        return -1;
    }
    return (int)pid;
}

static inline int protonParentPid(void) {
    return (int)getppid();
}

static inline bool protonIsInterrupted(int nativeCode) {
    return nativeCode == EINTR;
}

static inline int protonForkChild(void) {
    return (int)fork();
}

static inline void protonExitChild(int code) {
    _exit(code);
}

static inline void protonStopChild(int pid) {
    if (pid <= 0) return;
    kill((pid_t)pid, SIGTERM);
    protonReapChild(pid);
}

#endif
