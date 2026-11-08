# Recovery artifact installation

`nix-build tests/recovery/artifacts --no-out-link` exercises the production BTRC
installer against temporary directories and a real systemd EFI image. It checks
mount and key availability, Standard-Boot guard ordering, idempotence, checksum
repair, stage cleanup, preservation of existing files after failed updates,
signature verification, mismatched keys and certificate rotation.

Nix declares the source/destination artifact list and obsolete files. BTRC locks
installation, stages and verifies every payload, then replaces each destination
by a rename on its own filesystem. This preserves the prior per-file replacement
behavior; several files across two filesystems cannot be one atomic transaction.
The manifest format is now the declarative JSON plan plus the certificate hash,
so the first run after migration refreshes the existing artifacts once.

The existing `recovery-boot-*` and `recovery-secure-boot-*` graph scenarios cover
installation, recovery firmware entry and booting the installed payload.

SIGINT and SIGTERM during a real staging child set an atomic cancellation flag.
The next process boundary throws through normal cleanup; both cases must remove
temporary artifacts and preserve the previously installed files and metadata.
