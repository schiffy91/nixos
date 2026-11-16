#ifndef NIXOS_SUNSHINE_CREDENTIALS_H
#define NIXOS_SUNSHINE_CREDENTIALS_H
#include <errno.h>
#include <fcntl.h>
#include <pwd.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/random.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

typedef struct SunshineBytesStorage {
    unsigned char *bytes;
    size_t length;
    int status;
} *SunshineBytesHandle;

typedef struct SunshineTextStorage {
    char *text;
} *SunshineTextHandle;

static inline void sunshineBytesRelease(SunshineBytesHandle handle) {
    if (!handle) return;
    if (handle->bytes && handle->length > 0) {
        explicit_bzero(handle->bytes, handle->length);
    }
    free(handle->bytes);
    free(handle);
}

static inline int sunshineBytesStatus(SunshineBytesHandle handle) {
    return handle ? handle->status : -2;
}

static inline const unsigned char *sunshineBytesData(SunshineBytesHandle handle) {
    static const unsigned char empty = 0;
    if (!handle || !handle->bytes) return &empty;
    return handle->bytes;
}

static inline int sunshineBytesLength(SunshineBytesHandle handle) {
    return handle && handle->bytes ? (int)handle->length : 0;
}

static inline void sunshineTextRelease(SunshineTextHandle handle) {
    if (!handle) return;
    free(handle->text);
    free(handle);
}

static inline const char *sunshineTextValue(SunshineTextHandle handle) {
    return handle && handle->text ? handle->text : "";
}

static inline SunshineTextHandle sunshineGenerateSalt(void) {
    static const char alphabet[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    SunshineTextHandle handle = calloc(1, sizeof(*handle));
    if (!handle) return NULL;
    handle->text = calloc(17, 1);
    if (!handle->text) {
        free(handle);
        return NULL;
    }
    size_t written = 0;
    while (written < 16) {
        unsigned char byte = 0;
        if (getrandom(&byte, 1, 0) != 1) {
            if (errno == EINTR) continue;
            sunshineTextRelease(handle);
            return NULL;
        }
        // Rejection sampling avoids modulo bias across the 62-character alphabet.
        if (byte < 248u) {
            handle->text[written++] = alphabet[byte % 62u];
        }
    }
    return handle;
}

static inline SunshineBytesHandle sunshineReadDescriptor(int descriptor, int limit) {
    SunshineBytesHandle handle = calloc(1, sizeof(*handle));
    if (!handle) return NULL;
    if (descriptor < 0 || limit < 0) {
        handle->status = -2;
        return handle;
    }
    size_t capacity = (size_t)limit + 1;
    handle->bytes = calloc(capacity > 0 ? capacity : 1, 1);
    if (!handle->bytes) {
        handle->status = -2;
        return handle;
    }
    unsigned char buffer[4096];
    while (handle->length <= (size_t)limit) {
        ssize_t count = read(descriptor, buffer, sizeof(buffer));
        if (count == 0) {
            explicit_bzero(buffer, sizeof(buffer));
            handle->status = 0;
            return handle;
        }
        if (count < 0) {
            if (errno == EINTR) continue;
            explicit_bzero(buffer, sizeof(buffer));
            handle->status = -2;
            return handle;
        }
        size_t chunk = (size_t)count;
        if (handle->length + chunk > (size_t)limit) {
            explicit_bzero(buffer, sizeof(buffer));
            handle->status = -3;
            return handle;
        }
        memcpy(handle->bytes + handle->length, buffer, chunk);
        handle->length += chunk;
    }
    explicit_bzero(buffer, sizeof(buffer));
    handle->status = -3;
    return handle;
}

static inline SunshineBytesHandle sunshineReadStdin(int limit) {
    return sunshineReadDescriptor(STDIN_FILENO, limit);
}

static inline int sunshineOpenStoreDir(const char *path) {
    if (!path || path[0] != '/') return -2;
    int current = open("/", O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    if (current < 0) return -1;
    const char *cursor = path + 1;
    while (*cursor != '\0') {
        const char *slash = strchr(cursor, '/');
        size_t partLen = slash ? (size_t)(slash - cursor) : strlen(cursor);
        if (partLen == 0 || partLen >= 256 ||
            (partLen == 1 && cursor[0] == '.') ||
            (partLen == 2 && cursor[0] == '.' && cursor[1] == '.')) {
            close(current);
            return -2;
        }
        char part[256];
        memcpy(part, cursor, partLen);
        part[partLen] = '\0';
        if (mkdirat(current, part, 0700) != 0 && errno != EEXIST) {
            close(current);
            return -3;
        }
        int child = openat(current, part, O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
        if (child < 0) {
            close(current);
            return -2;
        }
        struct stat info;
        bool safe = fstat(child, &info) == 0 &&
            (info.st_uid == 0 || info.st_uid == getuid()) &&
            (info.st_mode & 0022) == 0;
        close(current);
        current = child;
        if (!safe) {
            close(current);
            return -2;
        }
        if (!slash) break;
        cursor = slash + 1;
    }
    struct stat info;
    if (fstat(current, &info) != 0 || info.st_uid != getuid() || fchmod(current, 0700) != 0) {
        close(current);
        return -4;
    }
    return current;
}

static inline bool sunshineVerifySameDir(int descriptor, const char *path) {
    if (descriptor < 0 || !path) return false;
    struct stat opened;
    struct stat named;
    return fstat(descriptor, &opened) == 0 &&
        lstat(path, &named) == 0 &&
        named.st_ino == opened.st_ino &&
        named.st_dev == opened.st_dev;
}

static inline SunshineBytesHandle sunshineReadLegacyState(int dirfd, int limit) {
    SunshineBytesHandle missing = calloc(1, sizeof(*missing));
    if (!missing) return NULL;
    if (dirfd < 0) {
        missing->status = -1;
        return missing;
    }
    int file = openat(dirfd, "sunshine_state.json", O_RDONLY | O_NOFOLLOW | O_NONBLOCK | O_CLOEXEC);
    if (file < 0) {
        missing->status = (errno == ENOENT) ? 1 : -1;
        return missing;
    }
    struct stat info;
    if (fstat(file, &info) != 0 || !S_ISREG(info.st_mode) || info.st_uid != getuid() ||
        info.st_nlink != 1 || (info.st_mode & 0022) != 0) {
        close(file);
        missing->status = -1;
        return missing;
    }
    free(missing);
    SunshineBytesHandle data = sunshineReadDescriptor(file, limit);
    close(file);
    return data;
}

static inline void sunshineCloseDescriptor(int descriptor) {
    if (descriptor >= 0) close(descriptor);
}

static inline bool sunshinePrepareProcess(void) {
    struct rlimit limit = { 0, 0 };
    if (setrlimit(RLIMIT_CORE, &limit) != 0) return false;
    umask(0077);
    return true;
}

static inline int sunshineAccountMatchesCurrentUid(const char *username) {
    if (!username || username[0] == '\0') return 0;
    struct passwd account;
    struct passwd *found = NULL;
    char buffer[16384];
    if (getpwnam_r(username, &account, buffer, sizeof(buffer), &found) != 0) return -1;
    if (!found || getuid() != account.pw_uid) return 0;
    return 1;
}
#endif
