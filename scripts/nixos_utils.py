import sys, subprocess, json, getpass, glob, contextlib

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
    def file_read(self, path): return Utils.stdout(self.run(f"cat '{path}'"))
    def file_get(self, path, start, end):
        try: return Utils.string_get(self.file_read(path), start, end)
        except BaseException: return None
    def file_get_value(self, path, key):
        try: return Utils.string_get(self.file_read(path).replace(" ", ""), f"{key}=\"", "\"")
        except BaseException: return None
    # Git
    def git_add_safe_directory(self, path):
        path = self.readlink(path)
        self.run(f"git config --global --add safe.directory '{path}'") # Git doesn't think sudo is the owner of a git path despite having admin privileges. SMH

class NixOSConfig:
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
    def secure(cls, username, nixos_path, path_to_secrets, sh=None):
        sh = cls.sh if sh is None else sh
        ignore_pattern = "*/{secrets}*" # Ignore secrets
        sh.chown(nixos_path, username) # $username owns everything in ~/nixos
        for directory_path in [nixos_path] + sh.find_directories(nixos_path, ignore_pattern=ignore_pattern): sh.chmod(directory_path, 755) # Directories are traversable
        for file_path in sh.find_files(nixos_path, ignore_pattern=ignore_pattern): sh.chmod(file_path, 644) # Owner can read write files
        for executable in sh.find(nixos_path, pattern="*/bin/* */scripts/*", ignore_pattern=ignore_pattern): sh.chmod(executable, 755)# Owner can execute
        for git_object in sh.find_files(f"{nixos_path}/.git/objects"): sh.chmod(git_object, 444)
        cls.secure_secrets(path_to_secrets, sh) # Secure the secrets using our shell (in case of chroot)
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
            cls.secure(cls.sh.whoami(), cls.get_nixos_path(), cls.get_secrets_path())
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
    # Readonly only
    @classmethod
    def get_host(cls): return cls.sh.basename(cls.get_host_path()).replace(".nix", "")
    @classmethod
    def get_architecture(cls): return cls.sh.parent_name(cls.get_host_path())
    @classmethod
    def get_hashed_password_path(cls): return cls.sh.file_get_value(f"{cls.get_nixos_path()}/modules/users.nix", key="hashedPasswordFile")
    @classmethod
    def get_secrets_path(cls): return cls.sh.dirname(cls.get_hashed_password_path())
    @classmethod
    def get_config_path(cls): return f"{cls.get_nixos_path()}/config.json"
    @classmethod
    def get_nixos_path(cls): return "/etc/nixos"

