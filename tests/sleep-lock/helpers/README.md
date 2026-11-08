# Sleep helper contracts

`nix-build tests/sleep-lock/helpers --no-out-link` compiles the real BTRC PAM and
DRM helpers before VM tests. A real child process supplies bounded password
responses: success, empty, embedded NUL, failure, missing executable, the maximum
accepted length and overflow. PAM conversation tests exercise message limits,
non-secret informational messages, rejected echo prompts and cleanup after a
partially completed conversation. Link-time interception observes every raw
buffer wipe, including glibc's fortified entrypoint; production binaries retain
real `explicit_bzero` calls.

CRTC and plane readback predicates reject missing or still-bound state. Generated
authentication code must contain no managed-string, ARC or threaded runtime; its
locked address space, raw pipe/stack buffers, PAM response allocation and
parent-death behavior remain independent of the managed controller. Actual VT,
PAM and suspend ordering are exercised by the existing sleep VM scenarios.
