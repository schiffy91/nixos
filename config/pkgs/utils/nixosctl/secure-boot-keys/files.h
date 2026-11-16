#ifndef NIXOS_SECURE_BOOT_FILES_H
#define NIXOS_SECURE_BOOT_FILES_H
#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif
#include <errno.h>
#include <fcntl.h>
#include <linux/fs.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/prctl.h>
#include <sys/random.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <unistd.h>

typedef struct KeySignalGuardStorage {
    sigset_t previous;
    bool active;
} *KeySignalGuardHandle;

typedef struct KeyBytesStorage {
    unsigned char *bytes;
    int length;
    int errorCode;
} *KeyBytesHandle;

typedef struct KeyTextStorage {
    char text[64];
} *KeyTextHandle;

static inline void keySignalGuardClose(KeySignalGuardHandle guard) {
    if (!guard || !guard->active) return;
    guard->active = false;
    sigprocmask(SIG_SETMASK, &guard->previous, NULL);
}

static inline void keySignalGuardRelease(KeySignalGuardHandle guard) {
    if (!guard) return;
    keySignalGuardClose(guard);
    free(guard);
}

static inline KeySignalGuardHandle keySignalGuardAcquire(void) {
    sigset_t blocked;
    sigemptyset(&blocked);
    sigaddset(&blocked, SIGINT);
    sigaddset(&blocked, SIGTERM);
    KeySignalGuardHandle guard = calloc(1, sizeof(*guard));
    if (!guard) return NULL;
    if (sigprocmask(SIG_BLOCK, &blocked, &guard->previous) != 0) {
        free(guard);
        return NULL;
    }
    guard->active = true;
    return guard;
}

static inline bool keyFilesDisableDumps(void) {
    struct rlimit noCore = { 0, 0 };
    return setrlimit(RLIMIT_CORE, &noCore) == 0 && prctl(PR_SET_DUMPABLE, 0, 0, 0, 0) == 0;
}

static inline bool keyFilesStdinIsTty(void) {
    return isatty(STDIN_FILENO) != 0;
}

static inline bool keyFilesDirectoryValid(int descriptor) {
    struct stat info;
    return descriptor >= 0 && fstat(descriptor, &info) == 0 && S_ISDIR(info.st_mode) &&
        info.st_uid == 0 && (info.st_mode & 0022) == 0;
}

static inline int keyFilesOpenInitialDirectory(int initial) {
    int descriptor = initial < 0 ? open("/", O_RDONLY | O_DIRECTORY | O_CLOEXEC) : fcntl(initial, F_DUPFD_CLOEXEC, 3);
    if (!keyFilesDirectoryValid(descriptor)) {
        if (descriptor >= 0) close(descriptor);
        return -1;
    }
    return descriptor;
}

