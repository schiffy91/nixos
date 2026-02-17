import contextlib
import json
import subprocess
import sys


class Shell:
    evals = {}
    def __init__(self, root_required=False):
        self.chroots = []
        if root_required:
            self.require_root()
    @classmethod
    def stdout(cls, completed_process):
        return completed_process.stdout.strip()
    # User
    def require_root(self):
        if Shell.stdout(self.run("id -u")) != "0":
            print("Please run this script with sudo.", file=sys.stderr)
            sys.exit(1)
    def whoami(self):
        return Shell.stdout(self.run("who")).split()[0]
    def hostname(self):
        return Shell.stdout(self.run("hostname"))
    # Execution
    @contextlib.contextmanager
    def chroot(self, path):
        previous_shells = {}
        try:
            self.chroots.append(path)
            for cls in chrootable_registry:
                previous_shells[cls] = cls.sh
                cls.sh = self
            yield self
        finally:
            for cls, old_sh in previous_shells.items():
                cls.sh = old_sh
            self.chroots.pop()
    def redact(self, text, sensitive):
        if not sensitive: return text
        for secret in (sensitive if isinstance(sensitive, list) else [sensitive]):
            text = text.replace(secret, "***")
        return text
    def run(self, cmd, env="", sudo=True, capture_output=True, check=True, sensitive=None):
        if self.chroots:
            cmd = f'nixos-enter --root {self.chroots[-1]} --command "{cmd}"'
        if sudo: cmd = f"{env} sudo {cmd}".strip()
        else: cmd = f"{env} {cmd}".strip()
        print(f"\033[90mLOG: {self.redact(cmd, sensitive)}\033[0m")
        try:
            return subprocess.run(cmd, shell=True, check=check, capture_output=capture_output, text=True)
        except subprocess.CalledProcessError as e:
            if e.stdout: print(f"\033[90mLOG: {self.redact(e.stdout, sensitive)}\033[0m")
            if e.stderr: print(f"\033[38;5;208mERROR: {self.redact(e.stderr, sensitive)}\033[0m", file=sys.stderr)
            raise
    # File System
    def mv(self, original, final):
        self.mkdir(self.dirname(final))
        return self.run(f"mv {original} {final}")
    def rm(self, *args):
        return self.run(f"rm -rf {' '.join(args)}")
    def mkdir(self, *args):
        return self.run(f"mkdir -p {' '.join(args)}")
    def cpdir(self, source, target):
        self.rm(target)
        self.mkdir(self.dirname(target))
        return self.run(f"cp -r {source} {target}")
    def find(self, path, pattern="*", ignore_pattern=None, ignore_files=False, ignore_directories=False):
        def format_patterns(prefix, patterns):
            if not patterns: return ""
            if " " in patterns:
                joined = " -o ".join(
                    f"-path '{p}'" for p in patterns.strip().split())
                return f"{prefix} \\( {joined} \\)"
            return f"{prefix} -path '{patterns}'"
        type_arg = ""
        if ignore_files: type_arg = "-type d"
        elif ignore_directories: type_arg = "-type f"
        pattern_arg = format_patterns("", pattern)
        ignore_arg = format_patterns("-not", ignore_pattern)
        path = self.realpath(path)
        command = f"find '{path}' {pattern_arg} {type_arg} {ignore_arg}"
        output = Shell.stdout(self.run(command.strip()))
        return [] if not output else output.split("\n")
    def find_directories(self, path, pattern="*", ignore_pattern=None):
        return self.find(path, pattern=pattern, ignore_pattern=ignore_pattern, ignore_files=True)
    def find_files(self, path, pattern="*", ignore_pattern=None):
        return self.find(path, pattern=pattern, ignore_pattern=ignore_pattern, ignore_directories=True)
    def symlink(self, source, target):
        return self.run(f"ln -s {source} {target}")
    def is_symlink(self, path):
        return self.run(f"[ -L '{path}' ]", check=False).returncode == 0
    def realpath(self, path):
        return Shell.stdout(self.run(f"realpath '{path}'")).splitlines()[0]
    def realpaths(self, *paths):
        cmd = "realpath " + " ".join(f"'{p}'" for p in paths)
        return Shell.stdout(self.run(cmd)).splitlines()
    def is_dir(self, path):
        return self.run(f"[ -d '{path}' ]", check=False).returncode == 0
    def exists(self, *args):
        cmd = " && ".join(f"[ -e '{a}' ]" for a in args) + " && true"
        return self.run(cmd, check=False).returncode == 0
    def basename(self, path):
        return Shell.stdout(self.run(f"basename '{path}'"))
    def dirname(self, path):
        return Shell.stdout(self.run(f"dirname '{path}'"))
    def parent_name(self, path):
        return self.basename(self.dirname(path))
    # Security
    def chmod(self, mode, *args):
        paths = self.realpaths(*args)
        self.run(f"chmod -R {mode} " + " ".join(f"'{p}'" for p in paths))
    def chown(self, user, *args):
        paths = self.realpaths(*args)
        self.run(f"chown -R {user} " + " ".join(f"'{p}'" for p in paths))
    def ssh_keygen(self, key_type, path, password=""):
        self.mkdir(self.dirname(path))
        self.run(f"ssh-keygen -t {key_type} -N \"{password}\" -f '{path}'")
    # I/O
    def file_write(self, path, string, sensitive=None):
        log_string = self.redact(string, sensitive)
        print(f"\033[90mLOG: file_write {path} ({log_string})\033[0m")
        self.rm(path)
        self.mkdir(self.dirname(path))
        if self.chroots: path = f"{self.chroots[-1]}{path}"
        with open(path, "w", encoding="utf-8") as f:
            f.write(string)
    def file_read(self, path):
        if not self.exists(path): return ""
        if self.chroots: path = f"{self.chroots[-1]}{path}"
        with open(path, "r", encoding="utf-8") as f:
            return f.read()
    def json_read(self, path):
        try: return json.loads(self.file_read(path))
        except (json.JSONDecodeError, ValueError): return {}
    def json_write(self, path, key, value):
        data = self.json_read(path)
        data[key] = value
        return self.file_write(path, json.dumps(data))
    def json_overwrite(self, path, data):
        self.rm(path)
        return self.file_write(path, json.dumps(data))
    # Git
    def git_add_safe_directory(self, path):
        path = self.realpath(path)
        self.run(f"git config --global --add safe.directory '{path}'")


chrootable_registry: list = []


def chrootable(cls):
    if not hasattr(cls, "sh"):
        raise TypeError(
            f"Class {cls.__name__} must have a 'sh' attribute to be chrootable.")
    chrootable_registry.append(cls)
    @classmethod
    @contextlib.contextmanager
    def chroot(cls, sh):
        previous = cls.sh
        cls.sh = sh
        try:
            yield cls
        finally:
            cls.sh = previous
    cls.chroot = chroot
    return cls
