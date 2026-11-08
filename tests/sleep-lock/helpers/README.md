# Sleep helper contracts

`nix-build tests/sleep-lock/helpers --no-out-link` compiles the real BTRC PAM and
DRM helpers before VM tests. A real child process supplies bounded password
responses: success, empty, embedded NUL, failure, missing executable, the maximum
accepted length and overflow. PAM conversation tests exercise message limits,
non-secret informational messages, rejected echo prompts and cleanup after a
partially completed conversation. Link-time interception observes every raw
buffer wipe, including glibc's fortified entrypoint; production binaries retain
real `explicit_bzero` calls.

CRTC and plane readback predicates reject missing or still-bound state. The
password response owns a fixed `OwnedBuffer<char>` and wipes it before closing;
repeated explicit close and scope cleanup are safe. Managed objects hold only
metadata. The helper process locks memory before PAM or password capture, and
the Plymouth child remains tied to its parent. Actual VT, PAM and suspend
ordering are exercised by the existing sleep VM scenarios.
