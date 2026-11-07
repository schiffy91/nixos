# Sleep-lock console experiment

Run through the repository graph:

```sh
make build
nix-shell tests/shell.nix --run 'NIXOS_CONFIG_ROOT=$PWD ./build/nixosctl graph tests/graph.json run sleep-lock-console'
```

This experiment verifies the Linux VT switch lock with the repository's actual
touch-enabled Plymouth package and a running Plasma/Wayland session. It checks
physical console shortcuts, activation of the actual logind seat session,
unprivileged and capability-restricted unlock attempts, touch and keyboard input,
Escape, UI termination/restart, and return to the original session.
The acquiring process exits before the tests: the kernel retains the lock.

This is **not a screen locker**. The root-only fixture can explicitly unlock
without authentication and is installed only in this test VM. The fake password
answer is written to a test file for assertions; a real implementation must use
private process pipes, clear secrets, and disable core dumps.

A deployment still requires account-password PAM authentication, checked lock
acknowledgement before sleep, protection against authenticating before a queued
sleep has finished, recovery from interrupted setup, and failed/aborted sleep
tests. Gamescope integration, OLED touch rotation/scaling, multiple
outputs, hardware suspend and hibernation must be verified separately. A console
switch lock does not revoke existing root access, remote sessions, or streaming.
