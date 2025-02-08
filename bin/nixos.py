#! /usr/bin/env nix-shell
#! nix-shell -i python3 -p python3
import sys, subprocess, json, getpass, glob, contextlib, inspect

class Shell:
    evals = {} # Cache nix eval since it's slow
    def __init__(self, root_required=False):
        self.chroots = []
        if root_required: self.require_root()
    @classmethod
    def stdout(cls, completed_process): return completed_process.stdout.strip()
    # User
    def require_root(self):
        if Shell.stdout(self.run("id -u")) != "0":
            Utils.abort("Please run this script with sudo.")
    def whoami(self): return Shell.stdout(self.run("who")).split()[0]
    def hostname(self): return Shell.stdout(self.run("hostname"))
    # Execution
    @contextlib.contextmanager
    def chroot(self, path):
        previous_shells = {}
        try:
            self.chroots.append(path)
            for cls in chrootable.registry:
                previous_shells[cls] = cls.sh
                cls.sh = self
            yield self
        finally:
            for cls, old_sh in previous_shells.items():
                cls.sh = old_sh
            self.chroots.pop()
    def run(self, cmd, env="", sensitive=None, capture_output=True, check=True):
        if self.chroots: cmd = f"nixos-enter --root {self.chroots[-1]} --command \"{cmd}\""
        cmd = f"{env} sudo {cmd}".strip()
        if sensitive: Utils.log(cmd.replace(sensitive, "***"))
        else: Utils.log(cmd)
        try:
            result = subprocess.run(cmd, shell=True, check=check, capture_output=capture_output, text=True)
        except subprocess.CalledProcessError as e:
            if e.stdout: Utils.log(e.stdout)
            if e.stderr: Utils.log_error(e.stderr)
            raise e
        return result
    # File System
    def mv(self, original, final):
        self.mkdir(self.dirname(final))
        return self.run(f"mv {original} {final}")
    def rm(self, *args): return self.run(f"rm -rf { ' '.join(args) }")
    def mkdir(self, *args): return self.run(f"mkdir -p { ' '.join(args) }")
    def cpdir(self, source, target):
        self.rm(target)
        self.mkdir(self.dirname(target)) # Make all missing parent directories
        return self.run(f"cp -r {source} {target}")
    def find(self, path, pattern="*", ignore_pattern=None, ignore_files=False, ignore_directories=False):
        def format_patterns(prefix, patterns):
            if not patterns: return ""
            if " " in patterns:
                patterns_joined = " -o ".join(f"-path '{p}'" for p in patterns.strip().split())
                return f"{prefix} \\( {patterns_joined} \\)"
            return f"{prefix} -path '{patterns}'"
        type_arg = "" if not ignore_files and not ignore_directories else "-type d" if ignore_files else "-type f"
        pattern_arg = format_patterns("", pattern)
        ignore_arg = format_patterns("-not", ignore_pattern)
        path = self.realpath(path) # find doesn't work on symlinks
        command_arg = f"find '{path}' {pattern_arg} {type_arg} {ignore_arg}".strip()
        output = Shell.stdout(self.run(command_arg))
        return [] if not output else output.split("\n")
    def find_directories(self, path, pattern="*", ignore_pattern=None):
        return self.find(path, pattern=pattern, ignore_pattern=ignore_pattern, ignore_files=True)
    def find_files(self, path, pattern="*", ignore_pattern=None):
        return self.find(path, pattern=pattern, ignore_pattern=ignore_pattern, ignore_directories=True)
    def symlink(self, source, target): return self.run(f"ln -s {source} {target}")
    def is_symlink(self, path): return self.run(f"[ -L '{path}' ]", check=False).returncode == 0
    def realpath(self, path): return Shell.stdout(self.run(f"realpath '{path}'")).splitlines()[0]
    def realpaths(self, *paths): return Shell.stdout(self.run("realpath " + " ".join(f"'{path}'" for path in paths))).splitlines()
    def is_dir(self, path): return self.run(f"[ -d '{path}' ]", check=False).returncode == 0
    def exists(self, *args): return self.run(" && ".join(f"[ -e '{arg}' ]" for arg in args) + " && true", check=False).returncode == 0
    def basename(self, path): return Shell.stdout(self.run(f"basename '{path}'"))
    def dirname(self, path): return Shell.stdout(self.run(f"dirname '{path}'"))
    def parent_name(self, path): return self.basename(self.dirname(path))
    # Security
    def chmod(self, mode, *args):
        paths = self.realpaths(*args)
        self.run(f"chmod -R {mode} " + " ".join(f"'{path}'" for path in paths))
    def chown(self, user, *args):
        paths = self.realpaths(*args)
        self.run(f"chown -R {user} " + " ".join(f"'{path}'" for path in paths))
    def ssh_keygen(self, key_type, path, password=""):
        self.mkdir(self.dirname(path))
        self.run(f"ssh-keygen -t {key_type} -N \"{password}\" -f '{path}'")
    # I/O
    def file_write(self, path, string, sensitive=None, chunk_size=1024):
        self.rm(path)
        self.mkdir(self.dirname(path))
        self.run(f"touch '{path}'")
        for i in range(0, len(string), chunk_size):
            chunk = string[i:i + chunk_size]
            command = f"echo -n '{chunk}' >> '{path}'"
            if self.run(command, sensitive=sensitive).returncode != 0: return False
        return True
    def file_read(self, path): return Shell.stdout(self.run(f"cat '{path}'")) if self.exists(path) else ""
    def json_read(self, path):
        try: return json.loads(self.file_read(path))
        except BaseException: return {}
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
        self.run(f"git config --global --add safe.directory '{path}'") # Git doesn't think sudo is the owner of a git path despite having admin privileges. SMH

