import json
import shlex

start_all()
machine.wait_for_unit("multi-user.target")
UEFI = "8be4df61-93ca-11d2-aa0d-00e098032b8c"
PK = "/sys/firmware/efi/efivars/PK-" + UEFI
KEK = "/sys/firmware/efi/efivars/KEK-" + UEFI
DB = "/sys/firmware/efi/efivars/db-d719b2cb-3d3a-4596-a3bc-dad00e67656f"
pki = "/var/lib/secure-boot-test"
efi_arch = {"x86_64": "x64", "aarch64": "aa64"}[machine.succeed("uname -m").strip()]


def firmware_variable(name):
    return int(machine.succeed(f"od -An -tu1 -j4 /sys/firmware/efi/efivars/{name}-{UEFI}").strip())


def firmware_state(secure, setup):
    actual = (firmware_variable("SecureBoot"), firmware_variable("SetupMode"))
    assert actual == (secure, setup), f"Firmware mode {actual}, expected {(secure, setup)}"


def reboot_to(marker):
    machine.shutdown()
    machine.start()
    machine.wait_for_unit("multi-user.target")
    assert machine.succeed("cat /etc/generation-marker").strip() == marker


def select(system):
    machine.succeed("nix-env -p /nix/var/nix/profiles/system --set " + shlex.quote(system))


def verify_signed():
    machine.succeed(f"sbverify --cert {pki}/keys/db/db.pem /boot/EFI/BOOT/BOOT{efi_arch.upper()}.EFI")
    machine.succeed(f"sbverify --cert {pki}/keys/db/db.pem /boot/EFI/systemd/systemd-boot{efi_arch}.efi")
    machine.succeed(f"for image in /boot/EFI/Linux/*.efi; do sbverify --cert {pki}/keys/db/db.pem \"$image\"; done")


firmware_state(0, 1)
systems = {
    name: machine.succeed("readlink -f /run/current-system/specialisation/" + name).strip()
    for name in ["enrolled", "maintenance", "recovery"]
}

with subtest("fresh private fixture keys enroll through the real boot loader"):
    machine.succeed("sbctl --config /etc/test-sbctl.conf create-keys")
    select(systems["enrolled"])
    machine.succeed(shlex.quote(systems["enrolled"]) + "/bin/switch-to-configuration switch")
    machine.wait_for_unit("prepare-sb-auto-enroll.service")
    machine.succeed("grep -q '^secure-boot-enroll force$' /boot/loader/loader.conf")
    machine.succeed("test -s /boot/loader/keys/auto/PK.auth")
    verify_signed()
    # systemd-boot performs an additional reset after enrollment.
    machine.shutdown()
    machine.start(allow_reboot=True)
    machine.connected = False
    machine.wait_for_unit("multi-user.target")
    assert machine.succeed("cat /etc/generation-marker").strip() == "enrolled"
    firmware_state(1, 0)

with subtest("a different private key cannot remove the enrolled platform key"):
    wrong_config = "keydir: /var/lib/untrusted-test/keys\nguid: /var/lib/untrusted-test/GUID\n"
    machine.succeed("printf %s " + shlex.quote(wrong_config) + " > /run/untrusted-sbctl.conf")
    machine.succeed("sbctl --config /run/untrusted-sbctl.conf create-keys")
    before = machine.succeed("sha256sum " + PK).split()[0]
    databases = machine.succeed("sha256sum " + KEK + " " + DB)
    machine.fail("sbctl --config /run/untrusted-sbctl.conf reset")
    assert machine.succeed("sha256sum " + PK).split()[0] == before
    firmware_state(1, 0)

