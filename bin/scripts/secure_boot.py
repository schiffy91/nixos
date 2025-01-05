#!/usr/bin/env python3
import sys
from utils import Utils, Config, Shell

sh = Shell(root_required=True)

def parse_args():
    if len(sys.argv) != 2 or sys.argv[1] not in ["--enable", "--disable"]:
        Utils.log("Usage: nixos_secure_boot.py [--enable|--disable]")
        return Utils.abort()
    return sys.argv[1]

def remove_old_efi_entries():
    sh.mkdir("/boot/EFI/Linux", "/var/lib/sbctl")
    sh.rm("/boot/EFI/Linux/linux-*.efi")

def create_keys():
    Utils.log("Creating Secure Boot keys...")
    sh.run("sbctl create-keys")

def are_keys_enrolled():
    status = sh.run("sbctl status")
    return "secure boot: ✓ enabled" in Utils.stdout(status).lower()

def enroll_keys():
    Utils.log("Enrolling Secure Boot keys...")
    sh.run("sbctl enroll-keys --microsoft")
    Utils.log("Secure Boot keys enrolled successfully")

def are_keys_signed():
    status = sh.run("sbctl verify")
    return "is signed" in Utils.stdout(status).lower()

def require_signed_boot_loader():
    if are_keys_signed():
        Utils.log("Successfully created and enrolled Secure Boot keys and switched boot loader")
        Utils.log("Please check that Secure Boot in enabled your firmware after reboot")
    else:
        Utils.log("No EFI stub found after rebuild")
        disable_secure_boot()

def disable_secure_boot():
    remove_old_efi_entries()
    Config.set_target(Config.get_standard_flake_target())
    return Config.update(rebuild_file_system=True)

def enable_secure_boot():
    remove_old_efi_entries()
    create_keys()
    if not are_keys_enrolled():
        enroll_keys()
    Config.set_target(Config.get_secure_boot_flake_target())
    Config.update(rebuild_file_system=True)
    require_signed_boot_loader()

def main():
    try:
        match parse_args():
            case "--enable":
                return enable_secure_boot()
            case "--disable":
                return disable_secure_boot()
            case _:
                return Utils.abort()
    except BaseException:
        disable_secure_boot()

if __name__ == "__main__": main()
