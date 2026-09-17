# Bounded WM_CANCELMODE on Wayland keyboard leave

## Symptom
A Wine process stops reacting to all Wayland input and window events while
one of its windows is busy, for example while a launcher tears down a large
window tree during a page transition. The process is not deadlocked, but
nothing on the Wayland side moves until that window's thread returns to its
message loop.

## Root cause
GE-Proton's wayland hotfix series (wine-wayland 0088, "Implement minimize &
restore of windows") makes `keyboard_handle_leave()` send `WM_CANCELMODE` to
the foreground window with a synchronous `send_message()`. That handler runs
on the driver's Wayland event reader thread, so the thread blocks until the
target window's thread replies, and no other Wayland event is dispatched for
the whole process in the meantime.

## Fix
Send `WM_CANCELMODE` with `SendNotifyMessage` semantics
(`NtUserMessageCall(..., NtUserSendNotifyMessage, ...)`). The reader thread
never owns the target window, so the message is queued and the call returns
immediately; the target still receives `WM_CANCELMODE` when it next
processes sent messages.

A `SendMessageTimeout` was rejected on purpose: win32u cancels a timed-out
send, so a short timeout would drop `WM_CANCELMODE` for exactly the busy
targets that need it.

## Affected upstream
`dlls/winewayland.drv/wayland_keyboard.c` (GE hotfix code, not upstream
Wine).
