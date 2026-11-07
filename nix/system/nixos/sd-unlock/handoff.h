/* Private, expiring initrd-to-userspace handoff. Never a password file. */
#pragma once
#include <linux/keyctl.h>
#include <sys/syscall.h>
#include <unistd.h>

#define SD_PASSPHRASE_KEY "nixos:sd-unlock"
#define SD_PASSPHRASE_TTL 45
#define SD_PASSPHRASE_PERMISSIONS 0x003f0000u /* owner only; no possessor rights */

static inline void sd_export_passphrase(const char *password, size_t length) {
        long serial = syscall(SYS_add_key, "user", SD_PASSPHRASE_KEY,
                              password, length, KEY_SPEC_USER_KEYRING);
        if (serial < 0)
                return;
        if (syscall(SYS_keyctl, KEYCTL_SETPERM, serial, SD_PASSPHRASE_PERMISSIONS) < 0 ||
            syscall(SYS_keyctl, KEYCTL_SET_TIMEOUT, serial, SD_PASSPHRASE_TTL) < 0)
                (void) syscall(SYS_keyctl, KEYCTL_REVOKE, serial);
}
