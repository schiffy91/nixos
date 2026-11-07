import base64

with subtest("missing or mismatched policy cannot replace an enrolled NV authority"):
    writable_store()
    before = machine.succeed("tpm2_getcap -T device:/dev/tpmrm0 handles-nv-index")
    header = machine.succeed("cryptsetup luksDump --dump-json-metadata /dev/vdb")
    machine.succeed("mv /var/lib/systemd/pcrlock.json /var/tmp/enrolled-policy.json")
    for command in ["nixos-tpm-policy update", "PASSWORD=testpass123 nixosctl tpm2 enable"]:
        failure = machine.fail(command + " 2>&1")
        assert "refusing to create replacement NV authority" in failure, failure
    machine.succeed("test ! -e /var/lib/systemd/pcrlock.json; mv /var/tmp/enrolled-policy.json /var/lib/systemd/pcrlock.json")
    assert machine.succeed("tpm2_getcap -T device:/dev/tpmrm0 handles-nv-index") == before
    assert machine.succeed("cryptsetup luksDump --dump-json-metadata /dev/vdb") == header
    machine.succeed("cp /var/lib/systemd/pcrlock.json /var/tmp/enrolled-policy.json; python3 -c \"import json; from pathlib import Path; p=Path('/var/lib/systemd/pcrlock.json'); d=json.loads(p.read_text()); d['nvHandle']='b3RoZXItYXV0aG9yaXR5'; p.write_text(json.dumps(d))\"")
    failure = machine.fail("nixos-tpm-policy update 2>&1")
    assert "policy NV identity differ" in failure, failure
    assert machine.succeed("tpm2_getcap -T device:/dev/tpmrm0 handles-nv-index") == before
    machine.succeed("mv /var/tmp/enrolled-policy.json /var/lib/systemd/pcrlock.json")
    machine.succeed(TOKEN_TEST)

with subtest("missing PCR4 fails before changing persistent policy"):
    machine.succeed("cp /var/lib/systemd/pcrlock.json /var/tmp/policy-before.json")
    machine.succeed("mv /var/lib/pcrlock.d/635-lanzaboote.pcrlock.d/2.pcrlock /var/tmp/2.pcrlock")
    machine.fail("nixos-tpm-policy update")
    machine.succeed("cmp /var/tmp/policy-before.json /var/lib/systemd/pcrlock.json")
    machine.succeed("LD_LIBRARY_PATH=/run/current-system/systemd/lib/cryptsetup cryptsetup open --test-passphrase --token-only /dev/vdb")
    machine.succeed("mv /var/tmp/2.pcrlock /var/lib/pcrlock.d/635-lanzaboote.pcrlock.d/2.pcrlock")

with subtest("normal update and intentional retained rollback"):
    machine.succeed(f"ln -s {shlex.quote(updated)} /nix/var/nix/profiles/system-3-link")
    machine.succeed(f"{shlex.quote(updated)}/bin/switch-to-configuration boot", timeout=120)
    after = json.loads(machine.succeed("nixos-tpm-policy status"))
    assert after["nvIndex"] == status["nvIndex"]
    machine.succeed("bootctl set-default $(basename /boot/EFI/Linux/nixos-generation-3-*.efi)")
    machine.shutdown()
    machine.start()
    machine.wait_for_unit("multi-user.target", timeout=180)
    assert "nixos.tpm.test=updated" in machine.succeed("cat /proc/cmdline")
    machine.succeed("bootctl set-default $(basename /boot/EFI/Linux/nixos-generation-2-*.efi)")
    machine.shutdown()
    machine.start()
    machine.wait_for_unit("multi-user.target", timeout=180)
    assert machine.succeed("readlink -f /run/booted-system").strip() == first

with subtest("strict PCR4 remains mandatory when another generation exists"):
    machine.succeed("cp /var/lib/systemd/pcrlock.json /var/tmp/policy-before.json; mv /var/lib/pcrlock.d/635-lanzaboote.pcrlock.d/2.pcrlock /var/tmp/2.pcrlock")
    machine.fail("nixos-tpm-policy update")
    machine.fail("systemctl restart systemd-pcrlock-make-policy.service")
    machine.succeed("cmp /var/tmp/policy-before.json /var/lib/systemd/pcrlock.json")
    machine.succeed(TOKEN_TEST)
    machine.succeed("mv /var/tmp/2.pcrlock /var/lib/pcrlock.d/635-lanzaboote.pcrlock.d/2.pcrlock; systemctl restart systemd-pcrlock-make-policy.service")