def chrootable(cls):
    if not hasattr(cls, "sh"): raise TypeError(f"Class {cls.__name__} must have a 'sh' attribute to be chrootable.")
    if not hasattr(chrootable, "registry"): chrootable.registry = []
    chrootable.registry.append(cls)
    @classmethod
    @contextlib.contextmanager
    def chroot(cls, sh):
        previous = cls.sh
        cls.sh = sh
        try: yield cls
        finally: cls.sh = previous
    cls.chroot = chroot
    return cls

@chrootable
class Config:
    sh = Shell()
    @classmethod
    def exists(cls): return cls.sh.exists(cls.get_config_path())
    @classmethod
    def read(cls): return cls.sh.json_read(cls.get_config_path())
    @classmethod
    def get(cls, key): return cls.read().get(key, None)
    @classmethod
    def set(cls, key, value): return cls.sh.json_write(cls.get_config_path(), key, value)
    @classmethod
    def reset_config(cls, host_path, target):
        cls.sh.rm(cls.get_config_path())
        cls.set_host_path(host_path)
        cls.set_target(target)
    @classmethod
    def create_secrets(cls, plain_text_password_path=None):
        if not cls.sh.exists(cls.get_secrets_path()):
            cls.sh.mkdir(cls.get_secrets_path())
        if not cls.sh.exists(cls.get_hashed_password_path()) or (plain_text_password_path and not cls.sh.exists(plain_text_password_path)):
            password = Interactive.ask_for_password()
            if plain_text_password_path: cls.sh.file_write(plain_text_password_path, password, sensitive=password)
            encrypted_password = Shell.stdout(cls.sh.run(f"mkpasswd -m sha-512 '{password}'", sensitive=password))
            cls.sh.file_write(cls.get_hashed_password_path(), encrypted_password, sensitive=encrypted_password)
    @classmethod
    def secure_secrets(cls):
        directories = cls.sh.find_directories(cls.get_secrets_path(), pattern="*")
        files = cls.sh.find_files(cls.get_secrets_path(), pattern="*")
        cls.sh.chown("root", cls.get_secrets_path(), *directories, *files) # Only root owns secrets
        cls.sh.chmod(700, cls.get_secrets_path(), *directories) # Traversable directories by anyone
        cls.sh.chmod(600, *files) # But only root can read or write files
    @classmethod
    def secure(cls, username):
        ignore_pattern = "*/{secrets}*" # Ignore secrets
        cls.sh.chown(username, cls.get_nixos_path()) # $username owns everything in /etc/nixos
        cls.sh.chmod(755, *cls.sh.find_directories(cls.get_nixos_path(), ignore_pattern=ignore_pattern)) # Directories are traversable
        cls.sh.chmod(644, *cls.sh.find_files(cls.get_nixos_path(), ignore_pattern=ignore_pattern)) # Owner can read write files
        cls.sh.chmod(755, *cls.sh.find(cls.get_nixos_path(), pattern="*/bin/* */scripts/*", ignore_pattern=ignore_pattern))# Owner can execute
        cls.sh.chmod(444, *cls.sh.find_files(f"{cls.get_nixos_path()}/.git/objects")) # Git files require specific permission
        cls.secure_secrets() # Secure the secrets using our shell (in case of chroot)
        cls.sh.git_add_safe_directory(cls.get_nixos_path())
    @classmethod
    def update(cls, rebuild_file_system=False, reboot=False):
        cls.create_secrets()
        cls.secure_secrets()
        if not cls.sh.exists(cls.get_config_path()):
            print("'CONFIG.JSON' IS MISSING.")
            host_path = Interactive.ask_for_host_path()
            cls.reset_config(host_path, Config.get_standard_flake_target())
            rebuild_file_system = True
        environment = ""
        if rebuild_file_system:
            environment = "NIXOS_INSTALL_BOOTLOADER=1"
            cls.secure(cls.sh.whoami())
        cls.sh.run(f"{environment} nixos-rebuild switch --flake {cls.sh.realpath(cls.get_nixos_path())}#{cls.get_host()}-{cls.get_target()}", capture_output=False)
        if reboot: Utils.reboot()
        else: Interactive.ask_to_reboot()
    @classmethod
    def eval(cls, attribute):
        cmd = f"nix --extra-experimental-features nix-command --extra-experimental-features flakes eval {cls.sh.realpath(cls.get_nixos_path())}#nixosConfigurations.{cls.get_host()}-{cls.get_target()}.{attribute}"
        if cmd in Shell.evals: return Shell.evals[cmd]
        output = Shell.stdout(cls.sh.run(cmd)).replace("\"", "")
        if output == "true": output = True
        if output == "false": output = False
        Shell.evals[cmd] = output
        return output
    @classmethod
    def metadata(cls, pkg): return json.loads(Shell.stdout(cls.sh.run(f"nix --extra-experimental-features nix-command --extra-experimental-features flakes flake metadata {pkg} --json -I {cls.sh.realpath(cls.get_nixos_path())}")))
    # Readwrite
    @classmethod
    def set_host_path(cls, host_path): return cls.set("host_path", host_path)
    @classmethod
    def get_host_path(cls): return cls.get("host_path")
    @classmethod
    def get_hosts_path(cls): return f"{cls.get_nixos_path()}/modules/hosts"
    @classmethod
    def set_target(cls, target): return cls.set("target", target)
    @classmethod
    def get_target(cls): return cls.get("target")
    # Readonly
    @classmethod
    def get_standard_flake_target(cls): return "Standard-Boot"
    @classmethod
    def get_secure_boot_flake_target(cls): return "Secure-Boot"
    @classmethod
    def get_disk_operation_target(cls): return "Disk-Operation"
    @classmethod
    def get_disk_by_part_label_root(cls): return cls.eval("config.settings.disk.by.partlabel.root")
    @classmethod
    def get_tpm_device(cls): return cls.eval("config.settings.tpm.device")
    @classmethod
    def get_tpm_version_path(cls): return cls.eval("config.settings.tpm.versionPath")
    @classmethod
    def get_host(cls): return cls.sh.basename(cls.get_host_path()).replace(".nix", "")
    @classmethod
    def get_architecture(cls): return cls.sh.parent_name(cls.get_host_path())
    @classmethod
    def get_hashed_password_path(cls): return cls.get_secrets_path() + "/" + cls.eval("config.settings.secrets.hashedPasswordFile")
    @classmethod
    def get_secrets_path(cls): return cls.eval("config.settings.secrets.path")
    @classmethod
    def get_settings_path(cls): return f"{Config.get_nixos_path()}/modules/settings.nix"
    @classmethod
    def get_config_path(cls): return f"{cls.get_nixos_path()}/config.json"
    @classmethod
    def get_flake_path(cls): return f"{Config.get_nixos_path()}/flake.nix"
    @classmethod
    def get_nixos_path(cls): return "/etc/nixos"

