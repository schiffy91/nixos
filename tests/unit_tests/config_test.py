import shlex
from contextlib import ExitStack
from unittest.mock import patch

import pytest

from lib.config import Config
from lib.interactive import Interactive
from lib.shell import Shell
from lib.utils import Utils

class TestConfigPaths:
    def test_nixos_path(self):
        assert Config.get_nixos_path() == "/etc/nixos"
    def test_config_path(self):
        assert Config.get_config_path() == "/etc/nixos/config.json"
    def test_flake_path(self):
        assert Config.get_flake_path() == "/etc/nixos/flake.nix"
    def test_settings_path(self):
        assert Config.get_settings_path() == "/etc/nixos/modules/settings.nix"
    def test_hosts_path(self):
        assert Config.get_hosts_path() == "/etc/nixos/modules/hosts"

class TestConfigTargets:
    def test_standard(self):
        assert Config.get_standard_flake_target() == "Standard-Boot"
    def test_secure_boot(self):
        assert Config.get_secure_boot_flake_target() == "Secure-Boot"
    def test_disk_operation(self):
        assert Config.get_disk_operation_target() == "Disk-Operation"

class TestConfigReadWrite:
    def test_exists_true(self, mock_shell):
        Config.sh = mock_shell
        with patch.object(mock_shell, "exists", return_value=True):
            assert Config.exists() is True
    def test_exists_false(self, mock_shell):
        Config.sh = mock_shell
        with patch.object(mock_shell, "exists", return_value=False):
            assert Config.exists() is False
    def test_read(self, mock_shell):
        Config.sh = mock_shell
        data = {"host_path": "/path", "target": "Standard-Boot"}
        with patch.object(mock_shell, "json_read", return_value=data):
            assert Config.read() == data
    def test_get_existing_key(self, mock_shell):
        Config.sh = mock_shell
        with patch.object(Config, "read", return_value={"key": "val"}):
            assert Config.get("key") == "val"
    def test_get_missing_key(self, mock_shell):
        Config.sh = mock_shell
        with patch.object(Config, "read", return_value={}):
            assert Config.get("missing") is None
    def test_get_none_value(self, mock_shell):
        Config.sh = mock_shell
        with patch.object(Config, "read", return_value={"k": None}):
            assert Config.get("k") is None
    def test_get_host_path(self, mock_shell):
        Config.sh = mock_shell
        with patch.object(Config, "get", return_value="/path/host.nix"):
            assert Config.get_host_path() == "/path/host.nix"
    def test_get_target(self, mock_shell):
        Config.sh = mock_shell
        with patch.object(Config, "get", return_value="Standard-Boot"):
            assert Config.get_target() == "Standard-Boot"
    def test_reset_config_calls_rm_first(self, mock_shell):
        Config.sh = mock_shell
        order = []
        with ExitStack() as s:
            s.enter_context(patch.object(
                mock_shell, "rm", side_effect=lambda *a: order.append("rm")))
            s.enter_context(patch.object(
                Config, "set_host_path",
                side_effect=lambda *a: order.append("set_host_path")))
            s.enter_context(patch.object(
                Config, "set_target",
                side_effect=lambda *a: order.append("set_target")))
            Config.reset_config("/path/host.nix", "Standard-Boot")
            assert order == ["rm", "set_host_path", "set_target"]
    def test_reset_config_passes_correct_args(self, mock_shell):
        Config.sh = mock_shell
        with ExitStack() as s:
            rm = s.enter_context(patch.object(mock_shell, "rm"))
            sh = s.enter_context(patch.object(Config, "set_host_path"))
            st = s.enter_context(patch.object(Config, "set_target"))
            Config.reset_config("/path/host.nix", "Secure-Boot")
            rm.assert_called_once_with("/etc/nixos/config.json")
            sh.assert_called_once_with("/path/host.nix")
            st.assert_called_once_with("Secure-Boot")

