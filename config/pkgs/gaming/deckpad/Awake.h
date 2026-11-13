#pragma once

#include <fcntl.h>
#include <linux/uinput.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

/* A pointer that exists only to be nudged: one step right and one step back,
 * which the Deck's own session takes for someone using the machine. The
 * kernel's uinput structures and request numbers are kept on this side. */

/* The device's descriptor, or -1 when one cannot be made. */
static inline int deckAwakeOpen(const char* name) {
	int pointer = open("/dev/uinput", O_WRONLY | O_NONBLOCK | O_CLOEXEC);
	if (pointer < 0) { return -1; }
	struct uinput_setup setup;
	memset(&setup, 0, sizeof(setup));
	setup.id.bustype = BUS_VIRTUAL;
	strncpy(setup.name, name, UINPUT_MAX_NAME_SIZE - 1);
	/* Two axes and a button: what the session's input stack calls a mouse. */
	if (ioctl(pointer, UI_SET_EVBIT, EV_REL) < 0 || ioctl(pointer, UI_SET_RELBIT, REL_X) < 0 || ioctl(pointer, UI_SET_RELBIT, REL_Y) < 0 ||
	    ioctl(pointer, UI_SET_EVBIT, EV_KEY) < 0 || ioctl(pointer, UI_SET_KEYBIT, BTN_LEFT) < 0 ||
	    ioctl(pointer, UI_DEV_SETUP, &setup) < 0 || ioctl(pointer, UI_DEV_CREATE) < 0) {
		close(pointer);
		return -1;
	}
	return pointer;
}

static inline void deckAwakeNudge(int pointer) {
	struct input_event steps[4];
	memset(steps, 0, sizeof(steps));
	steps[0].type = EV_REL; steps[0].code = REL_X; steps[0].value = 1;
	steps[1].type = EV_SYN; steps[1].code = SYN_REPORT;
	steps[2].type = EV_REL; steps[2].code = REL_X; steps[2].value = -1;
	steps[3].type = EV_SYN; steps[3].code = SYN_REPORT;
	if (write(pointer, steps, sizeof(steps)) < 0) { return; }
}

static inline void deckAwakeClose(int pointer) {
	ioctl(pointer, UI_DEV_DESTROY);
	close(pointer);
}
