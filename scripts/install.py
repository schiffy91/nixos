#!/usr/bin/env python3
from utils import Utils, Config, Shell, Interactive

class Installer:
    sh = Shell()
    @classmethod
    def install_nixos(cls):
        # Paths
        sh = cls.sh
        remote_root = cls.get_remote_root_path()
        nixos_path = cls.get_nixos_path() # ~/nixos
        remote_nixos_path = cls.get_nixos_path(remote=True) # ~/mnt/nixos
        install_path = cls.get_install_path() # /etc/nixos
        remote_install_path = cls.get_install_path(remote=True) # /mnt/etc/nixos
        remote_tmp_path = cls.get_nix_tmp_path(remote=True) # /mnt/nix/tmp
        username = cls.get_username() # alexanderschiffhauer
        # nixos-install args
        host = Config.get_host()
        target = Config.get_target()
        env = f"TMPDIR={remote_tmp_path}"
        flake_arg = f"--flake {remote_install_path}#{host}-{target}"
        root_arg = f"--root {remote_root}"
        options = "--no-channel-copy --show-trace --no-root-password --cores 0"
        cmd = f"nixos-install {flake_arg} {root_arg} {options}"
        # Prepare nixos-install
        sh.rm(remote_nixos_path, remote_install_path) # Remove up destinations
        sh.mkdir(cls.get_etc_path(remote=True), cls.get_store_path(remote=True), remote_tmp_path) # Ensure dependent directories exist
        sh.cpdir(install_path, remote_install_path) # e.g. cp -R /etc/nixos /mnt/etc/nixos
        # nixos-install
        sh.run(cmd=cmd, env=env, capture_output=False)
        # Symlink and permission within chroot
        with sh.chroot(remote_root):
            sh.mv(install_path, nixos_path) # Move nixos to home directory
            Config.secure(username, sh) # Pass in sh for chroot
            sh.symlink(nixos_path, install_path) # Smylink ~/nixos to e.g. /etc/nixos
        # Cleanup
        sh.rm(f"{remote_tmp_path}")
    @classmethod
    def run_disko(cls, mode):
        version = cls.sh.file_read_string_between(Config.get_flake_path(), start="github:nix-community/disko/", end='";')
        command = f"nix --extra-experimental-features \"nix-command flakes\" run github:nix-community/disko/{version} --verbose -- " \
                f"--show-trace --flake {Config.get_nixos_path()}#{Config.get_host()}-mount --mode {mode}"
        return cls.sh.run(command, capture_output=False)
    # Readonly
    @classmethod
    def get_remote_root_path(cls): return "/mnt"
    @classmethod
    def get_install_path(cls, remote=False): return f"{cls.get_remote_root_path()}{Config.get_nixos_path()}" if remote else Config.get_nixos_path()
    @classmethod
    def get_etc_path(cls, remote=False): return f"{cls.get_remote_root_path()}/etc" if remote else "/etc"
    @classmethod
    def get_store_path(cls, remote=False): return f"{cls.get_remote_root_path()}/nix/store" if remote else "/nix/store"
    @classmethod
    def get_nix_tmp_path(cls, remote=False): return f"{cls.get_remote_root_path()}/nix/tmp" if remote else "/nix/tmp"
    @classmethod
    def get_home_path(cls, remote=False): return f"{cls.get_remote_root_path()}/home" if remote else "/home"
    @classmethod
    def get_user_path(cls, remote=False): return f"{cls.get_remote_root_path()}/home/{cls.get_username()}" if remote else f"/home/{cls.get_username()}"
    @classmethod
    def get_nixos_path(cls, remote=False): return f"{cls.get_remote_root_path()}{cls.get_user_path()}/nixos" if remote else f"{cls.get_user_path()}/nixos"
    @classmethod
    def get_disk_path(cls): return Utils.get_config_value_from_file(Config.get_host_path(), key="config.variables.disk.device")
    @classmethod
    def get_plain_text_password_path(cls): return Utils.get_default_option_value_from_variables("tmpPasswordFile")
    @classmethod
    def get_username(cls): return Utils.get_default_option_value_from_variables("admin")
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