class TestConfigEval:
    def setup_method(self):
        Shell.evals.clear()
    def _patch_eval_deps(self, s, mock_shell):
        s.enter_context(patch.object(
            mock_shell, "realpath", return_value="/etc/nixos"))
        s.enter_context(patch.object(Config, "get_host", return_value="host"))
        s.enter_context(patch.object(
            Config, "get_target", return_value="Standard-Boot"))
    def test_caches_results(self, mock_shell):
        Config.sh = mock_shell
        with ExitStack() as s:
            run = s.enter_context(patch.object(mock_shell, "run"))
            self._patch_eval_deps(s, mock_shell)
            run.return_value.stdout = "test-value"
            r1 = Config.eval("config.test.attr")
            r2 = Config.eval("config.test.attr")
            assert r1 == r2 == "test-value"
            assert run.call_count == 1
    def test_boolean_true(self, mock_shell):
        Config.sh = mock_shell
        with ExitStack() as s:
            run = s.enter_context(patch.object(mock_shell, "run"))
            self._patch_eval_deps(s, mock_shell)
            run.return_value.stdout = '"true"'
            assert Config.eval("config.test.bool") is True
    def test_boolean_false(self, mock_shell):
        Config.sh = mock_shell
        with ExitStack() as s:
            run = s.enter_context(patch.object(mock_shell, "run"))
            self._patch_eval_deps(s, mock_shell)
            run.return_value.stdout = '"false"'
            assert Config.eval("config.test.bool") is False
    def test_string_value(self, mock_shell):
        Config.sh = mock_shell
        with ExitStack() as s:
            run = s.enter_context(patch.object(mock_shell, "run"))
            self._patch_eval_deps(s, mock_shell)
            run.return_value.stdout = '"some-string"'
            assert Config.eval("config.test.str") == "some-string"
    def test_strips_quotes(self, mock_shell):
        Config.sh = mock_shell
        with ExitStack() as s:
            run = s.enter_context(patch.object(mock_shell, "run"))
            self._patch_eval_deps(s, mock_shell)
            run.return_value.stdout = '"/etc/nixos/secrets"'
            result = Config.eval("config.test.path")
            assert '"' not in result
            assert result == "/etc/nixos/secrets"
    def test_different_attributes_cached_separately(self, mock_shell):
        Config.sh = mock_shell
        with ExitStack() as s:
            run = s.enter_context(patch.object(mock_shell, "run"))
            self._patch_eval_deps(s, mock_shell)
            run.return_value.stdout = "val1"
            Config.eval("config.attr1")
            run.return_value.stdout = "val2"
            Config.eval("config.attr2")
            assert run.call_count == 2
    def test_eval_cmd_includes_flake_reference(self, mock_shell):
        Config.sh = mock_shell
        with ExitStack() as s:
            run = s.enter_context(patch.object(mock_shell, "run"))
            self._patch_eval_deps(s, mock_shell)
            run.return_value.stdout = "x"
            Config.eval("config.test.attr")
            cmd = run.call_args[0][0]
            assert "nixosConfigurations.host-Standard-Boot" in cmd
            assert "/etc/nixos#" in cmd
    def test_eval_uses_nix_experimental_features(self, mock_shell):
        Config.sh = mock_shell
        with ExitStack() as s:
            run = s.enter_context(patch.object(mock_shell, "run"))
            self._patch_eval_deps(s, mock_shell)
            run.return_value.stdout = "x"
            Config.eval("config.test")
            cmd = run.call_args[0][0]
            assert "--extra-experimental-features nix-command" in cmd
            assert "--extra-experimental-features flakes" in cmd

class TestConfigHostInfo:
    def test_get_host_strips_nix(self, mock_shell):
        Config.sh = mock_shell
        with ExitStack() as s:
            s.enter_context(patch.object(
                Config, "get_host_path", return_value="/path/hostname.nix"))
            s.enter_context(patch.object(
                mock_shell, "basename", return_value="hostname.nix"))
            assert Config.get_host() == "hostname"
    def test_get_architecture(self, mock_shell):
        Config.sh = mock_shell
        with ExitStack() as s:
            s.enter_context(patch.object(
                Config, "get_host_path",
                return_value="/hosts/x86_64/host.nix"))
            s.enter_context(patch.object(
                mock_shell, "parent_name", return_value="x86_64"))
            assert Config.get_architecture() == "x86_64"
    def test_get_architecture_aarch64(self, mock_shell):
        Config.sh = mock_shell
        with ExitStack() as s:
            s.enter_context(patch.object(
                Config, "get_host_path",
                return_value="/hosts/aarch64/host.nix"))
            s.enter_context(patch.object(
                mock_shell, "parent_name", return_value="aarch64"))
            assert Config.get_architecture() == "aarch64"

@pytest.mark.usefixtures("mock_config_eval")
class TestConfigEvalDerived:
    def test_get_disk_by_part_label_root(self):
        assert (Config.get_disk_by_part_label_root()
                == "/dev/disk/by-partlabel/disk-main-root")
    def test_get_tpm_device(self):
        assert Config.get_tpm_device() == "/dev/tpmrm0"
    def test_get_tpm_version_path(self):
        assert (Config.get_tpm_version_path()
                == "/sys/class/tpm/tpm0/tpm_version_major")
    def test_get_hashed_password_path(self):
        assert (Config.get_hashed_password_path()
                == "/etc/nixos/secrets/hashed_password.txt")
    def test_get_secrets_path(self):
        assert Config.get_secrets_path() == "/etc/nixos/secrets"

