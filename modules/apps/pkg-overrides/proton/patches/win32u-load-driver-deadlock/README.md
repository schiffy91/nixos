# win32u desktop-driver readiness wait

## Symptom
Some Battle.net startup paths enter `load_desktop_driver()` while the
desktop window owner is also waiting on driver initialization. With an
unbounded `WM_NULL` send, the caller can deadlock before `winevulkan`
finishes loading, which then trips a libcef Vulkan startup assert.

## Fix
Skip the readiness send when the caller already owns the desktop window
thread, and use `send_message_timeout()` for cross-thread waits. The
message is only a readiness barrier; if it times out, the existing
desktop-driver property lookup still decides whether the driver is
available.

## Test Focus
* `winevulkan` loads without `vkCreateInstance Unix call failed`.
* Battle.net reaches the launcher on Wayland with CEF's GPU process
  enabled.
