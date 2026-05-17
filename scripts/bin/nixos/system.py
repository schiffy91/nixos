import subprocess, sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parent.parent.parent))
from lib import Utils

class System:
    @classmethod
    def update(cls):
        cls.terminal("sudo nixos update")
    @classmethod
    def upgrade(cls):
        cls.terminal("sudo nixos upgrade")
    @classmethod
    def terminal(cls, command):
        subprocess.Popen(["konsole", "-e", "bash", "-c",
                          f'{command}; echo; read -rp "Press enter to close..."'])

def main(argv=None):
    args = Utils.parse_args({"update": [], "upgrade": []}, argv)
    if   args.command == "update":  System.update()
    elif args.command == "upgrade": System.upgrade()

if __name__ == "__main__":
    main()
