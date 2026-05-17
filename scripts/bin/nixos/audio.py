import json, subprocess, sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parent.parent.parent))
from lib import Utils
sys.path.insert(0, str(Path(__file__).resolve().parent))
from labels import AUDIO

class Audio:
    @classmethod
    def list(cls):
        current = cls.default()
        return cls.preset_entries(current) + cls.extra_entries(current)
    @classmethod
    def preset_entries(cls, current):
        return [cls.preset_entry(preset, current) for preset in AUDIO]
    @classmethod
    def preset_entry(cls, preset, current):
        sink = preset.get("sink", "")
        return {
            "name":    sink or preset.get("label", ""),
            "label":   preset.get("label", sink),
            "default": bool(sink) and sink == current,
        }
    @classmethod
    def extra_entries(cls, current):
        covered = {preset.get("sink") for preset in AUDIO if preset.get("sink")}
        return [cls.sink_entry(sink, current) for sink in cls.sinks() if sink.get("name") not in covered]
    @classmethod
    def sink_entry(cls, sink, current):
        name = sink.get("name", "")
        return {
            "name":    name,
            "label":   sink.get("description") or name,
            "default": name == current,
        }
    @classmethod
    def default(cls):
        return cls.read("get-default-sink").strip()
    @classmethod
    def set(cls, selector):
        preset = cls.find_preset(selector)
        if preset: cls.apply_preset(preset)
        else: cls.run("set-default-sink", selector)
    @classmethod
    def find_preset(cls, selector):
        return next((preset for preset in AUDIO
                     if selector in (preset.get("label"), preset.get("sink"))), None)
    @classmethod
    def apply_preset(cls, preset):
        card    = preset.get("card")
        profile = preset.get("profile")
        sink    = preset.get("sink")
        volume  = preset.get("volume")
        if card and profile:   cls.run("set-card-profile", card, profile)
        if sink:               cls.run("set-default-sink", sink)
        if sink and volume:    cls.run("set-sink-volume", sink, volume)
    @classmethod
    def sinks(cls):
        try: return json.loads(cls.read("-f", "json", "list", "sinks"))
        except json.JSONDecodeError: return []
    @classmethod
    def read(cls, *args):
        result = subprocess.run(["pactl", *args], capture_output=True, text=True, check=False)
        return result.stdout
    @classmethod
    def run(cls, *args):
        subprocess.run(["pactl", *args], capture_output=False, check=False)

def main(argv=None):
    Utils.LOG_INFO = False
    args = Utils.parse_args({
        "list":    [],
        "current": [],
        "set":     ["sink"],
    }, argv)
    if   args.command == "list":    print(json.dumps(Audio.list(), indent=2))
    elif args.command == "current": print(Audio.default())
    elif args.command == "set":     Audio.set(args.sink)

if __name__ == "__main__":
    main()
