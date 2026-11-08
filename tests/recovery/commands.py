"""Command boundary for mount orchestration tests; never mounts real devices."""
import json
import os
from pathlib import Path
import sys

path = Path(os.environ["MOUNT_FIXTURE_STATE"])
state = json.loads(path.read_text())
command = Path(sys.argv[0]).name
arguments = sys.argv[1:]
state["commands"].append([command, *arguments])
path.write_text(json.dumps(state))
if command == "findmnt":
    if arguments[0] == "-R":
        print("fixture mounts")
    else:
        target = arguments[arguments.index("--mountpoint") + 1]
        if target not in state["mounts"]:
            sys.exit(1)
        print(json.dumps({"filesystems": [state["mounts"][target]]}))
elif command == "cryptsetup":
    Path(state["root"]).write_text("unlocked fixture")
elif command == "btrfs":
    sys.exit(0 if arguments[-1] in state["available"] else 1)
elif command == "mount":
    target = arguments[-1]
    device = arguments[-2]
    options = arguments[arguments.index("-o") + 1] if "-o" in arguments else "rw"
    state["mounts"][target] = {"source": device, "fstype": "btrfs" if "-t" in arguments else "vfat", "options": options}
    path.write_text(json.dumps(state))
else:
    raise SystemExit(f"Unexpected fixture command: {command}")
