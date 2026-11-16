#ifndef NIXOS_PASSWORD_HASH_H
#define NIXOS_PASSWORD_HASH_H

#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#if defined(__linux__)
#include <crypt.h>
#endif

static inline void credentialWipe(void *memory, size_t size) {
    volatile unsigned char *bytes = (volatile unsigned char *)memory;
    while (size-- > 0) *bytes++ = 0;
}

static inline bool credentialHashMatches(const char *hash, const char *password) {
    if (!hash || !password || hash[0] != '$') return false;
#if defined(__linux__)
    struct crypt_data data;
    memset(&data, 0, sizeof(data));
    char *computed = crypt_r(password, hash, &data);
    bool matches = computed != NULL && computed[0] == '$' && strcmp(computed, hash) == 0;
    credentialWipe(&data, sizeof(data));
    return matches;
#else
    return false;
#endif
}

#endif
