# BTRC Native C11 Binding Reference

Use this reference whenever wrapping a C library or OS facility in `btrc.toml` + `.h`.

## 1. Boundary division of labor

| Belongs in `native.h` + `btrc.toml` | Belongs in `Module.btrc` |
|---|---|
| `#include <...>` headers, C structs, unions, bitmask flags, `errno` checks | Domain types, state transitions, validation, orchestration |
| `malloc`/`calloc`/`free`, `g_free`, `crypt_free`, `drmModeFree*` | Zero raw pointers; holds opaque handles as ARC-managed locals/fields |
| Out-parameters (`&count`, `&err`) collapsed into handle state or scalar status | Nullable `T?` checks, f-string error formatting, `throw` on failure |
| C callbacks and `void *userdata` glue | High-level BTRC methods and `I*` interfaces |

Keep `.h` headers small, `static inline` (or paired with a registered `.c` file when large), C11-clean under `-Wall -Wextra -Werror`, and free of process-global side effects (`signal(SIGIO, ...)`, `umask(...)`, `chdir(...)`).

## 2. Pattern A: Unique resource handle + copied string results

Modelled on `config/system/nixos/networking/` and `config/pkgs/utils/chrome-launchers/`.

### `btrc.toml`

```toml
manifest-version = 1

[package]
name = "nixos_chrome_launchers"
exports = ["DesktopEntry", "Launchers"]

[[native.pkg-config]]
name = "glib-2.0"
modules = ["DesktopEntry"]

[[native.bindings]]
module = "DesktopEntry"
header = "native.h"
language = "c"
standard = "c11"
symbols = [
  "DesktopEntryHandle",
  "desktopEntryOpen",
  "desktopEntryRelease",
  "desktopEntryValue",
  "desktopEntryHidden",
  "desktopEntryPublish",
]
owned-results = ["desktopEntryOpen"]
read-only-borrows = [
  "desktopEntryOpen.path",
  "desktopEntryValue.key",
  "desktopEntryPublish.path",
  "desktopEntryPublish.text",
]
borrowed-parameters = [
  "desktopEntryValue.entry",
  "desktopEntryHidden.entry",
]

[native.bindings.resources.DesktopEntryHandle]
ownership = "unique"
release = "desktopEntryRelease"

[native.bindings.copied-results.desktopEntryValue]
kind = "string"
owner = "entry"
```

### `native.h`

```c
#ifndef NIXOS_DESKTOP_ENTRY_H
#define NIXOS_DESKTOP_ENTRY_H
#include <glib.h>
#include <stdbool.h>
#include <stdlib.h>

typedef struct DesktopEntryStorage {
    GKeyFile *file;
    char *scratch;
} *DesktopEntryHandle;

static inline void desktopEntryRelease(DesktopEntryHandle entry) {
    if (!entry) return;
    if (entry->file) g_key_file_unref(entry->file);
    g_free(entry->scratch);
    free(entry);
}

static inline DesktopEntryHandle desktopEntryOpen(const char *path) {
    DesktopEntryHandle entry = calloc(1, sizeof(*entry));
    if (!entry) return NULL;
    entry->file = g_key_file_new();
    if (!g_key_file_load_from_file(entry->file, path, G_KEY_FILE_KEEP_COMMENTS, NULL)) {
        desktopEntryRelease(entry);
        return NULL;
    }
    return entry;
}

static inline const char *desktopEntryValue(DesktopEntryHandle entry, const char *key) {
    g_free(entry->scratch);
    entry->scratch = g_key_file_get_string(entry->file, "Desktop Entry", key, NULL);
    return entry->scratch;
}

static inline bool desktopEntryHidden(DesktopEntryHandle entry) {
    return g_key_file_get_boolean(entry->file, "Desktop Entry", "Hidden", NULL);
}

static inline bool desktopEntryPublish(const char *path, const char *text) {
    return g_file_set_contents_full(path, text, -1, G_FILE_SET_CONTENTS_CONSISTENT, 0644, NULL);
}
#endif
```

### `DesktopEntry.btrc`

```btrc
class DesktopEntry {
    DesktopEntryHandle handle;

    DesktopEntry(DesktopEntryHandle handle) {
        self.handle = handle;
    }

    class DesktopEntry? open(string path) {
        var handle = desktopEntryOpen(path);
        if (handle == null) {
            return null;
        }
        return DesktopEntry(handle);
    }

    string? value(string key) {
        return desktopEntryValue(self.handle, key);
    }

    bool hidden() {
        return desktopEntryHidden(self.handle);
    }
}
```

Why this works:
- `DesktopEntryHandle` is an opaque unique resource; BTRC releases it automatically when the last owner goes out of scope (or early via `.close()`).
- `desktopEntryValue` stores the temporary buffer on `entry->scratch`, and `copied-results` (`kind = "string"`, `owner = "entry"`) copies it into a managed BTRC `string?` while holding a borrow lease on `entry`.
- `DesktopEntry.btrc` has zero `#include`s, zero raw pointers, and zero manual `free` calls.

## 3. Pattern B: Copying binary buffers into `Bytes`

Modelled on the testkit package in btrc, `nix/lib/testkit/btrc.toml` (`Framebuffer`). When a C handle owns a byte buffer of known length, return `Bytes?` directly via `copied-results` (`kind = "bytes"`):

```toml
[[native.libraries]]
name = "vncclient"
modules = ["Framebuffer"]

[[native.bindings]]
module = "Framebuffer"
header = "vnc.h"
language = "c"
standard = "c11"
symbols = ["TestVNC", "testVNCOpen", "testVNCClose", "testVNCLength", "testVNCPixels"]
owned-results = ["testVNCOpen"]
read-only-borrows = ["testVNCOpen.path"]
borrowed-parameters = ["testVNCLength.session", "testVNCPixels.session"]

[native.bindings.resources.TestVNC]
ownership = "unique"
release = "testVNCClose"

[native.bindings.copied-results.testVNCPixels]
kind = "bytes"
owner = "session"
length-function = "testVNCLength"
length-arguments = ["session"]
length-preserves-result = true
```

Note: `length-function` must return a signed `int` byte count (`static inline int testVNCLength(...)`), never `size_t`.

## 4. Nix and CI registration

1. Build the binary through `inputs.btrc.lib.buildPackage pkgs` with `linkPlan = true` so `btrc-native-plan` reads `[[native.pkg-config]]` and `[[native.libraries]]` directly from `btrc.toml`.
2. Add the `.h` (and any `.c`) file to `ci/native.nix` with its `buildInputs` so CI compiles it standalone with `-Wall -Wextra -Werror`.

## 5. Anti-patterns to reject in review

- Including `"native.h"` directly inside `Module.btrc` (`#include "native.h"` bypasses `btrc.toml` ownership and leaks raw C into the generated translation unit).
- Listing raw C struct types (`drmModeConnector`, `crypt_device`, `struct pollfd`, `CURL`, `X509`) in `symbols = [...]` without wrapping them as opaque `resources` in `.h`.
- Creating `Native.btrc` or `Posix.btrc` pass-through modules so multiple `.btrc` files can call raw POSIX functions.
- Passing `bytes.data` as `unsigned char*` across module boundaries instead of keeping binary framing in `Bytes`.
