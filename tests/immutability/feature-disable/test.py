from typing import TYPE_CHECKING

if TYPE_CHECKING:
    # NixOS supplies these names when executing the test script.
    from nixos_test import machine, start_all, subtest  # ty: ignore[unresolved-import]

import json
import shlex

start_all()
machine.wait_for_unit("multi-user.target")

top = "/run/persistence-top"
home = "/fixture/user"
feature = f"{home}/.config/rclone"
key = "fixture!user!.config!rclone"
store = f"{top}/@persist/dirs/{key}"
profiles = json.loads(machine.succeed("cat /etc/persistence-features/profiles.json"))
assert profiles["enabled"]["enabled"] and feature in profiles["enabled"]["paths"]
assert not profiles["disabled"]["enabled"] and feature not in profiles["disabled"]["paths"]
assert not profiles["retained"]["enabled"] and feature in profiles["retained"]["paths"]


def reset(profile):
    # The production initrd runs before the reset volume and its children mount.
    machine.succeed("if mountpoint -q /fixture; then umount -R /fixture; fi")
    machine.succeed(
        f"IMMUTABILITY_MOUNT_PATH={top} immutability --assume-mounted -- "
        f"/dev/vdb @snapshots CLEAN reset @persist "
        f"/etc/persistence-features/{profile}.toml @fixture=/fixture"
    )
    machine.succeed("mount -t btrfs -o subvol=@fixture /dev/vdb /fixture")
    machine.succeed(
        f"IMMUTABILITY_MOUNT_PATH={top} immutability --assume-mounted -- "
        f"mount /dev/vdb @persist /etc/persistence-features/{profile}.toml"
    )


machine.succeed(
    f"mkfs.btrfs -f /dev/vdb && mkdir -p {top} /fixture && "
    f"mount -t btrfs -o subvolid=5 /dev/vdb {top} && "
    f"btrfs subvolume create {top}/@fixture && "
    f"btrfs subvolume create {top}/@snapshots && "
    f"mkdir -p {top}/@snapshots/@fixture "
    f"{top}/@fixture/user/.config/rclone {top}/@fixture/shared && "
    f"btrfs subvolume snapshot -r {top}/@fixture {top}/@snapshots/@fixture/CLEAN && "
    f"printf original-rclone-state > {top}/@fixture/user/.config/rclone/token && "
    f"printf host-owned-state > {top}/@fixture/shared/token"
)

with subtest("enabled feature and host contributions persist together"):
    reset("enabled")
    machine.succeed(f"mountpoint -q {feature}")
    assert machine.succeed(f"cat {feature}/token") == "original-rclone-state"
    machine.succeed(f"printf updated-rclone-state > {feature}/token")
    reset("enabled")
    assert machine.succeed(f"cat {feature}/token") == "updated-rclone-state"
    assert machine.succeed("cat /fixture/shared/token") == "host-owned-state"

with subtest("disabling the feature quarantines its state and keeps host state"):
    reset("disabled")
    machine.fail(f"mountpoint -q {feature}")
    machine.fail(f"test -e {feature}/token")
    machine.fail(f"test -e {store}")
    assert machine.succeed("cat /fixture/shared/token") == "host-owned-state"
    orphan = machine.succeed(
        f"find {top}/@persist/.orphans -mindepth 1 -maxdepth 1 -name '{key}-*'"
    ).strip()
    assert orphan and "\n" not in orphan, f"expected one quarantined directory: {orphan!r}"
    assert machine.succeed(f"cat {shlex.quote(orphan)}/token") == "updated-rclone-state"

with subtest("re-enabling starts fresh and preserves the recoverable orphan"):
    reset("enabled")
    machine.succeed(f"mountpoint -q {feature}")
    machine.fail(f"test -e {feature}/token")
    assert machine.succeed(f"cat {shlex.quote(orphan)}/token") == "updated-rclone-state"

with subtest("explicit recovery restores the quarantined directory"):
    machine.succeed(
        f"umount {feature} && btrfs subvolume delete {store} && "
        f"mv {shlex.quote(orphan)} {store}"
    )
    machine.succeed(
        f"IMMUTABILITY_MOUNT_PATH={top} immutability --assume-mounted -- "
        "mount /dev/vdb @persist /etc/persistence-features/enabled.toml"
    )
    assert machine.succeed(f"cat {feature}/token") == "updated-rclone-state"
    reset("enabled")
    assert machine.succeed(f"cat {feature}/token") == "updated-rclone-state"

with subtest("a host retains feature data after disabling the package"):
    reset("retained")
    machine.succeed(f"mountpoint -q {feature}")
    assert machine.succeed(f"cat {feature}/token") == "updated-rclone-state"
    assert machine.succeed("cat /fixture/shared/token") == "host-owned-state"
    assert machine.succeed(f"find {top}/@persist/.orphans -mindepth 1 -maxdepth 1").strip() == ""
