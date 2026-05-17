#! /usr/bin/env nix-shell
#! nix-shell -i python3 -p python3 kdePackages.libkscreen systemd kdePackages.konsole pulseaudio
import argparse, json, os, re, signal, subprocess, sys, tomllib
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parent.parent.parent))
from lib import Shell, Utils

Utils.LOG_INFO = False  # CLI emits JSON on stdout; never noise it up

ANSI_ESCAPE = re.compile(r"\x1b\[[0-9;]*m")
DRM_DEVICES_PATH = Path("/sys/class/drm")
CAFFEINE_PID_FILE = Path(os.environ.get("XDG_RUNTIME_DIR", "/tmp")) / "nixos-helper-caffeine.pid"
INHIBIT_WHO = "nixos-helper"
LABELS_PATH = Path(__file__).resolve().parent / "labels.toml"

def load_labels():
    try:
        with open(LABELS_PATH, "rb") as f: return tomllib.load(f)
    except (OSError, tomllib.TOMLDecodeError): return {}

LABELS = load_labels()
DISPLAY_LABELS = LABELS.get("displays", {})
AUDIO_PRESETS = LABELS.get("audio", [])


class Displays:
    sh = Shell()

    @classmethod
    def list(cls):
        outputs_by_name = {output["name"]: output for output in cls._parse_kscreen()}
        for connector in cls._drm_connectors():
            if connector["status"] != "connected": continue
            output = outputs_by_name.get(connector["name"]) or {
                "name": connector["name"], "index": None, "enabled": False,
                "connected": True, "priority": None,
            }
            output["label"] = DISPLAY_LABELS.get(output["name"], output.get("type") or "Display")
            output["card"] = connector["card"]
            outputs_by_name[output["name"]] = output
        return sorted(outputs_by_name.values(), key=lambda output: (output.get("priority") or 999, output["name"]))

    @classmethod
    def enable(cls, name): cls._kscreen(f"output.{name}.enable")

    @classmethod
    def disable(cls, name): cls._kscreen(f"output.{name}.disable")

    @classmethod
    def primary(cls, name): cls._kscreen(f"output.{name}.priority.1")

    @classmethod
    def dpms(cls, state): cls._kscreen("--dpms", state)

    @classmethod
    def _kscreen(cls, *args):
        subprocess.run(["kscreen-doctor", *args], capture_output=False, check=False)

    @classmethod
    def _parse_kscreen(cls):
        result = subprocess.run(["kscreen-doctor", "-o"], capture_output=True, text=True, check=False)
        text = ANSI_ESCAPE.sub("", result.stdout)
        outputs = []
        for block in re.split(r"(?=^Output: )", text, flags=re.MULTILINE):
            header = re.match(r"Output: (\d+) (\S+)", block)
            if not header: continue
            output = {"index": int(header.group(1)), "name": header.group(2)}
            output["enabled"] = bool(re.search(r"^\tenabled$", block, re.MULTILINE))
            output["connected"] = bool(re.search(r"^\tconnected$", block, re.MULTILINE))
            priority_match = re.search(r"^\tpriority (\d+)$", block, re.MULTILINE)
            output["priority"] = int(priority_match.group(1)) if priority_match else None
            type_match = re.search(r"^\t(HDMI|DisplayPort|VGA|DVI|Panel|TV|Unknown)$", block, re.MULTILINE)
            output["type"] = type_match.group(1) if type_match else None
            geometry_match = re.search(r"^\tGeometry: (\d+),(\d+) (\d+)x(\d+)$", block, re.MULTILINE)
            if geometry_match:
                output["geometry"] = {
                    "x": int(geometry_match.group(1)), "y": int(geometry_match.group(2)),
                    "w": int(geometry_match.group(3)), "h": int(geometry_match.group(4)),
                }
            hdr_match = re.search(r"^\tHDR: (enabled|disabled|incapable)", block, re.MULTILINE)
            if hdr_match: output["hdr"] = hdr_match.group(1)
            outputs.append(output)
        return outputs

    @classmethod
    def _drm_connectors(cls):
        if not DRM_DEVICES_PATH.exists(): return []
        connectors = []
        for path in sorted(DRM_DEVICES_PATH.glob("card*-*")):
            if not path.is_dir(): continue
            card, _, name = path.name.partition("-")
            status_file, enabled_file = path / "status", path / "enabled"
            status = status_file.read_text().strip() if status_file.exists() else "unknown"
            enabled = enabled_file.read_text().strip() if enabled_file.exists() else "unknown"
            connectors.append({"name": name, "card": card, "status": status, "enabled": enabled})
        return connectors


