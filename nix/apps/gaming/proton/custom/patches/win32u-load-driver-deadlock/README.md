# Bounded desktop driver readiness wait in win32u

## Symptom
Every process that loads the display driver hangs forever inside
`load_desktop_driver()` when explorer's desktop thread is stalled, for
example blocked in its own driver initialization. Nothing is logged.

## Root cause
`load_desktop_driver()` sends `WM_NULL` to the desktop window with a plain
`send_message()` as a readiness barrier: the display device GUID property is
read only after the desktop thread has answered, i.e. after it finished
initializing its driver. The send has no timeout, so a stalled desktop
thread holds every client in the barrier indefinitely.

This is not a two-process cycle: `wait_message_reply()` keeps servicing
incoming sent messages while it waits, so two threads sending to each other
do not deadlock. The problem is purely the unbounded wait on one stalled
thread.

## Fix
Send the barrier with `send_message_timeout(SMTO_ABORTIFHUNG, 5000)` and
`WARN` when it times out. The barrier only orders the property lookup after
driver initialization; when it expires, the existing property lookup still
decides whether a driver is available and `load_display_driver()` falls back
to the null driver exactly as it does when the property is missing.

## Affected upstream
`dlls/win32u/driver.c`.
