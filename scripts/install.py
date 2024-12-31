#!/usr/bin/env python3
from nixos_utils import *

def main():
    Utils.require_root()
    Config.reset_config(Interactive.ask_for_host_path(), "standard") # Create config.json based on the selected host
    Config.reset_secrets(plain_text_password_path=Installer.get_plain_text_password_path()) # Setup passwords for encryption
    if Interactive.confirm(f"Format {Installer.get_disk_path()}?"): Installer.disk_erase_and_mount() # Ask whether to erase and mount disk
    else: Installer.disk_mount() # Otherwise just mount the disk
    if Interactive.confirm("Install nixos?"): Installer.install_nixos() # Install NixOS
    Interactive.ask_to_reboot()

if __name__=="__main__": main()
