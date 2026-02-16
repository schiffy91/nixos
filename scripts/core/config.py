#! /usr/bin/env nix-shell
#! nix-shell -i python3 -p python3
import json

from .shell import Shell, chrootable
from .utils import Utils
from .interactive import Interactive


@chrootable
class Config:
    sh = Shell()

    @classmethod
    def exists(cls):
        return cls.sh.exists(cls.get_config_path())

    @classmethod
    def read(cls):
        return cls.sh.json_read(cls.get_config_path())

    @classmethod
    def get(cls, key):
        return cls.read().get(key, None)

    @classmethod
    def set(cls, key, value):
        return cls.sh.json_write(cls.get_config_path(), key, value)

    @classmethod
    def reset_config(cls, host_path, target):
        cls.sh.rm(cls.get_config_path())
        cls.set_host_path(host_path)
        cls.set_target(target)

    @classmethod
    def create_secrets(cls, plain_text_password_path=None):
        if not cls.sh.exists(cls.get_secrets_path()):
            cls.sh.mkdir(cls.get_secrets_path())
        needs_password = (
            not cls.sh.exists(cls.get_hashed_password_path())
            or (plain_text_password_path
                and not cls.sh.exists(plain_text_password_path))
        )
        if needs_password:
            password = Interactive.ask_for_password()
            if plain_text_password_path:
                cls.sh.file_write(
                    plain_text_password_path, password, sensitive=password
                )
            encrypted = Shell.stdout(cls.sh.run(
                f"mkpasswd -m sha-512 '{password}'", sensitive=password
            ))
            cls.sh.file_write(
                cls.get_hashed_password_path(), encrypted, sensitive=encrypted
            )

    @classmethod
    def secure_secrets(cls):
        if not cls.sh.exists(cls.get_secrets_path()):
            return
        directories = cls.sh.find_directories(
            cls.get_secrets_path(), pattern="*"
        )
        files = cls.sh.find_files(cls.get_secrets_path(), pattern="*")
        cls.sh.chown("root", cls.get_secrets_path(), *directories, *files)
        cls.sh.chmod(700, cls.get_secrets_path(), *directories)
        cls.sh.chmod(600, *files)

    @classmethod
    def secure(cls, username):
        ignore_pattern = "*/{secrets}*"
        cls.sh.chown(username, cls.get_nixos_path())
        cls.sh.chmod(
            755,
            *cls.sh.find_directories(
                cls.get_nixos_path(), ignore_pattern=ignore_pattern
            )
        )
        cls.sh.chmod(
            644,
            *cls.sh.find_files(
                cls.get_nixos_path(), ignore_pattern=ignore_pattern
            )
        )
        cls.sh.chmod(
            755,
            *cls.sh.find(
                cls.get_nixos_path(),
                pattern="*/scripts/* */bin/*",
                ignore_pattern=ignore_pattern
            )
        )
        cls.sh.chmod(
            444,
            *cls.sh.find_files(f"{cls.get_nixos_path()}/.git/objects")
        )
        cls.sh.git_add_safe_directory(cls.get_nixos_path())
        cls.secure_secrets()

    @classmethod
    def update(cls, rebuild_file_system=False, reboot=False,
               delete_cache=False, upgrade=False):
        if delete_cache:
            cls.sh.run("nix-collect-garbage -d", capture_output=False)
            cls.sh.rm("/root/.cache")
            cls.sh.run("nix-store --verify --repair", capture_output=False)
        if upgrade:
            cls.sh.rm("/root/.cache")
            cls.sh.run(
                f"nix flake update --flake {Config.get_nixos_path()}",
                capture_output=False
            )
        cls.create_secrets()
        cls.secure(cls.sh.whoami())
        if not cls.sh.exists(cls.get_config_path()):
            Utils.print_error(f"'{cls.get_config_path()}' IS MISSING.")
            host_path = Interactive.ask_for_host_path(
                cls.get_hosts_path()
            )
            cls.reset_config(host_path, Config.get_standard_flake_target())
            rebuild_file_system = True
        environment = ""
        if rebuild_file_system:
            environment = "NIXOS_INSTALL_BOOTLOADER=1"
        nixos_path = cls.sh.realpath(cls.get_nixos_path())
        host = cls.get_host()
        target = cls.get_target()
        cls.sh.run(
            f"{environment} nixos-rebuild switch "
            f"--flake {nixos_path}#{host}-{target}",
            capture_output=False
        )
        if reboot:
            Utils.reboot()
        else:
            Interactive.ask_to_reboot()

    @classmethod
    def eval(cls, attribute) -> "str | bool":
        nixos_path = cls.sh.realpath(cls.get_nixos_path())
        host = cls.get_host()
        target = cls.get_target()
        cmd = (
            f"nix --extra-experimental-features nix-command "
            f"--extra-experimental-features flakes eval "
            f"{nixos_path}#nixosConfigurations.{host}-{target}.{attribute}"
        )
        if cmd in Shell.evals:
            return Shell.evals[cmd]
        output = Shell.stdout(cls.sh.run(cmd)).replace("\"", "")
        if output == "true":
            output = True  # type: ignore[assignment]
        if output == "false":
            output = False  # type: ignore[assignment]
        Shell.evals[cmd] = output
        return output

    @classmethod
    def metadata(cls, pkg):
        nixos_path = cls.sh.realpath(cls.get_nixos_path())
        cmd = (
            f"nix --extra-experimental-features nix-command "
            f"--extra-experimental-features flakes flake metadata "
            f"{pkg} --json -I {nixos_path}"
        )
        return json.loads(Shell.stdout(cls.sh.run(cmd)))

    # Readwrite
    @classmethod
    def set_host_path(cls, host_path):
        return cls.set("host_path", host_path)

    @classmethod
    def get_host_path(cls):
        return cls.get("host_path")

    @classmethod
    def get_hosts_path(cls):
        return f"{cls.get_nixos_path()}/modules/hosts"

    @classmethod
    def set_target(cls, target):
        return cls.set("target", target)

    @classmethod
    def get_target(cls):
        return cls.get("target")

    # Readonly
    @classmethod
    def get_standard_flake_target(cls):
        return "Standard-Boot"

    @classmethod
    def get_secure_boot_flake_target(cls):
        return "Secure-Boot"

    @classmethod
    def get_disk_operation_target(cls):
        return "Disk-Operation"

    @classmethod
    def get_disk_by_part_label_root(cls):
        return cls.eval("config.settings.disk.by.partlabel.root")

    @classmethod
    def get_tpm_device(cls):
        return cls.eval("config.settings.tpm.device")

    @classmethod
    def get_tpm_version_path(cls):
        return cls.eval("config.settings.tpm.versionPath")

    @classmethod
    def get_host(cls):
        return cls.sh.basename(cls.get_host_path()).replace(".nix", "")

    @classmethod
    def get_architecture(cls):
        return cls.sh.parent_name(cls.get_host_path())

    @classmethod
    def get_hashed_password_path(cls):
        return (
            str(cls.get_secrets_path()) + "/"
            + str(cls.eval("config.settings.secrets.hashedPasswordFile"))
        )

    @classmethod
    def get_secrets_path(cls):
        return cls.eval("config.settings.secrets.path")

    @classmethod
    def get_settings_path(cls):
        return f"{Config.get_nixos_path()}/modules/settings.nix"

    @classmethod
    def get_config_path(cls):
        return f"{cls.get_nixos_path()}/config.json"

    @classmethod
    def get_flake_path(cls):
        return f"{Config.get_nixos_path()}/flake.nix"

    @classmethod
    def get_nixos_path(cls):
        return "/etc/nixos"