with subtest("untrusted ESP credentials cannot inject a root service drop-in"):
    name = "systemd.unit-dropin.systemd-pcrlock-make-policy.service~injected"
    dropin = "[Service]\nExecStartPre=/run/current-system/sw/bin/touch /run/credential-injection-executed\n"
    machine.succeed(f"printf %s {shlex.quote(dropin)} > /var/tmp/injected.conf; systemd-creds encrypt --with-key=null --name={shlex.quote(name)} /var/tmp/injected.conf /boot/loader/credentials/{shlex.quote(name)}.cred")
    cold_boot()
    machine.succeed("test ! -e /run/credential-injection-executed")
    machine.succeed(f"journalctl -b --no-pager | grep -F {shlex.quote('Failed to read credential ' + chr(39) + name + chr(39))}")
    machine.fail(f"systemd-creds decrypt --name={shlex.quote(name)} /boot/loader/credentials/{shlex.quote(name)}.cred -")
    machine.succeed(TOKEN_TEST)
    machine.succeed(f"rm /boot/loader/credentials/{shlex.quote(name)}.cred")

with subtest("managed stub refuses an executable companion before Linux starts"):
    image = machine.succeed("echo /boot/EFI/Linux/nixos-generation-2-*.efi").strip()
    machine.succeed(f"mkdir -p {shlex.quote(image + '.extra')}; printf untrusted-extension > {shlex.quote(image + '.extra/untrusted.raw')}; cp /var/lib/systemd/pcrlock.json /var/tmp/policy-before-extension.json")
    default_generation(3)
    machine.succeed("bootctl set-oneshot $(basename /boot/EFI/Linux/nixos-generation-2-*.efi)")
    machine.shutdown()
    machine.start()
    machine.wait_for_console_text("Managed TPM policy refuses executable companion extensions", timeout=90)
    machine.crash()
    machine.start()
    machine.wait_for_unit("multi-user.target", timeout=180)
    assert machine.succeed("readlink -f /run/booted-system").strip() == updated
    machine.succeed(f"rm {shlex.quote(image + '.extra/untrusted.raw')}; rmdir {shlex.quote(image + '.extra')}")
    default_generation(2)
    cold_boot()
    machine.succeed(TOKEN_TEST)

with subtest("changed PCR13 cannot unlock or rewrite NV authority"):
    machine.succeed("cp /var/lib/systemd/pcrlock.json /var/tmp/policy-before-extension.json")
    machine.succeed("/run/current-system/systemd/lib/systemd/systemd-pcrextend --pcr=13 --bank=sha256 untrusted-extension")
    machine.fail(TOKEN_TEST)
    machine.fail("nixos-tpm-policy update")
    # Bypass our wrapper entirely: a compromised pre-root environment must not
    # recover the sealed policy-update secret by requesting a weaker new mask.
    downgrade = "/run/current-system/systemd/lib/systemd/systemd-pcrlock --strict=yes --pcr=4 --pcr=7 --components=/etc/pcrlock.d --components=/var/lib/pcrlock.d --policy=/var/lib/systemd/pcrlock.json --location=770"
    predicted = json.loads(machine.succeed(downgrade + " --json=short predict"))
    assert {entry["pcr"] for entry in predicted["sha256"]} == {4, 7}
    denied = machine.fail(downgrade + " --force make-policy 2>&1")
    assert "policy" in denied.lower() and "TPM" in denied, denied
    machine.succeed("cmp /var/tmp/policy-before-extension.json /var/lib/systemd/pcrlock.json")
    cold_boot()
    machine.succeed(TOKEN_TEST)

with subtest("an ordinary user cannot change policy authority"):
    machine.fail("su -s /bin/sh admin -c 'nixos-tpm-policy update'")
    for path in ["/var/lib/nixos/tpm-policy", "/var/lib/systemd/pcrlock.json", "/var/lib/pcrlock.d"]:
        machine.fail(f"su -s /bin/sh admin -c 'test -w {path}'")

