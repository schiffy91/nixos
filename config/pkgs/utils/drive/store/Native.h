#pragma once

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/xattr.h>
#include <time.h>
#include <unistd.h>

// What the store needs from Linux that BTRC cannot call directly: extended
// attributes, sparse files and the calls whose constants are Linux-only.

struct DriveStatus {
	unsigned long device;
	unsigned long number;
	unsigned long size;
	unsigned long blocks;
	long modified;
	long modifiedNanoseconds;
	unsigned int mode;
	unsigned int links;
};

static inline int driveInspect(int descriptor, struct DriveStatus* out) {
	struct stat status;
	if (fstat(descriptor, &status) != 0) { return -1; }
	out->device = (unsigned long)status.st_dev;
	out->number = (unsigned long)status.st_ino;
	out->size = (unsigned long)status.st_size;
	out->blocks = (unsigned long)status.st_blocks;
	out->modified = (long)status.st_mtim.tv_sec;
	out->modifiedNanoseconds = status.st_mtim.tv_nsec;
	out->mode = status.st_mode;
	out->links = (unsigned int)status.st_nlink;
	return 0;
}

// Open without following a final symbolic link; `mode` 0 a directory or file
// to look at, 1 a file to read and write, 2 the same, created if missing.
static inline int driveOpen(const char* path, int mode) {
	int flags = O_CLOEXEC | O_NOFOLLOW | (mode == 0 ? O_RDONLY : O_RDWR) | (mode == 2 ? O_CREAT : 0);
	return open(path, flags, 0644);
}

static inline int driveOpenAt(int directory, const char* name, int mode) {
	int flags = O_CLOEXEC | O_NOFOLLOW | (mode == 0 ? O_RDONLY : O_RDWR) | (mode == 2 ? O_CREAT : 0);
	return openat(directory, name, flags, 0644);
}

// Open the file a path descriptor names, for real.
static inline int driveReopen(int descriptor, int write) {
	char name[40];
	if (snprintf(name, sizeof(name), "/proc/self/fd/%d", descriptor) >= (int)sizeof(name)) { return -1; }
	return open(name, (write ? O_RDWR : O_RDONLY) | O_CLOEXEC);
}

// Read at an offset: the count, or a negative errno.
static inline long driveReadAt(int descriptor, unsigned char* buffer, unsigned long size, long offset) {
	ssize_t length = pread(descriptor, buffer, size, (off_t)offset);
	return length < 0 ? -(long)errno : (long)length;
}

// An attribute's text into `value`: its length, or a negative errno.
static inline long driveAttribute(int descriptor, const char* name, char* value, unsigned long capacity) {
	ssize_t length = fgetxattr(descriptor, name, value, capacity - 1);
	if (length < 0) { return -(long)errno; }
	value[length] = 0;
	return (long)length;
}

static inline int driveSetAttribute(int descriptor, const char* name, const char* value) { return fsetxattr(descriptor, name, value, strlen(value), 0) != 0 ? -errno : 0; }

static inline int driveRemoveAttribute(int descriptor, const char* name) { return fremovexattr(descriptor, name) != 0 ? -errno : 0; }

static inline int driveResize(int descriptor, long size) { return ftruncate(descriptor, (off_t)size); }

// Set the modification time and leave the access time alone.
static inline int driveSetModified(int descriptor, long seconds, long nanoseconds) {
	struct timespec times[2];
	times[0].tv_sec = 0;
	times[0].tv_nsec = UTIME_OMIT;
	times[1].tv_sec = (time_t)seconds;
	times[1].tv_nsec = nanoseconds;
	return futimens(descriptor, times);
}

// Give back the disk blocks of a range; the file keeps its size and reads
// there find a hole again.
static inline int driveRelease(int descriptor, long offset, long length) { return fallocate(descriptor, FALLOC_FL_PUNCH_HOLE | FALLOC_FL_KEEP_SIZE, (off_t)offset, (off_t)length); }

// Where the next data (`hole` 0) or hole (1) starts at or after `offset`:
// the offset, or -1 when there is none before the end.
static inline long driveSeek(int descriptor, long offset, int hole) {
	off_t found = lseek(descriptor, (off_t)offset, hole ? SEEK_HOLE : SEEK_DATA);
	return found < 0 ? -1 : (long)found;
}

// Copy `length` bytes from the start of `source` to `offset` in `target`.
static inline int driveCopyInto(int target, long offset, int source, long length) {
	unsigned char buffer[65536];
	long done = 0;
	while (done < length) {
		size_t want = (size_t)(length - done < (long)sizeof(buffer) ? length - done : (long)sizeof(buffer));
		ssize_t got = pread(source, buffer, want, (off_t)done);
		if (got <= 0) {
			if (got == 0) { errno = EIO; }
			return -1;
		}
		if (pwrite(target, buffer, (size_t)got, (off_t)(offset + done)) != got) { return -1; }
		done += got;
	}
	return 0;
}

// Read a small file to its end: the count, or -1. For files that do not
// say how long they are, such as those under /proc.
static inline long driveReadAll(const char* path, unsigned char* buffer, unsigned long capacity) {
	int descriptor = open(path, O_RDONLY | O_CLOEXEC);
	unsigned long filled = 0;
	if (descriptor < 0) { return -1; }
	while (filled < capacity) {
		ssize_t got = read(descriptor, buffer + filled, capacity - filled);
		if (got <= 0) { break; }
		filled += (unsigned long)got;
	}
	close(descriptor);
	return (long)filled;
}

static inline int driveLink(const char* existing, const char* created) { return link(existing, created); }

static inline int driveRename(const char* from, const char* to) { return rename(from, to); }

static inline int driveRemove(const char* path) { return unlink(path); }

static inline int driveRemoveDirectory(const char* path) { return rmdir(path); }

static inline int driveSynchronize(int descriptor) { return fsync(descriptor); }
