#ifndef NIXOS_NETWORK_PREFIX_H
#define NIXOS_NETWORK_PREFIX_H
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct NetworkPrefixStorage { char cidr[INET6_ADDRSTRLEN + 5]; } *NetworkPrefixHandle;
static inline void networkPrefixRelease(NetworkPrefixHandle prefix) { free(prefix); }
static inline NetworkPrefixHandle networkPrefixParse(const char *address, int width, int length) {
    if (!address || (width != 32 && width != 128) || length <= 0 || length > width) return NULL;
    int family = width == 32 ? AF_INET : AF_INET6;
    unsigned char bytes[16] = {0};
    if (inet_pton(family, address, bytes) != 1) return NULL;
    for (int index = 0; index < width / 8; index++) {
        int remaining = length - index * 8;
        if (remaining <= 0) bytes[index] = 0;
        else if (remaining < 8) bytes[index] &= (unsigned char)(255 << (8 - remaining));
    }
    char text[INET6_ADDRSTRLEN];
    if (!inet_ntop(family, bytes, text, sizeof(text))) return NULL;
    NetworkPrefixHandle prefix = malloc(sizeof(*prefix));
    if (!prefix) return NULL;
    snprintf(prefix->cidr, sizeof(prefix->cidr), "%s/%d", text, length);
    return prefix;
}
static inline const char *networkPrefixText(NetworkPrefixHandle prefix) { return prefix->cidr; }
#endif