with subtest("external command line cannot replace the signed command line"):
    image = machine.succeed("echo /boot/EFI/Linux/nixos-generation-2-*.efi").strip()
    entry = "title External command line\nefi " + image.removeprefix("/boot") + "\noptions init=/bin/sh nixos.tpm.test=external\n"
    machine.succeed(f"printf %s {shlex.quote(entry)} > /boot/loader/entries/external.conf; bootctl set-default external.conf")
    cold_boot()
    assert "nixos.tpm.test=external" not in machine.succeed("cat /proc/cmdline")
    machine.succeed(TOKEN_TEST)
    default_generation(2)

with subtest("managed stub refuses an external Type-1 initrd before Linux starts"):
    image = machine.succeed("echo /boot/EFI/Linux/nixos-generation-2-*.efi").strip()
    machine.succeed(f"objcopy --dump-section .initrd=/var/tmp/initrd-path {shlex.quote(image)} /var/tmp/dump-copy.efi")
    initrd = machine.succeed("tr -d '\\000' < /var/tmp/initrd-path").strip().replace("\\", "/")
    assert initrd.startswith("/EFI/nixos/")
    systemd = machine.succeed("readlink -f /run/current-system/systemd").strip()
    payload = f"#!/bin/sh\necho EXTERNAL_INITRD_EXECUTED\nexec {systemd}/lib/systemd/systemd\n".encode()
    archive = cpio_entry("init", payload, 0o100755) + cpio_entry("TRAILER!!!", b"", 0) + b"\0" * 512
    encoded = base64.b64encode(archive).decode()
    machine.succeed(f"cp {shlex.quote('/boot' + initrd)} /boot/EFI/test/external.img; printf %s {shlex.quote(encoded)} | base64 -d >> /boot/EFI/test/external.img")
    entry = "title External initrd\nlinux " + image.removeprefix("/boot") + "\ninitrd /EFI/test/external.img\n"
    machine.succeed(f"printf %s {shlex.quote(entry)} > /boot/loader/entries/external-initrd.conf; bootctl set-oneshot external-initrd.conf")
    machine.shutdown()
    machine.start()
    machine.wait_for_console_text("Managed TPM policy refuses an externally supplied initrd", timeout=90)
    assert "EXTERNAL_INITRD_EXECUTED" not in machine.get_console_log()
    machine.crash()
    machine.start()
    machine.wait_for_unit("multi-user.target", timeout=180)
    assert machine.succeed("readlink -f /run/booted-system").strip() == first
    machine.succeed(TOKEN_TEST)

with subtest("same-key signed command line outside the policy requires the passphrase"):
    machine.succeed(f"objcopy --dump-section .cmdline=/var/tmp/cmdline {shlex.quote(image)} /var/tmp/dump-copy.efi")
    # Keep the PE section length unchanged: objcopy preserves VirtualSize.
    machine.succeed("python3 -c \"from pathlib import Path; p=Path('/var/tmp/cmdline'); s=p.read_bytes(); assert b'loglevel=7' in s; p.write_bytes(s.replace(b'loglevel=7', b'loglevel=6'))\"")
    machine.succeed(f"objcopy --update-section .cmdline=/var/tmp/cmdline {shlex.quote(image)} /var/tmp/unauthorized.efi")
    machine.succeed("sbsign --key /var/lib/lanzaboote-test-fixture/keys/db/db.key --cert /var/lib/lanzaboote-test-fixture/keys/db/db.pem --output /boot/EFI/test/unauthorized.efi /var/tmp/unauthorized.efi")
    machine.succeed("printf 'title Unauthorized signed image\nefi /EFI/test/unauthorized.efi\n' > /boot/loader/entries/unauthorized.conf; bootctl set-default unauthorized.conf")
    cold_boot(password=True)
    assert "loglevel=6" in machine.succeed("cat /proc/cmdline")
    assert "Secure Boot: enabled" in machine.succeed("bootctl status")
    machine.fail(TOKEN_TEST)
    machine.fail("nixos-tpm-policy update")
    default_generation(2)
    cold_boot()