@chrootable
class Snapshot:
    sh = Shell()
    @classmethod
    def get_snapshots_path(cls): return Config.eval("config.settings.disk.subvolumes.snapshots.mountPoint")
    @classmethod
    def get_clean_snapshot_name(cls): return Config.eval("config.settings.disk.immutability.persist.snapshots.cleanName")
    @classmethod
    def get_subvolumes_to_reset_on_boot(cls): return [ pair.split("=") for pair in Config.eval("config.settings.disk.subvolumes.nameMountPointPairs.resetOnBoot").split() ]
    @classmethod
    def get_clean_snapshot_path(cls, subvolume_name): return f"{cls.get_snapshots_path()}/{subvolume_name}/{cls.get_clean_snapshot_name()}"
    @classmethod
    def create_initial_snapshots(cls):
        for subvolume_name, subvolume_mount_point in cls.get_subvolumes_to_reset_on_boot():
            clean_snapshot_path = cls.get_clean_snapshot_path(subvolume_name)
            try:
                cls.sh.rm(clean_snapshot_path)
                cls.sh.mkdir(cls.sh.dirname(clean_snapshot_path))
                cls.sh.run(f"btrfs subvolume snapshot -r {subvolume_mount_point} {clean_snapshot_path}")
            except BaseException as e:
                Utils.log_error(f"Failed to create a clean snapshot for {subvolume_name}\n{e}")

