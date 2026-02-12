#! /usr/bin/env nix-shell
#! nix-shell -i python3 -p python3
import argparse, getpass
from nixos import Utils, Config, Shell

sh = Shell(root_required=True)

def ask_for_old_password():
    return getpass.getpass("Enter current LUKS password: ")

def ask_for_new_password():
    while True:
        password = getpass.getpass("Enter new password: ")
        if password == getpass.getpass("Confirm new password: "): return password
        Utils.log_error("Passwords do not match.")

def change_luks_password(old_password, new_password):
    Utils.log("Changing LUKS encryption password...")
    result = sh.run(f"echo -e '{old_password}\\n{new_password}' | cryptsetup luksChangeKey {Config.get_disk_by_part_label_root()}", sensitive=old_password, check=False)
    if result.returncode != 0: Utils.abort("Failed to change LUKS password. Check your current password and try again.")
    Utils.log("Successfully changed LUKS encryption password")

def change_user_password(password):
    Utils.log("Changing user account password...")
    hashed_password = Shell.stdout(sh.run(f"mkpasswd -m sha-512 '{password}'", sensitive=password))
    sh.file_write(Config.get_hashed_password_path(), hashed_password, sensitive=hashed_password)
    Utils.log("Rebuilding system with new user password...")
    Config.update(rebuild_file_system=False, reboot=False, delete_cache=False, upgrade=False)
    Utils.log("Successfully changed user account password")

def update_tpm2():
    Utils.log("Re-enrolling TPM2 with new LUKS password...")
    result = sh.run(f"{Config.get_nixos_path()}/bin/tpm2.py --enable", capture_output=False, check=False)
    if result.returncode != 0: Utils.log_error("TPM2 enrollment failed. You can manually re-enroll later with: sudo /etc/nixos/bin/tpm2.py --enable")
    else: Utils.log("Successfully re-enrolled TPM2")

def main():
    parser = argparse.ArgumentParser(description="Change LUKS encryption and/or user account passwords")
    parser.add_argument("--full-disk-encryption-only", action="store_true", help="Only change LUKS encryption password")
    parser.add_argument("--user-account-only", action="store_true", help="Only change user account password")
    parser.add_argument("--update-tpm2", action="store_true", help="Re-enroll TPM2 after changing LUKS password")
    args = parser.parse_args()

    if args.full_disk_encryption_only and args.user_account_only:
        Utils.abort("Cannot use both --full-disk-encryption-only and --user-account-only")

    change_fde = not args.user_account_only
    change_user = not args.full_disk_encryption_only

    if change_fde and change_user:
        old_password = ask_for_old_password()
        new_password = ask_for_new_password()
        change_luks_password(old_password, new_password)
        change_user_password(new_password)
        if args.update_tpm2: update_tpm2()
    elif change_fde:
        old_password = ask_for_old_password()
        new_password = ask_for_new_password()
        change_luks_password(old_password, new_password)
        if args.update_tpm2: update_tpm2()
    elif change_user:
        new_password = ask_for_new_password()
        change_user_password(new_password)

    Utils.log("Password change complete!")

if __name__ == "__main__": main()
