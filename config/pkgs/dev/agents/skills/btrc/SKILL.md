---
name: btrc
description: Write, review, and refactor idiomatic BTRC code, btrc.toml package manifests, native C11 header bindings, and BTRC standard library modules. Use before touching any .btrc file, btrc.toml manifest, native .c/.h shim, config/lib/btrc.nix build definition, or upstream /home/alexanderschiffhauer/Projects/btrc compiler and stdlib code.
metadata:
  short-description: Idiomatic BTRC standards, stdlib-first escalation, and native binding contracts
---

# BTRC

BTRC is an owned language and standard library (`/home/alexanderschiffhauer/Projects/btrc`), used across `/etc/nixos` for system daemons, desktop tools, installers, gaming utilities, and VM test harnesses. Domain and test `.btrc` files must read as clean, high-signal BTRC, with all C pointers, structs, and library lifecycles isolated behind `btrc.toml` declarative bindings or upstream `Library.*` modules.

## 1. Pick the right layer first (stdlib before local shims)

When a `.btrc` call site feels awkward or tempts you to drop into C or `/bin/sh`, check these layers in order:

1. Use existing `Library.*` modules first. Check `Library.FileSystem`, `Library.Process`, `Library.Platform`, `Library.CLI`, `Library.Testing`, `Library.Digest` (`SHA256`), `Library.Crypto` (`Base64`, `Sodium`, `SecureChannel`), `Library.Bytes` (`toHex`, `fromHex`, `zeros`, endian helpers), `Library.Network` (`TCPStream`, `PollSet`), `Library.JSON`, `Library.TOML`, `Library.Timer`, `Library.Terminal` (`PTYProcess`), and `Vector.sortBy` before writing helpers or spawning `sha256sum`, `base64`, `uname`, `stat`, `mktemp`, or `sleep`.
2. Promote general primitives to `/home/alexanderschiffhauer/Projects/btrc/src/stdlib`. General-purpose OS, filesystem, process, CLI, byte-encoding, socket, polling, and test-assertion APIs belong in `src/stdlib`, followed by a flake input update in `/etc/nixos`. Do not copy `PipeProcess`, `BackgroundProcess`, atomic-write helpers, or test assertion libraries across packages.
3. Use a package-local `btrc.toml` + `.h` binding only when wrapping a domain-specific third-party C library (`libcryptsetup`, `libdrm`, `pam`, `glib`, `libcrypto`, `libvncclient`) or a subsystem-specific kernel `ioctl` that does not belong in `src/stdlib`.
4. Use a standalone `.c` executable only when a helper must run in a child process without the BTRC runtime (for example an external ABI test oracle or a standalone Wayland/PipeWire probe).

## 2. Zero C inside domain and test `.btrc` files

Application, library, and test `.btrc` files must contain zero raw C:

- No C preprocessor `#include` directives inside `.btrc` files.
- No `extern` function declarations, `typedef`, C `struct`, or `union` declarations in `.btrc`.
- No raw pointers (`T*`, `char*`, `unsigned char*`, `void*`), pointer dereferences (`->`, `*ptr`), or address-of out-parameters (`&status`). Use `main(Vector<string> args)` or `main()` instead of `int main(int argc, char** argv)`. Express optionality with nullable BTRC types (`T?`). Do not reach into collection backing pointers (`bytes.data`, `vec.data`).
- No C heap or string functions (`malloc`, `calloc`, `realloc`, `free`, `strdup`, `memcpy`, `memset`, `memcmp`, `strcmp`, `strncmp`, `strlen`, `strtol`, `snprintf`).
- No raw POSIX or libc calls (`open`, `openat`, `close`, `read`, `write`, `fork`, `pipe`, `poll`, `waitpid`, `execvp`, `unlink`, `rename`, `stat`, `lstat`, `chmod`, `umask`, `getuid`, `geteuid`, `setenv`, `fflush`, `signal`, `sigprocmask`, `alarm`, `usleep`).
- No `#include <assert.h>` or C `assert(...)` in tests.

Inside `/home/alexanderschiffhauer/Projects/btrc/src/stdlib`, prefer `[[native.bindings]]` in functional groups (`GPU`, `Digest`, `GUI` style). Keep raw pointers `private` inside the lowest-level runtime/provider leaf when unavoidable, and never leak raw pointers, file descriptors, or C `snake_case` struct fields in exported stdlib APIs.