class TestConfigCreateSecrets:
    def _setup_create_secrets(self, s, mock_shell, exists_side_effect):
        s.enter_context(patch.object(
            mock_shell, "exists", side_effect=exists_side_effect))
        s.enter_context(patch.object(
            Config, "get_secrets_path", return_value="/secrets"))
        s.enter_context(patch.object(
            Config, "get_hashed_password_path",
            return_value="/secrets/hash.txt"))
    def test_creates_secrets_dir_if_missing(self, mock_shell):
        Config.sh = mock_shell
        with ExitStack() as s:
            # exists: secrets_dir=False, hashed_pw=False
            self._setup_create_secrets(s, mock_shell, [False, False])
            mkdir = s.enter_context(patch.object(mock_shell, "mkdir"))
            s.enter_context(patch.object(
                Interactive, "ask_for_password", return_value="pw"))
            run = s.enter_context(patch.object(mock_shell, "run"))
            run.return_value.stdout = "$6$hash"
            s.enter_context(patch.object(mock_shell, "file_write"))
            Config.create_secrets()
            mkdir.assert_called_once_with("/secrets")
    def test_skips_mkdir_if_secrets_dir_exists(self, mock_shell):
        Config.sh = mock_shell
        with ExitStack() as s:
            # exists: secrets_dir=True, hashed_pw=True
            self._setup_create_secrets(s, mock_shell, [True, True])
            mkdir = s.enter_context(patch.object(mock_shell, "mkdir"))
            Config.create_secrets()
            mkdir.assert_not_called()
    def test_skips_password_if_hashed_exists(self, mock_shell):
        Config.sh = mock_shell
        with ExitStack() as s:
            # exists: secrets_dir=True, hashed_pw=True
            self._setup_create_secrets(s, mock_shell, [True, True])
            ask = s.enter_context(patch.object(
                Interactive, "ask_for_password"))
            Config.create_secrets()
            ask.assert_not_called()
    def test_writes_plain_text_password(self, mock_shell):
        Config.sh = mock_shell
        with ExitStack() as s:
            # exists: secrets_dir=True, hashed_pw=False, plain_text=False
            self._setup_create_secrets(s, mock_shell, [True, False, False])
            s.enter_context(patch.object(
                Interactive, "ask_for_password", return_value="mypass"))
            run = s.enter_context(patch.object(mock_shell, "run"))
            run.return_value.stdout = "$6$hashed"
            fw = s.enter_context(patch.object(mock_shell, "file_write"))
            Config.create_secrets(plain_text_password_path="/tmp/pw.txt")
            plain_call = fw.call_args_list[0]
            assert plain_call[0][0] == "/tmp/pw.txt"
            assert plain_call[0][1] == "mypass"
    def test_writes_hashed_password(self, mock_shell):
        Config.sh = mock_shell
        with ExitStack() as s:
            # exists: secrets_dir=True, hashed_pw=False
            self._setup_create_secrets(s, mock_shell, [True, False])
            s.enter_context(patch.object(
                Interactive, "ask_for_password", return_value="pw"))
            run = s.enter_context(patch.object(mock_shell, "run"))
            run.return_value.stdout = "$6$salted$hash"
            fw = s.enter_context(patch.object(mock_shell, "file_write"))
            Config.create_secrets()
            hash_call = fw.call_args_list[0]
            assert hash_call[0][0] == "/secrets/hash.txt"
            assert hash_call[0][1] == "$6$salted$hash"
    def test_uses_shlex_quote_for_special_chars(self, mock_shell):
        Config.sh = mock_shell
        password = "p@ss'word"
        with ExitStack() as s:
            # exists: secrets_dir=True, hashed_pw=False
            self._setup_create_secrets(s, mock_shell, [True, False])
            s.enter_context(patch.object(
                Interactive, "ask_for_password", return_value=password))
            run = s.enter_context(patch.object(mock_shell, "run"))
            run.return_value.stdout = "$6$hash"
            s.enter_context(patch.object(mock_shell, "file_write"))
            Config.create_secrets()
            cmd = run.call_args[0][0]
            assert shlex.quote(password) in cmd
    def test_shlex_quote_not_naive_single_quotes(self, mock_shell):
        Config.sh = mock_shell
        password = "it's a test"
        with ExitStack() as s:
            self._setup_create_secrets(s, mock_shell, [True, False])
            s.enter_context(patch.object(
                Interactive, "ask_for_password", return_value=password))
            run = s.enter_context(patch.object(mock_shell, "run"))
            run.return_value.stdout = "$6$hash"
            s.enter_context(patch.object(mock_shell, "file_write"))
            Config.create_secrets()
            cmd = run.call_args[0][0]
            # Old broken code would produce: mkpasswd -m sha-512 'it's a test'
            # shlex.quote produces: mkpasswd -m sha-512 "it's a test"
            assert f"'{password}'" not in cmd
            assert shlex.quote(password) in cmd
    def test_skips_plain_text_when_path_is_none(self, mock_shell):
        Config.sh = mock_shell
        with ExitStack() as s:
            # exists: secrets_dir=True, hashed_pw=False
            self._setup_create_secrets(s, mock_shell, [True, False])
            s.enter_context(patch.object(
                Interactive, "ask_for_password", return_value="pw"))
            run = s.enter_context(patch.object(mock_shell, "run"))
            run.return_value.stdout = "$6$hash"
            fw = s.enter_context(patch.object(mock_shell, "file_write"))
            Config.create_secrets(plain_text_password_path=None)
            # Only hashed password written, no plain text
            assert fw.call_count == 1
            assert fw.call_args[0][0] == "/secrets/hash.txt"
    def test_password_with_single_quotes_both_files(self, mock_shell):
        Config.sh = mock_shell
        password = "it's got 'quotes'"
        with ExitStack() as s:
            # exists: secrets_dir=True, hashed_pw=False, plain_text=False
            self._setup_create_secrets(s, mock_shell, [True, False, False])
            s.enter_context(patch.object(
                Interactive, "ask_for_password", return_value=password))
            run = s.enter_context(patch.object(mock_shell, "run"))
            run.return_value.stdout = "$6$hashed_quotes"
            fw = s.enter_context(patch.object(mock_shell, "file_write"))
            Config.create_secrets(plain_text_password_path="/tmp/pw.txt")
            # Plain text written correctly
            plain_call = fw.call_args_list[0]
            assert plain_call[0][1] == password
            # Hashed password written
            hash_call = fw.call_args_list[1]
            assert hash_call[0][1] == "$6$hashed_quotes"
            # Command used shlex.quote
            cmd = run.call_args[0][0]
            assert shlex.quote(password) in cmd
    def test_sensitive_kwarg_passed_for_plain_text(self, mock_shell):
        Config.sh = mock_shell
        with ExitStack() as s:
            self._setup_create_secrets(s, mock_shell, [True, False, False])
            s.enter_context(patch.object(
                Interactive, "ask_for_password", return_value="secret"))
            run = s.enter_context(patch.object(mock_shell, "run"))
            run.return_value.stdout = "$6$hash"
            fw = s.enter_context(patch.object(mock_shell, "file_write"))
            Config.create_secrets(plain_text_password_path="/tmp/pw.txt")
            plain_call = fw.call_args_list[0]
            assert plain_call[1].get("sensitive") == "secret"
    def test_sensitive_kwarg_passed_for_hashed(self, mock_shell):
        Config.sh = mock_shell
        with ExitStack() as s:
            self._setup_create_secrets(s, mock_shell, [True, False])
            s.enter_context(patch.object(
                Interactive, "ask_for_password", return_value="pw"))
            run = s.enter_context(patch.object(mock_shell, "run"))
            run.return_value.stdout = "$6$enc"
            fw = s.enter_context(patch.object(mock_shell, "file_write"))
            Config.create_secrets()
            hash_call = fw.call_args_list[0]
            assert hash_call[1].get("sensitive") == "$6$enc"
    def test_sensitive_kwarg_passed_for_run(self, mock_shell):
        Config.sh = mock_shell
        with ExitStack() as s:
            self._setup_create_secrets(s, mock_shell, [True, False])
            s.enter_context(patch.object(
                Interactive, "ask_for_password", return_value="pw"))
            run = s.enter_context(patch.object(mock_shell, "run"))
            run.return_value.stdout = "$6$hash"
            s.enter_context(patch.object(mock_shell, "file_write"))
            Config.create_secrets()
            assert run.call_args[1].get("sensitive") == "pw"
    def test_needs_password_when_plain_text_missing(self, mock_shell):
        Config.sh = mock_shell
        with ExitStack() as s:
            # exists: secrets_dir=True, hashed_pw=True, plain_text=False
            self._setup_create_secrets(s, mock_shell, [True, True, False])
            ask = s.enter_context(patch.object(
                Interactive, "ask_for_password", return_value="pw"))
            run = s.enter_context(patch.object(mock_shell, "run"))
            run.return_value.stdout = "$6$hash"
            s.enter_context(patch.object(mock_shell, "file_write"))
            Config.create_secrets(plain_text_password_path="/tmp/pw.txt")
            ask.assert_called_once()
    def test_no_password_needed_when_both_exist(self, mock_shell):
        Config.sh = mock_shell
        with ExitStack() as s:
            # exists: secrets_dir=True, hashed_pw=True, plain_text=True
            self._setup_create_secrets(s, mock_shell, [True, True, True])
            ask = s.enter_context(patch.object(
                Interactive, "ask_for_password"))
            Config.create_secrets(plain_text_password_path="/tmp/pw.txt")
            ask.assert_not_called()
    def test_mkpasswd_command_format(self, mock_shell):
        Config.sh = mock_shell
        with ExitStack() as s:
            self._setup_create_secrets(s, mock_shell, [True, False])
            s.enter_context(patch.object(
                Interactive, "ask_for_password", return_value="test"))
            run = s.enter_context(patch.object(mock_shell, "run"))
            run.return_value.stdout = "$6$hash"
            s.enter_context(patch.object(mock_shell, "file_write"))
            Config.create_secrets()
            cmd = run.call_args[0][0]
            assert cmd.startswith("mkpasswd -m sha-512 ")

