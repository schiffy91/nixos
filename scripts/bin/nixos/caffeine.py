import os, signal, subprocess, sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parent.parent.parent))
from lib import Utils

class Caffeine:
    PID_FILE = Path(os.environ.get("XDG_RUNTIME_DIR", "/tmp")) / "nixos-caffeine.pid"
    WHO = "nixos"
    @classmethod
    def pid(cls):
        if not cls.PID_FILE.exists(): return None
        try:
            existing = int(cls.PID_FILE.read_text().strip())
            os.kill(existing, 0)
            return existing
        except (ValueError, ProcessLookupError, PermissionError):
            cls.clear_pid()
            return None
    @classmethod
    def enable(cls):
        if cls.pid(): return
        process = subprocess.Popen(cls.inhibit_command(), start_new_session=True)
        cls.PID_FILE.write_text(str(process.pid))
    @classmethod
    def disable(cls):
        existing = cls.pid()
        if existing is None: return
        try: os.kill(existing, signal.SIGTERM)
        except ProcessLookupError: pass
        cls.clear_pid()
    @classmethod
    def toggle(cls):
        (cls.disable if cls.pid() else cls.enable)()
    @classmethod
    def inhibit_command(cls):
        return ["systemd-inhibit", "--what=idle", f"--who={cls.WHO}",
                "--why=User disabled sleep", "sleep", "infinity"]
    @classmethod
    def clear_pid(cls):
        cls.PID_FILE.unlink(missing_ok=True)

def main(argv=None):
    args = Utils.parse_args({"status": [], "enable": [], "disable": [], "toggle": []}, argv)
    if   args.command == "status":  sys.exit(0 if Caffeine.pid() else 1)
    elif args.command == "enable":  Caffeine.enable()
    elif args.command == "disable": Caffeine.disable()
    elif args.command == "toggle":  Caffeine.toggle()

if __name__ == "__main__":
    main()
