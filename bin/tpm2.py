#! /usr/bin/env nix-shell
#! nix-shell -i python3 -p python3
import sys
from nixos import Utils, Config, Shell

sh = Shell(root_required=True)

def tpm2_exists(): return sh.exists(Config.get_tpm_device()) and int(sh.file_read(Config.get_tpm_version_path())) == 2

def get_enrolled_tpm2_devices(): return Shell.stdout(sh.run("systemd-cryptenroll --tpm2-device=list", check=False))

def disk_encrypted(): return sh.run(f"cryptsetup isLuks {Config.get_disk_by_part_label_root()}", check=False).returncode == 0

def enroll_tpm2():
    pcrs = "0+2+7+12"  # 0: Firmware, 2: Extended code, 7: SecureBoot, 12: Kernel config
    return sh.run(f"systemd-cryptenroll {Config.get_disk_by_part_label_root()} --wipe-slot=tpm2 --tpm2-device={Config.get_tpm_device()} --tpm2-pcrs={pcrs}", capture_output=False, check=False).returncode == 0

def enable_tpm2():
    if not tpm2_exists(): return Utils.abort("TPM2 does not exist.")
    if not disk_encrypted(): return Utils.abort(f"{Config.get_disk_by_part_label_root()} isn't encrypted with LUKS.")
    if not enroll_tpm2(): Utils.abort("Enrolling TPM2 failed")
    Utils.log("Successfully enrolled TPM2")
    Utils.log("Please check that TPM2 performs automatic disk decryption after reboot")

def disable_tpm2():
    if not tpm2_exists(): return Utils.abort("TPM2 does not exist.")
    if sh.run(f"systemd-cryptenroll {Config.get_disk_by_part_label_root()} --wipe-slot=tpm2", capture_output=False, check=False).returncode != 0: Utils.abort("Failed removing TPM2 enrollment")

def main(): Utils.toggle(sys.argv, on_enable=enable_tpm2, on_disable=disable_tpm2, on_exception=disable_tpm2)

if __name__ == "__main__": main()
