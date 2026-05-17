#! /usr/bin/env nix-shell
#! nix-shell -i python3 -p python3 kdePackages.libkscreen systemd kdePackages.konsole pulseaudio
import importlib, os, sys
from pathlib import Path
HERE = Path(__file__).resolve().parent
sys.path[:0] = [str(HERE), str(HERE.parent.parent)]
from lib import Utils

ROOT_COMMANDS    = {"update", "upgrade", "tpm2", "secure-boot", "change-password"}
COMMANDS         = ["update", "upgrade", "tpm2", "secure-boot", "change-password",
                    "displays", "audio", "caffeine", "system"]
MODULE_ALIASES   = {"upgrade": "update", "secure-boot": "secure_boot", "change-password": "change_password"}

def usage():
    Utils.print(f"Usage: nixos {{{','.join(COMMANDS)}}} [args...]")

def dispatch(command, argv):
    if command == "upgrade": argv = ["--upgrade", *argv]
    module = importlib.import_module(MODULE_ALIASES.get(command, command))
    module.main(argv)

def main():
    if len(sys.argv) < 2: usage(); sys.exit(2)
    if sys.argv[1] in ("-h", "--help"): usage(); sys.exit(0)
    command = sys.argv[1]
    if command not in COMMANDS:
        Utils.print_error(f"Unknown command: {command}")
        usage(); sys.exit(2)
    if command in ROOT_COMMANDS and os.geteuid() != 0:
        os.execvp("sudo", ["sudo", "nixos", *sys.argv[1:]])
    dispatch(command, sys.argv[2:])

if __name__ == "__main__":
    main()