class NixOSInstaller:
    sh = Shell()
    @classmethod
    def get_remote_root_path(cls): return "/mnt"
    @classmethod
    def get_install_path(cls, remote=False): return f"{cls.get_remote_root_path()}{NixOSConfig.get_nixos_path()}" if remote else NixOSConfig.get_nixos_path()
    @classmethod
    def get_etc_path(cls, remote=False): return f"{cls.get_remote_root_path()}/etc" if remote else "/etc"
    @classmethod
    def get_store_path(cls, remote=False): return f"{cls.get_remote_root_path()}/nix/store" if remote else "/nix/store"
    @classmethod
    def get_nix_tmp_path(cls, remote=False): return f"{cls.get_remote_root_path()}/nix/tmp" if remote else "/nix/tmp"
    @classmethod
    def get_home_path(cls, remote=False): return f"{cls.get_remote_root_path()}/home" if remote else "/home"
    @classmethod
    def get_user_path(cls, remote=False): return f"{cls.get_remote_root_path()}/home/{cls.get_username()}" if remote else f"/home/{cls.get_username()}"
    @classmethod
    def get_nixos_path(cls, remote=False): return f"{cls.get_remote_root_path()}{cls.get_user_path()}/nixos" if remote else f"{cls.get_user_path()}/nixos"
    @classmethod
    def get_disk_path(cls): return cls.sh.file_get_value(NixOSConfig.get_host_path(), key="diskOverrides.device")
    @classmethod
    def get_plain_text_password_path(cls): return cls.sh.file_get_value(f"{cls.get_install_path()}/modules/disk.nix", key="passwordFile")
    @classmethod
    def get_username(cls): return cls.sh.file_get_value(f"{cls.get_install_path()}/modules/users.nix", key="rootUser")
    @classmethod
    def disk_mount(cls): return cls.run_disko("mount")
    @classmethod
    def disk_erase_and_mount(cls): return cls.run_disko("destroy,format,mount")
    @classmethod
    def run_disko(cls, mode):
        version = cls.sh.file_get(f"{NixOSConfig.get_nixos_path()}/flake.nix", start="github:nix-community/disko/", end='";')
        command = f"nix --extra-experimental-features \"nix-command flakes\" run github:nix-community/disko/{version} --verbose -- " \
                f"--show-trace --flake {NixOSConfig.get_nixos_path()}#{NixOSConfig.get_host()}-mount --mode {mode}"
        return cls.sh.run(command, capture_output=False)
    @classmethod
    def install_nixos(cls):
        # Paths
        sh = cls.sh
        remote_root = cls.get_remote_root_path()
        nixos_path = cls.get_nixos_path() # ~/nixos
        remote_nixos_path = cls.get_nixos_path(remote=True) # ~/mnt/nixos
        install_path = cls.get_install_path() # /etc/nixos
        remote_install_path = cls.get_install_path(remote=True) # /mnt/etc/nixos
        remote_tmp_path = cls.get_nix_tmp_path(remote=True) # /mnt/nix/tmp
        username = cls.get_username() # alexanderschiffhauer
        secrets = sh.basename(sh.dirname(NixOSConfig.get_hashed_password_path())) # secrets
        # nixos-install args
        host = NixOSConfig.get_host()
        target = NixOSConfig.get_target()
        env = f"TMPDIR={remote_tmp_path}"
        flake_arg = f"--flake {remote_install_path}#{host}-{target}"
        root_arg = f"--root {remote_root}"
        options = "--no-channel-copy --show-trace --no-root-password --cores 0"
        cmd = f"nixos-install {flake_arg} {root_arg} {options}"
        # Prepare nixos-install
        sh.rm(remote_nixos_path, remote_install_path) # Remove up destinations
        sh.mkdir(cls.get_etc_path(remote=True), cls.get_store_path(remote=True), remote_tmp_path) # Ensure dependent directories exist
        sh.cpdir(install_path, remote_install_path) # e.g. cp -R /etc/nixos /mnt/etc/nixos
        # nixos-install
        sh.run(cmd=cmd, env=env, capture_output=False)
        # Symlink and permission within chroot
        with sh.chroot(remote_root):
            sh.mv(install_path, nixos_path) # Move nixos to home directory
            NixOSConfig.secure(username, nixos_path, f"{nixos_path}/{secrets}", sh) # Secure
            sh.symlink(nixos_path, install_path) # Smylink ~/nixos to e.g. /etc/nixos
        # Cleanup
        sh.rm(f"{remote_tmp_path}")

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
        hosts_paths = glob.glob(f"{NixOSConfig.get_nixos_path()}/hosts/**/*.nix", recursive=True)
        formatted_hosts_paths = [ cls.sh.basename(host_path).replace(".nix", "") for host_path in hosts_paths ]
        potential_matches = [ formatted_hosts_path for formatted_hosts_path in formatted_hosts_paths if formatted_hosts_path == cls.sh.hostname() ]
        if potential_matches:
            match = hosts_paths[formatted_hosts_paths.index(potential_matches[0])]
            if Interactive.confirm(f"Use {match}?"): return match
        while True:
            for i, name in enumerate(formatted_hosts_paths): Utils.print(f"{i+1}) {name}")
            try: return hosts_paths[int(input("> ")) - 1]
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
    def string_get(cls, string, start, end):
        try: return string.split(start)[1].split(end)[0]
        except BaseException: return None
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
