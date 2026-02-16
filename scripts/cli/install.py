#! /usr/bin/env nix-shell
#! nix-shell -i python3 -p python3
import argparse

from core import Utils, Config, Shell, Snapshot, Interactive


class Installer:
    sh = Shell()

    @classmethod
    def install_nixos(cls):
        mount = cls.get_mount_point()
        nixos = Config.get_nixos_path()
        host = Config.get_host()
        target = Config.get_target()
        cls.sh.cpdir(nixos, f"{mount}{nixos}")
        cmd = (
            f"nixos-install --flake {mount}{nixos}#{host}-{target} "
            f"--root {mount} --no-channel-copy --show-trace "
            f"--no-root-password --cores 0"
        )
        tmp = f"{mount}/nix/tmp"
        cls.sh.run(cmd=cmd, env=f"TMPDIR={tmp}", capture_output=False)
        cls.sh.rm(tmp)
        cls.permission_nixos()

    @classmethod
    def permission_nixos(cls):
        with cls.sh.chroot(cls.get_mount_point()):
            Config.secure(cls.get_username())
            Snapshot.create_initial_snapshots()

    @classmethod
    def run_disko(cls, mode, args=""):
        rev = Config.metadata("disko")["locked"]["rev"]
        nixos = Config.get_nixos_path()
        host = Config.get_host()
        target = Config.get_disk_operation_target()
        mount = cls.get_mount_point()
        command = (
            f"nix --extra-experimental-features nix-command "
            f"--extra-experimental-features flakes run "
            f"github:nix-community/disko/{rev} --verbose -- "
            f"--show-trace --flake {nixos}#{host}-{target} "
            f"--mode {mode} --root-mountpoint {mount} {args}"
        )
        return cls.sh.run(command, capture_output=False)

    @classmethod
    def parse_args(cls):
        parser = argparse.ArgumentParser()
        parser.add_argument("--collect-garbage", action="store_true")
        parser.add_argument("--debug", action="store_true")
        args = parser.parse_args()
        if args.collect_garbage:
            cls.sh.run("nix-collect-garbage -d")
        if args.debug:
            vscodium = (
                "nix --extra-experimental-features nix-command "
                "--extra-experimental-features flakes run nixpkgs#vscodium "
                "-- --no-sandbox --user-data-dir /tmp/vscodium-data"
            )
            cls.sh.run(f"{vscodium} --install-extension ms-python.python")
            cls.sh.run(f"{vscodium} --install-extension rogalmic.bash-debug")
            cls.sh.run(f"{vscodium} {Config.get_nixos_path()}")
            return Utils.abort("Please continue in VSCodium")

    @classmethod
    def get_mount_point(cls):
        return "/mnt"

    @classmethod
    def get_username(cls):
        return Config.eval("config.settings.user.admin.username")

    @classmethod
    def get_installation_disk(cls):
        return Config.eval("config.settings.disk.device")

    @classmethod
    def get_plain_text_password_path(cls):
        if Config.eval("config.settings.disk.encryption.enable"):
            return Config.eval(
                "config.settings.disk.encryption.plainTextPasswordFile"
            )
        return None

    @classmethod
    def mount_disk(cls):
        return cls.run_disko("mount")

    @classmethod
    def erase_and_mount_disk(cls):
        return cls.run_disko("destroy,format,mount", "--yes-wipe-all-disks")


def main():
    Utils.require_root()
    Installer.parse_args()
    if Installer.sh.exists(Config.get_config_path()):
        Utils.print(f"Found {Config.get_config_path()}")
    else:
        Config.reset_config(
            Interactive.ask_for_host_path(Config.get_hosts_path()),
            Config.get_standard_flake_target(),
        )
    Config.create_secrets(
        plain_text_password_path=Installer.get_plain_text_password_path()
    )
    if Interactive.confirm(f"Format {Installer.get_installation_disk()}?"):
        Installer.erase_and_mount_disk()
    else:
        Installer.mount_disk()
    if Interactive.confirm("Install NixOS?"):
        Installer.install_nixos()
    elif Interactive.confirm("Permission NixOS?"):
        Installer.permission_nixos()
    Interactive.ask_to_reboot()


if __name__ == "__main__":
    main()
