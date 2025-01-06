#!/usr/bin/env python3
import sys
from utils import Utils, Config, Shell

sh = Shell(root_required=True)

def tpm2_exists(): return sh.exists(Config.get_tpm_device()) and int(sh.file_read(Config.get_tpm_version_path())) == 2

def get_enrolled_tpm2_devices(): return Utils.stdout(sh.run("systemd-cryptenroll --tpm2-device=list", check=False))


def data_disk_encrypted(): return sh.run(f"cryptsetup isLuks {Config.get_data_disk_path()}", check=False).returncode == 0

def enroll_tpm2():
    pcrs = "0+2+7+12"  # 0: Firmware, 2: Extended code, 7: SecureBoot, 12: Kernel config
    return sh.run(f"systemd-cryptenroll {Config.get_data_disk_path()} --wipe-slot=tpm2 --tpm2-device={Config.get_tpm_device()} --tpm2-pcrs={pcrs}", capture_output=False, check=False).returncode == 0

def enable_tpm2():
    if not tpm2_exists(): return Utils.abort("TPM2 does not exist.")
    if not data_disk_encrypted(): return Utils.abort(f"{Config.get_data_disk_path} isn't encrypted with LUKS.")
    if not enroll_tpm2(): Utils.abort("Enrolling TPM2 failed")
    Utils.log("Successfully enrolled TPM2")
    Utils.log("Please check that TPM2 performs automatic disk decryption after reboot")

def disable_tpm2():
    if not tpm2_exists(): return Utils.abort("TPM2 does not exist.")
    if sh.run(f"systemd-cryptenroll {Config.get_data_disk_path()} --wipe-slot=tpm2", capture_output=False, check=False).returncode != 0: Utils.abort("Failed removing TPM2 enrollment")

def main():
    operation = None
    match Utils.parse_args(sys.argv[1:], "enable", "disable"):
        case ["enable"]:
            operation = enable_tpm2
        case ["disable"]:
            operation = disable_tpm2
        case _:
            return Utils.abort("Usage: tpm2.py (enable | disable)")
    try:
        operation()
    except BaseException as exception:
        Utils.log_error(f"Caught exception: {exception}.")
        raise

if __name__ == "__main__":
    main()
