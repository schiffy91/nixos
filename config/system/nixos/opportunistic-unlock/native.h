#ifndef NIXOS_OPPORTUNISTIC_UNLOCK_NATIVE_H
#define NIXOS_OPPORTUNISTIC_UNLOCK_NATIVE_H
#define _GNU_SOURCE
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <limits.h>
#include <linux/fs.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <libcryptsetup.h>
#include "handoff.h"

/* The header reader cannot evaluate ioctl macros containing sizeof. */
enum { unlockDiskSequence = BLKGETDISKSEQ, unlockReadOnly = BLKROGET };
#endif
