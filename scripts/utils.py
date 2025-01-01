import sys, subprocess, json, getpass, glob, contextlib, re

class Shell:
    def __init__(self, root_required=False):
        self.chroots = []
        if root_required: self.require_root()
    # User
    def require_root(self):
        if Utils.stdout(self.run("id -u")) != "0":
            Utils.print_error("Please run this script with sudo.")
            Utils.abort()
    def whoami(self): return Utils.stdout(self.run("who")).split()[0]
    def hostname(self): return Utils.stdout(self.run("hostname"))
    # Execution
    @contextlib.contextmanager
    def chroot(self, path):
        try:
            self.chroots.append(path)
            yield self
        finally: self.chroots.pop()
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
        path = self.readlink(path) # find doesn't work on symlinks
        command_arg = f"find '{path}' {pattern_arg} {type_arg} {ignore_arg}".strip()
        output = Utils.stdout(self.run(command_arg))
        return [] if not output else output.split("\n")
    def find_directories(self, path, pattern="*", ignore_pattern=None):
        return self.find(path, pattern=pattern, ignore_pattern=ignore_pattern, ignore_files=True)
    def find_files(self, path, pattern="*", ignore_pattern=None):
        return self.find(path, pattern=pattern, ignore_pattern=ignore_pattern, ignore_directories=True)
    def symlink(self, source, target): return self.run(f"ln -s {source} {target}")
    def is_symlink(self, path): return self.run(f"[ -L '{path}' ]", check=False).returncode == 0
    def readlink(self, path): return Utils.stdout(self.run(f"readlink '{path}'")) if self.is_symlink(path) else path
    def is_dir(self, path): return self.run(f"[ -d '{path}' ]", check=False).returncode == 0
    def exists(self, *args): return self.run(" ".join(f"[ -e {arg} ]" for arg in args) + " && true", check=False).returncode == 0
    def basename(self, path): return Utils.stdout(self.run(f"basename '{path}'"))
    def dirname(self, path): return Utils.stdout(self.run(f"dirname '{path}'"))
    def parent_name(self, path): return self.basename(self.dirname(path))
    # Security
    def chmod(self, path, mode):
        path = self.readlink(path)
        self.run(f"chmod {'-R' if self.is_dir(path) else ''} {mode} '{path}'")
    def chown(self, path, user):
        path = self.readlink(path)
        self.run(f"chown {'-R' if self.is_dir(path) else ''} {user} '{path}'")
    # I/O
    def file_write(self, path, string, sensitive=None, **kwargs):
        self.rm(path)
        self.mkdir(self.dirname(path))
        return self.run(f"echo -n '{string}' > '{path}'", sensitive=sensitive, **kwargs).returncode == 0
    def file_read(self, path): return Utils.stdout(self.run(f"cat '{path}'")) if self.exists(path) else ""
    # Git
    def git_add_safe_directory(self, path):
        path = self.readlink(path)
        self.run(f"git config --global --add safe.directory '{path}'") # Git doesn't think sudo is the owner of a git path despite having admin privileges. SMH