with subtest("export interruption after NV update falls back to password and repairs on retry"):
    machine.succeed("cp /var/lib/systemd/pcrlock.json /var/tmp/policy-with-3.json; cp /boot/loader/credentials/pcrlock.nixosctl.cred /var/tmp/credential-with-3.cred")
    # Revoke the other eligible generation while keeping this boot eligible.
    machine.succeed("mv /nix/var/nix/profiles/system-3-link /var/tmp/system-3-link; mount --bind /boot/loader/credentials /boot/loader/credentials; mount -o remount,bind,ro /boot/loader/credentials")
    machine.fail("nixos-tpm-policy update")
    machine.fail("cmp /var/tmp/policy-with-3.json /var/lib/systemd/pcrlock.json")
    machine.succeed("cmp /var/tmp/credential-with-3.cred /boot/loader/credentials/pcrlock.nixosctl.cred; umount /boot/loader/credentials")
    default_generation(2)
    cold_boot(password=True)
    machine.wait_for_unit("systemd-pcrlock-make-policy.service")
    machine.succeed("systemd-creds decrypt --allow-null --name=pcrlock.nixosctl /boot/loader/credentials/pcrlock.nixosctl.cred /var/tmp/export.json; jq -S . /var/tmp/export.json > /var/tmp/export.sorted; jq -S . /var/lib/systemd/pcrlock.json > /var/tmp/policy.sorted; cmp /var/tmp/export.sorted /var/tmp/policy.sorted")
    cold_boot()
    machine.succeed(TOKEN_TEST)

with subtest("revoked signed generation and replayed old credential cannot unlock"):
    default_generation(3)
    machine.succeed("cp /var/tmp/credential-with-3.cred /boot/loader/credentials/pcrlock.nixosctl.cred")
    cold_boot(password=True)
    assert "nixos.tpm.test=updated" in machine.succeed("cat /proc/cmdline")
    machine.fail(TOKEN_TEST)
    machine.fail("nixos-tpm-policy update")
    default_generation(2)
    cold_boot(password=True)
    machine.wait_for_unit("systemd-pcrlock-make-policy.service")

with subtest("actual signed recovery requires the LUKS passphrase"):
    machine.succeed("bootctl set-default recovery.conf")
    machine.shutdown()
    machine.start()
    wait_recovery()
    recovery_command("test ! -e /run/booted-system/tpm-policy-version; test ! -e /dev/mapper/root; bootctl status | grep -F 'Secure Boot: enabled'; mkdir -p /mnt/esp; mount /dev/disk/by-partlabel/esp /mnt/esp; systemd-creds decrypt --allow-null --name=pcrlock.nixosctl /mnt/esp/loader/credentials/pcrlock.nixosctl.cred /var/lib/systemd/pcrlock.json", "RECOVERY_BOOT")
    recovery_command("if LD_LIBRARY_PATH=/run/current-system/systemd/lib/cryptsetup cryptsetup open --test-passphrase --token-only /dev/vdb; then exit 1; fi", "RECOVERY_DENIED")
    recovery_command("printf testpass123 | cryptsetup open --key-file=- /dev/vdb target; mkdir -p /mnt/target; mount /dev/mapper/target /mnt/target; test -f /mnt/target/var/lib/systemd/pcrlock.json", "RECOVERY_PASSWORD")
    recovery_command("bootctl --esp-path=/mnt/esp set-default $(basename /mnt/esp/EFI/Linux/nixos-generation-2-*.efi); umount /mnt/target; cryptsetup close target; sync", "RECOVERY_RETURN")
    machine.send_console("poweroff\n")
    machine.wait_for_shutdown()
    machine.start()
    machine.wait_for_unit("multi-user.target", timeout=180)
    machine.succeed(TOKEN_TEST)

