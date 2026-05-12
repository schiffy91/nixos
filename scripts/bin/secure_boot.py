#! /usr/bin/env nix-shell
#! nix-shell -i python3 -p python3 -p sbctl
import json, sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
from lib import Config, Shell, Utils

sh = Shell(root_required=True)

def remove_old_efi_entries():
    sh.mkdir("/boot/EFI/Linux", "/var/lib/sbctl")
    for path in sh.find_files("/boot/EFI/Linux", pattern="*/linux-*.efi"):
        sh.rm(path)
    sh.rm("/etc/secureboot")

def create_keys():
    Utils.log("Creating Secure Boot keys...")
    sh.run("sbctl create-keys", check=False)

def enroll_keys(microsoft=False):
    Utils.log("Enrolling Secure Boot keys...")
    flag = "--microsoft" if microsoft else "--yes-this-might-brick-my-machine"
    sh.run(f"sbctl enroll-keys {flag}")
    Utils.log("Secure Boot keys enrolled successfully")

def verify():
    output = Shell.stdout(sh.run("sbctl verify --json", check=False)) or "null"
    result = json.loads(output)
    if not result: return Utils.log_error("No EFI binaries registered")
    unsigned = [f for f, info in result.items() if not info.get("is_signed")]
    if not unsigned: return Utils.log("All EFI binaries are signed")
    for f in unsigned: Utils.log_error(f"NOT signed: {f}")

def status():
    sh.run("sbctl status", capture_output=False, check=False)
    sh.run("sbctl verify", capture_output=False, check=False)

def disable_secure_boot():
    remove_old_efi_entries()
    Config.set_target(Config.get_standard_flake_target())
    Config.update(rebuild_file_system=True, delete_cache=True)

def enable_secure_boot(microsoft=False):
    remove_old_efi_entries()
    create_keys()
    enroll_keys(microsoft=microsoft)
    Config.set_target(Config.get_secure_boot_flake_target())
    Config.update(rebuild_file_system=True, delete_cache=True)
    verify()

def main():
    args = Utils.parse_args({
        "enable": ["--microsoft"], "disable": [], "status": []})
    if args.command == "enable": enable_secure_boot(microsoft=args.microsoft)
    elif args.command == "disable": disable_secure_boot()
    elif args.command == "status": status()

if __name__ == "__main__": main()
