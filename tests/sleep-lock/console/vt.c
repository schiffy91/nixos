/* VM fixture only: exercise the kernel boundary independently of a locker. */
#include <assert.h>
#include <fcntl.h>
#include <linux/vt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

static void check(int result) {
    if (result < 0) {
        perror("console ioctl");
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char **argv) {
    assert(argc == 2);
    int fd = open("/dev/tty63", O_RDWR | O_CLOEXEC);
    if (fd < 0) {
        perror("open console");
        return 1;
    }
    if (!strcmp(argv[1], "lock")) {
        check(ioctl(fd, VT_ACTIVATE, 63));
        check(ioctl(fd, VT_WAITACTIVE, 63));
        check(ioctl(fd, VT_LOCKSWITCH));
    } else if (!strcmp(argv[1], "relock")) {
        check(ioctl(fd, VT_LOCKSWITCH));
    } else if (!strcmp(argv[1], "unlock")) {
        check(ioctl(fd, VT_UNLOCKSWITCH));
    } else if (!strcmp(argv[1], "state")) {
        struct vt_stat state;
        check(ioctl(fd, VT_GETSTATE, &state));
        printf("%u\n", state.v_active);
    } else {
        return 2;
    }
    close(fd);
    return 0;
}