class TestConfigSecureSecrets:
    def test_chmod_700_for_dirs(self, mock_shell):
        Config.sh = mock_shell
        with ExitStack() as s:
            s.enter_context(patch.object(
                mock_shell, "exists", return_value=True))
            s.enter_context(patch.object(
                Config, "get_secrets_path", return_value="/secrets"))
            s.enter_context(patch.object(
                mock_shell, "find_directories",
                return_value=["/secrets/sub"]))
            s.enter_context(patch.object(
                mock_shell, "find_files", return_value=["/secrets/f"]))
            s.enter_context(patch.object(mock_shell, "chown"))
            chmod = s.enter_context(patch.object(mock_shell, "chmod"))
            Config.secure_secrets()
            chmod.assert_any_call(700, "/secrets", "/secrets/sub")
    def test_chmod_600_for_files(self, mock_shell):
        Config.sh = mock_shell
        with ExitStack() as s:
            s.enter_context(patch.object(
                mock_shell, "exists", return_value=True))
            s.enter_context(patch.object(
                Config, "get_secrets_path", return_value="/secrets"))
            s.enter_context(patch.object(
                mock_shell, "find_directories", return_value=[]))
            s.enter_context(patch.object(
                mock_shell, "find_files",
                return_value=["/secrets/a", "/secrets/b"]))
            s.enter_context(patch.object(mock_shell, "chown"))
            chmod = s.enter_context(patch.object(mock_shell, "chmod"))
            Config.secure_secrets()
            chmod.assert_any_call(600, "/secrets/a", "/secrets/b")
    def test_skips_when_secrets_path_missing(self, mock_shell):
        Config.sh = mock_shell
        with ExitStack() as s:
            s.enter_context(patch.object(
                mock_shell, "exists", return_value=False))
            s.enter_context(patch.object(
                Config, "get_secrets_path", return_value="/secrets"))
            chown = s.enter_context(patch.object(mock_shell, "chown"))
            chmod = s.enter_context(patch.object(mock_shell, "chmod"))
            Config.secure_secrets()
            chown.assert_not_called()
            chmod.assert_not_called()

