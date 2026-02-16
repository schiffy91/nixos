#! /usr/bin/env nix-shell
#! nix-shell -i python3 -p python3
import getpass
import glob

from .shell import Shell, chrootable
from .utils import Utils


@chrootable
class Interactive:
    sh = Shell()

    @classmethod
    def confirm(cls, prompt):
        while True:
            response = input(f"{prompt} (y/n): ").lower()
            if response in ("y", "yes"):
                return True
            if response in ("n", "no"):
                return False
            Utils.print("Invalid input. Enter 'y' or 'n'.")

    @classmethod
    def ask_for_host_path(cls, hosts_path):
        hosts_paths = glob.glob(
            f"{hosts_path}/**/*.nix", recursive=True
        )
        formatted = [
            cls.sh.basename(p).replace(".nix", "")
            + " (" + cls.sh.parent_name(p) + ")"
            for p in hosts_paths
        ]
        while True:
            for i, name in enumerate(formatted):
                Utils.print(f"{i + 1}) {name}")
            try:
                return hosts_paths[int(input("> ")) - 1]
            except KeyboardInterrupt:
                Utils.abort()
            except (ValueError, IndexError):
                Utils.print_error("Invalid choice.")

    @classmethod
    def ask_for_password(cls):
        while True:
            password = getpass.getpass("Set your password: ")
            if password == getpass.getpass("Confirm your password: "):
                return password
            Utils.log_error("Passwords do not match.")

    @classmethod
    def ask_to_reboot(cls):
        if Interactive.confirm("Restart now?"):
            return Utils.reboot()
        return False