with subtest("renumbering an unsafe old generation does not admit it"):
    machine.succeed(f"ln -s {shlex.quote(base)} /nix/var/nix/profiles/system-505-link")
    machine.succeed(f"{shlex.quote(first)}/bin/switch-to-configuration boot", timeout=120)
    machine.succeed("bootctl set-default $(basename $(find /boot/EFI/Linux -name 'nixos-generation-505-*.efi' ! -name '*specialisation*'))")
    cold_boot()
    assert machine.succeed("readlink -f /run/booted-system").strip() == base
    assert machine.succeed("findmnt -n -o SOURCE /").strip() != "/dev/mapper/root"
    machine.succeed("systemd-creds decrypt --allow-null --name=pcrlock.nixosctl /boot/loader/credentials/pcrlock.nixosctl.cred /var/lib/systemd/pcrlock.json")
    machine.fail(TOKEN_TEST)
    machine.succeed("printf testpass123 | cryptsetup open --test-passphrase --key-file=- /dev/vdb")
    default_generation(2)
    cold_boot()
    machine.succeed("rm /nix/var/nix/profiles/system-505-link")
    machine.succeed(TOKEN_TEST)

for section, kind in [(".linux", "Kernel"), (".initrd", "Initrd")]:
    with subtest(f"{kind.lower()} mutation is rejected by the signed thin image"):
        image = machine.succeed("echo /boot/EFI/Linux/nixos-generation-2-*.efi").strip()
        machine.succeed(f"objcopy --dump-section {section}=/var/tmp/component-path {shlex.quote(image)} /var/tmp/dump-copy.efi")
        component = machine.succeed("tr -d '\\000' < /var/tmp/component-path").strip().replace("\\", "/")
        assert component.startswith("/EFI/nixos/")
        machine.succeed(f"cp {shlex.quote('/boot' + component)} /boot/EFI/test/original-component; printf tampered >> {shlex.quote('/boot' + component)}; sync")
        machine.succeed("bootctl set-default recovery.conf; bootctl set-oneshot $(basename /boot/EFI/Linux/nixos-generation-2-*.efi)")
        machine.shutdown()
        machine.start()
        machine.wait_for_console_text(kind + " hash does not match", timeout=90)
        machine.crash()
        machine.start()
        wait_recovery()
        recovery_command(f"mkdir -p /mnt/esp; mount /dev/disk/by-partlabel/esp /mnt/esp; mv /mnt/esp/EFI/test/original-component {shlex.quote('/mnt/esp' + component)}; bootctl --esp-path=/mnt/esp set-default $(basename /mnt/esp/EFI/Linux/nixos-generation-2-*.efi); sync", "RESTORE_" + kind.upper())
        machine.send_console("poweroff\n")
        machine.wait_for_shutdown()
        machine.start()
        machine.wait_for_unit("multi-user.target", timeout=180)
        machine.succeed(TOKEN_TEST)

with subtest("two changed loader images prepared before reboot retain the current loader"):
    install = shlex.split(machine.succeed("cat /etc/tpm-test/install-command").replace("\\\n", " "), comments=True)
    systemd_index = install.index("--systemd") + 1
    original_systemd = install[systemd_index]
    for version in [263, 264]:
        candidate = f"/var/tmp/systemd-{version}"
        script = f"from pathlib import Path; source=Path({original_systemd!r})/'lib/systemd/boot/efi/systemd-bootx64.efi'; target=Path({candidate!r})/'lib/systemd/boot/efi/systemd-bootx64.efi'; target.parent.mkdir(parents=True, exist_ok=True); data=source.read_bytes(); assert data.count(b'262') == 8; target.write_bytes(data.replace(b'262', str({version}).encode()).replace('262'.encode('utf-16-le'), str({version}).encode('utf-16-le')))"
        machine.succeed("python3 -c " + shlex.quote(script))
        command = list(install)
        command[systemd_index] = candidate
        command += ["--public-key=/var/lib/lanzaboote-test-fixture/keys/db/db.pem", "--private-key=/var/lib/lanzaboote-test-fixture/keys/db/db.key", "--pcrlock-directory=/var/lib/pcrlock.d", "--protected-system=" + first, "/boot", "/nix/var/nix/profiles/system-2-link"]
        machine.succeed("PATH=/run/current-system/systemd/lib/systemd:$PATH " + shlex.join(command), timeout=120)
        machine.succeed("nixos-tpm-policy update")
        machine.succeed(f"test -f /var/lib/pcrlock.d/630-bootloader.pcrlock.d/262.pcrlock; test -f /var/lib/pcrlock.d/630-bootloader.pcrlock.d/{version}.pcrlock")
        machine.succeed(TOKEN_TEST)
    machine.fail("test -e /var/lib/pcrlock.d/630-bootloader.pcrlock.d/263.pcrlock")
    default_generation(2)
    cold_boot()
    assert "systemd-boot 264" in machine.succeed("bootctl status")
    machine.succeed(TOKEN_TEST)