## 3. Package manifests and native boundaries

Read `references/native-bindings.md` whenever creating or editing `btrc.toml` or `.h` files.

- Every BTRC package directory owns one `btrc.toml` with `manifest-version = 1`, `[package] name = "..."`, and an explicit `exports = [...]` list naming only its public modules.
- Cross-package imports declare `[dependencies]` in `btrc.toml` (`foo = { path = "../foo" }`). Never climb out of a package root with `../../../` relative imports.
- Build Nix packages through `config/lib/btrc.nix` with `linkPlan = true` so `[[native.pkg-config]]` and `[[native.libraries]]` in `btrc.toml` drive compiler and linker flags from one source of truth. Register every `.c` and `.h` file in `ci/native.nix` so `-Wall -Wextra -Werror` covers it.
- Bind one focused C11 `.h` header to one BTRC wrapper module (`module = "..."`, `header = "..."`, `language = "c"`, `standard = "c11"`).
- Keep C ceremony in `.h` and domain policy in `.btrc`. The `.h` file translates C structs, bitmasks, `errno`, callbacks, and out-parameters into opaque handle typedefs, scalar inputs/outputs, and owned/copied results declared in `btrc.toml` (`[native.bindings.resources.<Handle>]`, `owned-results`, `borrowed-parameters`, `read-only-borrows`, `[native.bindings.copied-results.<fn>]`).
- Never write symbol-dump manifests that export raw C structs or 30+ unbound C functions into `.btrc`, and never create 1-line stub modules (`Native.btrc`, `Posix.btrc`) whose sole job is re-exporting C headers.

## 4. Naming and module design

- Files: `PascalCase.btrc`, matching the primary exported type (`DesktopEntry.btrc`, `TrustedFirewall.btrc`). Do not repeat the enclosing directory name on every file (`Planner.btrc` and `Slots.btrc` inside `immutability/`, never `ImmutabilityPlanner.btrc`).
- Types: `PascalCase` classes and enums (`NetworkPrefix`, `SleepPhase`), `I`-prefixed interfaces (`IControllerLink`, `ISecretStore`).
- Members and variables: `camelCase` methods, fields, parameters, and locals (`prefixLength`, `normalize`, `relativePath`). Never define `snake_case` BTRC identifiers.
- Enum values: `UPPER_SNAKE_CASE` (`SLEEP_LOCKED`, `MOUNT_READY`) for plain enums; `PascalCase` variants for `enum class` algebraic sums. Use enums instead of classes full of static string getter methods.
- Domain nouns over `*Manager`: Name classes for the state, resource, or concept they represent (`EncryptedSdCard`, `ScreenLayout`, `SnapshotStore`, `DesktopEntry`). Avoid generic `*Manager` bags and 20-line stateless wrapper classes whose single method belongs on an existing type or module.
- Imports: Import specific `Library.*` or package modules (`import Library.FileSystem { FileSystem, Path };`). Never use legacy `std.*` imports or wildcard imports, and delete unused imports whenever refactoring.

## 5. Idiomatic expressions, types, and collections

- Entrypoints: Write `void main(Vector<string> args)`, `int main(Vector<string> args)`, or `int main()`. Never declare `int main(int argc, char** argv)`.
- Universal `.length`: Read size via the `.length` property across strings, `Bytes`, `Vector`, `Array`, `List`, `Map`, `Set`, `Span`, `OwnedBuffer`, and `CLIArgs` (`items.length`, `text.length`), and emptiness via `.isEmpty()`.
- String equality: Compare strings with `==` and `!=` (`mode == "active"`, `name != ""`), which BTRC lowers to null-safe `strcmp`. Do not write Java-style `mode.equals("active")`.
- `var`: Use `var` when the initializer makes the type clear (`var slash = cidr.indexOf("/");`, `var headers = Map<string, string>();`). Write an explicit type when initializing a nullable local to `null` (`string? token = null;`) or choosing a specific integer/float width (`uint32_t`, `long`).
- String formatting: Use f-strings (`f"mount {device} at {target}"`) instead of `+` concatenation chains. Inside loops, build text with `StringBuilder` or `Vector<string>` + `.join("\n")`.
- Collection literals: Initialize vectors and maps with `[]` and `{}` (`Vector<string> names = [];` or `var names = Vector<string>();`). Never write `new Vector<T>()` or `new Map<K, V>()`.
- Iteration and algorithms: Prefer `for item in items` on sequences and `for key, value in map` on maps. Use `Vector.sortBy` / `sortedBy`, `Bytes` endian helpers, `PathTools`, and `Strings` methods instead of hand-written loops.
- Octal file modes: Write octal literals (`0o755`, `0o644`, `0o600`, `0o700`), never opaque decimal numbers like `493`, `420`, `384`, or `63`.

