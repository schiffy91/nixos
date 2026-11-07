/* Kernel uinput fixtures: Plymouth consumes actual udev/libinput events. */
#define _GNU_SOURCE
#include <assert.h>
#include <fcntl.h>
#include <linux/uinput.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static void
emit(int fd, unsigned type, unsigned code, int value)
{
        struct input_event event = { .type = type, .code = code, .value = value };
        assert(write(fd, &event, sizeof(event)) == sizeof(event));
}

static void
abs_axis(int fd, unsigned code, int maximum)
{
        struct uinput_abs_setup axis = { .code = code, .absinfo = { .maximum = maximum } };
        assert(ioctl(fd, UI_SET_ABSBIT, code) == 0);
        assert(ioctl(fd, UI_ABS_SETUP, &axis) == 0);
}

static int
create_device(bool touch, unsigned number)
{
        int fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
        struct uinput_setup setup = {
                .id = { .bustype = BUS_USB, .vendor = 0xfeed, .product = touch ? 1 + 2 * number : 2 }
        };

        assert(fd >= 0);
        assert(ioctl(fd, UI_SET_EVBIT, EV_KEY) == 0);
        if (touch) {
                assert(ioctl(fd, UI_SET_KEYBIT, BTN_TOUCH) == 0);
                assert(ioctl(fd, UI_SET_EVBIT, EV_ABS) == 0);
                assert(ioctl(fd, UI_SET_PROPBIT, INPUT_PROP_DIRECT) == 0);
                abs_axis(fd, ABS_X, 32767);
                abs_axis(fd, ABS_Y, 32767);
                abs_axis(fd, ABS_MT_SLOT, 9);
                abs_axis(fd, ABS_MT_TRACKING_ID, 65535);
                abs_axis(fd, ABS_MT_POSITION_X, 32767);
                abs_axis(fd, ABS_MT_POSITION_Y, 32767);
        } else {
                for (int key = KEY_ESC; key <= KEY_COMPOSE; key++)
                        assert(ioctl(fd, UI_SET_KEYBIT, key) == 0);
        }
        snprintf(setup.name, sizeof(setup.name), "Plymouth test %s%s",
                 touch ? "touchscreen" : "keyboard", number ? " 2" : "");
        assert(ioctl(fd, UI_DEV_SETUP, &setup) == 0);
        assert(ioctl(fd, UI_DEV_CREATE) == 0);
        return fd;
}

int
main(int argc, char **argv)
{
        int touch[2] = { -1, -1 }, keyboard = -1, tracking = 0;
        unsigned active_slots[2] = { 0, 0 };
        char line[128], command[32];
        FILE *input;

        if (argc > 1 && !strcmp(argv[1], "keyboard"))
                keyboard = create_device(false, 0);
        if (argc > 1 && !strcmp(argv[1], "touchscreen"))
                touch[0] = create_device(true, 0);
        assert(mkfifo("/run/test-input", 0600) == 0);
        input = fdopen(open("/run/test-input", O_RDWR), "r");
        assert(input != NULL);

        while (fgets(line, sizeof(line), input)) {
                int x = 0, y = 0, slot = 0;
                unsigned device = 0;
                size_t length;

                assert(sscanf(line, "%31s %d %d %d", command, &x, &y, &slot) >= 1);
                if (!strcmp(command, "touch2-on")) {
                        device = 1;
                        strcpy(command, "touch-on");
                } else if (!strcmp(command, "touch2-off")) {
                        device = 1;
                        strcpy(command, "touch-off");
                } else {
                        length = strlen(command);
                        if (length > 0 && command[length - 1] == '2') {
                                device = 1;
                                command[length - 1] = '\0';
                        }
                }
                assert(slot >= 0 && slot < 10);
                if (!strcmp(command, "touch-on") && touch[device] < 0) {
                        touch[device] = create_device(true, device);
                } else if (!strcmp(command, "touch-off") && touch[device] >= 0) {
                        close(touch[device]);
                        touch[device] = -1;
                        active_slots[device] = 0;
                } else if (!strcmp(command, "keyboard-on") && keyboard < 0) {
                        keyboard = create_device(false, 0);
                } else if (!strcmp(command, "keyboard-off") && keyboard >= 0) {
                        close(keyboard);
                        keyboard = -1;
                } else if ((!strcmp(command, "down") || !strcmp(command, "move")) && touch[device] >= 0) {
                        int fd = touch[device];
                        emit(fd, EV_ABS, ABS_MT_SLOT, slot);
                        if (!strcmp(command, "down")) {
                                emit(fd, EV_ABS, ABS_MT_TRACKING_ID, ++tracking);
                                active_slots[device] |= 1u << slot;
                        }
                        emit(fd, EV_ABS, ABS_MT_POSITION_X, x);
                        emit(fd, EV_ABS, ABS_MT_POSITION_Y, y);
                        emit(fd, EV_ABS, ABS_X, x);
                        emit(fd, EV_ABS, ABS_Y, y);
                        emit(fd, EV_KEY, BTN_TOUCH, active_slots[device] != 0);
                        emit(fd, EV_SYN, SYN_REPORT, 0);
                } else if (!strcmp(command, "up") && touch[device] >= 0) {
                        int fd = touch[device];
                        active_slots[device] &= ~(1u << slot);
                        emit(fd, EV_ABS, ABS_MT_SLOT, slot);
                        emit(fd, EV_ABS, ABS_MT_TRACKING_ID, -1);
                        emit(fd, EV_KEY, BTN_TOUCH, active_slots[device] != 0);
                        emit(fd, EV_SYN, SYN_REPORT, 0);
                } else if (!strcmp(command, "key") && keyboard >= 0) {
                        emit(keyboard, EV_KEY, x, 1);
                        emit(keyboard, EV_SYN, SYN_REPORT, 0);
                        emit(keyboard, EV_KEY, x, 0);
                        emit(keyboard, EV_SYN, SYN_REPORT, 0);
                }
        }
        return 0;
}
