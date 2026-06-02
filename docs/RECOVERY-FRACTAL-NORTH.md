# FRACTAL-NORTH Recovery Runbook

This is the operator runbook for bringing FRACTAL-NORTH out of the P2.1
lockout state and onto a generation with the structural Phase 3 fix in
source. All steps assume you are SSH'd in as `alexanderschiffhauer` and
`/etc/nixos` is on branch `fix/p2.1-structural` with HEAD at `6df087e`
or later.

The plan: build a new generation with **immutability disabled** (recovery
commit `59c550e`), migrate persist content back into the underlying
@root / @home subvolumes, prune lockout-bearing generations from
lanzaboote, reboot. Re-enabling immutability with the Phase 3 fix is a
separate, later step (see end of doc).

---

## Pre-flight

```sh
cd /etc/nixos
git status                  # should be clean
git log --oneline -10       # should show Phases 0..5 + 59c550e
git rev-parse HEAD          # note this hash; you'll verify it post-reboot
findmnt /etc/nixos          # confirm /etc/nixos is bind-mounted from @persist
```

If `/etc/nixos` is NOT bind-mounted, immutability is already off and the
migration is unnecessary — skip to the re-enable section.

---

## Step 1 — Build + activate the recovery generation

```sh
sudo nixos-rebuild switch
```

This builds a generation containing:

- the Phase 3 fix (initrd `immutability-mounts` ordered before
  `initrd-nixos-activation`), present in source but **not exercised**
  because the generation has immutability disabled
- the Phase 4 special-file strip behavior in the engine, also dormant
- `immutability.enable = false` on FRACTAL-NORTH (recovery commit
  `59c550e`)

After it finishes:

```sh
sudo bootctl status | head -20
# Confirm that the "default" lanzaboote entry is the newest generation.
```

The OLD lockout-bearing generations are still in `/boot/EFI/Linux/`.
Don't reboot yet — we delete those after migration.

---

## Step 2 — Dry-run the migration script

```sh
sudo /etc/nixos/scripts/recovery/migrate-persist-to-root.sh --dry-run
```

Read every `DRY  …` line. Confirm:

- The btrfs device matches what you expect (look for `btrfs device:`)
- Roughly the right set of persist paths gets listed
- The `--- open file descriptors under persist paths ---` block tells
  you which daemons hold files in persist. Note any unfamiliar names —
  this is the list you may need to stop in Step 3 if `umount` later
  refuses to detach.

The script does not write anything in `--dry-run` mode.

---

## Step 3 — (Optional) Stop daemons holding fds in persist

The script's `umount` step at the end will EBUSY-fail if anything still
has files open in `/run/migrate-$$/@root` or `/run/migrate-$$/@home`.
For the most part this is fine because we're side-mounting (the live
binds keep working). But if the side-mount itself has loud holders,
stop the obvious offenders first:

```sh
# Examples — only run if shown in Step 2's lsof output:
sudo systemctl stop sunshine
sudo systemctl stop rclone-drive
# 1Password is harder to stop cleanly; leave it running and accept the
# warning if it appears. The migration completes regardless.
```

You can also stop the graphical session entirely (logs out KDE):

```sh
sudo systemctl stop display-manager
# Reconnect via SSH if needed; you're still logged in.
```

This is **optional**. The script reports unmount failures clearly; you
can skip ahead and only come back if needed.

---

## Step 4 — Apply the migration

```sh
sudo /etc/nixos/scripts/recovery/migrate-persist-to-root.sh --apply
```

Watch for any `ERROR:` lines. The script:

1. Snapshots every active persist subvolume read-only under
   `@snapshots/migrate-<pid>-<epoch>/<key>` for point-in-time consistency
2. Side-mounts `@root` and `@home`
3. rsyncs each snapshot into the corresponding side-mount target
4. Verifies the `/etc/nixos` git HEAD matches between live and migrated
   `@root`
5. Tears down side-mounts; **leaves the snapshots in place** (your
   rollback if reboot fails)
6. Prints next-step operator instructions

At the end you'll see:

```
migrate-persist-to-root.sh: done.
```

If it dies partway through, **do not reboot**. Re-read the error and
the journal:

```sh
journalctl -b 0 -n 100
```

---

## Step 5 — Verify migration

Side-mount `@root` again (the script left it unmounted) and confirm
what's on disk matches what was visible at the live persist paths:

