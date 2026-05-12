import argparse, sys
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
    def parse_args(cls, schema):
        def add_args(parser, args):
            for arg in args:
                if isinstance(arg, tuple): parser.add_argument(arg[0], type=arg[1], default=None)
                elif arg.startswith("--"): parser.add_argument(arg, action="store_true")
                else: parser.add_argument(arg)
        parser = argparse.ArgumentParser()
        if isinstance(schema, dict):
            subparsers = parser.add_subparsers(dest="command", required=True)
            for command, args in schema.items():
                add_args(subparsers.add_parser(command), args)
        else:
            add_args(parser, schema)
        return parser.parse_args()
    @classmethod
    def require_root(cls):
        cls.sh.require_root()
    @classmethod
    def abort(cls, message=""):
        if message: cls.log_error(message)
        return sys.exit(1)
    @classmethod
    def reboot(cls):
        return cls.sh.run("shutdown -r now")
    @classmethod
    def log(cls, message):
        if cls.LOG_INFO: print(f"{cls.GRAY}LOG: {message}{cls.RESET}")
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
    def print_error(cls, message):
        print(f"{cls.RED}{message}{cls.RESET}", file=sys.stderr)
