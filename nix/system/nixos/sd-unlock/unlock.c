#define _GNU_SOURCE
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <libcryptsetup.h>
#include <systemd/sd-bus.h>
#include "handoff.h"

#define PASSWORD_LIMIT 4096
#define ATTEMPT_SECONDS 20
#define DEVICE_LIMIT 32

struct candidate { char name[NAME_MAX + 1]; char cid[64]; dev_t device; };

static double monotonic(void) {
        struct timespec now;
        if (clock_gettime(CLOCK_MONOTONIC, &now) < 0)
                _exit(1);
        return now.tv_sec + now.tv_nsec / 1000000000.0;
}

static void quiet_cryptsetup(int level, const char *message, void *data) {
        (void) level; (void) message; (void) data;
}

static bool read_text(const char *path, char *text, size_t size) {
        int fd = open(path, O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
        if (fd < 0)
                return false;
        ssize_t n = read(fd, text, size - 1);
        close(fd);
        if (n < 0 || (size_t) n == size - 1)
                return false;
        text[n] = 0;
        text[strcspn(text, "\r\n")] = 0;
        return true;
}

/* Return the native SD disk name; partitions must belong to that same disk. */
static bool sd_disk(const char *name, char *disk, size_t size) {
        if (strncmp(name, "mmcblk", 6) != 0)
                return false;
        size_t n = 6;
        if (!isdigit((unsigned char) name[n]))
                return false;
        while (isdigit((unsigned char) name[n])) n++;
        if (n >= size)
                return false;
        memcpy(disk, name, n); disk[n] = 0;
        if (name[n]) {
                if (name[n++] != 'p' || !isdigit((unsigned char) name[n]))
                        return false;
                while (isdigit((unsigned char) name[n])) n++;
                if (name[n]) return false;
        }
        char path[PATH_MAX], type[16];
        snprintf(path, sizeof path, "/sys/class/block/%s/device/type", disk);
        return read_text(path, type, sizeof type) && strcmp(type, "SD") == 0;
}

static bool device_identity(const char *name, dev_t *device) {
        char path[PATH_MAX], numbers[64];
        unsigned major_number, minor_number;
        char extra;
        snprintf(path, sizeof path, "/sys/class/block/%s/dev", name);
        if (!read_text(path, numbers, sizeof numbers) ||
            sscanf(numbers, "%u:%u%c", &major_number, &minor_number, &extra) != 2)
                return false;
        snprintf(path, sizeof path, "/dev/%s", name);
        struct stat st;
        if (lstat(path, &st) < 0 || !S_ISBLK(st.st_mode) ||
            st.st_rdev != makedev(major_number, minor_number))
                return false;
        *device = st.st_rdev;
        return true;
}

static bool kernel_name(dev_t device, char *name, size_t size) {
        char path[PATH_MAX], resolved[PATH_MAX];
        snprintf(path, sizeof path, "/sys/dev/block/%u:%u", major(device), minor(device));
        if (!realpath(path, resolved))
                return false;
        const char *base = strrchr(resolved, '/');
        if (!base || strlen(base + 1) >= size)
                return false;
        strcpy(name, base + 1);
        return true;
}

static bool still_inserted(const struct candidate *candidate) {
        char disk[NAME_MAX + 1], path[PATH_MAX], cid[64];
        dev_t device;
        if (!sd_disk(candidate->name, disk, sizeof disk) ||
            !device_identity(candidate->name, &device) || device != candidate->device)
                return false;
        snprintf(path, sizeof path, "/sys/class/block/%s/device/cid", disk);
        return read_text(path, cid, sizeof cid) && strcmp(cid, candidate->cid) == 0;
}

static void mount_mapping(const char *mapping, const char *user) {
        char path[PATH_MAX], name[NAME_MAX + 1], object[PATH_MAX];
        struct stat st;
        snprintf(path, sizeof path, "/dev/mapper/%.255s", mapping);
        if (stat(path, &st) < 0 || !kernel_name(st.st_rdev, name, sizeof name))
                return;
        strcpy(object, "/org/freedesktop/UDisks2/block_devices/");
        size_t at = strlen(object);
        for (const unsigned char *c = (const unsigned char *) name; *c; c++) {
                if (isalnum(*c)) object[at++] = *c;
                else at += snprintf(object + at, sizeof object - at, "_%02x", *c);
        }
        object[at] = 0;
        sd_bus *bus = NULL;
        if (sd_bus_open_system(&bus) < 0)
                return;
        sd_bus_set_method_call_timeout(bus, 2000000);
        double deadline = monotonic() + 3;
        do {
                sd_bus_error error = SD_BUS_ERROR_NULL;
                sd_bus_message *reply = NULL;
                int r = sd_bus_call_method(bus, "org.freedesktop.UDisks2", object,
                        "org.freedesktop.UDisks2.Filesystem", "Mount", &error, &reply,
                        "a{sv}", 2, "as-user", "s", user,
                        "auth.no_user_interaction", "b", 1);
                sd_bus_error_free(&error);
                sd_bus_message_unref(reply);
                if (r >= 0) break;
                usleep(100000);
        } while (monotonic() < deadline);
        sd_bus_unref(bus);
}

static void attempt_cards(const char *root, const char *user, char *password, size_t length) {
        struct crypt_device *root_device = NULL;
        if (crypt_init_by_name(&root_device, root) < 0)
                return;
        const char *root_path = crypt_get_device_name(root_device);
        struct stat root_stat;
        char root_name[NAME_MAX + 1], root_disk[NAME_MAX + 1] = "";
        bool known_root = root_path && stat(root_path, &root_stat) == 0 &&
                kernel_name(root_stat.st_rdev, root_name, sizeof root_name);
        if (known_root)
                (void) sd_disk(root_name, root_disk, sizeof root_disk);
        crypt_free(root_device);
        if (!known_root)
                return;

        DIR *directory = opendir("/sys/class/block");
        if (!directory)
                return;
        struct candidate candidates[DEVICE_LIMIT];
        size_t count = 0;
        struct dirent *entry;
        while ((entry = readdir(directory)) && count < DEVICE_LIMIT) {
                char disk[NAME_MAX + 1]; dev_t device;
                if (!sd_disk(entry->d_name, disk, sizeof disk) ||
                    strcmp(disk, root_disk) == 0 ||
                    !device_identity(entry->d_name, &device) || device == root_stat.st_rdev)
                        continue;
                char cid_path[PATH_MAX];
                snprintf(cid_path, sizeof cid_path, "/sys/class/block/%s/device/cid", disk);
                if (!read_text(cid_path, candidates[count].cid, sizeof candidates[count].cid))
                        continue;
                snprintf(candidates[count].name, sizeof candidates[count].name, "%s", entry->d_name);
                candidates[count++].device = device;
        }
        closedir(directory);

        char mappings[DEVICE_LIMIT][NAME_MAX + 1];
        size_t opened = 0;
        for (size_t i = 0; i < count; i++) {
                char path[PATH_MAX];
                if (!still_inserted(&candidates[i]))
                        continue;
                snprintf(path, sizeof path, "/dev/%.255s", candidates[i].name);
                struct crypt_device *cd = NULL;
                if (crypt_init(&cd, path) < 0)
                        continue;
                if (crypt_load(cd, CRYPT_LUKS, NULL) == 0) {
                        const char *uuid = crypt_get_uuid(cd);
                        if (uuid && strlen(uuid) == 36 && strspn(uuid, "0123456789abcdefABCDEF-") == 36) {
                                char mapping[NAME_MAX + 1];
                                snprintf(mapping, sizeof mapping, "luks-%s", uuid);
                                if (crypt_status(cd, mapping) == CRYPT_INACTIVE &&
                                    still_inserted(&candidates[i]) &&
                                    crypt_activate_by_passphrase(cd, mapping, CRYPT_ANY_SLOT,
                                                                 password, length, 0) >= 0)
                                        strcpy(mappings[opened++], mapping);
                        }
                }
                crypt_free(cd);
        }
        explicit_bzero(password, PASSWORD_LIMIT);
        for (size_t i = 0; i < opened; i++)
                mount_mapping(mappings[i], user);
}

int main(int argc, char **argv) {
        if (argc != 3 || getuid() != 0 || geteuid() != 0)
                return 1;
        struct rlimit no_core = {0, 0};
        if (setrlimit(RLIMIT_CORE, &no_core) < 0 || prctl(PR_SET_DUMPABLE, 0) < 0)
                return 0;
        long key = syscall(SYS_keyctl, KEYCTL_SEARCH, KEY_SPEC_USER_KEYRING,
                           "user", SD_PASSPHRASE_KEY, 0);
        if (key < 0)
                return 0;
        char description[256];
        long n = syscall(SYS_keyctl, KEYCTL_DESCRIBE, key, description, sizeof description);
        unsigned uid, gid, permissions;
        if (n <= 0 || n > (long) sizeof description ||
            sscanf(description, "user;%u;%u;%x;", &uid, &gid, &permissions) != 3 ||
            uid != 0 || permissions != SD_PASSPHRASE_PERMISSIONS) {
                (void) syscall(SYS_keyctl, KEYCTL_REVOKE, key);
                return 0;
        }
        char *password = mmap(NULL, PASSWORD_LIMIT, PROT_READ | PROT_WRITE,
                              MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (password == MAP_FAILED) {
                (void) syscall(SYS_keyctl, KEYCTL_REVOKE, key);
                return 0;
        }
        if (mlock(password, PASSWORD_LIMIT) < 0) {
                munmap(password, PASSWORD_LIMIT);
                (void) syscall(SYS_keyctl, KEYCTL_REVOKE, key);
                return 0;
        }
        n = syscall(SYS_keyctl, KEYCTL_READ, key, password, PASSWORD_LIMIT);
        long revoked = syscall(SYS_keyctl, KEYCTL_REVOKE, key);
        if (revoked < 0 || n <= 0 || n > PASSWORD_LIMIT) {
                explicit_bzero(password, PASSWORD_LIMIT);
                munmap(password, PASSWORD_LIMIT);
                return 0;
        }
        crypt_set_log_callback(NULL, quiet_cryptsetup, NULL);
        pid_t parent = getpid();
        pid_t child = fork();
        if (child == 0) {
                if (prctl(PR_SET_PDEATHSIG, SIGKILL) < 0 || getppid() != parent ||
                    mlock(password, PASSWORD_LIMIT) < 0)
                        _exit(0);
                attempt_cards(argv[1], argv[2], password, n);
                explicit_bzero(password, PASSWORD_LIMIT);
                _exit(0);
        }
        explicit_bzero(password, PASSWORD_LIMIT);
        munmap(password, PASSWORD_LIMIT);
        if (child < 0)
                return 0;
        double deadline = monotonic() + ATTEMPT_SECONDS;
        int status;
        while (monotonic() < deadline) {
                pid_t exited = waitpid(child, &status, WNOHANG);
                if (exited == child || (exited < 0 && errno != EINTR))
                        return 0;
                usleep(100000);
        }
        kill(child, SIGKILL);
        /* A failing device may leave a task in uninterruptible kernel I/O.
         * Never wait for that task on the boot path. */
        return 0;
}
