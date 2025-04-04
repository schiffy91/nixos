#! /usr/bin/env nix-shell
#! nix-shell -i python3 -p python3
# cd / && sudo rm -rf /etc/nixos && sudo git clone https://github.com/schiffy91/nixos.git /etc/nixos && sudo /etc/nixos/bin/install.py
import argparse
from nixos import Utils, Config, Shell, Snapshot, Interactive

class Installer:
    sh = Shell()
    @classmethod
    def install_nixos(cls):
        cls.sh.cpdir(Config.get_nixos_path(), f"{cls.get_mount_point()}{Config.get_nixos_path()}")
        cmd = f"nixos-install --flake {cls.get_mount_point()}{Config.get_nixos_path()}#{Config.get_host()}-{Config.get_target()} --root {cls.get_mount_point()} --no-channel-copy --show-trace --no-root-password --cores 0"
        tmp = f"{cls.get_mount_point()}/nix/tmp"
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
        command = f"nix --extra-experimental-features nix-command --extra-experimental-features flakes run github:nix-community/disko/{Config.metadata('disko')['locked']['rev']} --verbose -- " \
                f"--show-trace --flake {Config.get_nixos_path()}#{Config.get_host()}-{Config.get_disk_operation_target()} --mode {mode} --root-mountpoint {cls.get_mount_point()} {args}"
        return cls.sh.run(command, capture_output=False)
    # Helpers
    @classmethod
    def parse_args(cls):
        parser = argparse.ArgumentParser()
        parser.add_argument("--collect-garbage", action="store_true", help="Collects Nix Store garbage")
        parser.add_argument("--debug", action="store_true", help="Opens the NixOS repository with VSCodium and Python debugging")
        args = parser.parse_args()
        if args.collect_garbage: cls.sh.run("nix-collect-garbage -d")
        if args.debug:
            vscodium_cmd = "nix --extra-experimental-features nix-command --extra-experimental-features flakes run nixpkgs#vscodium -- --no-sandbox --user-data-dir /tmp/vscodium-data"
            cls.sh.run(f"{vscodium_cmd} --install-extension ms-python.python")
            cls.sh.run(f"{vscodium_cmd} --install-extension rogalmic.bash-debug")
            cls.sh.run(f"{vscodium_cmd} {Config.get_nixos_path()}")
            return Utils.abort("Please continue in VSCodium")
    @classmethod
    def get_mount_point(cls): return "/mnt"
    @classmethod
    def get_username(cls): return Config.eval("config.settings.user.admin.username")
    @classmethod
    def get_installation_disk(cls): return Config.eval("config.settings.disk.device")
    @classmethod
    def get_plain_text_password_path(cls): return Config.eval("config.settings.disk.encryption.plainTextPasswordFile") if Config.eval("config.settings.disk.encryption.enable") else None
    @classmethod
    def mount_disk(cls): return cls.run_disko("mount")
    @classmethod
    def erase_and_mount_disk(cls): return cls.run_disko("destroy,format,mount", "--yes-wipe-all-disks")

def main():
    Utils.require_root()
    Installer.parse_args()
    if Installer.sh.exists(Config.get_config_path()): Utils.print(f"Found {Config.get_config_path()}")
    else: Config.reset_config(Interactive.ask_for_host_path(), Config.get_standard_flake_target()) # Create config.json based on the selected host
    Config.create_secrets(plain_text_password_path=Installer.get_plain_text_password_path()) # Create all secrets
    if Interactive.confirm(f"Format {Installer.get_installation_disk()}?"): Installer.erase_and_mount_disk() # Format disk
    else: Installer.mount_disk() # Or just mount disk
    if Interactive.confirm("Install NixOS?"): Installer.install_nixos() # Install NixOS
    elif Interactive.confirm("Permission NixOS?"): Installer.permission_nixos() # Permission NixOS if installation is skipped
    Interactive.ask_to_reboot()

if __name__=="__main__": main()