@chrootable
class Interactive:
    sh = Shell()
    @classmethod
    def confirm(cls, prompt):
        while True:
            response = input(f"{prompt} (y/n): ").lower()
            if response in ("y","yes"):
                return True
            if response in ("n","no"):
                return False
            Utils.print("Invalid input. Enter 'y' or 'n'.")
    @classmethod
    def ask_for_host_path(cls):
        hosts_paths = glob.glob(f"{Config.get_hosts_path()}/**/*.nix", recursive=True)
        formatted_hosts_paths = [ cls.sh.basename(host_path).replace(".nix", "") + " (" + cls.sh.parent_name(host_path) + ")" for host_path in hosts_paths ]
        while True:
            for i, name in enumerate(formatted_hosts_paths): Utils.print(f"{i+1}) {name}")
            try: return hosts_paths[int(input("> ")) - 1]
            except KeyboardInterrupt: Utils.abort()
            except BaseException: Utils.print_error("Invalid choice.")
    @classmethod
    def ask_for_password(cls):
        while True:
            password = getpass.getpass("Set your password: ")
            if password == getpass.getpass("Confirm your password: "): return password
            Utils.log_error("Passwords do not match.")
    @classmethod
    def ask_to_reboot(cls): return Utils.reboot() if Interactive.confirm("Restart now?") else False

@chrootable
class Utils:
    sh = Shell()
    # LOG LEVEL
    LOG_INFO = False
    # Color constants
    GRAY = "\033[90m"
    ORANGE = "\033[38;5;208m"
    RED = "\033[31m"
    RESET = "\033[0m"
    @classmethod
    def toggle(cls, argv, on_enable = None, on_disable = None, on_exception = None):
        try:
            match Utils.parse_args(argv, "enable", "disable"):
                case ["enable"]: on_enable()
                case ["disable"]: on_disable()
                case _: return Utils.abort(f"Usage: {cls.sh.basename(inspect.stack()[1].filename)} (enable | disable)")
        except BaseException as exception:
            Utils.log_error(f"Caught exception: {exception}.")
            if on_exception: on_exception()
            raise
    @classmethod
    def parse_args(cls, argv, *accepted_args): return [] if not argv or not accepted_args else [arg for arg in argv if arg in set(accepted_args)]
    @classmethod
    def require_root(cls): cls.sh.require_root()
    @classmethod
    def abort(cls, message=""):
        if message: cls.log_error(message)
        return sys.exit(1)
    @classmethod
    def reboot(cls): return cls.sh.run("shutdown -r now") 
    @classmethod
    def log(cls, message): 
        if cls.LOG_INFO: print(f"{cls.GRAY}LOG: {message}{cls.RESET}")
    @classmethod
    def log_error(cls, message): print(f"{cls.ORANGE}ERROR: {message}{cls.RESET}", file=sys.stderr)
    @classmethod
    def print(cls, message): print(message)
    @classmethod
    def print_warning(cls, message): cls.print_error(message)
    @classmethod
    def print_error(cls, message): print(f"{cls.RED}{message}{cls.RESET}", file=sys.stderr)
