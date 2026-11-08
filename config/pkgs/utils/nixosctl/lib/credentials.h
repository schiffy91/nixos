#ifndef NIXOS_CREDENTIAL_STORAGE_H
#define NIXOS_CREDENTIAL_STORAGE_H
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/stat.h>
#if defined(__linux__)
#include <shadow.h>
#endif

static inline bool credentialRootDirectory(const char *path) {
    struct stat info;
    return lstat(path, &info) == 0 && S_ISDIR(info.st_mode)
        && info.st_uid == 0 && (info.st_mode & 0022) == 0;
}

typedef struct AccountHashStorage {
    char *buffer;
    const char *hash;
} *AccountHashHandle;
static inline void accountHashRelease(AccountHashHandle account) {
    if (!account) return;
    free(account->buffer);
    free(account);
}
static inline bool accountHashSupported(void) {
#if defined(__linux__)
    return true;
#else
    return false;
#endif
}
static inline AccountHashHandle accountHashRead(const char *user) {
#if defined(__linux__)
    AccountHashHandle account = calloc(1, sizeof(*account));
    if (!account) return NULL;
    for (size_t capacity = 1024; capacity <= 1048576; capacity *= 2) {
        char *buffer = realloc(account->buffer, capacity);
        if (!buffer) break;
        account->buffer = buffer;
        struct spwd entry, *found = NULL;
        int status = getspnam_r(user, &entry, buffer, capacity, &found);
        if (status == ERANGE) continue;
        if (status == 0 && found && found->sp_pwdp) {
            account->hash = found->sp_pwdp;
            return account;
        }
        break;
    }
    accountHashRelease(account);
#else
    (void)user;
#endif
    return NULL;
}
static inline const char *accountHashValue(AccountHashHandle account) { return account->hash; }
#endif
