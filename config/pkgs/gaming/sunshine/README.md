# Sunshine administration

Sunshine can use the administrator's SSH keys for Web UI login:

```nix
settings.apps.sunshine.sshLogin.enable = true;
```

`sshLogin.publicKeys` defaults to `settings.users.admin.authorizedKeys` and can
be overridden with plain Ed25519 or RSA public keys. Each key grants full
Sunshine administration. The configured public keys are installed read-only as
`/etc/sunshine/ssh-keys/key-1.pub`, `key-2.pub`, etc.; private keys stay in your
SSH agent. Check each key's fingerprint with `ssh-keygen -lf` and choose the one
matching your SSH item in 1Password. Existing Sunshine passwords keep working;
a new instance needs no shared password or 1Password CLI provisioning.

Open `https://HOST:47990/ssh-login`, press **Start SSH login**, and run the
displayed command on the computer with your SSH agent. Replace its `-i` argument
with the matching public-key file. For example, after copying Sunshine's PEM
certificate from its configured `cert` path over an already trusted connection:

```sh
sunshine-ssh-login https://HOST:47990 --challenge CHALLENGE_FROM_BROWSER \
  -i /etc/sunshine/ssh-keys/key-1.pub --certificate /path/to/sunshine-cert.pem
```

The explicit certificate option pins the exact certificate before requesting
an agent signature. It is needed for Sunshine's default self-signed certificate;
a browser certificate exception does not configure this helper's trust. Approve
the signing request in 1Password, and the originating browser signs in. The
challenge expires after two minutes, the session after one hour, and logout,
password changes, key removal followed by restart, or Sunshine restart revoke
access. No SSH-agent forwarding is needed. This currently requires copying and
running the displayed command; browsers cannot directly access SSH agents.

## Password compatibility and startup

When SSH login is enabled, a new instance starts without asking 1Password CLI
for a shared password. Existing valid `credentials.json` continues to work. If
only legacy credentials in `sunshine_state.json` exist, preparation imports the
username, salt, and password verifier without modifying pairing state. Existing
credential files still must have safe ownership and mode; SSH login does not
bypass those checks.

Passwords created or changed through the Web UI are written with mode 0600 on
Unix, including manual launches. Both systemd user services also use UMask 0077.
Failed writes are reported as failures and leave the SSH session available.
An existing password cannot be reset merely by signing in with an SSH key: the
current password remains required to change it.

Optional password provisioning remains available with `nixosctl sunshine sync`
or `nixosctl sunshine provision`, followed by a Sunshine restart. Those are
explicit operations; normal startup never fetches secrets from 1Password.

## Package and verification

`ssh-login.patch` backports the SSHSIG feature to the existing
Sunshine v2026.914.233613 package. It patches both the server and separately
built web assets. The existing CUDA, capture, KWin HDR and credential guards
remain in place. This contribution has not been submitted or accepted upstream.

The BTRC helper uses libcurl and OpenSSL for HTTPS and OpenSSH for signing. Server verification uses OpenSSL and
accepts plain Ed25519 or RSA 2048–8192 public keys, not SSH certificates or FIDO
key types. The helper accepts an explicit public-key file; it never exports a
private key or automatically probes your agent.

The graph runs the focused runtime contracts before the Sunshine VM regressions:

```sh
make build
nix-shell tests/shell.nix --run 'NIXOS_CONFIG_ROOT=$PWD ./build/nixosctl graph tests/graph.json run sunshine-ssh-login sunshine-credentials'
```

The SSH scenario uses an isolated fixture agent and generated test-only
credentials, a non-localhost HTTPS origin, the installed helper and real
Sunshine endpoints. It verifies browser binding, replay rejection, first-run
account protection, same-origin mutations, logout, credential permissions,
legacy password preservation and reboot invalidation. It does not contact
1Password or any live user SSH agent. The other scenario retains coverage for
password provisioning and Moonlight pairing compatibility.
