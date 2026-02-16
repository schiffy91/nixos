#! /usr/bin/env nix-shell
#! nix-shell -i python3 -p python3
import inspect
import sys
from .shell import Shell, chrootable


@chrootable
class Utils:
    sh = Shell()
    LOG_INFO: bool = True
    GRAY = "\033[90m"
    ORANGE = "\033[38;5;208m"
    RED = "\033[31m"
    RESET = "\033[0m"

    @classmethod
    def toggle(cls, argv, on_enable=None, on_disable=None,
               on_exception=None):
        try:
            match Utils.parse_args(argv, "enable", "disable"):
                case ["enable"] if on_enable:
                    on_enable()
                case ["disable"] if on_disable:
                    on_disable()
                case _:
                    name = cls.sh.basename(inspect.stack()[1].filename)
                    return Utils.abort(
                        f"Usage: {name} (enable | disable)"
                    )
        except BaseException as exception:
            Utils.log_error(f"Caught exception: {exception}.")
            if on_exception:
                on_exception()
            raise

    @classmethod
    def parse_args(cls, argv, *accepted_args):
        if not argv or not accepted_args:
            return []
        return [arg for arg in argv if arg in set(accepted_args)]

    @classmethod
    def require_root(cls):
        cls.sh.require_root()

    @classmethod
    def abort(cls, message=""):
        if message:
            cls.log_error(message)
        return sys.exit(1)

    @classmethod
    def reboot(cls):
        return cls.sh.run("shutdown -r now")

    @classmethod
    def log(cls, message):
        if cls.LOG_INFO:
            print(f"{cls.GRAY}LOG: {message}{cls.RESET}")

    @classmethod
    def log_error(cls, message):
        print(f"{cls.ORANGE}ERROR: {message}{cls.RESET}", file=sys.stderr)

    @classmethod
    def print(cls, message):
        print(message)

    @classmethod
    def print_inline(cls, message):
        print(f"\r{message}", end="")

    @classmethod
    def print_warning(cls, message):
        cls.print_error(message)

    @classmethod
    def print_error(cls, message):
        print(f"{cls.RED}{message}{cls.RESET}", file=sys.stderr)
