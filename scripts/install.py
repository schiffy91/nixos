#!/usr/bin/env python3
from utils import Utils, Config, Shell, Interactive

class Installer:
    sh = Shell()
    @classmethod
    def install_nixos(cls):
        host = Config.get_host()
        target = Config.get_host()
        mount_point = cls.get_mount_point()
        source = cls.get_install_path()
        destination = {mount_point}/{source}
        flake_arg = f"--flake {destination}#{host}-{target}"
        root_arg = f"--root {mount_point}"
        options = "--no-channel-copy --show-trace --no-root-password --cores 0"
        cmd = f"nixos-install {flake_arg} {root_arg} {options}"
        tmp = f"{mount_point}/nix/tmp"
        env = f"TMPDIR={tmp}"
        cls.sh.cpdir(source, destination)
        cls.sh.run(cmd=cmd, env=env, capture_output=False)
        with cls.sh.chroot(mount_point):
            cls.sh.symlink(source, Config.get_nixos_path())
            Config.secure(cls.get_username(), sh=cls.sh)
        cls.sh.rm(tmp)
    @classmethod
    def run_disko(cls, mode):
        disko_key = "github:nix-community/disko/"
        version = Utils.get_value_from_path(Config.get_flake_path(), key=disko_key, start=disko_key, end='"')
        command = f"nix --extra-experimental-features \"nix-command flakes\" run github:nix-community/disko/{version} --verbose -- " \
                f"--show-trace --flake {Config.get_nixos_path()}#{Config.get_host()}-mount --mode {mode}"
        return cls.sh.run(command, capture_output=False)
    # Readonly
    @classmethod
    def get_mount_point(cls): return "/mnt"
    @classmethod
    def get_username(cls): return Utils.get_value_from_variables("admin")
    @classmethod
    def get_install_path(cls): return f"/home/{cls.get_username()}/nixos"
    @classmethod
    def get_disk_path(cls): return Utils.get_value_from_path(Config.get_host_path(), "variables.disk.device")
    @classmethod
    def get_plain_text_password_path(cls): return Utils.get_value_from_variables("tmpPasswordFile")
    @classmethod
    def disk_mount(cls): return cls.run_disko("mount")
    @classmethod
    def disk_erase_and_mount(cls): return cls.run_disko("destroy,format,mount")

def main():
    Utils.require_root()
    Config.reset_config(Interactive.ask_for_host_path(), "standard") # Create config.json based on the selected host
    Config.reset_secrets(plain_text_password_path=Installer.get_plain_text_password_path()) # Setup passwords for encryption
    if Interactive.confirm(f"Format {Installer.get_disk_path()}?"): Installer.disk_erase_and_mount() # Ask whether to erase and mount disk
    else: Installer.disk_mount() # Otherwise just mount the disk
    if Interactive.confirm("Install nixos?"): Installer.install_nixos() # Install NixOS
    Interactive.ask_to_reboot()

if __name__=="__main__": main()