with subtest("signed maintenance disables auto-enrollment before PK removal"):
    # Retain an independently selectable signed recovery generation.
    select(systems["recovery"])
    recovery_link = machine.succeed("readlink /nix/var/nix/profiles/system").strip()
    recovery_generation = recovery_link.split("-")[1]
    select(systems["maintenance"])
    machine.succeed(shlex.quote(systems["maintenance"]) + "/bin/switch-to-configuration switch")
    machine.succeed("grep -q '^secure-boot-enroll off$' /boot/loader/loader.conf")
    machine.fail("systemctl is-active prepare-sb-auto-enroll.service")
    # Disabling the Nix option does not promise to remove old ESP assets.
    machine.succeed("install -d -m700 /var/lib/secure-boot-removal; mv /boot/loader/keys/auto /var/lib/secure-boot-removal/enrollment")
    machine.succeed("test ! -e /boot/loader/keys/auto")
    verify_signed()
    reboot_to("maintenance")
    firmware_state(1, 0)
    machine.succeed("test ! -e /boot/loader/keys/auto")
    # The first install protects the previously booted auto-enrolling system.
    # Once maintenance has booted, rebuild the menu to retain only the newest
    # maintenance/recovery pair. Retire the initial unsigned Type #1 entries too.
    machine.succeed(shlex.quote(systems["maintenance"]) + "/bin/switch-to-configuration boot")
    machine.succeed("install -d -m700 /var/lib/secure-boot-removal/entries; mv /boot/loader/entries/nixos-*.conf /var/lib/secure-boot-removal/entries/")
    entries = json.loads(machine.succeed("bootctl list --json=short"))
    recovery_entries = [entry["id"] for entry in entries
                        if entry["type"] == "type2" and entry["id"].startswith("nixos-generation-" + recovery_generation + "-")]
    assert len(recovery_entries) == 1, entries
    recovery = recovery_entries[0]
    images = machine.succeed("find /boot/EFI/Linux -name '*.efi' -printf '%f\n'").splitlines()
    assert len(images) == 2, images
    machine.succeed("bootctl set-oneshot " + shlex.quote(recovery))
    reboot_to("recovery")
    firmware_state(1, 0)
    machine.succeed("test ! -e /boot/loader/keys/auto")
    reboot_to("maintenance")
    firmware_state(1, 0)

with subtest("authenticated PK removal survives two boots without re-enrollment"):
    machine.succeed("sbctl --config /etc/test-sbctl.conf reset")
    machine.succeed("test ! -e " + PK)
    assert machine.succeed("sha256sum " + KEK + " " + DB) == databases
    # SecureBoot is the mode sampled for this boot; validate the new mode
    # after a firmware reset rather than assuming an immediate variable change.
    for _ in range(2):
        reboot_to("maintenance")
        firmware_state(0, 1)
        machine.succeed("test ! -e /boot/loader/keys/auto")
        verify_signed()

with subtest("signed recovery also boots with PK removed without re-enrolling"):
    machine.succeed("bootctl set-oneshot " + shlex.quote(recovery))
    reboot_to("recovery")
    firmware_state(0, 1)
    machine.succeed("test ! -e /boot/loader/keys/auto")
    machine.succeed("test ! -e " + PK)
    reboot_to("maintenance")
    firmware_state(0, 1)

with subtest("only explicit enrollment restores enforcing boot"):
    # PK-only removal preserves KEK/db. Re-enroll only PK, without rewriting
    # the retained signature databases or changing their efivarfs flags.
    # sbctl 0.18's precheck scans all variables even for a partial enrollment.
    # Skip that precheck for the absent PK, not the untouched KEK/db variables.
    machine.succeed("test ! -e " + PK)
    machine.succeed("sbctl --config /etc/test-sbctl.conf enroll-keys --partial PK --ignore-immutable --yes-this-might-brick-my-machine")
    assert machine.succeed("sha256sum " + KEK + " " + DB) == databases
    reboot_to("maintenance")
    firmware_state(1, 0)
    machine.succeed("grep -q '^secure-boot-enroll off$' /boot/loader/loader.conf")
    machine.succeed("test ! -e /boot/loader/keys/auto")
    verify_signed()

with subtest("the retained signed recovery still boots under enforcement"):
    machine.succeed("bootctl set-oneshot " + shlex.quote(recovery))
    reboot_to("recovery")
    firmware_state(1, 0)
    reboot_to("maintenance")
    firmware_state(1, 0)
