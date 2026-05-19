# win32u: cross-process deadlock in load_desktop_driver

## Symptom
Battle.net.exe (and any wine process started while explorer.exe is also
loading its driver) hangs at startup with a single thread blocked in
`futex_wait_multiple` for an unbounded duration.  No CPU usage; no
forward progress.

## Root cause
`load_desktop_driver()` sends `WM_NULL` to the desktop window with the
comment "wait for graphics driver to be ready", using `send_message()` -
which is `send_message_timeout(..., timeout=0)`, an unbounded wait.

Two failure modes:

1. **Self-deadlock**: when the caller IS the desktop owner thread
   (wine-wayland explorer.exe `/desktop` thread is itself running
   `load_display_driver`), wine routes the `SendMessage` as an
   inter-thread message and parks the thread waiting for a reply that
   would have to come from itself.

2. **Cross-process deadlock**: when two wine processes both reach
   `load_desktop_driver` near-simultaneously, each thread sends
   `WM_NULL` to a desktop hwnd owned by the OTHER thread, and both
   park forever in
   `send_inter_thread_message → wait_message_reply →
    NtWaitForMultipleObjects`.

gdb backtrace from a stuck process:

```
send_message → send_message_timeout → process_message
              → send_inter_thread_message
              → wait_message_reply → NtWaitForMultipleObjects
```

## Fix
* Skip the send entirely when the caller already owns the desktop
  window's thread (eliminates self-deadlock).
* For the cross-process case, bound the wait with `SMTO_ABORTIFHUNG`
  and a 5-second timeout so a peer in the same state cannot wedge us.

The send is purely a sync barrier - timing out and proceeding with
`NtUserGetProp` returns at worst a stale guid, which matches behaviour
before this barrier was added.

## Affected upstream
`dlls/win32u/driver.c`.