class Config:
    sh = Shell()
    @classmethod
    def exists(cls): return cls.sh.exists(cls.get_config_path())
    @classmethod
    def read(cls):
        try: return json.loads(cls.sh.file_read(cls.get_config_path()))
        except BaseException: return {}
    @classmethod
    def get(cls, key): return cls.read().get(key, None)
    @classmethod
    def set(cls, key, value):
        data = cls.read()
        data[key] = value
        return cls.sh.file_write(cls.get_config_path(), json.dumps(data))
    @classmethod
    def reset_config(cls, host_path, target):
        cls.sh.rm(cls.get_config_path())
        cls.set_host_path(host_path)
        cls.set_target(target)
    @classmethod
    def reset_secrets(cls, plain_text_password_path=None):
        cls.sh.rm(cls.get_secrets_path())
        cls.sh.mkdir(cls.get_secrets_path())
        password = Interactive.ask_for_password()
        if plain_text_password_path: cls.sh.file_write(plain_text_password_path, password, sensitive=password)
        encrypted_password = Utils.stdout(Utils.encrypt_password(password))
        cls.sh.file_write(cls.get_hashed_password_path(), encrypted_password, sensitive=encrypted_password)
    @classmethod
    def secure_secrets(cls, path_to_secrets, sh=None):
        sh = cls.sh if sh is None else sh
        secrets = [(path_to_secrets, "700", "root")]
        secrets += [(path, "700", "root") for path in sh.find_directories(path_to_secrets, pattern="*")] # 700 for sub directories
        secrets += [(path, "600", "root") for path in sh.find_files(path_to_secrets, pattern="*")] # 600 for sub files
        for (path, mode, user) in secrets:
            sh.chown(path, user)
            sh.chmod(path, mode)
    @classmethod
    def secure(cls, username, sh=None):
        sh = cls.sh if sh is None else sh
        ignore_pattern = "*/{secrets}*" # Ignore secrets
        nixos_path = cls.get_nixos_path() # /etc/nixos
        sh.chown(nixos_path, username) # $username owns everything in /etc/nixos
        for directory_path in [nixos_path] + sh.find_directories(nixos_path, ignore_pattern=ignore_pattern): sh.chmod(directory_path, 755) # Directories are traversable
        for file_path in sh.find_files(nixos_path, ignore_pattern=ignore_pattern): sh.chmod(file_path, 644) # Owner can read write files
        for executable in sh.find(nixos_path, pattern="*/bin/* */scripts/*", ignore_pattern=ignore_pattern): sh.chmod(executable, 755)# Owner can execute
        for git_object in sh.find_files(f"{nixos_path}/.git/objects"): sh.chmod(git_object, 444)
        cls.secure_secrets(cls.get_secrets_path(), sh) # Secure the secrets using our shell (in case of chroot)
        sh.git_add_safe_directory(nixos_path)
    @classmethod
    def update(cls, rebuild_file_system=False):
        if not cls.sh.exists(cls.get_secrets_path()):
            Utils.log_error("'SECRETS' IS MISSING.")
            cls.reset_secrets()
            cls.secure_secrets(cls.get_secrets_path())
            rebuild_file_system = True
        if not cls.sh.exists(cls.get_config_path()):
            print("'CONFIG.JSON' IS MISSING.")
            host_path = Interactive.ask_for_host_path()
            cls.reset_config(host_path, "standard")
            rebuild_file_system = True
        environment = ""
        if rebuild_file_system:
            environment = "NIXOS_INSTALL_BOOTLOADER=1"
            cls.secure(cls.sh.whoami(), cls.get_secrets_path())
        return cls.sh.run(f"{environment} nixos-rebuild switch --flake {cls.sh.readlink(cls.get_nixos_path())}#{cls.get_host()}-{cls.get_target()}")
    # Readwrite
    @classmethod
    def set_host_path(cls, host_path): return cls.set("host_path", host_path)
    @classmethod
    def get_host_path(cls): return cls.get("host_path")
    @classmethod
    def set_target(cls, target): return cls.set("target", target)
    @classmethod
    def get_target(cls): return cls.get("target")
    # Readonly
    @classmethod
    def get_host(cls): return cls.sh.basename(cls.get_host_path()).replace(".nix", "")
    @classmethod
    def get_architecture(cls): return cls.sh.parent_name(cls.get_host_path())
    @classmethod
    def get_hashed_password_path(cls): return cls.get_secrets_path() + "/" + Utils.get_value_from_variables("hashedPasswordFile")
    @classmethod
    def get_secrets_path(cls): return Utils.get_value_from_variables("secrets")
    @classmethod
    def get_variables_path(cls): return f"{Config.get_nixos_path()}/variables.nix"
    @classmethod
    def get_config_path(cls): return f"{cls.get_nixos_path()}/config.json"
    @classmethod
    def get_flake_path(cls): return f"{Config.get_nixos_path()}/flake.nix"
    @classmethod
    def get_nixos_path(cls): return "/etc/nixos"

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
        hosts_paths = glob.glob(f"{Config.get_nixos_path()}/hosts/**/*.nix", recursive=True)
        formatted_hosts_paths = [ cls.sh.basename(host_path).replace(".nix", "") for host_path in hosts_paths ]
        potential_matches = [ formatted_hosts_path for formatted_hosts_path in formatted_hosts_paths if formatted_hosts_path == cls.sh.hostname() ]
        if potential_matches:
            match = hosts_paths[formatted_hosts_paths.index(potential_matches[0])]
            if Interactive.confirm(f"Use {match}?"): return match
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
    def ask_to_reboot(cls): return cls.sh.run("shutdown -r now") if Interactive.confirm("Restart now?") else False

class Utils:
    sh = Shell()
    # Color constants
    GRAY = "\033[90m"
    ORANGE = "\033[38;5;208m"
    RED = "\033[31m"
    RESET = "\033[0m"
    @classmethod
    def require_root(cls): cls.sh.require_root()
    @classmethod
    def abort(cls): return sys.exit(1)
    @classmethod
    def get_value_from_path(cls, path, key, start='"', end='"', trim_whitespace=True):
        file_contents = cls.sh.file_read(path)
        return Utils.get_string_between(file_contents, start=start, end=end, start_from=key, trim_whitespace=trim_whitespace)
    @classmethod
    def get_value_from_variables(cls, key): return cls.get_value_from_path(Config.get_variables_path(), key)
    @classmethod
    def get_string_between(cls, text, start, end, start_from=None, trim_whitespace=False):
        def trimmer(x): return x.replace(" ", "") if trim_whitespace else x
        text, start, end, start_from = [ trimmer(text), trimmer(start), trimmer(end), trimmer(start_from) if start_from else start_from ]
        text = text[text.find(start_from):] if start_from else text
        pattern = re.escape(start) + r"(.*?)" + re.escape(end)
        match = re.search(pattern, text, re.DOTALL)
        return match.group(1) if match else None
    @classmethod
    def stdout(cls, completed_process): return completed_process.stdout.strip()
    @classmethod
    def encrypt_password(cls, password): return cls.sh.run(f"mkpasswd -m sha-512 '{password}'", sensitive=password)
    @classmethod
    def log(cls, message): print(f"{cls.GRAY}LOG: {message}{cls.RESET}")
    @classmethod
    def log_error(cls, message): print(f"{cls.ORANGE}ERROR: {message}{cls.RESET}", file=sys.stderr)
    @classmethod
    def print(cls, message): print(message)
    @classmethod
    def print_error(cls, message): print(f"{cls.RED}{message}{cls.RESET}", file=sys.stderr)
