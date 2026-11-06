# libc thread pointer for PsCreateSystemThread threads

Inactive since GE-Proton11-12: GE's `em-fixups/0073` skips the `%fs` restore
for Unix-only system threads, keeping their existing libc thread pointer.
This local patch is retained for reference and is no longer applied.

## Symptom
Every 32-bit CEF renderer of the Battle.net launcher faults while exiting.
Each one leaves a core dump and a DrKonqi crash notification, so a single
launcher shutdown produces a pile of crash reports even though the launcher
itself closed normally. The same fault can hit a suspend (`SIGUSR1`), not
only process exit (`SIGQUIT`).

## Root cause
`PsCreateSystemThread` threads start in `start_system_thread()` and only ever
run Unix code, so they never reach `init_syscall_frame()`. That is the one
place a normal thread records its libc thread pointer in
`amd64_thread_data.pthread_teb`.

In a WoW64 process `init_handler()` reloads `%fs` from that field on every
signal, because PE code there runs with the 32-bit selector. On a system
thread the field is still zero, so the handler restores `%fs = 0` and the
first `pthread_getspecific()` call faults inside the signal handler.

`start_system_thread()` comes from GE's `em-fixups/0030`
(`ntdll: export PsCreateSystemThread`), not from upstream Wine.

## Fix
Add a per-architecture `signal_init_system_thread()` hook and call it from
`start_system_thread()` after `pthread_setspecific(teb_key, teb)`. On x86_64
it records the thread pointer the same way `init_syscall_frame()` does —
`arch_prctl(ARCH_GET_FS, ...)` when `fs32_sel` is set on Linux, or
`mac_thread_gsbase()` on macOS. The arm, arm64 and i386 implementations are
empty: none of them reloads a selector from `pthread_teb` in its signal
handler.

A hook rather than an inline `#ifdef` keeps the WoW64-specific knowledge in
the per-architecture signal files, which is where the matching write in
`init_syscall_frame()` already lives.

## Affected upstream
`dlls/ntdll/unix/thread.c`, `dlls/ntdll/unix/unix_private.h`, and
`dlls/ntdll/unix/signal_{i386,x86_64,arm,arm64}.c`.
