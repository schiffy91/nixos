#ifndef NIXOS_SLEEP_PAM_H
#define NIXOS_SLEEP_PAM_H
#include <errno.h>
#include <fcntl.h>
#include <security/pam_appl.h>
#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

typedef struct SleepPasswordStorage {
    char *bytes;
    int capacity;
    int used;
} *SleepPasswordHandle;

static inline int sleepPamMaxResponseSize(void) {
    return PAM_MAX_RESP_SIZE;
}

static inline void sleepPasswordClose(SleepPasswordHandle password) {
    if (!password || !password->bytes) return;
    explicit_bzero(password->bytes, (size_t)password->capacity);
    free(password->bytes);
    password->bytes = NULL;
    password->used = 0;
}

static inline void sleepPasswordRelease(SleepPasswordHandle password) {
    if (!password) return;
    sleepPasswordClose(password);
    free(password);
}

static inline int sleepPasswordLength(SleepPasswordHandle password) {
    return password && password->bytes ? password->used : -1;
}

static inline bool sleepPasswordEquals(SleepPasswordHandle password, const char *expected) {
    if (!password || !password->bytes || !expected) return false;
    return strcmp(password->bytes, expected) == 0;
}

static inline SleepPasswordHandle sleepPasswordReadPrompt(const char *executable, const char *label, int deathSignalOption) {
    if (!executable || !label) return NULL;
    int descriptors[2];
    if (pipe2(descriptors, O_CLOEXEC) < 0) return NULL;
    pid_t parent = getpid();
    pid_t child = fork();
    if (child == 0) {
        if (prctl(deathSignalOption, SIGKILL) < 0 || getppid() != parent || dup2(descriptors[1], STDOUT_FILENO) < 0) {
            _exit(1);
        }
        close(descriptors[0]);
        close(descriptors[1]);
        execl(executable, executable, "ask-for-password", label, (char *)NULL);
        _exit(1);
    }
    close(descriptors[1]);
    if (child < 0) {
        close(descriptors[0]);
        return NULL;
    }
    SleepPasswordHandle answer = calloc(1, sizeof(*answer));
    if (!answer) {
        close(descriptors[0]);
        kill(child, SIGKILL);
        int status = 0;
        while (waitpid(child, &status, 0) < 0 && errno == EINTR) {}
        return NULL;
    }
    answer->capacity = PAM_MAX_RESP_SIZE + 1;
    answer->bytes = calloc((size_t)answer->capacity, 1);
    bool valid = answer->bytes != NULL;
    while (valid && answer->used < answer->capacity) {
        ssize_t count = read(descriptors[0], answer->bytes + answer->used, (size_t)(answer->capacity - answer->used));
        if (count < 0 && errno == EINTR) continue;
        if (count < 0) { valid = false; break; }
        if (count == 0) break;
        answer->used += (int)count;
    }
    close(descriptors[0]);
    if (valid) {
        valid = answer->used > 0 && answer->used < answer->capacity &&
            memchr(answer->bytes, 0, (size_t)answer->used) == NULL;
    }
    if (!valid) kill(child, SIGKILL);
    int status = 0;
    pid_t waited;
    do { waited = waitpid(child, &status, 0); } while (waited < 0 && errno == EINTR);
    if (!valid || waited != child || !WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        sleepPasswordRelease(answer);
        return NULL;
    }
    return answer;
}

struct SleepPamPromptData {
    const char *prompt;
    const char *plymouth;
    int deathSignalOption;
};

static inline void sleepFreePamResponses(struct pam_response *values, int count) {
    if (!values) return;
    for (int index = 0; index < count; index++) {
        char *response = values[index].resp;
        if (response) {
            explicit_bzero(response, strlen(response));
            free(response);
        }
    }
    free(values);
}

static inline int sleepPamConverse(int count, const struct pam_message **messages, struct pam_response **responses, void *data) {
    if (count <= 0 || count > PAM_MAX_NUM_MSG || !messages || !responses || !data) return PAM_CONV_ERR;
    const struct SleepPamPromptData *request = (const struct SleepPamPromptData *)data;
    struct pam_response *values = calloc((size_t)count, sizeof(*values));
    if (!values) return PAM_BUF_ERR;
    for (int index = 0; index < count; index++) {
        int style = messages[index]->msg_style;
        if (style == PAM_TEXT_INFO || style == PAM_ERROR_MSG) continue;
        if (style != PAM_PROMPT_ECHO_OFF) {
            sleepFreePamResponses(values, count);
            return PAM_CONV_ERR;
        }
        SleepPasswordHandle password = sleepPasswordReadPrompt(request->plymouth, request->prompt, request->deathSignalOption);
        if (!password) {
            sleepFreePamResponses(values, count);
            return PAM_CONV_ERR;
        }
        values[index].resp = strdup(password->bytes);
        sleepPasswordRelease(password);
        if (!values[index].resp) {
            sleepFreePamResponses(values, count);
            return PAM_CONV_ERR;
        }
    }
    *responses = values;
    return PAM_SUCCESS;
}

static inline bool sleepPamAuthenticate(const char *user, const char *plymouth, bool retry, int deathSignalOption) {
    if (!user || !plymouth) return false;
    const char *label = retry ? "--prompt=Authentication failed. Try again." : "--prompt=Unlock session";
    struct SleepPamPromptData request = { label, plymouth, deathSignalOption };
    struct pam_conv conversation = { sleepPamConverse, &request };
    pam_handle_t *pam = NULL;
    int result = pam_start("nixos-sleep-lock", user, &conversation, &pam);
    if (result == PAM_SUCCESS) result = pam_authenticate(pam, PAM_DISALLOW_NULL_AUTHTOK);
    if (result == PAM_SUCCESS) result = pam_acct_mgmt(pam, PAM_SILENT);
    if (pam != NULL) pam_end(pam, result);
    return result == PAM_SUCCESS;
}
#endif
