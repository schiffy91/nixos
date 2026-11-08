#ifndef NIXOS_ADDON_FILES_H
#define NIXOS_ADDON_FILES_H
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static inline bool addonFileMove(const char *source, const char *target) { return rename(source, target) == 0; }
static inline bool addonFileRemove(const char *path) { return unlink(path) == 0; }
static inline bool addonLockPublish(const char *path, const char *pattern, const char *text) {
    struct stat existing;
    bool present = stat(path, &existing) == 0;
    if (!present && errno != ENOENT) return false;
    char *temporary = strdup(pattern);
    if (!temporary) return false;
    int descriptor = mkstemp(temporary);
    if (descriptor < 0) { free(temporary); return false; }
    bool valid = true;
    size_t remaining = strlen(text);
    while (remaining) {
        ssize_t count = write(descriptor, text, remaining);
        if (count < 0 && errno == EINTR) continue;
        if (count <= 0) { valid = false; break; }
        text += count; remaining -= (size_t)count;
    }
    if (present && fchown(descriptor, existing.st_uid, existing.st_gid) != 0) valid = false;
    if (fchmod(descriptor, 0644) != 0 || fsync(descriptor) != 0) valid = false;
    if (close(descriptor) != 0) valid = false;
    if (!valid || rename(temporary, path) != 0) { unlink(temporary); valid = false; }
    free(temporary);
    return valid;
}
#endif
