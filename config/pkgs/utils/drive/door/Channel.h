#pragma once

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

// The door and the engine talk over one local socket that keeps message
// boundaries and tells each side when the other is gone.

static inline int driveChannelAddress(struct sockaddr_un* address, const char* path) {
	memset(address, 0, sizeof(*address));
	address->sun_family = AF_UNIX;
	if (strlen(path) >= sizeof(address->sun_path)) {
		errno = ENAMETOOLONG;
		return -1;
	}
	strcpy(address->sun_path, path);
	return 0;
}

// Listen at `path`, replacing a socket left there: the descriptor, or -1.
static inline int driveChannelListen(const char* path) {
	struct sockaddr_un address;
	int descriptor;
	if (driveChannelAddress(&address, path) != 0) { return -1; }
	descriptor = socket(AF_UNIX, SOCK_SEQPACKET | SOCK_CLOEXEC | SOCK_NONBLOCK, 0);
	if (descriptor < 0) { return -1; }
	unlink(path);
	if (bind(descriptor, (struct sockaddr*)&address, sizeof(address)) != 0 || listen(descriptor, 4) != 0) {
		close(descriptor);
		return -1;
	}
	return descriptor;
}

// The next waiting connection, or -1 when there is none.
static inline int driveChannelAccept(int listener) { return accept4(listener, NULL, NULL, SOCK_CLOEXEC | SOCK_NONBLOCK); }

static inline int driveChannelConnect(const char* path) {
	struct sockaddr_un address;
	int descriptor;
	if (driveChannelAddress(&address, path) != 0) { return -1; }
	descriptor = socket(AF_UNIX, SOCK_SEQPACKET | SOCK_CLOEXEC, 0);
	if (descriptor < 0) { return -1; }
	if (connect(descriptor, (struct sockaddr*)&address, sizeof(address)) != 0) {
		close(descriptor);
		return -1;
	}
	return descriptor;
}

// 0, or -1 when the other side is gone or not keeping up.
static inline int driveChannelSend(int descriptor, const char* message) { return send(descriptor, message, strlen(message), MSG_NOSIGNAL | MSG_DONTWAIT) < 0 ? -1 : 0; }

// One message into `message`: its length, 0 when none is waiting, or -1
// when the other side is gone.
static inline long driveChannelReceive(int descriptor, char* message, unsigned long capacity) {
	ssize_t length = recv(descriptor, message, capacity - 1, MSG_DONTWAIT);
	if (length < 0) { return errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR ? 0 : -1; }
	if (length == 0) { return -1; }
	message[length] = 0;
	return (long)length;
}

// Wait until one of up to three descriptors can be read, at most
// `milliseconds`. Returns a bit per descriptor that is ready (or gone).
static inline int driveChannelWait(int first, int second, int third, int milliseconds) {
	struct pollfd waiting[3];
	int descriptors[3];
	int count = 0;
	int ready = 0;
	int index;
	descriptors[0] = first;
	descriptors[1] = second;
	descriptors[2] = third;
	for (index = 0; index < 3; index++) {
		waiting[index].fd = descriptors[index];
		waiting[index].events = POLLIN;
		waiting[index].revents = 0;
		if (descriptors[index] >= 0) { count = index + 1; }
	}
	if (poll(waiting, (nfds_t)count, milliseconds) <= 0) { return 0; }
	for (index = 0; index < count; index++) {
		if (waiting[index].revents != 0) { ready |= 1 << index; }
	}
	return ready;
}
