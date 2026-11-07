# Managed TPM boot policy

`settings.tpm.managedPolicy.enable` defaults to `false`. It prepares a managed
no-PIN LUKS unlock policy; it does not enroll a token automatically. Successful
explicit enrollment and actual token verification establish the new unlock
policy. Enabling the setting alone leaves existing LUKS tokens in place.

The policy covers SHA256 PCR4, PCR7 and empty PCR13. The pinned Lanzaboote image
binds its kernel, initrd and command line; strict predictions admit only installed
generations carrying this module's eligibility marker. Recovery and older
unmarked generations are excluded. Executable ESP sysext companions and a
preexisting external Linux-initrd provider are refused before Linux starts.
General boot credentials retain strict trust; the dedicated pcrlock credential
carries policy metadata authenticated by the TPM NV policy, not a disk key.

## Before enabling

Use an attended maintenance session with a privately verified LUKS passphrase,
usable account password, independently backed-up Secure Boot keys, and working
signed recovery media. The module requires Lanzaboote, an encrypted root,
systemd TPM support, and disabled effective display-manager/getty autologin.
Automatic disk unlock does not authenticate the person at the login screen.

Enable the setting for the intended host, prepare its signed boot generation,
and boot that generation with Secure Boot active. A fresh installation without
a token needs its passphrase. An existing valid legacy token may still unlock
this first boot; it is not retired merely by enabling the module.

From the eligible boot, run:

```sh
sudo /etc/nixosctl/tpm-policy preflight
sudo nixosctl tpm2 enable
sudo nixosctl tpm2 status
```

Enrollment asks for the existing LUKS passphrase. Do not put a real passphrase
in command-line arguments or shell history. The helper verifies the new token
against the actual TPM before removing old TPM slots from the current header;
a separate passphrase/recovery slot must remain. `status` describes policy and
token metadata. `preflight` additionally tests an existing managed token's
actual unseal and refuses predictable enrollment failures before
`change-password --update-tpm2` can change either password.

Preflight checks the actual eligible boot and Secure Boot state, TPM support,
protected policy paths, token/NV identity, a separate passphrase slot, strict
current PCR predictions and existing-token unseal. It creates only its protected
lock and temporary component snapshot: it does not regenerate components, write
NV, export a policy or enroll a token. Later unexpected failures are not an
atomic transaction across LUKS and account credentials.

## Updates, rollback and persistence

Normal bootloader installation updates the same NV authority with predictions
for retained eligible generations. An intentionally retained generation remains
bootable and can unlock automatically; removing it and completing the policy
update revokes its managed prediction. Replaying its old exported credential
does not undo that NV revocation. Installing an arbitrary signed kernel or
renumbering an ineligible generation does not admit it. An identical policy
update does not rewrite NV, but still verifies an existing token before claiming
success. The installer retains the current boot's loader measurement while
preparing updates.

Keep `/var/lib/systemd` (including `pcrlock.json`), `/var/lib/pcrlock.d`, and
`/var/lib/nixos/tpm-policy` persistent. The intended hosts already keep `/var` on
non-reset `@var`. A custom layout that resets `/var` must retain these complete
directories; a bind mount of only `pcrlock.json` prevents atomic replacement.
Producer services run after local filesystem mounts. Recovery still uses the
LUKS passphrase and is deliberately outside automatic unlock admission.

## Failure and recovery boundaries

An interrupted ESP export leaves passphrase fallback available. Once the ESP is
mounted and writable, `sudo /etc/nixosctl/tpm-policy update` verifies persistent
policy/token state and repairs the exported credential, including when the
prediction itself is unchanged.

A TPM NV update interrupted before JSON persistence is different: upstream
cannot commit those two stores atomically. The helper detects unsuccessful token
unseal, refuses stale-metadata success, and does not silently allocate replacement
NV authority. Missing policy JSON with an existing pcrlock token, or a mismatched
NV identity, likewise refuses before mutation. Keep using the passphrase; do not
try deleting the JSON or clearing the TPM as a repair shortcut.

Recreating authority is a separate attended recovery operation, not an automatic
CLI workflow. First establish passphrase access and independent backups; identify
the exact old token/NV identity from the affected volume and saved metadata;
retire that specific NV authority and associated TPM slots while preserving the
working passphrase; then prepare and verify a fresh policy from an eligible boot.
Do not delete unrelated NV indices, keys, systemd state, or recovery slots. This
stage does not supply a general reset/TPM-clear command or claim that copied old
authority can be abandoned safely.

Removing a historical static PCR7+12 token from the current header cannot revoke
previously copied static token/header blobs. Those have no managed NV revocation
identity. Addressing such copies requires a separately planned retirement of the
old authority or volume key; automatic disk reencryption is outside this feature.
This policy also does not protect keys already present in a running system's RAM
or replace session locking, DMA defenses or kernel lockdown.

## Validation

The graph nodes `tpm-managed-policy` and `tpm-managed-reset` use actual pinned
Lanzaboote, own-key OVMF Secure Boot, swtpm, encrypted-root boots, and the
repository's BTRFS root-reset module. The complete policy matrix passed as
`77a648f519c3`, including actual firmware boots, update/rollback, executable-input
and recovery negatives, interrupted NV/JSON and ESP writes, and password-change
preflight preserving both credentials. BTRFS reset/update/rollback passed as
`7b216db519f7`; legacy-token refusal/cleanup passed as `e5318b079302`; the existing
encrypted password-change regression passed as `183244d8c38a` after actual
encrypted installation `9392ca598293`. `make build check` also passed. Product
code is unchanged between these runs; only documentation and a test-only import
location changed. Hardware activation remains an attended step.
