/* Private, expiring initrd-to-userspace handoff. Never a password file. */
#pragma once
#include <linux/keyctl.h>
#include <sys/syscall.h>
#include <unistd.h>

#define UNLOCK_PASSPHRASE_KEY "nixos:opportunistic-unlock"
#define UNLOCK_PASSPHRASE_TTL 45
#define UNLOCK_PASSPHRASE_PERMISSIONS 0x003f0000u /* owner only; no possessor rights */

static inline void unlock_export_passphrase(const char *password, size_t length) {
        long serial = syscall(SYS_add_key, "user", UNLOCK_PASSPHRASE_KEY,
                              password, length, KEY_SPEC_USER_KEYRING);
        if (serial < 0)
                return;
        if (syscall(SYS_keyctl, KEYCTL_SETPERM, serial, UNLOCK_PASSPHRASE_PERMISSIONS) < 0 ||
            syscall(SYS_keyctl, KEYCTL_SET_TIMEOUT, serial, UNLOCK_PASSPHRASE_TTL) < 0)
                (void) syscall(SYS_keyctl, KEYCTL_REVOKE, serial);
}
