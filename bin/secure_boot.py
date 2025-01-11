#! /usr/bin/env nix-shell
#! nix-shell -i python3 -p python3 -I nixpkgs=https://github.com/NixOS/nixpkgs/archive/93dc9803a1ee435e590b02cde9589038d5cc3a4e.tar.gz -p sbctl
import sys
from nixos import Utils, Config, Shell

sh = Shell(root_required=True)

def remove_old_efi_entries():
    sh.mkdir("/boot/EFI/Linux", "/var/lib/sbctl")
    sh.rm("/boot/EFI/Linux/linux-*.efi")

def create_keys():
    Utils.log("Creating Secure Boot keys...")
    sh.run("sbctl create-keys")

def are_keys_enrolled():
    status = sh.run("sbctl status")
    return "secure boot: ✓ enabled" in Shell.stdout(status).lower()

def enroll_keys():
    Utils.log("Resetting Secure Boot keys...")
    sh.run("sbctl reset", check=False)
    Utils.log("Enrolling Secure Boot keys...")
    sh.run("sbctl enroll-keys --microsoft")
    Utils.log("Secure Boot keys enrolled successfully")

def are_keys_signed():
    status = sh.run("sbctl verify")
    return "is signed" in Shell.stdout(status).lower()

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
    operation = None
    match Utils.parse_args(sys.argv[1:], "enable", "disable"):
        case ["enable"]:
            operation = enable_secure_boot
        case ["disable"]:
            operation = disable_secure_boot
        case _:
            return Utils.abort("Usage: secure_boot.py (enable | disable)")
    try:
        operation()
    except BaseException as exception:
        Utils.log_error(f"Caught exception: {exception}.")
        Utils.log_error("Disabling Secure Boot.")
        disable_secure_boot()
        raise

if __name__ == "__main__": main()
