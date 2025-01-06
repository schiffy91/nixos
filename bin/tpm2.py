#!/usr/bin/env python3
import sys
from utils import Utils, Config, Shell

sh = Shell(root_required=True)

def is_device_enrolled(): return Config.get_data_disk_path() in Utils.stdout(sh.run("systemd-cryptenroll --tpm2-device=list", check=False))

def validate_tpm2_exists():
    if not sh.exists(Utils.get_value_from_variables("variables.disk.tpm.device")): return Utils.abort("TPM2 device does not exist.")
    if int(sh.file_read(Utils.get_value_from_variables("variables.disk.tpm.versionMajorPath"))) != 2: return Utils.abort("TPM device is not version 2")

def validate_tpm2_unenrolled():
    if Config.get_data_disk_path() in Utils.stdout(sh.run("systemd-cryptenroll --tpm2-device=list", check=False)): return Utils.abort(f"An existing TPM2 is already enrolled to decrypt {Config.get_data_disk_path()}")

def validate_luks_partition():
    if sh.run(f"cryptsetup isLuks {Config.get_data_disk_path()}", check=False).returncode != 0: return Utils.abort(f"{Config.get_data_disk_path()} is not encrypted with LUKS")

def enroll_tpm2():
    pcrs = "0+2+7+12"  # 0: Firmware, 2: Extended code, 7: SecureBoot, 12: Kernel config
    tpm_device = Utils.get_value_from_variables("variables.disk.tpm.device")
    data_disk_path = Config.get_data_disk_path()
    if sh.run(f"systemd-cryptenroll --tpm2-device={tpm_device} --tpm2-pcrs={pcrs} --wipe-slot=tpm2 {data_disk_path}", capture_output=False, check=False).returncode != 0: Utils.abort("Enrolling TPM2 failed")
    Utils.log("Successfully enrolled TPM2")
    Utils.log("Please check that TPM2 performs automatic disk decryption after reboot")

def enable_tpm2():
    validate_tpm2_exists()
    validate_tpm2_unenrolled()
    validate_luks_partition()
    enroll_tpm2()

def disable_tpm2():
    if not is_device_enrolled(): return Utils.abort("TPM2 is not enrolled")
    if sh.run(f"systemd-cryptenroll --wipe-slot=tpm2 {Config.get_data_disk_path()}", capture_output=False, check=False).returncode != 0: Utils.abort("Failed removing TPM2 enrollment")

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
