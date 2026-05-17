import json, re, subprocess, sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parent.parent.parent))
from lib import Utils
sys.path.insert(0, str(Path(__file__).resolve().parent))
from labels import DISPLAYS, LAYOUT

ANSI_ESCAPE = re.compile(r"\x1b\[[0-9;]*m")
DRM_PATH = Path("/sys/class/drm")

class Displays:
    @classmethod
    def list(cls):
        connected = cls.connected_names()
        outputs = [output | {"label": cls.label(output)}
                   for output in cls.outputs()
                   if output["name"] in connected
                   and (output["name"] in DISPLAYS or output.get("enabled"))]
        return sorted(outputs, key=lambda output: (output.get("priority") or 999, output["name"]))
    @classmethod
    def enable(cls, name):
        cls.run(f"output.{name}.enable")
        cls.apply_layout(name)
    @classmethod
    def disable(cls, name):
        cls.run(f"output.{name}.disable")
    @classmethod
    def primary(cls, name):
        cls.run(f"output.{name}.priority.1")
    @classmethod
    def dpms(cls, state):
        cls.run("--dpms", state)
    @classmethod
    def layout(cls):
        for rule in LAYOUT: cls.apply_layout(rule.get("display"))
    @classmethod
    def apply_layout(cls, name):
        rule = cls.layout_rule(name)
        if not rule: return
        outputs = cls.outputs_by_name()
        if not outputs.get(name, {}).get("enabled"): return
        position = cls.position(rule, outputs)
        if position: cls.run(f"output.{name}.position.{position[0]},{position[1]}")
    @classmethod
    def layout_rule(cls, name):
        return next((rule for rule in LAYOUT if rule.get("display") == name), None)
    @classmethod
    def position(cls, rule, outputs):
        display = outputs.get(rule.get("display"))
        anchor = outputs.get(rule.get("relative_to"))
        if not display or not anchor: return None
        return cls.direction_offset(
            rule.get("position"),
            display.get("geometry", {}),
            anchor.get("geometry", {}))
    @classmethod
    def direction_offset(cls, direction, display_geometry, anchor_geometry):
        anchor_x       = anchor_geometry.get("x", 0)
        anchor_y       = anchor_geometry.get("y", 0)
        anchor_width   = anchor_geometry.get("w", 0)
        anchor_height  = anchor_geometry.get("h", 0)
        display_width  = display_geometry.get("w", 0)
        display_height = display_geometry.get("h", 0)
        if direction == "left-of":  return (anchor_x - display_width, anchor_y)
        if direction == "right-of": return (anchor_x + anchor_width, anchor_y)
        if direction == "above":    return (anchor_x, anchor_y - display_height)
        if direction == "below":    return (anchor_x, anchor_y + anchor_height)
        return None
    @classmethod
    def label(cls, output):
        return DISPLAYS.get(output["name"], output.get("type") or "Display")
    @classmethod
    def outputs(cls):
        return [parsed for parsed in (cls.parse_block(block) for block in cls.blocks()) if parsed]
    @classmethod
    def outputs_by_name(cls):
        return {output["name"]: output for output in cls.outputs()}
    @classmethod
    def blocks(cls):
        return re.split(r"(?=^Output: )", cls.read_kscreen(), flags=re.MULTILINE)
    @classmethod
    def parse_block(cls, block):
        header = re.match(r"Output: (\d+) (\S+)", block)
        if not header: return None
        return {
            "index":     int(header.group(1)),
            "name":      header.group(2),
            "enabled":   bool(re.search(r"^\tenabled$",   block, re.MULTILINE)),
            "connected": bool(re.search(r"^\tconnected$", block, re.MULTILINE)),
            "priority":  cls.parse_priority(block),
            "type":      cls.parse_type(block),
            "geometry":  cls.parse_geometry(block),
        }
    @classmethod
    def parse_priority(cls, block):
        match = re.search(r"^\tpriority (\d+)$", block, re.MULTILINE)
        return int(match.group(1)) if match else None
    @classmethod
    def parse_type(cls, block):
        match = re.search(r"^\t(HDMI|DisplayPort|VGA|DVI|Panel|TV|Unknown)$", block, re.MULTILINE)
        return match.group(1) if match else None
    @classmethod
    def parse_geometry(cls, block):
        match = re.search(r"^\tGeometry: (\d+),(\d+) (\d+)x(\d+)$", block, re.MULTILINE)
        if not match: return None
        return {"x": int(match.group(1)), "y": int(match.group(2)),
                "w": int(match.group(3)), "h": int(match.group(4))}
    @classmethod
    def connected_names(cls):
        if not DRM_PATH.exists(): return set()
        return {cls.connector_name(path) for path in DRM_PATH.glob("card*-*") if cls.is_connected(path)}
    @classmethod
    def connector_name(cls, path):
        return path.name.partition("-")[2]
    @classmethod
    def is_connected(cls, path):
        status = path / "status"
        return status.exists() and status.read_text().strip() == "connected"
    @classmethod
    def read_kscreen(cls):
        result = subprocess.run(["kscreen-doctor", "-o"], capture_output=True, text=True, check=False)
        return ANSI_ESCAPE.sub("", result.stdout)
    @classmethod
    def run(cls, *args):
        subprocess.run(["kscreen-doctor", *args], capture_output=False, check=False)

def main(argv=None):
    Utils.LOG_INFO = False
    args = Utils.parse_args({
        "list":    [],
        "layout":  [],
        "enable":  ["output"],
        "disable": ["output"],
        "primary": ["output"],
        "dpms":    ["state"],
    }, argv)
    if   args.command == "list":    print(json.dumps(Displays.list(), indent=2))
    elif args.command == "layout":  Displays.layout()
    elif args.command == "enable":  Displays.enable(args.output)
    elif args.command == "disable": Displays.disable(args.output)
    elif args.command == "primary": Displays.primary(args.output)
    elif args.command == "dpms":    Displays.dpms(args.state)

if __name__ == "__main__":
    main()
