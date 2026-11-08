#ifndef NIXOS_OPPORTUNISTIC_UNLOCK_NATIVE_H
#define NIXOS_OPPORTUNISTIC_UNLOCK_NATIVE_H
#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <linux/fs.h>
#include <signal.h>
#include <stdint.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <sys/wait.h>
#include <unistd.h>
#include <libcryptsetup.h>
#include "handoff.h"

/* The header reader cannot evaluate ioctl macros containing sizeof. */
enum { unlockDiskSequence = BLKGETDISKSEQ, unlockReadOnly = BLKROGET };
#endif
