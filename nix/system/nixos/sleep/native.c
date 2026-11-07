#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <linux/vt.h>
#include <security/pam_appl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

static void require(int result, const char *operation) {
    if (result < 0) {
        perror(operation);
        exit(EXIT_FAILURE);
    }
}

static unsigned active_vt(int fd) {
    struct vt_stat state;
    require(ioctl(fd, VT_GETSTATE, &state), "VT_GETSTATE");
    return state.v_active;
}

static char *ask_password(const char *prompt) {
    int fds[2], status;
    char buffer[PAM_MAX_RESP_SIZE + 1];
    size_t used = 0;
    int read_failed = 0;
    ssize_t n;
    pid_t child, parent = getpid();
    if (pipe2(fds, O_CLOEXEC) < 0)
        return NULL;
    child = fork();
    if (child == 0) {
        if (prctl(PR_SET_PDEATHSIG, SIGKILL) < 0 || getppid() != parent)
            _exit(1);
        if (dup2(fds[1], STDOUT_FILENO) < 0)
            _exit(1);
        close(fds[0]);
        close(fds[1]);
        execl(PLYMOUTH, PLYMOUTH, "ask-for-password", prompt, NULL);
        _exit(1);
    }
    close(fds[1]);
    if (child < 0) {
        close(fds[0]);
        return NULL;
    }
    while (used < sizeof(buffer)) {
        n = read(fds[0], buffer + used, sizeof(buffer) - used);
        if (n < 0 && errno == EINTR)
            continue;
        if (n <= 0) {
            read_failed = n < 0;
            break;
        }
        used += (size_t)n;
    }
    close(fds[0]);
    if (read_failed || used == sizeof(buffer))
        kill(child, SIGKILL);
    do { n = waitpid(child, &status, 0); } while (n < 0 && errno == EINTR);
    char *answer = NULL;
    if (!read_failed && n == child && WIFEXITED(status) && WEXITSTATUS(status) == 0 &&
        used > 0 && used < sizeof(buffer) && memchr(buffer, 0, used) == NULL) {
        buffer[used] = 0;
        answer = strdup(buffer);
    }
    explicit_bzero(buffer, sizeof(buffer));
    return answer;
}

static int converse(int count, const struct pam_message **messages,
                    struct pam_response **responses, void *data) {
    if (count <= 0 || count > PAM_MAX_NUM_MSG)
        return PAM_CONV_ERR;
    struct pam_response *answers = calloc((size_t)count, sizeof(*answers));
    if (!answers)
        return PAM_BUF_ERR;
    for (int i = 0; i < count; i++) {
        if (messages[i]->msg_style == PAM_PROMPT_ECHO_OFF) {
            answers[i].resp = ask_password(data);
            if (answers[i].resp)
                continue;
        } else if (messages[i]->msg_style == PAM_TEXT_INFO ||
                   messages[i]->msg_style == PAM_ERROR_MSG) {
            continue;
        }
        for (int j = 0; j < count; j++) {
            if (answers[j].resp) {
                explicit_bzero(answers[j].resp, strlen(answers[j].resp));
                free(answers[j].resp);
            }
        }
        free(answers);
        return PAM_CONV_ERR;
    }
    *responses = answers;
    return PAM_SUCCESS;
}

int main(int argc, char **argv) {
    if (getuid() != 0 || geteuid() != 0 || argc < 2)
        return 1;
    struct rlimit core = { 0, 0 };
    require(setrlimit(RLIMIT_CORE, &core), "disable core dumps");
    require(prctl(PR_SET_DUMPABLE, 0), "disable process dumps");
    if (!strcmp(argv[1], "authenticate") && (argc == 3 || argc == 4)) {
        if (argc == 4 && strcmp(argv[3], "retry"))
            return 1;
        char *prompt = argc == 4 ? "--prompt=Authentication failed. Try again." : "--prompt=Unlock session";
        /* A short-lived, locked address space contains PAM's copies as well as
         * our pipe buffer. The controller never receives the password. */
        require(mlockall(MCL_CURRENT | MCL_FUTURE), "lock authentication memory");
        pam_handle_t *pam = NULL;
        struct pam_conv conversation = { converse, prompt };
        int result = pam_start("nixos-sleep-lock", argv[2], &conversation, &pam);
        if (result == PAM_SUCCESS)
            result = pam_authenticate(pam, PAM_DISALLOW_NULL_AUTHTOK);
        if (result == PAM_SUCCESS)
            result = pam_acct_mgmt(pam, PAM_SILENT);
        if (pam)
            pam_end(pam, result);
        return result == PAM_SUCCESS ? 0 : 1;
    }

    int fd = open("/dev/tty63", O_RDWR | O_CLOEXEC | O_NOCTTY);
    require(fd, "open lock console");
    if (!strcmp(argv[1], "secure") && argc == 2) {
        struct termios term;
        require(tcgetattr(fd, &term), "read console input mode");
        term.c_lflag &= ~(ECHO | ECHONL | ISIG);
        term.c_iflag &= ~IXON;
        require(tcsetattr(fd, TCSAFLUSH, &term), "secure console input");
        if (active_vt(fd) != 63) {
            require(ioctl(fd, VT_ACTIVATE, 63), "activate lock console");
            require(ioctl(fd, VT_WAITACTIVE, 63), "wait for lock console");
        }
        require(ioctl(fd, VT_LOCKSWITCH), "lock console switching");
        if (active_vt(fd) != 63)
            return 1;
    } else if (!strcmp(argv[1], "verify") && argc == 2) {
        if (active_vt(fd) != 63)
            return 1;
        require(ioctl(fd, VT_LOCKSWITCH), "retain console lock");
    } else if (!strcmp(argv[1], "release") && argc == 3) {
        char *end;
        long vt = strtol(argv[2], &end, 10);
        if (*end || vt <= 0 || vt >= 63)
            return 1;
        require(tcflush(fd, TCIFLUSH), "discard lock console input");
        require(ioctl(fd, VT_UNLOCKSWITCH), "release console lock");
        require(ioctl(fd, VT_ACTIVATE, (int)vt), "restore session console");
        require(ioctl(fd, VT_WAITACTIVE, (int)vt), "wait for session console");
    } else {
        return 1;
    }
    close(fd);
    return 0;
}
