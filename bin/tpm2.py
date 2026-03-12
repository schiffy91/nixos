#! /usr/bin/env nix-shell
#! nix-shell -i python3 -p python3
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
from lib import Config, Shell, Utils

sh = Shell(root_required=True)

PCRS = "7+12"

def require_tpm2():
    if not (sh.exists(Config.get_tpm_device())
            and int(sh.file_read(Config.get_tpm_version_path())) == 2):
        Utils.abort("TPM2 device not found")

def require_luks():
    root = Config.get_disk_by_part_label_root()
    if sh.run(f"cryptsetup isLuks {root}", check=False).returncode != 0:
        Utils.abort(f"{root} is not LUKS encrypted")

def enroll():
    root = Config.get_disk_by_part_label_root()
    device = Config.get_tpm_device()
    result = sh.run(
        f"systemd-cryptenroll {root} --wipe-slot=tpm2 "
        f"--tpm2-device={device} --tpm2-pcrs={PCRS}",
        capture_output=False, check=False)
    if result.returncode != 0: Utils.abort("TPM2 enrollment failed")
    Utils.log("TPM2 enrolled successfully")

def wipe():
    root = Config.get_disk_by_part_label_root()
    result = sh.run(f"systemd-cryptenroll {root} --wipe-slot=tpm2",
                    capture_output=False, check=False)
    if result.returncode != 0: Utils.abort("TPM2 wipe failed")
    Utils.log("TPM2 enrollment wiped")

def status():
    root = Config.get_disk_by_part_label_root()
    device = Config.get_tpm_device()
    Utils.print(f"TPM2 device: {device}")
    Utils.print(Shell.stdout(
        sh.run(f"systemd-cryptenroll --tpm2-device=list {root}", check=False)))
    Utils.print(Shell.stdout(
        sh.run(f"cryptsetup luksDump {root}", check=False)))

def enable():
    require_tpm2()
    require_luks()
    enroll()

def disable():
    require_tpm2()
    require_luks()
    wipe()

def main():
    args = Utils.parse_args({"enable": [], "disable": [], "status": []})
    if args.command == "enable": enable()
    elif args.command == "disable": disable()
    elif args.command == "status": status()

if __name__ == "__main__":
    main()
