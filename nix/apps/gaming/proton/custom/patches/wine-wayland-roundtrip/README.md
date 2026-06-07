# CEF startup deadlock from blocking second Wayland roundtrip

## Symptom
Battle.net's CEF (Chromium Embedded Framework) launcher never produces a
window.  The launcher process is alive but stalled; the CEF child process
exits silently with an IPC timeout.

## Root cause
`wayland_process_init` performs two `wl_display_roundtrip` calls in series.
The second is needed to drain bindings registered against globals returned
by the first.  Both are blocking, which deadlocks CEF: the parent process
parks inside the second roundtrip while the CEF child blocks waiting for
the parent over `IPC::Channel::Send`.

## Fix
Replace the second blocking roundtrip with `wl_display_roundtrip_queue` on
a dedicated event queue, flushed via `poll()`.  Other Wayland event sources
keep draining while we wait for registry bindings to settle.

## Affected upstream
`dlls/winewayland.drv/wayland.c`.
