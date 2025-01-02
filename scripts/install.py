#!/usr/bin/env python3
from utils import Utils, Config, Shell, Interactive

class Installer:
    sh = Shell()
    @classmethod
    def install_nixos(cls):
        cls.sh.cpdir(Config.get_nixos_path(), f"{cls.get_mount_point()}{Config.get_nixos_path()}")
        cmd = f"nixos-install --flake {cls.get_mount_point()}{Config.get_nixos_path()}#{Config.get_host()}-{Config.get_target()} --root {cls.get_mount_point()} --no-channel-copy --show-trace --no-root-password --cores 0"
        tmp = f"{cls.get_mount_point()}/nix/tmp"
        cls.sh.run(cmd=cmd, env=f"TMPDIR={tmp}", capture_output=False)
        with cls.sh.chroot(cls.get_mount_point()):
            #cls.sh.mv(Config.get_nixos_path(), cls.get_final_nixos_path())
            #cls.sh.symlink(cls.get_final_nixos_path(), Config.get_nixos_path())
            Config.secure(cls.get_username(), sh=cls.sh)
        cls.sh.rm(tmp)
    @classmethod
    def run_disko(cls, mode, args=""):
        disko_key = "github:nix-community/disko/"
        version = Utils.get_value_from_path(Config.get_flake_path(), key=disko_key, start=disko_key, end='"')
        command = f"nix --extra-experimental-features \"nix-command flakes\" run github:nix-community/disko/{version} --verbose -- " \
                f"--show-trace --flake {Config.get_nixos_path()}#{Config.get_host()}-{Config.get_disk_operation_target()} --mode {mode} --root-mountpoint {cls.get_mount_point()} {args}"
        return cls.sh.run(command, capture_output=False)
    # Helpers
    @classmethod
    def get_mount_point(cls): return "/mnt"
    @classmethod
    def get_username(cls): return Utils.get_value_from_variables("admin")
    @classmethod
    def get_final_nixos_path(cls): return f"/home/{cls.get_username()}/nixos"
    @classmethod
    def get_installation_disk(cls): return Utils.get_value_from_path(Config.get_host_path(), "variables.disk.device")
    @classmethod
    def get_plain_text_password_path(cls): return Utils.get_value_from_variables("tmpPasswordFile")
    @classmethod
    def get_required_secrets(cls): return [ Config.get_secrets_path(), Config.get_hashed_password_path, cls.get_plain_text_password_path() ]
    @classmethod
    def required_secrets_are_valid(cls): return Utils.encrypt_password(cls.sh.file_read(Installer.get_plain_text_password_path())) == cls.sh.file_read(Config.get_hashed_password_path())
    @classmethod
    def mount_disk(cls): return cls.run_disko("mount")
    @classmethod
    def erase_and_mount_disk(cls): return cls.run_disko("destroy,format,mount", "--yes-wipe-all-disks")

def main():
    Utils.require_root()
    if Installer.sh.exists(Config.get_config_path()): Utils.print(f"Found {Config.get_config_path()}")
    else: Config.reset_config(Interactive.ask_for_host_path(), Config.get_standard_flake_target()) # Create config.json based on the selected host
    if Installer.sh.exists(*Installer.get_required_secrets()) and Installer.required_secrets_are_valid(): Utils.print(f"Found {" ".join(Installer.get_required_secrets())}")
    else: Config.reset_secrets(plain_text_password_path=Installer.get_plain_text_password_path()) # Setup passwords for encryption
    if Interactive.confirm(f"Format {Installer.get_installation_disk()}?"): Installer.erase_and_mount_disk() # Format disk
    else: Installer.mount_disk() # Or just mount disk
    if Interactive.confirm("Install nixos?"): Installer.install_nixos() # Install NixOS
    Interactive.ask_to_reboot()

if __name__=="__main__": main()
