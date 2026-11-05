#!/usr/bin/env python3
"""Assert one boot's health block from a VM serial log.

The installed real hosts grant the harness no SSH, so every post-boot fact
arrives as an E2E-* line the guest printed to its serial console.
"""
import re
import sys

log_path, boot_index, secure_boot = sys.argv[1], int(sys.argv[2]), sys.argv[3] == "true"
require_enforced = len(sys.argv) > 4 and sys.argv[4] == "true"
text = open(log_path, "rb").read().decode("utf-8", "replace").replace("\r", "")
blocks = re.findall(r"E2E-HEALTH-BEGIN.*?E2E-HEALTH-END", text, re.S)
if len(blocks) <= boot_index:
    sys.exit(f"FAIL: wanted health block {boot_index}, found {len(blocks)}")
block = blocks[boot_index]


def value(tag, default=""):
    found = re.search(rf"^E2E-{tag} (.*)$", block, re.M)
    return found.group(1).strip() if found else default


failures = []
if value("STATE") != "running":
    failures.append(f"system state is {value('STATE') or 'unknown'}, not running")
for unit in re.findall(r"^E2E-FAILED (.*)$", block, re.M):
    failures.append(f"failed unit: {unit}")
for refused in re.findall(r"^E2E-REFUSED (.*)$", block, re.M):
    failures.append(f"systemd refused a unit: {refused}")
if value("HOMEMANAGER") != "active":
    failures.append(f"home-manager is {value('HOMEMANAGER') or 'missing'}, not active")
if value("TEXTPROMPT", "0") != "0":
    failures.append("the passphrase was asked on a text console, so nothing owned the screen")
# a blank panel and plymouth's splash look alike, so the unit's own result is
# what shows the agent actually ran
unit_result = value("UNLOCKUNIT", "")
if unit_result != "success":
    failures.append(
        f"the unlock agent's unit did not run to success: {value('UNLOCKAGENT')} reported "
        f"{unit_result or 'nothing, so the unit does not exist'}")
agent = value("UNLOCKAGENT").split()
if len(agent) < 2 or int(agent[1] or 0) < 1:
    failures.append(f"no graphical unlock agent ran: {value('UNLOCKAGENT') or 'nothing reported'}")
if int(value("SNAPSHOTS", "0") or 0) < 1:
    failures.append("no CLEAN snapshot exists after install")
if not value("SWAP"):
    failures.append("no swap is active, so hibernation cannot work")
if require_enforced and "enabled" not in value("SECUREBOOT").lower():
    failures.append(f"secure boot is not enforcing: {value('SECUREBOOT') or 'nothing'}")
if require_enforced and int(value("SIGNED", "0") or 0) < 3:
    failures.append(f"only {value('SIGNED')} signed EFI binaries; expected the fallback loader, systemd-boot and a UKI")
if value("VENDORKEYS", "0") != "0":
    failures.append(f"{value('VENDORKEYS')} Microsoft keys are enrolled; this repository enrolls its own keys only")
if value("CMDLINE-QUIET", "0") == "0":
    failures.append("the kernel was not told to be quiet")
if value("CMDLINE-LOGLEVEL", "0") == "0":
    failures.append("loglevel=3 is not in effect")
if require_enforced and value("SBCTL") not in ("0", "unavailable"):
    unsigned = re.findall(r"^E2E-UNSIGNED (.*)$", block, re.M)
    failures.append(f"sbctl reports {value('SBCTL')} unsigned: {'; '.join(unsigned) or 'unknown'}")

print(f"log={log_path.rsplit('/', 1)[-1]} " + " | ".join(
    f"{tag}={value(tag)}" for tag in ("STATE", "HOMEMANAGER", "UNLOCKAGENT", "SNAPSHOTS", "UNLOCKUNIT", "SBCTL", "SIGNED", "VENDORKEYS", "SECUREBOOT")))
if failures:
    sys.exit("FAIL: " + "; ".join(failures))
print("host-healthy")