```sh
DEV=$(findmnt -no SOURCE / | sed 's/\[.*//')
sudo mkdir -p /tmp/verify-root
sudo mount -t btrfs -o subvol=@root "$DEV" /tmp/verify-root

# Recovery commits visible in @root's /etc/nixos
git -C /tmp/verify-root/etc/nixos log --oneline -10
git -C /tmp/verify-root/etc/nixos rev-parse HEAD
# Compare to the HEAD you recorded in Pre-flight. They must match.

# Hash secret present
ls -la /tmp/verify-root/etc/nixos/secrets/hashed_password.txt

# sbctl PKI present (lanzaboote)
ls -la /tmp/verify-root/var/lib/sbctl/keys/db/

# /home not touched here (you'd verify @home separately if needed)
sudo umount /tmp/verify-root && sudo rmdir /tmp/verify-root
```

If git HEAD does **not** match, **do not reboot**. The migration didn't
work as intended; investigate before going further.

---

## Step 6 — Reboot

```sh
sudo reboot
```

On reboot, lanzaboote will offer the OLD lockout-bearing generations
*and* the new recovery generation. The new one is the default. **Do
not press any keys during boot** — the lanzaboote timeout is short and
an accidental keystroke could pick the broken gen.

---

## Step 7 — Post-reboot verification

After SSH-ing back in (or logging in at the console):

```sh
# You should be able to log in with your normal password — no chpasswd
# needed. PAM auth via shadow works end-to-end.

# Confirm immutability is off
sudo systemctl status immutability.service immutability-mounts.service
# Both should be "inactive (dead)" or "not loaded".

# Confirm /etc/nixos is on @root (no bind)
findmnt /etc/nixos
# Should produce no output (or show /etc/nixos with FSROOT pointing at
# the @root subvolume, depending on kernel version).

# Confirm git is intact
cd /etc/nixos && git log --oneline -10
# Should show all the recovery commits, matching pre-reboot.
```

---

## Step 8 — Prune lockout-bearing generations from the bootloader

Now that you're running the new generation, delete every other:

```sh
sudo nix-env --delete-generations +1 --profile /nix/var/nix/profiles/system
# +1 keeps only the current generation.

sudo nixos-rebuild switch
# Re-runs the lanzaboote install command. Only the current generation
# remains in /boot/EFI/Linux/. The lockout-bearing entries are gone.

ls /boot/EFI/Linux/
# Should show just one .efi file (the current gen).
```

---

## Rollback path (if anything goes wrong)

The migration script leaves the `@snapshots/migrate-*` read-only
snapshots in place. If the post-migration `@root` is wrong, you can:

```sh
# Boot from a NixOS ISO USB
# Mount btrfs root:
mount -t btrfs -o subvolid=5 /dev/mapper/root /mnt
ls /mnt/@snapshots/migrate-*    # find your snapshots

# Snapshots are read-only, so use them as a source to rsync back.
# Example — restore /etc/nixos:
mkdir -p /tmp/target-root
mount -t btrfs -o subvol=@root /dev/mapper/root /tmp/target-root
rsync -aHAX --delete /mnt/@snapshots/migrate-XXXXX/etc!nixos/ \
    /tmp/target-root/etc/nixos/
umount /tmp/target-root
```

---

## Later — re-enabling immutability with the Phase 3 fix

When you're ready (have used the recovery gen for a while and trust
the source):

1. Edit `nix/hosts/x86_64/FRACTAL-NORTH/FRACTAL-NORTH.nix`:
   ```nix
   immutability.enable = true;
   immutability.enforce.onReboot = true;
   immutability.enforce.onUpdate = true;
   ```
2. `nixos-rebuild switch`
   - The `immutabilitySnapshotClean` activation script captures a
     fresh CLEAN snapshot of the current `@root`/`@home` — your post-
     recovery state becomes the new baseline.
3. `sudo reboot`
   - Initrd runs `immutability.service` (reset @root from CLEAN) →
     `immutability-mounts.service` (Phase 3 — bind persist subvols onto
     /sysroot) → `initrd-nixos-activation` (activate against the bind-
     mounted view).
   - On the very first re-enable boot, `@persist/dirs/*` may be empty;
     the engine creates the subvolumes on the fly from CLEAN content.
4. Log in normally. The bind-mount race that caused P2.1 is structurally
   gone.
