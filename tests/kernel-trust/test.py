from typing import TYPE_CHECKING

if TYPE_CHECKING:
    # NixOS supplies these names when executing the test script.
    from nixos_test import machine, start_all, subtest  # ty: ignore[unresolved-import]

import json
import shlex

start_all()
machine.wait_for_unit("multi-user.target")


def module_boundary():
    paths = json.loads(machine.succeed("cat /etc/fixture-paths"))
    expected = paths["image"] + "/" + paths["kernelFile"]
    assert machine.succeed("readlink -f /run/current-system/kernel").strip() == expected
    spec = json.loads(machine.succeed("cat /run/current-system/boot.json"))
    assert spec["org.nixos.bootspec.v1"]["kernel"] == expected
    assert machine.succeed("cat /sys/module/module/parameters/sig_enforce").strip() == "Y"
    machine.succeed("modprobe trusted_probe")
    machine.succeed("test -d /sys/module/trusted_probe")
    machine.succeed("rmmod trusted_probe")
    machine.succeed("python3 /etc/module-adversary.py")
    trusted = machine.succeed("find " + shlex.quote(paths["trusted"]) + " -name trusted_probe.ko.xz").strip()
    foreign = machine.succeed("find " + shlex.quote(paths["foreign"]) + " -name foreign_probe.ko.xz").strip()
    machine.succeed("xz -dc " + shlex.quote(trusted) + " > /run/trusted.ko")
    machine.succeed("xz -dc " + shlex.quote(foreign) + " > /run/foreign.ko")
    machine.succeed("python3 - <<'PY'\nfrom pathlib import Path\nimport struct\nb=Path('/run/trusted.ko').read_bytes()\nmarker=b'~Module signature appended~\\n'\nassert b.endswith(marker)\nend=len(b)-len(marker)-12\nn=struct.unpack('>I',b[end+8:end+12])[0]\nPath('/run/unsigned.ko').write_bytes(b[:end-n])\nb=bytearray(b); b[b.index(b'module trust fixture loaded')]^=1\nPath('/run/tampered.ko').write_bytes(b)\nPY")
    for name in ["unsigned", "foreign", "tampered"]:
        status, output = machine.execute("insmod /run/" + name + ".ko 2>&1")
        assert status != 0, name + " module was accepted"
        assert "Key was rejected" in output or "Required key not available" in output, output
    machine.succeed("insmod /run/trusted.ko")
    machine.succeed("rmmod trusted_probe")


with subtest("signature enforcement uses embedded module certificates without lockdown"):
    assert machine.succeed("cat /etc/generation-marker").strip() == "signature-only"
    assert "[none]" in machine.succeed("cat /sys/kernel/security/lockdown")
    assert "disk" in machine.succeed("cat /sys/power/state").split()
    module_boundary()

with subtest("own-key Secure Boot preserves the trust set under integrity lockdown"):
    target = machine.succeed("readlink -f /run/current-system/specialisation/lockdown").strip()
    machine.succeed("sbctl --config /etc/test-sbctl.conf create-keys")
    machine.succeed("nix-env -p /nix/var/nix/profiles/system --set " + shlex.quote(target))
    machine.succeed(shlex.quote(target) + "/bin/switch-to-configuration switch")
    machine.wait_for_unit("prepare-sb-auto-enroll.service")
    machine.succeed("for image in /boot/EFI/Linux/*.efi; do sbverify --cert /var/lib/kernel-trust-fixture/keys/db/db.pem \"$image\"; done")
    machine.shutdown()
    machine.start(allow_reboot=True)
    machine.connected = False
    machine.wait_for_unit("multi-user.target")
    assert machine.succeed("cat /etc/generation-marker").strip() == "lockdown"
    assert "[integrity]" in machine.succeed("cat /sys/kernel/security/lockdown")
    assert "disk" not in machine.succeed("cat /sys/power/state").split()
    assert int(machine.succeed("od -An -tu1 -j4 /sys/firmware/efi/efivars/SecureBoot-8be4df61-93ca-11d2-aa0d-00e098032b8c")) == 1
    module_boundary()

machine.shutdown()
