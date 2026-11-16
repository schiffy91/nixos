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
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
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

typedef struct UnlockSecretStorage {
    char *memory;
    int capacity;
} *UnlockSecretHandle;

typedef struct UnlockKeyDescriptionStorage {
    char text[256];
} *UnlockKeyDescriptionHandle;

typedef struct UnlockCryptStorage {
    struct crypt_device *device;
} *UnlockCryptHandle;

static inline void unlockSecretErase(UnlockSecretHandle secret) {
    if (secret && secret->memory && secret->capacity > 0) {
        explicit_bzero(secret->memory, (size_t)secret->capacity);
    }
}

static inline void unlockSecretRelease(UnlockSecretHandle secret) {
    if (!secret) return;
    if (secret->memory) {
        unlockSecretErase(secret);
        munmap(secret->memory, (size_t)secret->capacity);
        secret->memory = NULL;
    }
    free(secret);
}

static inline UnlockSecretHandle unlockSecretCreate(int capacity) {
    if (capacity <= 0 || capacity > 4096) return NULL;
    UnlockSecretHandle secret = calloc(1, sizeof(*secret));
    if (!secret) return NULL;
    secret->capacity = capacity;
    void *mapped = mmap(NULL, (size_t)capacity, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mapped == MAP_FAILED) {
        free(secret);
        return NULL;
    }
    secret->memory = (char *)mapped;
    if (mlock(secret->memory, (size_t)capacity) != 0 || madvise(secret->memory, (size_t)capacity, MADV_DONTDUMP) != 0) {
        unlockSecretRelease(secret);
        return NULL;
    }
    return secret;
}

static inline void unlockSecretCloseMemory(UnlockSecretHandle secret) {
    if (!secret || !secret->memory) return;
    unlockSecretErase(secret);
    munmap(secret->memory, (size_t)secret->capacity);
    secret->memory = NULL;
}

static inline bool unlockSecretOpened(UnlockSecretHandle secret) {
    return secret && secret->memory != NULL;
}

static inline bool unlockSecretLockAfterFork(UnlockSecretHandle secret) {
    return secret && secret->memory && mlock(secret->memory, (size_t)secret->capacity) == 0;
}

static inline void unlockSecretFill(UnlockSecretHandle secret, int value) {
    if (secret && secret->memory && secret->capacity > 0) {
        memset(secret->memory, value & 0xff, (size_t)secret->capacity);
    }
}

static inline bool unlockSecretAllEqual(UnlockSecretHandle secret, int value) {
    if (!secret || !secret->memory || secret->capacity <= 0) return false;
    unsigned char expected = (unsigned char)(value & 0xff);
    const unsigned char *bytes = (const unsigned char *)secret->memory;
    for (int index = 0; index < secret->capacity; index++) {
        if (bytes[index] != expected) return false;
    }
    return true;
}

static inline int unlockSecretLoadText(UnlockSecretHandle secret, const char *text) {
    if (!secret || !secret->memory || !text) return -1;
    size_t length = strlen(text);
    if (length == 0 || length > (size_t)secret->capacity) return -1;
    memcpy(secret->memory, text, length);
    return (int)length;
}

static inline void unlockKeyDescriptionRelease(UnlockKeyDescriptionHandle description) {
    free(description);
}

static inline UnlockKeyDescriptionHandle unlockKeyDescriptionRead(long key) {
    UnlockKeyDescriptionHandle description = calloc(1, sizeof(*description));
    if (!description) return NULL;
    long size = syscall(SYS_keyctl, KEYCTL_DESCRIBE, key, description->text, sizeof(description->text));
    if (size <= 0 || (size_t)size > sizeof(description->text)) {
        free(description);
        return NULL;
    }
    description->text[sizeof(description->text) - 1] = '\0';
    return description;
}

static inline const char *unlockKeyDescriptionText(UnlockKeyDescriptionHandle description) {
    return description ? description->text : NULL;
}

static inline long unlockKeyringSearchPassphrase(void) {
    return syscall(SYS_keyctl, KEYCTL_SEARCH, KEY_SPEC_USER_KEYRING, "user", UNLOCK_PASSPHRASE_KEY, 0);
}

static inline int unlockKeyringReadPassphrase(long key, UnlockSecretHandle secret) {
    if (!secret || !secret->memory) return -1;
    long length = syscall(SYS_keyctl, KEYCTL_READ, key, secret->memory, (size_t)secret->capacity);
    if (length <= 0 || length > (long)secret->capacity) return -1;
    return (int)length;
}

static inline bool unlockKeyringRevoke(long key) {
    return syscall(SYS_keyctl, KEYCTL_REVOKE, key) >= 0;
}

static inline long unlockMaxOffset(void) {
    return LONG_MAX;
}

static inline long long unlockBlockDeviceAt(const char *path) {
    struct stat status;
    if (!path || stat(path, &status) != 0 || !S_ISBLK(status.st_mode)) return -1LL;
    return ((long long)(unsigned int)major(status.st_rdev) << 32) | (long long)(unsigned int)minor(status.st_rdev);
}

static inline bool unlockBlockFdMatches(int descriptor, int expectedMajor, int expectedMinor, long expectedSequence) {
    struct stat status;
    uint64_t sequence = 0;
    if (descriptor < 0 || fstat(descriptor, &status) != 0 || !S_ISBLK(status.st_mode)) return false;
    if ((int)major(status.st_rdev) != expectedMajor || (int)minor(status.st_rdev) != expectedMinor) return false;
    return ioctl(descriptor, BLKGETDISKSEQ, &sequence) == 0 && sequence == (uint64_t)expectedSequence;
}

static inline void unlockQuietCryptsetupCallback(int level, const char *message, void *data) {
    (void)level;
    (void)message;
    (void)data;
}