class Audio:
    @classmethod
    def list(cls):
        active_sinks = cls._pactl_json("list", "sinks")
        current = cls.current()
        results, covered = [], set()
        for preset in AUDIO_PRESETS:
            sink_name = preset.get("sink", "")
            results.append({
                "name": sink_name or preset.get("label", ""),
                "label": preset.get("label", sink_name),
                "default": bool(sink_name) and sink_name == current,
            })
            if sink_name: covered.add(sink_name)
        for sink in active_sinks:  # extras: anything live but not in presets
            sink_name = sink.get("name", "")
            if sink_name in covered: continue
            results.append({
                "name": sink_name,
                "label": sink.get("description") or sink_name,
                "default": sink_name == current,
            })
        return results

    @classmethod
    def current(cls):
        result = subprocess.run(["pactl", "get-default-sink"], capture_output=True, text=True, check=False)
        return result.stdout.strip()

    @classmethod
    def set(cls, selector):
        preset = next((p for p in AUDIO_PRESETS
                       if selector in (p.get("label"), p.get("sink"))), None)
        if preset:
            card, profile, sink = preset.get("card"), preset.get("profile"), preset.get("sink")
            if card and profile:
                subprocess.run(["pactl", "set-card-profile", card, profile], capture_output=False, check=False)
            if sink:
                subprocess.run(["pactl", "set-default-sink", sink], capture_output=False, check=False)
        else:
            subprocess.run(["pactl", "set-default-sink", selector], capture_output=False, check=False)

    @classmethod
    def _pactl_json(cls, *args):
        result = subprocess.run(["pactl", "-f", "json", *args], capture_output=True, text=True, check=False)
        try: return json.loads(result.stdout)
        except json.JSONDecodeError: return []


class Caffeine:
    @classmethod
    def pid(cls):
        if not CAFFEINE_PID_FILE.exists(): return None
        try:
            existing_pid = int(CAFFEINE_PID_FILE.read_text().strip())
            os.kill(existing_pid, 0)
            return existing_pid
        except (ValueError, ProcessLookupError, PermissionError):
            CAFFEINE_PID_FILE.unlink(missing_ok=True)
            return None

    @classmethod
    def enable(cls):
        if cls.pid(): return
        process = subprocess.Popen(
            ["systemd-inhibit", "--what=idle", f"--who={INHIBIT_WHO}",
             "--why=User disabled sleep", "sleep", "infinity"],
            start_new_session=True,
        )
        CAFFEINE_PID_FILE.write_text(str(process.pid))

    @classmethod
    def disable(cls):
        existing_pid = cls.pid()
        if existing_pid is None: return
        try: os.kill(existing_pid, signal.SIGTERM)
        except ProcessLookupError: pass
        CAFFEINE_PID_FILE.unlink(missing_ok=True)

    @classmethod
    def toggle(cls):
        (cls.disable if cls.pid() else cls.enable)()


class System:
    @classmethod
    def update(cls): cls._terminal("sudo nixos update")

    @classmethod
    def upgrade(cls): cls._terminal("sudo nixos upgrade")

    @classmethod
    def _terminal(cls, command):
        subprocess.Popen(["konsole", "-e", "bash", "-c",
                          f'{command}; echo; read -rp "Press enter to close..."'])


def parse_args():
    parser = argparse.ArgumentParser(prog="nixos-helper-cli")
    commands = parser.add_subparsers(dest="command", required=True)

    displays = commands.add_parser("displays")
    displays_ops = displays.add_subparsers(dest="operation", required=True)
    displays_ops.add_parser("list")
    for name in ("enable", "disable", "primary"):
        action_parser = displays_ops.add_parser(name)
        action_parser.add_argument("output")
    dpms_parser = displays_ops.add_parser("dpms")
    dpms_parser.add_argument("state", choices=["on", "off", "standby"])

    audio = commands.add_parser("audio")
    audio_ops = audio.add_subparsers(dest="operation", required=True)
    audio_ops.add_parser("list")
    audio_ops.add_parser("current")
    audio_set = audio_ops.add_parser("set")
    audio_set.add_argument("sink")

    caffeine = commands.add_parser("caffeine")
    caffeine_ops = caffeine.add_subparsers(dest="operation", required=True)
    for name in ("status", "enable", "disable", "toggle"): caffeine_ops.add_parser(name)

    system = commands.add_parser("system")
    system_ops = system.add_subparsers(dest="operation", required=True)
    for name in ("update", "upgrade"): system_ops.add_parser(name)

    return parser.parse_args()


def main():
    args = parse_args()
    if args.command == "displays":
        if args.operation == "list": print(json.dumps(Displays.list(), indent=2))
        elif args.operation == "enable": Displays.enable(args.output)
        elif args.operation == "disable": Displays.disable(args.output)
        elif args.operation == "primary": Displays.primary(args.output)
        elif args.operation == "dpms": Displays.dpms(args.state)
    elif args.command == "audio":
        if args.operation == "list": print(json.dumps(Audio.list(), indent=2))
        elif args.operation == "current": print(Audio.current())
        elif args.operation == "set": Audio.set(args.sink)
    elif args.command == "caffeine":
        if args.operation == "status": sys.exit(0 if Caffeine.pid() else 1)
        elif args.operation == "enable": Caffeine.enable()
        elif args.operation == "disable": Caffeine.disable()
        elif args.operation == "toggle": Caffeine.toggle()
    elif args.command == "system":
        {"update": System.update, "upgrade": System.upgrade}[args.operation]()


if __name__ == "__main__":
    main()
