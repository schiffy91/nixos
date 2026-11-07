# Inactive compositor restart control

Run the `sleep-lock-restart-control` graph node. This diagnostic uses the same
KWin/KScreenLocker packages as the sleep candidate, but never invokes sleep
prepare, its root PAM, Plymouth or its VT lock. It starts the capture virtual
output, switches to VT63, crashes KWin, restores the original VT, and requires
the physical output to return. It checks that no root sleep-lock state exists.

Run `7cda389d6bcf` on 2026-11-07 failed after 49.89 seconds: after VT restoration,
logind reported `Active=yes` and the graphics card ACL allowed the session user
read/write access, but KWin's output list was empty. The VM was cleaned up.
This shows that the missing-output failure does not require the root sleep-lock
transaction. It does not prove an upstream-only bug: the candidate's native
locker and restart patches remain present in this control. Compare the same
transition with unpatched KWin and capture the DRM/logind descriptor timeline
before changing production behavior. This is not a passing security test.

The node is diagnostic and is not part of the default/required gate sets.
The full capture regression must still pass before enabling the sleep lock.