static inline int keyFilesStepDirectory(int descriptor, const char *part, bool create) {
    if (descriptor < 0 || !part) return -1;
    if (create) {
        if (mkdirat(descriptor, part, 0700) == 0) {
            if (fsync(descriptor) != 0) { close(descriptor); return -2; }
        } else if (errno != EEXIST) {
            close(descriptor);
            return -3;
        }
    }
    int child = openat(descriptor, part, O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
    close(descriptor);
    if (!keyFilesDirectoryValid(child)) {
        if (child >= 0) close(child);
        return -1;
    }
    return child;
}

static inline void keyFilesCloseDescriptor(int descriptor) {
    if (descriptor >= 0) close(descriptor);
}

static inline void keyBytesRelease(KeyBytesHandle handle) {
    if (!handle) return;
    if (handle->bytes && handle->length > 0) explicit_bzero(handle->bytes, (size_t)handle->length);
    free(handle->bytes);
    free(handle);
}

static inline const unsigned char *keyBytesData(KeyBytesHandle handle) {
    return handle ? handle->bytes : NULL;
}

static inline int keyBytesLength(KeyBytesHandle handle) {
    return handle && handle->length > 0 ? handle->length : 0;
}

static inline int keyBytesError(KeyBytesHandle handle) {
    return handle ? handle->errorCode : -1;
}

static inline KeyBytesHandle keyFilesReadFd(int descriptor, int limit) {
    KeyBytesHandle result = calloc(1, sizeof(*result));
    if (!result) return NULL;
    if (descriptor < 0 || limit <= 0) {
        result->errorCode = 1;
        return result;
    }
    result->bytes = malloc((size_t)limit);
    if (!result->bytes) {
        free(result);
        return NULL;
    }
    unsigned char chunk[8192];
    while (true) {
        ssize_t count = read(descriptor, chunk, sizeof(chunk));
        if (count < 0) {
            if (errno == EINTR) continue;
            result->errorCode = 1;
            return result;
        }
        if (count == 0) break;
        if (result->length > limit - (int)count) {
            result->errorCode = 2;
            return result;
        }
        memcpy(result->bytes + result->length, chunk, (size_t)count);
        result->length += (int)count;
    }
    explicit_bzero(chunk, sizeof(chunk));
    return result;
}

static inline KeyBytesHandle keyFilesReadStdin(int limit) {
    return keyFilesReadFd(STDIN_FILENO, limit);
}

static inline KeyBytesHandle keyFilesReadMember(int parent, const char *name, bool privateOnly, int limit) {
    if (parent < 0 || !name) return NULL;
    int descriptor = openat(parent, name, O_RDONLY | O_NOFOLLOW | O_NONBLOCK | O_CLOEXEC);
    struct stat info;
    if (descriptor < 0 || fstat(descriptor, &info) != 0) {
        if (descriptor >= 0) close(descriptor);
        KeyBytesHandle failed = calloc(1, sizeof(*failed));
        if (failed) failed->errorCode = 10;
        return failed;
    }
    int mode = (int)(info.st_mode & 07777);
    bool permitted = mode == 0400 || mode == 0600 || (!privateOnly && (mode == 0444 || mode == 0644));
    if (!S_ISREG(info.st_mode) || info.st_uid != 0 || info.st_nlink != 1 || !permitted) {
        close(descriptor);
        KeyBytesHandle unsafe = calloc(1, sizeof(*unsafe));
        if (unsafe) unsafe->errorCode = 11;
        return unsafe;
    }
    KeyBytesHandle readResult = keyFilesReadFd(descriptor, limit);
    close(descriptor);
    if (readResult && readResult->errorCode == 0 && readResult->length == 0) {
        readResult->errorCode = 12;
    }
    return readResult;
}

static inline int keyFilesHexNibble(char digit) {
    if (digit >= '0' && digit <= '9') return digit - '0';
    if (digit >= 'a' && digit <= 'f') return digit - 'a' + 10;
    if (digit >= 'A' && digit <= 'F') return digit - 'A' + 10;
    return -1;
}

static inline bool keyFilesWriteFdHex(int descriptor, const char *hex) {
    if (descriptor < 0 || !hex) return false;
    size_t hexLength = strlen(hex);
    if ((hexLength % 2) != 0) return false;
    size_t byteLength = hexLength / 2;
    unsigned char *decoded = malloc(byteLength > 0 ? byteLength : 1);
    if (!decoded) return false;
    for (size_t index = 0; index < byteLength; index++) {
        int high = keyFilesHexNibble(hex[index * 2]);
        int low = keyFilesHexNibble(hex[index * 2 + 1]);
        if (high < 0 || low < 0) {
            explicit_bzero(decoded, byteLength);
            free(decoded);
            return false;
        }
        decoded[index] = (unsigned char)((high << 4) | low);
    }
    size_t offset = 0;
    while (offset < byteLength) {
        ssize_t count = write(descriptor, decoded + offset, byteLength - offset);
        if (count <= 0) {
            if (count < 0 && errno == EINTR) continue;
            explicit_bzero(decoded, byteLength);
            free(decoded);
            return false;
        }
        offset += (size_t)count;
    }
    explicit_bzero(decoded, byteLength);
    free(decoded);
    return true;
}

static inline bool keyFilesWriteStdoutHex(const char *hex) {
    return keyFilesWriteFdHex(STDOUT_FILENO, hex);
}

static inline void keyTextRelease(KeyTextHandle handle) {
    free(handle);
}

static inline const char *keyTextValue(KeyTextHandle handle) {
    return handle ? handle->text : NULL;
}

static inline KeyTextHandle keyFilesStagingName(void) {
    unsigned char randomBytes[16];
    if (getrandom(randomBytes, sizeof(randomBytes), 0) != (ssize_t)sizeof(randomBytes)) return NULL;
    KeyTextHandle handle = calloc(1, sizeof(*handle));
    if (!handle) return NULL;
    memcpy(handle->text, ".secure-boot-keys-", 18);
    static const char digits[] = "0123456789abcdef";
    for (int index = 0; index < 16; index++) {
        handle->text[18 + index * 2] = digits[randomBytes[index] / 16];
        handle->text[18 + index * 2 + 1] = digits[randomBytes[index] % 16];
    }
    handle->text[50] = '\0';
    return handle;
}

static inline bool keyFilesDestinationAbsent(int parent, const char *name) {
    struct stat existing;
    return parent >= 0 && name && fstatat(parent, name, &existing, AT_SYMLINK_NOFOLLOW) != 0 && errno == ENOENT;
}

static inline bool keyFilesCreateStagingRoot(int parent, const char *staged) {
    return parent >= 0 && staged && mkdirat(parent, staged, 0700) == 0;
}

static inline int keyFilesWriteStagedMemberHex(int directory, const char *name, const char *hex) {
    if (directory < 0 || !name || !hex) return -1;
    int descriptor = openat(directory, name, O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW | O_CLOEXEC, 0600);
    if (descriptor < 0) return -1;
    if (!keyFilesWriteFdHex(descriptor, hex)) {
        close(descriptor);
        return -2;
    }
    int synced = (fsync(descriptor) == 0 && fsync(directory) == 0) ? 0 : -3;
    close(descriptor);
    return synced;
}

static inline bool keyFilesPublishStaged(int parent, int stage, const char *staged, const char *name) {
    return parent >= 0 && stage >= 0 && staged && name &&
        fsync(stage) == 0 && renameat2(parent, staged, parent, name, RENAME_NOREPLACE) == 0;
}

static inline bool keyFilesSyncDirectory(int descriptor) {
    return descriptor >= 0 && fsync(descriptor) == 0;
}

static inline void keyFilesRemoveStagedFile(int stage, const char *member) {
    if (stage >= 0 && member) unlinkat(stage, member, 0);
}

static inline void keyFilesRemoveStagedDirectory(int parent, const char *directory) {
    if (parent >= 0 && directory) unlinkat(parent, directory, AT_REMOVEDIR);
}
#endif