class TestConfigSecure:
    def _setup_secure(self, s, mock_shell):
        s.enter_context(patch.object(
            Config, "get_nixos_path", return_value="/etc/nixos"))
        s.enter_context(patch.object(
            mock_shell, "find_directories", return_value=["/d"]))
        s.enter_context(patch.object(
            mock_shell, "find_files", return_value=["/f"]))
        s.enter_context(patch.object(
            mock_shell, "find", return_value=["/s"]))
    def test_chown_called_with_username(self, mock_shell):
        Config.sh = mock_shell
        with ExitStack() as s:
            self._setup_secure(s, mock_shell)
            chown = s.enter_context(patch.object(mock_shell, "chown"))
            s.enter_context(patch.object(mock_shell, "chmod"))
            s.enter_context(patch.object(Config, "secure_secrets"))
            s.enter_context(patch.object(
                mock_shell, "git_add_safe_directory"))
            Config.secure("testuser")
            chown.assert_called_once_with("testuser", "/etc/nixos")
    def test_chmod_755_for_directories(self, mock_shell):
        Config.sh = mock_shell
        with ExitStack() as s:
            self._setup_secure(s, mock_shell)
            s.enter_context(patch.object(mock_shell, "chown"))
            chmod = s.enter_context(patch.object(mock_shell, "chmod"))
            s.enter_context(patch.object(Config, "secure_secrets"))
            s.enter_context(patch.object(
                mock_shell, "git_add_safe_directory"))
            Config.secure("testuser")
            chmod.assert_any_call(755, "/d")
    def test_chmod_644_for_files(self, mock_shell):
        Config.sh = mock_shell
        with ExitStack() as s:
            self._setup_secure(s, mock_shell)
            s.enter_context(patch.object(mock_shell, "chown"))
            chmod = s.enter_context(patch.object(mock_shell, "chmod"))
            s.enter_context(patch.object(Config, "secure_secrets"))
            s.enter_context(patch.object(
                mock_shell, "git_add_safe_directory"))
            Config.secure("testuser")
            chmod.assert_any_call(644, "/f")
    def test_chmod_755_for_scripts(self, mock_shell):
        Config.sh = mock_shell
        with ExitStack() as s:
            self._setup_secure(s, mock_shell)
            s.enter_context(patch.object(mock_shell, "chown"))
            chmod = s.enter_context(patch.object(mock_shell, "chmod"))
            s.enter_context(patch.object(Config, "secure_secrets"))
            s.enter_context(patch.object(
                mock_shell, "git_add_safe_directory"))
            Config.secure("testuser")
            # 755 called for dirs and scripts
            calls_755 = [c for c in chmod.call_args_list
                         if c[0][0] == 755]
            assert len(calls_755) == 2
    def test_chmod_444_for_git_objects(self, mock_shell):
        Config.sh = mock_shell
        with ExitStack() as s:
            self._setup_secure(s, mock_shell)
            s.enter_context(patch.object(mock_shell, "chown"))
            chmod = s.enter_context(patch.object(mock_shell, "chmod"))
            s.enter_context(patch.object(Config, "secure_secrets"))
            s.enter_context(patch.object(
                mock_shell, "git_add_safe_directory"))
            Config.secure("testuser")
            calls_444 = [c for c in chmod.call_args_list
                         if c[0][0] == 444]
            assert len(calls_444) == 1
    def test_git_add_safe_directory_called(self, mock_shell):
        Config.sh = mock_shell
        with ExitStack() as s:
            self._setup_secure(s, mock_shell)
            s.enter_context(patch.object(mock_shell, "chown"))
            s.enter_context(patch.object(mock_shell, "chmod"))
            s.enter_context(patch.object(Config, "secure_secrets"))
            git_safe = s.enter_context(patch.object(
                mock_shell, "git_add_safe_directory"))
            Config.secure("testuser")
            git_safe.assert_called_once_with("/etc/nixos")
    def test_find_directories_uses_ignore_pattern(self, mock_shell):
        Config.sh = mock_shell
        with ExitStack() as s:
            s.enter_context(patch.object(
                Config, "get_nixos_path", return_value="/etc/nixos"))
            find_dirs = s.enter_context(patch.object(
                mock_shell, "find_directories", return_value=[]))
            s.enter_context(patch.object(
                mock_shell, "find_files", return_value=[]))
            s.enter_context(patch.object(
                mock_shell, "find", return_value=[]))
            s.enter_context(patch.object(mock_shell, "chown"))
            s.enter_context(patch.object(mock_shell, "chmod"))
            s.enter_context(patch.object(Config, "secure_secrets"))
            s.enter_context(patch.object(
                mock_shell, "git_add_safe_directory"))
            Config.secure("testuser")
            assert find_dirs.call_args[1].get("ignore_pattern") == "*/{secrets}*"
    def test_find_scripts_uses_pattern(self, mock_shell):
        Config.sh = mock_shell
        with ExitStack() as s:
            s.enter_context(patch.object(
                Config, "get_nixos_path", return_value="/etc/nixos"))
            s.enter_context(patch.object(
                mock_shell, "find_directories", return_value=[]))
            s.enter_context(patch.object(
                mock_shell, "find_files", return_value=[]))
            find = s.enter_context(patch.object(
                mock_shell, "find", return_value=[]))
            s.enter_context(patch.object(mock_shell, "chown"))
            s.enter_context(patch.object(mock_shell, "chmod"))
            s.enter_context(patch.object(Config, "secure_secrets"))
            s.enter_context(patch.object(
                mock_shell, "git_add_safe_directory"))
            Config.secure("testuser")
            assert "*/scripts/* */bin/*" in str(find.call_args)

