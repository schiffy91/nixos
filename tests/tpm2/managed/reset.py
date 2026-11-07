with subtest("repository BTRFS root reset preserves managed policy and automatic unlock"):
    machine.succeed("test $(findmnt -n -o FSTYPE /) = btrfs; test $(findmnt -n -o FSROOT /) = /@root; test $(findmnt -n -o FSROOT /var) = /@var; test $(findmnt -n -o FSROOT /nix) = /@nix")
    machine.succeed("touch /root/ephemeral-tpm-test; echo retained > /var/lib/nixos/tpm-policy/persistence-test; cp /var/lib/systemd/pcrlock.json /var/tmp/reset-policy.json; cp /var/lib/pcrlock.d/635-lanzaboote.pcrlock.d/2.pcrlock /var/tmp/reset-generation.pcrlock")
    cold_boot()
    machine.succeed("test ! -e /root/ephemeral-tpm-test; grep -Fx retained /var/lib/nixos/tpm-policy/persistence-test; cmp /var/tmp/reset-policy.json /var/lib/systemd/pcrlock.json; cmp /var/tmp/reset-generation.pcrlock /var/lib/pcrlock.d/635-lanzaboote.pcrlock.d/2.pcrlock; journalctl -b -u immutability --no-pager | grep -F 'Prepared @root NEXT'")
    machine.succeed(TOKEN_TEST)

with subtest("ordinary update and rollback retain TPM authority across real root reset"):
    machine.succeed(f"ln -s {shlex.quote(updated)} /nix/var/nix/profiles/system-3-link")
    machine.succeed(f"{shlex.quote(updated)}/bin/switch-to-configuration boot", timeout=120)
    default_generation(3)
    cold_boot()
    assert "nixos.tpm.test=updated" in machine.succeed("cat /proc/cmdline")
    assert json.loads(machine.succeed("nixos-tpm-policy status"))["nvIndex"] == status["nvIndex"]
    machine.succeed(TOKEN_TEST)
    machine.succeed("touch /root/second-ephemeral-tpm-test")
    default_generation(2)
    cold_boot()
    assert machine.succeed("readlink -f /run/booted-system").strip() == first
    machine.succeed("test ! -e /root/second-ephemeral-tpm-test; printf testpass123 | cryptsetup open --test-passphrase --key-file=- /dev/vdb")
    machine.succeed(TOKEN_TEST)

print("Managed TPM authority survived repository BTRFS reset, update and rollback")