## 6. Error handling and scoped cleanup

- One error model (`T?` + `enum class` + `throw`):
  - Return `T?` (using `?.` and `??`) when absence needs no explanation (`find`, `lookup`, `normalize`).
  - Return an `enum class` when callers branch on expected domain outcomes (`Ready(string path)`, `Locked`, `Missing`).
  - Use `throw` for operational or unexpected failures, catching only at recovery boundaries or at `main`. Do not use `Result<T, E>`, and never call `exit(1)` or `abort()` inside library code because `exit(1)` bypasses destructors and `finally` blocks.
- Prefer RAII scopes: Bind cleanup to scope-owned objects (`__del__` or `unique` native resource handles like `AdvisoryFileLock`, `RootSecretFile`, `TestDirectory`) whenever a resource has a lexical lifetime, reserving `try`/`finally` for ad-hoc state restoration.
- Quiet libraries: Library modules must not write directly to `stderr` or `stdout` unless terminal output is their explicit responsibility. Include the operation, target path or command, and failure reason in thrown messages.

## 7. Processes, CLI, structured data, and secrets

- Unified `Process` execution:
  - Use `Process.run(executable, args, ...)` or `ChildProcess.run` (pipe-backed `stdin` / `stdinBytes`, `Bytes`-capable output, explicit `timeoutMs`, `check=true` when non-zero exit should throw) for direct commands.
  - Use `Process.shell` / `UnixShell` only when shell pipelines or redirections express the operation more clearly, quoting every dynamic value with `ShellWords.quote(...)`.
  - Use `Process.spawn` for managed background or streaming processes and `Process.exec` for `execvp` handoff.
- Structured tool output: Pass `--json` (`ip -json`, `findmnt --json`, `kscreen-doctor --json`, `lsblk --json`, `qemu-img info --output=json`) and parse with `Library.JSON`. Do not scrape human-formatted CLI output with `sed`, `awk`, `cut`, or fragile substring offsets when structured output exists.
- Declarative CLI specs: Define subcommands, boolean flags, value options, help text, and shell completions in one schema (`Library.CLI` `CLISpec`), rejecting unknown flags with a clear usage error.
- Secret hygiene: Never pass passwords, LUKS keys, tokens, or signing keys in `argv` (`/proc/<pid>/cmdline`), child environment variables, or `/tmp` files. Deliver secrets via pipe-backed `stdin`/`stdinBytes`, an inherited file descriptor, or a `0o600` file under `/run` (`RootSecretFile`).
- Root state paths: Services and root tools must store caches, sockets, locks, and staging files under `/run/<name>` (with explicit directory modes), never world-writable `/tmp`.

## 8. Testing

- Assertions and temp state: Use `Library.Testing` (`Test`, `Expect`, and RAII `TestDir` / `TestDirectory`) for unit and integration tests. Never `#include <assert.h>` and never hardcode fixed `/tmp/...` paths that collide across parallel runs.
- Test the real boundary: Unit-test pure parsers, planners, and state machines directly. When changing boot, disk, encryption, sleep/lock, recovery, GUI, or LSP behavior, also run the simulator, VM lab, or protocol test that exercises the user-facing workflow.

## 9. Pre-completion checklist

Before finishing any BTRC task:

1. Search neighboring modules with `rg` so you fix the whole pattern across the tree.
2. Confirm zero `#include`, `extern`, raw pointers, `malloc`/`free`, raw POSIX calls, or `<assert.h>` remain in touched `.btrc` files.
3. Confirm `btrc.toml` sets explicit `exports = [...]`, `btrc.nix` builds cleanly with `linkPlan = true`, and any `.c`/`.h` file is registered in `ci/native.nix`.
4. Confirm no secret crosses `argv`, env vars, or `/tmp`, and file modes use `0o` octal literals.
5. Run `repo-tools lint` and the relevant unit/VM test suites.