class TestConfigUpdate:
    def _setup_update(self, s, mock_shell):
        s.enter_context(patch.object(Config, "create_secrets"))
        s.enter_context(patch.object(Config, "secure"))
        s.enter_context(patch.object(
            mock_shell, "whoami", return_value="root"))
        s.enter_context(patch.object(
            mock_shell, "realpath", return_value="/etc/nixos"))
        s.enter_context(patch.object(
            Config, "get_host", return_value="host"))
        s.enter_context(patch.object(
            Config, "get_target", return_value="Standard-Boot"))
    def test_basic_update(self, mock_shell):
        Config.sh = mock_shell
        with ExitStack() as s:
            s.enter_context(patch.object(
                mock_shell, "exists", return_value=True))
            self._setup_update(s, mock_shell)
            run = s.enter_context(patch.object(mock_shell, "run"))
            s.enter_context(patch.object(Interactive, "ask_to_reboot"))
            Config.update()
            assert any("nixos-rebuild" in str(c) for c in run.call_args_list)
    def test_nixos_rebuild_includes_host_and_target(self, mock_shell):
        Config.sh = mock_shell
        with ExitStack() as s:
            s.enter_context(patch.object(
                mock_shell, "exists", return_value=True))
            self._setup_update(s, mock_shell)
            run = s.enter_context(patch.object(mock_shell, "run"))
            s.enter_context(patch.object(Interactive, "ask_to_reboot"))
            Config.update()
            rebuild_calls = [
                str(c) for c in run.call_args_list
                if "nixos-rebuild" in str(c)
            ]
            assert any("host-Standard-Boot" in c for c in rebuild_calls)
    def test_nixos_rebuild_uses_switch(self, mock_shell):
        Config.sh = mock_shell
        with ExitStack() as s:
            s.enter_context(patch.object(
                mock_shell, "exists", return_value=True))
            self._setup_update(s, mock_shell)
            run = s.enter_context(patch.object(mock_shell, "run"))
            s.enter_context(patch.object(Interactive, "ask_to_reboot"))
            Config.update()
            assert any(
                "nixos-rebuild switch" in str(c)
                for c in run.call_args_list)
    def test_update_with_delete_cache(self, mock_shell):
        Config.sh = mock_shell
        with ExitStack() as s:
            s.enter_context(patch.object(
                mock_shell, "exists", return_value=True))
            self._setup_update(s, mock_shell)
            run = s.enter_context(patch.object(mock_shell, "run"))
            s.enter_context(patch.object(mock_shell, "rm"))
            s.enter_context(patch.object(Interactive, "ask_to_reboot"))
            Config.update(delete_cache=True)
            assert any(
                "nix-collect-garbage" in str(c)
                for c in run.call_args_list)
    def test_delete_cache_removes_root_cache(self, mock_shell):
        Config.sh = mock_shell
        with ExitStack() as s:
            s.enter_context(patch.object(
                mock_shell, "exists", return_value=True))
            self._setup_update(s, mock_shell)
            s.enter_context(patch.object(mock_shell, "run"))
            rm = s.enter_context(patch.object(mock_shell, "rm"))
            s.enter_context(patch.object(Interactive, "ask_to_reboot"))
            Config.update(delete_cache=True)
            assert any("/root/.cache" in str(c) for c in rm.call_args_list)
    def test_delete_cache_runs_nix_store_verify(self, mock_shell):
        Config.sh = mock_shell
        with ExitStack() as s:
            s.enter_context(patch.object(
                mock_shell, "exists", return_value=True))
            self._setup_update(s, mock_shell)
            run = s.enter_context(patch.object(mock_shell, "run"))
            s.enter_context(patch.object(mock_shell, "rm"))
            s.enter_context(patch.object(Interactive, "ask_to_reboot"))
            Config.update(delete_cache=True)
            assert any(
                "nix-store --verify --repair" in str(c)
                for c in run.call_args_list)
    def test_update_with_upgrade(self, mock_shell):
        Config.sh = mock_shell
        with ExitStack() as s:
            s.enter_context(patch.object(
                mock_shell, "exists", return_value=True))
            self._setup_update(s, mock_shell)
            run = s.enter_context(patch.object(mock_shell, "run"))
            s.enter_context(patch.object(mock_shell, "rm"))
            s.enter_context(patch.object(Interactive, "ask_to_reboot"))
            Config.update(upgrade=True)
            assert any(
                "nix flake update" in str(c)
                for c in run.call_args_list)
    def test_upgrade_includes_flake_path(self, mock_shell):
        Config.sh = mock_shell
        with ExitStack() as s:
            s.enter_context(patch.object(
                mock_shell, "exists", return_value=True))
            self._setup_update(s, mock_shell)
            run = s.enter_context(patch.object(mock_shell, "run"))
            s.enter_context(patch.object(mock_shell, "rm"))
            s.enter_context(patch.object(Interactive, "ask_to_reboot"))
            Config.update(upgrade=True)
            flake_calls = [
                str(c) for c in run.call_args_list
                if "nix flake update" in str(c)
            ]
            assert any("/etc/nixos" in c for c in flake_calls)
    def test_upgrade_removes_root_cache(self, mock_shell):
        Config.sh = mock_shell
        with ExitStack() as s:
            s.enter_context(patch.object(
                mock_shell, "exists", return_value=True))
            self._setup_update(s, mock_shell)
            s.enter_context(patch.object(mock_shell, "run"))
            rm = s.enter_context(patch.object(mock_shell, "rm"))
            s.enter_context(patch.object(Interactive, "ask_to_reboot"))
            Config.update(upgrade=True)
            assert any("/root/.cache" in str(c) for c in rm.call_args_list)
    def test_update_missing_config_uses_standard_target(self, mock_shell):
        Config.sh = mock_shell
        with ExitStack() as s:
            s.enter_context(patch.object(
                mock_shell, "exists", return_value=False))
            self._setup_update(s, mock_shell)
            s.enter_context(patch.object(Utils, "print_error"))
            s.enter_context(patch.object(
                Interactive, "ask_for_host_path",
                return_value="/path/host.nix"))
            reset = s.enter_context(patch.object(Config, "reset_config"))
            s.enter_context(patch.object(mock_shell, "run"))
            s.enter_context(patch.object(Interactive, "ask_to_reboot"))
            Config.update()
            reset.assert_called_once_with(
                "/path/host.nix", "Standard-Boot")
    def test_update_missing_config_prints_error(self, mock_shell):
        Config.sh = mock_shell
        with ExitStack() as s:
            s.enter_context(patch.object(
                mock_shell, "exists", return_value=False))
            self._setup_update(s, mock_shell)
            pe = s.enter_context(patch.object(Utils, "print_error"))
            s.enter_context(patch.object(
                Interactive, "ask_for_host_path",
                return_value="/path/host.nix"))
            s.enter_context(patch.object(Config, "reset_config"))
            s.enter_context(patch.object(mock_shell, "run"))
            s.enter_context(patch.object(Interactive, "ask_to_reboot"))
            Config.update()
            pe.assert_called_once()
            assert "config.json" in str(pe.call_args)
    def test_update_rebuild_file_system(self, mock_shell):
        Config.sh = mock_shell
        with ExitStack() as s:
            s.enter_context(patch.object(
                mock_shell, "exists", return_value=True))
            self._setup_update(s, mock_shell)
            run = s.enter_context(patch.object(mock_shell, "run"))
            s.enter_context(patch.object(Interactive, "ask_to_reboot"))
            Config.update(rebuild_file_system=True)
            assert any(
                "NIXOS_INSTALL_BOOTLOADER=1" in str(c)
                for c in run.call_args_list)
    def test_update_no_rebuild_no_bootloader_env(self, mock_shell):
        Config.sh = mock_shell
        with ExitStack() as s:
            s.enter_context(patch.object(
                mock_shell, "exists", return_value=True))
            self._setup_update(s, mock_shell)
            run = s.enter_context(patch.object(mock_shell, "run"))
            s.enter_context(patch.object(Interactive, "ask_to_reboot"))
            Config.update(rebuild_file_system=False)
            rebuild_calls = [
                str(c) for c in run.call_args_list
                if "nixos-rebuild" in str(c)
            ]
            assert all(
                "NIXOS_INSTALL_BOOTLOADER" not in c
                for c in rebuild_calls)
    def test_update_calls_secure_with_whoami(self, mock_shell):
        Config.sh = mock_shell
        with ExitStack() as s:
            s.enter_context(patch.object(
                mock_shell, "exists", return_value=True))
            s.enter_context(patch.object(Config, "create_secrets"))
            sec = s.enter_context(patch.object(Config, "secure"))
            s.enter_context(patch.object(
                mock_shell, "whoami", return_value="admin"))
            s.enter_context(patch.object(
                mock_shell, "realpath", return_value="/etc/nixos"))
            s.enter_context(patch.object(
                Config, "get_host", return_value="h"))
            s.enter_context(patch.object(
                Config, "get_target", return_value="Standard-Boot"))
            s.enter_context(patch.object(mock_shell, "run"))
            s.enter_context(patch.object(Interactive, "ask_to_reboot"))
            Config.update()
            sec.assert_called_once_with("admin")
    def test_update_nixos_rebuild_not_capture_output(self, mock_shell):
        Config.sh = mock_shell
        with ExitStack() as s:
            s.enter_context(patch.object(
                mock_shell, "exists", return_value=True))
            self._setup_update(s, mock_shell)
            run = s.enter_context(patch.object(mock_shell, "run"))
            s.enter_context(patch.object(Interactive, "ask_to_reboot"))
            Config.update()
            rebuild_calls = [
                c for c in run.call_args_list
                if "nixos-rebuild" in str(c)
            ]
            assert len(rebuild_calls) == 1
            assert rebuild_calls[0][1].get("capture_output") is False

