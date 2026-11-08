#ifndef NIXOS_USER_ACCOUNT_H
#define NIXOS_USER_ACCOUNT_H

#include <errno.h>
#include <pwd.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>

/* NSS owns the lookup policy; this snapshot owns all of its returned storage. */
typedef struct UserAccountStorage {
    struct passwd entry;
    char *buffer;
} *UserAccountRecord;

static inline void userAccountRelease(UserAccountRecord account) {
    if (!account) return;
    free(account->buffer);
    free(account);
}

static inline UserAccountRecord userAccountFind(const char *name) {
    if (!name || !*name) return NULL;
    UserAccountRecord account = calloc(1, sizeof(*account));
    if (!account) return NULL;
    /* NSS records can exceed the usual passwd-file size. Retry ERANGE while
       bounding allocations from an unavailable or misconfigured provider. */
    for (size_t capacity = 1024; capacity <= 1048576; capacity *= 2) {
        char *buffer = realloc(account->buffer, capacity);
        if (!buffer) break;
        account->buffer = buffer;
        struct passwd *found = NULL;
        int status = getpwnam_r(name, &account->entry, buffer, capacity, &found);
        if (status == ERANGE) continue;
        if (status == 0 && found && found->pw_name && found->pw_dir) return account;
        break;
    }
    userAccountRelease(account);
    return NULL;
}

static inline const char *userAccountName(UserAccountRecord account) {
    return account->entry.pw_name;
}

static inline const char *userAccountHome(UserAccountRecord account) {
    return account->entry.pw_dir;
}

static inline bool userAccountIsCurrent(UserAccountRecord account) {
    return account->entry.pw_uid == geteuid();
}

#endif
