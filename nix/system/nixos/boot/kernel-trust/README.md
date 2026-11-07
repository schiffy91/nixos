# Kernel module trust and lockdown

Both settings default off. No host enables them automatically:

```nix
settings.boot.verifyModules = true;
# Independently chosen after deciding the sleep policy:
# settings.boot.lockdown = "integrity";
```

`verifyModules` embeds the selected modules' public certificates in the kernel
image and adds `module.sig_enforce=1`. It does not enable lockdown or disable
hibernation. `lockdown = "integrity"` or `"confidentiality"` also enables module
verification; a configuration that requests lockdown and hibernation is rejected.

Pinned nixpkgs signs kernel and external modules with kmodigest and aggregates
their certificates in `modules.cer`. Its `rebuildImage` helper inserts that
public bundle into reserved kernel space and rebuilds the compressed image,
without recompiling kernel C or introducing module private keys. Our module
selects this image consistently in the system link, bootspec and standalone UKI.
The independently built recovery system inherits the selected verification and
lockdown policy and embeds its own module set, rather than the host's drivers.
Lanzaboote continues signing boot artifacts with the existing host key outside
the Nix store. This preserves the own-key firmware policy.

The build checks that the image contains the aggregate, that every external
module has a certificate in it, and that the certificate verifies the actual
module bytes. Missing or mismatched certificates fail the image build. Logitech
TrueForce now uses Kbuild's normal install/sign/compress path instead of copying
an unsigned module and suppressing aggregation failures.

NVIDIA's five modules, Xone and Xpad use the existing nixpkgs signing path. A
blank `modinfo -F signer` does not establish that a kmodigest module is unsigned;
use cryptographic verification and an enforcing-kernel load test.

Enrolling a UEFI `db` certificate alone does not establish module trust: upstream
places those certificates in the platform keyring, while module verification
uses the secondary/builtin keyrings. Embedding each module's certificate avoids
adding shim/MOK or granting a general-purpose module-signing authority.

Kmodigest does not derive a reusable private key from the digest. It hashes the
module to a curve point whose discrete logarithm is unknown, derives signature
values, and recovers a public verification key. Recomputing this for modified
code produces another key, which the current kernel does not trust. The VM
checks that attack, including under lockdown. Its certificate self-signature
padding is unused because the public certificate is a builtin trust anchor;
the module signature itself uses normal ECDSA/SHA-256 verification. See the
[pinned implementation](https://github.com/emilazy/boot-security-tools/blob/kmodigest/v0.1.0/kmodigest/src/lib.rs).

The configured kernel has early lockdown support. Do not infer lockdown state
from the presence or absence of `lockdown` in `lsm=`. Check
`/sys/kernel/security/lockdown` after boot. Upstream integrity lockdown blocks
hibernation even when swap is encrypted. Module verification alone is narrower
protection and does not close the other interfaces restricted by lockdown.

Run the graph nodes `kernel-trust` and `kernel-trust-host`. The first boots a real
VM, accepts an included external module, rejects unsigned, foreign-signed and
tampered modules, then repeats under own-key UEFI Secure Boot and integrity
lockdown. It also checks re-signing modified code, an embedded foreign signer
paired with an unrelated trusted certificate, and the hibernation distinction. The second builds the
Fractal kernel image with the actual NVIDIA/Logitech/Xone/Xpad packages and
cryptographically checks every external module against the embedded set.

Before enabling either setting on hardware, keep a known-working generation and
verified recovery path. Verify GPU driver loading, Plasma/PRIME, gaming,
Sunshine/NVENC, controllers/wheel, Decky hardware controls, and the selected
sleep/wake policy. A VM cannot establish GPU or Deck hardware compatibility.
Reboot after a kernel/module update: the running kernel only trusts its own
generation's module set. Secure Boot signing keys still authorize replacement
boot environments; module enforcement does not make an already-authorized root
rebuild trustworthy.

Primary sources: [kmodigest design and usage](https://github.com/emilazy/boot-security-tools/blob/kmodigest/v0.1.0/kmodigest/README.md),
[pinned image builder](https://github.com/NixOS/nixpkgs/blob/1d060086d60cb76a40cf37109f979d88a5bd6396/pkgs/os-specific/linux/kernel/rebuild-image.nix),
[kernel module verification](https://github.com/torvalds/linux/blob/v7.3/kernel/module/signing.c),
[UEFI keyring handling](https://github.com/torvalds/linux/blob/v7.3/security/integrity/platform_certs/keyring_handler.c),
[lockdown initialization](https://github.com/torvalds/linux/blob/v7.3/security/lockdown/lockdown.c),
[hibernation availability](https://github.com/torvalds/linux/blob/v7.3/kernel/power/hibernate.c).