with subtest("disabled Secure Boot cannot unlock or update policy"):
    # Keep prepared auth files so the preparation service remains a no-op,
    # but explicitly prevent the loader from auto-enrolling them on this boot.
    machine.succeed("cp /boot/loader/loader.conf /var/tmp/loader-before-insecure.conf; printf '\\nsecure-boot-enroll off\\n' >> /boot/loader/loader.conf; sbctl reset --partial PK")
    cold_boot(password=True)
    assert "Secure Boot: enabled" not in machine.succeed("bootctl status")
    machine.fail(TOKEN_TEST)
    machine.fail("nixos-tpm-policy update")
    writable_store()
    password_change_refused("Managed TPM policy requires active Secure Boot")
    machine.succeed("chattr -i /sys/firmware/efi/efivars/KEK-8be4df61-93ca-11d2-aa0d-00e098032b8c /sys/firmware/efi/efivars/db-d719b2cb-3d3a-4596-a3bc-dad00e67656f; sbctl enroll-keys --yes-this-might-brick-my-machine; cp /var/tmp/loader-before-insecure.conf /boot/loader/loader.conf")
    cold_boot()
    assert "Secure Boot: enabled" in machine.succeed("bootctl status")
    machine.succeed(TOKEN_TEST)

with subtest("NV update interrupted before JSON persistence preserves passphrase recovery"):
    machine.succeed("cp /var/lib/systemd/pcrlock.json /var/tmp/policy-before-json-failure.json; cp /boot/loader/credentials/pcrlock.nixosctl.cred /var/tmp/credential-before-json-failure.cred; cp -a /var/lib/pcrlock.d /var/tmp/components-before-json-failure")
    machine.succeed(f"ln -s {shlex.quote(updated)} /nix/var/nix/profiles/system-3-link; mount --bind /var/lib/systemd /var/lib/systemd; mount -o remount,bind,ro /var/lib/systemd")
    failure = machine.fail(f"{shlex.quote(updated)}/bin/switch-to-configuration boot 2>&1", timeout=120)
    assert "Updated NV index" in failure and "Failed to write new configuration" in failure, failure
    machine.succeed("umount /var/lib/systemd; cmp /var/tmp/policy-before-json-failure.json /var/lib/systemd/pcrlock.json; cmp /var/tmp/credential-before-json-failure.cred /boot/loader/credentials/pcrlock.nixosctl.cred")
    machine.fail(TOKEN_TEST)
    machine.succeed("printf testpass123 | cryptsetup open --test-passphrase --key-file=- /dev/vdb")
    default_generation(2)
    cold_boot(password=True)
    machine.fail("nixos-tpm-policy update")
    writable_store()
    machine.fail("PASSWORD=testpass123 nixosctl tpm2 enable")
    machine.fail(TOKEN_TEST)
    machine.succeed("printf testpass123 | cryptsetup open --test-passphrase --key-file=- /dev/vdb")
    # Restore the old prediction set. Upstream skips its TPM access here, so
    # only an actual token check can detect that JSON still trails NV state.
    machine.succeed("rm /nix/var/nix/profiles/system-3-link; rm -r /var/lib/pcrlock.d; cp -a /var/tmp/components-before-json-failure /var/lib/pcrlock.d")
    failure = machine.fail("nixos-tpm-policy update 2>&1")
    assert "Prediction is identical to current policy, skipping update." in failure, failure
    assert "boot credential verified" not in failure
    machine.succeed("cmp /var/tmp/policy-before-json-failure.json /var/lib/systemd/pcrlock.json; cmp /var/tmp/credential-before-json-failure.cred /boot/loader/credentials/pcrlock.nixosctl.cred")
    machine.fail(TOKEN_TEST)
    machine.succeed("printf testpass123 | cryptsetup open --test-passphrase --key-file=- /dev/vdb")

print("Managed TPM boot/update/rollback and denial checks passed")
