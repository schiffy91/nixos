#!/usr/bin/env python3
from nixos_utils import *

def main():
    Utils.require_root()
    NixOSConfig.reset_config(Interactive.ask_for_host_path(), "standard") # Create config.json based on the selected host
    NixOSConfig.reset_secrets(plain_text_password_path=NixOSInstaller.get_plain_text_password_path()) # Setup passwords for encryption
    if Interactive.confirm(f"Format {NixOSInstaller.get_disk_path()}?"): NixOSInstaller.disk_erase_and_mount() # Ask whether to erase and mount disk
    else: NixOSInstaller.disk_mount() # Otherwise just mount the disk
    if Interactive.confirm("Install nixos?"): NixOSInstaller.install_nixos() # Install NixOS
    Interactive.ask_to_reboot()

if __name__=="__main__": main()