static inline void unlockQuietCryptsetup(void) {
    crypt_set_log_callback(NULL, unlockQuietCryptsetupCallback, NULL);
}

static inline void unlockCryptRelease(UnlockCryptHandle handle) {
    if (!handle) return;
    if (handle->device) {
        crypt_free(handle->device);
        handle->device = NULL;
    }
    free(handle);
}

static inline UnlockCryptHandle unlockCryptOpen(const char *path) {
    if (!path) return NULL;
    UnlockCryptHandle handle = calloc(1, sizeof(*handle));
    if (!handle) return NULL;
    if (crypt_init(&handle->device, path) < 0 || crypt_load(handle->device, CRYPT_LUKS, NULL) < 0) {
        unlockCryptRelease(handle);
        return NULL;
    }
    return handle;
}

static inline bool unlockCryptSoftwareOnly(UnlockCryptHandle handle) {
    if (!handle || !handle->device) return false;
    const char *type = crypt_get_type(handle->device);
    if (!type) return false;
    if (strcmp(type, "LUKS1") == 0) return true;
    if (strcmp(type, "LUKS2") != 0) return false;
    struct crypt_params_integrity integrity;
    memset(&integrity, 0, sizeof(integrity));
    bool hasIntegrity = crypt_get_integrity_info(handle->device, &integrity) == 0 &&
        integrity.integrity != NULL && integrity.integrity[0] != 0;
    return crypt_get_hw_encryption_type(handle->device) == CRYPT_SW_ONLY &&
        crypt_reencrypt_status(handle->device, NULL) == CRYPT_REENCRYPT_NONE && !hasIntegrity;
}

static inline bool unlockCryptInactive(UnlockCryptHandle handle, const char *mapping) {
    return handle && handle->device && mapping && crypt_status(handle->device, mapping) == CRYPT_INACTIVE;
}

static inline int unlockCryptKeySize(UnlockCryptHandle handle) {
    return handle && handle->device ? crypt_get_volume_key_size(handle->device) : 0;
}

static inline bool unlockCryptVerifyKey(UnlockCryptHandle handle, UnlockSecretHandle key, UnlockSecretHandle passphrase, int passphraseLength) {
    if (!handle || !handle->device || !key || !key->memory || !passphrase || !passphrase->memory || passphraseLength <= 0) return false;
    size_t keySize = (size_t)key->capacity;
    int verified = crypt_volume_key_get(handle->device, CRYPT_ANY_SLOT, key->memory, &keySize,
        passphrase->memory, (size_t)passphraseLength);
    return verified >= 0 && keySize == (size_t)key->capacity;
}

static inline int unlockCryptActivate(UnlockCryptHandle handle, int descriptor, const char *mapping, UnlockSecretHandle key) {
    if (!handle || !handle->device || descriptor < 0 || !mapping || !key || !key->memory) return -1;
    int readonly = 1;
    if (ioctl(descriptor, BLKROGET, &readonly) != 0) return 0;
    uint32_t flags = (uint32_t)CRYPT_ACTIVATE_IGNORE_PERSISTENT | (readonly != 0 ? (uint32_t)CRYPT_ACTIVATE_READONLY : 0u);
    return crypt_activate_by_volume_key(handle->device, mapping, key->memory, (size_t)key->capacity, flags) >= 0 ? 1 : -1;
}

static inline void unlockCryptDeactivate(UnlockCryptHandle handle, const char *mapping) {
    if (handle && handle->device && mapping) {
        crypt_deactivate(handle->device, mapping);
    }
}

static inline long long unlockCryptMappingDeviceAt(const char *mapping) {
    if (!mapping) return -1LL;
    struct crypt_device *active = NULL;
    if (crypt_init_by_name(&active, mapping) < 0) return -1LL;
    const char *path = crypt_get_device_name(active);
    long long encoded = path ? unlockBlockDeviceAt(path) : -1LL;
    crypt_free(active);
    return encoded;
}

static inline int unlockOpenVolumeDescriptor(const char *path) {
    if (!path) return -1;
    return open(path, O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK);
}

static inline void unlockCloseDescriptor(int descriptor) {
    if (descriptor >= 0) close(descriptor);
}

static inline bool unlockDisableProcessDumps(void) {
    struct rlimit limit = { 0, 0 };
    return setrlimit(RLIMIT_CORE, &limit) == 0 && prctl(PR_SET_DUMPABLE, 0) == 0;
}

static inline bool unlockPrepareChildLimits(int parentPid) {
    struct rlimit memory;
    memory.rlim_cur = 1536UL * 1024UL * 1024UL;
    memory.rlim_max = memory.rlim_cur;
    return prctl(PR_SET_PDEATHSIG, SIGKILL) == 0 && (int)getppid() == parentPid && setrlimit(RLIMIT_AS, &memory) == 0;
}

static inline int unlockForkProcess(void) {
    return (int)fork();
}

static inline void unlockExitProcess(int code) {
    _exit(code);
}

static inline bool unlockChildRunning(int pid) {
    if (pid <= 0) return false;
    int status = 0;
    pid_t exited = waitpid((pid_t)pid, &status, WNOHANG);
    if (exited == (pid_t)pid || (exited < 0 && errno != EINTR)) return false;
    return true;
}

static inline bool unlockWaitChildSuccess(int pid) {
    if (pid <= 0) return false;
    int status = 0;
    return waitpid((pid_t)pid, &status, 0) == (pid_t)pid && WIFEXITED(status) && WEXITSTATUS(status) == 0;
}

static inline void unlockKillProcess(int pid) {
    if (pid > 0) kill((pid_t)pid, SIGKILL);
}
#endif