class TestConfigMetadata:
    def test_metadata_valid_json(self, mock_shell):
        Config.sh = mock_shell
        with ExitStack() as s:
            run = s.enter_context(patch.object(mock_shell, "run"))
            s.enter_context(patch.object(
                mock_shell, "realpath", return_value="/etc/nixos"))
            run.return_value.stdout = '{"locked": {"rev": "abc123"}}'
            result = Config.metadata("disko")
            assert result["locked"]["rev"] == "abc123"
    def test_metadata_cmd_includes_package_name(self, mock_shell):
        Config.sh = mock_shell
        with ExitStack() as s:
            run = s.enter_context(patch.object(mock_shell, "run"))
            s.enter_context(patch.object(
                mock_shell, "realpath", return_value="/etc/nixos"))
            run.return_value.stdout = '{"name": "test"}'
            Config.metadata("nixpkgs")
            cmd = run.call_args[0][0]
            assert "nixpkgs" in cmd
    def test_metadata_cmd_includes_json_flag(self, mock_shell):
        Config.sh = mock_shell
        with ExitStack() as s:
            run = s.enter_context(patch.object(mock_shell, "run"))
            s.enter_context(patch.object(
                mock_shell, "realpath", return_value="/etc/nixos"))
            run.return_value.stdout = '{}'
            Config.metadata("disko")
            cmd = run.call_args[0][0]
            assert "--json" in cmd
    def test_metadata_cmd_includes_flake_metadata(self, mock_shell):
        Config.sh = mock_shell
        with ExitStack() as s:
            run = s.enter_context(patch.object(mock_shell, "run"))
            s.enter_context(patch.object(
                mock_shell, "realpath", return_value="/etc/nixos"))
            run.return_value.stdout = '{}'
            Config.metadata("pkg")
            cmd = run.call_args[0][0]
            assert "flake metadata" in cmd
    def test_metadata_cmd_includes_nixos_path(self, mock_shell):
        Config.sh = mock_shell
        with ExitStack() as s:
            run = s.enter_context(patch.object(mock_shell, "run"))
            s.enter_context(patch.object(
                mock_shell, "realpath", return_value="/etc/nixos"))
            run.return_value.stdout = '{}'
            Config.metadata("pkg")
            cmd = run.call_args[0][0]
            assert "-I /etc/nixos" in cmd
    def test_metadata_returns_parsed_dict(self, mock_shell):
        Config.sh = mock_shell
        with ExitStack() as s:
            run = s.enter_context(patch.object(mock_shell, "run"))
            s.enter_context(patch.object(
                mock_shell, "realpath", return_value="/etc/nixos"))
            run.return_value.stdout = '{"a": 1, "b": [2, 3]}'
            result = Config.metadata("pkg")
            assert isinstance(result, dict)
            assert result["a"] == 1
            assert result["b"] == [2, 3]
    def test_metadata_uses_experimental_features(self, mock_shell):
        Config.sh = mock_shell
        with ExitStack() as s:
            run = s.enter_context(patch.object(mock_shell, "run"))
            s.enter_context(patch.object(
                mock_shell, "realpath", return_value="/etc/nixos"))
            run.return_value.stdout = '{}'
            Config.metadata("pkg")
            cmd = run.call_args[0][0]
            assert "--extra-experimental-features nix-command" in cmd
            assert "--extra-experimental-features flakes" in cmd
