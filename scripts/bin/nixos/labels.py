import os, tomllib

CONFIG = {}
_path = os.environ.get("NIXOS_CONFIG")
if _path:
    try:
        with open(_path, "rb") as f: CONFIG = tomllib.load(f)
    except (OSError, tomllib.TOMLDecodeError): pass

DISPLAYS = CONFIG.get("displays", {})
LAYOUT   = CONFIG.get("layout", [])
AUDIO    = CONFIG.get("audio", [])
