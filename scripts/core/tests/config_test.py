from unittest.mock import patch

import pytest

from core.config import Config
from core.interactive import Interactive
from core.shell import Shell
from core.utils import Utils


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

    def test_get_existing(self, mock_shell):
        Config.sh = mock_shell
        with patch.object(Config, "read", return_value={"key": "val"}):
            assert Config.get("key") == "val"

    def test_get_missing(self, mock_shell):
        Config.sh = mock_shell
        with patch.object(Config, "read", return_value={}):
            assert Config.get("missing") is None

    def test_set(self, mock_shell):
        Config.sh = mock_shell
        with patch.object(mock_shell, "json_write") as m:
            Config.set("key", "value")
            m.assert_called_once_with(
                Config.get_config_path(), "key", "value"
            )

    def test_set_host_path(self, mock_shell):
        Config.sh = mock_shell
        with patch.object(Config, "set") as m:
            Config.set_host_path("/path/host.nix")
            m.assert_called_once_with("host_path", "/path/host.nix")

    def test_get_host_path(self, mock_shell):
        Config.sh = mock_shell
        with patch.object(Config, "get", return_value="/path/host.nix"):
            assert Config.get_host_path() == "/path/host.nix"

    def test_set_target(self, mock_shell):
        Config.sh = mock_shell
        with patch.object(Config, "set") as m:
            Config.set_target("Secure-Boot")
            m.assert_called_once_with("target", "Secure-Boot")

    def test_get_target(self, mock_shell):
        Config.sh = mock_shell
        with patch.object(Config, "get", return_value="Standard-Boot"):
            assert Config.get_target() == "Standard-Boot"

    def test_reset_config(self, mock_shell):
        Config.sh = mock_shell
        with patch.object(mock_shell, "rm") as rm:
            with patch.object(Config, "set_host_path") as sh:
                with patch.object(Config, "set_target") as st:
                    Config.reset_config("/path/host.nix", "Standard-Boot")
                    rm.assert_called_once()
                    sh.assert_called_once_with("/path/host.nix")
                    st.assert_called_once_with("Standard-Boot")


class TestConfigEval:
    def setup_method(self):
        Shell.evals.clear()

    def test_caches_results(self, mock_shell):
        Config.sh = mock_shell
        with patch.object(mock_shell, "run") as run:
            with patch.object(mock_shell, "realpath",
                              return_value="/etc/nixos"):
                with patch.object(Config, "get_host", return_value="test"):
                    with patch.object(Config, "get_target",
                                      return_value="Standard-Boot"):
                        run.return_value.stdout = "test-value"
                        r1 = Config.eval("config.test.attr")
                        r2 = Config.eval("config.test.attr")
                        assert r1 == r2 == "test-value"
                        assert run.call_count == 1

    def test_boolean_true(self, mock_shell):
        Config.sh = mock_shell
        with patch.object(mock_shell, "run") as run:
            with patch.object(mock_shell, "realpath",
                              return_value="/etc/nixos"):
                with patch.object(Config, "get_host", return_value="t"):
                    with patch.object(Config, "get_target",
                                      return_value="Standard-Boot"):
                        run.return_value.stdout = '"true"'
                        assert Config.eval("config.test.bool") is True

    def test_boolean_false(self, mock_shell):
        Config.sh = mock_shell
        with patch.object(mock_shell, "run") as run:
            with patch.object(mock_shell, "realpath",
                              return_value="/etc/nixos"):
                with patch.object(Config, "get_host", return_value="t"):
                    with patch.object(Config, "get_target",
                                      return_value="Standard-Boot"):
                        run.return_value.stdout = '"false"'
                        assert Config.eval("config.test.bool") is False


class TestConfigHostInfo:
    def test_get_host(self, mock_shell):
        Config.sh = mock_shell
        with patch.object(Config, "get_host_path",
                          return_value="/path/hostname.nix"):
            with patch.object(mock_shell, "basename",
                              return_value="hostname.nix"):
                assert Config.get_host() == "hostname"

    def test_get_architecture(self, mock_shell):
        Config.sh = mock_shell
        with patch.object(Config, "get_host_path",
                          return_value="/hosts/x86_64/host.nix"):
            with patch.object(mock_shell, "dirname",
                              return_value="/hosts/x86_64"):
                with patch.object(mock_shell, "basename",
                                  return_value="x86_64"):
                    assert Config.get_architecture() == "x86_64"


@pytest.mark.usefixtures("mock_config_eval")
class TestConfigEvalDerived:
    def test_get_disk_by_part_label_root(self):
        result = Config.get_disk_by_part_label_root()
        assert result == "/dev/disk/by-partlabel/disk-main-root"

    def test_get_tpm_device(self):
        assert Config.get_tpm_device() == "/dev/tpmrm0"

    def test_get_tpm_version_path(self):
        result = Config.get_tpm_version_path()
        assert result == "/sys/class/tpm/tpm0/tpm_version_major"

    def test_get_hashed_password_path(self):
        result = Config.get_hashed_password_path()
        assert result == "/etc/nixos/secrets/hashed_password.txt"

    def test_get_secrets_path(self):
        assert Config.get_secrets_path() == "/etc/nixos/secrets"


class TestConfigCreateSecrets:
    def test_creates_secrets_dir_if_missing(self, mock_shell):
        Config.sh = mock_shell
        with patch.object(mock_shell, "exists", side_effect=[False, False]):
            with patch.object(mock_shell, "mkdir") as mkdir:
                with patch.object(Interactive, "ask_for_password",
                                  return_value="pw"):
                    with patch.object(mock_shell, "run") as run:
                        run.return_value.stdout = "$6$hash"
                        with patch.object(mock_shell, "file_write"):
                            with patch.object(Config, "get_secrets_path",
                                              return_value="/secrets"):
                                with patch.object(
                                    Config, "get_hashed_password_path",
                                    return_value="/secrets/hash.txt"
                                ):
                                    Config.create_secrets()
                                    mkdir.assert_called_once_with("/secrets")

    def test_skips_if_password_exists(self, mock_shell):
        Config.sh = mock_shell
        with patch.object(mock_shell, "exists", return_value=True):
            with patch.object(Config, "get_secrets_path",
                              return_value="/secrets"):
                with patch.object(Config, "get_hashed_password_path",
                                  return_value="/secrets/hash.txt"):
                    Config.create_secrets()
                    # No password prompt if hash exists

    def test_writes_plain_text_password(self, mock_shell):
        Config.sh = mock_shell
        with patch.object(mock_shell, "exists",
                          side_effect=[True, False, False]):
            with patch.object(Interactive, "ask_for_password",
                              return_value="pw"):
                with patch.object(mock_shell, "run") as run:
                    run.return_value.stdout = "$6$hash"
                    with patch.object(mock_shell, "file_write") as fw:
                        with patch.object(Config, "get_secrets_path",
                                          return_value="/secrets"):
                            with patch.object(
                                Config, "get_hashed_password_path",
                                return_value="/secrets/hash.txt"
                            ):
                                Config.create_secrets(
                                    plain_text_password_path="/tmp/pw.txt"
                                )
                                assert any(
                                    "/tmp/pw.txt" in str(c)
                                    for c in fw.call_args_list
                                )


class TestConfigSecureSecrets:
    def test_secures_permissions(self, mock_shell):
        Config.sh = mock_shell
        with patch.object(Config, "get_secrets_path",
                          return_value="/secrets"):
            with patch.object(mock_shell, "find_directories",
                              return_value=["/secrets/sub"]):
                with patch.object(mock_shell, "find_files",
                                  return_value=["/secrets/f"]):
                    with patch.object(mock_shell, "chown") as chown:
                        with patch.object(mock_shell, "chmod") as chmod:
                            Config.secure_secrets()
                            chown.assert_called_once_with(
                                "root", "/secrets",
                                "/secrets/sub", "/secrets/f"
                            )
                            assert chmod.call_count == 2


class TestConfigSecure:
    def test_secures_entire_tree(self, mock_shell):
        Config.sh = mock_shell
        with patch.object(Config, "get_nixos_path",
                          return_value="/etc/nixos"):
            with patch.object(mock_shell, "chown") as chown:
                with patch.object(mock_shell, "chmod"):
                    with patch.object(mock_shell, "find_directories",
                                      return_value=["/d"]):
                        with patch.object(mock_shell, "find_files",
                                          return_value=["/f"]):
                            with patch.object(mock_shell, "find",
                                              return_value=["/s"]):
                                with patch.object(
                                    Config, "secure_secrets"
                                ):
                                    with patch.object(
                                        mock_shell,
                                        "git_add_safe_directory"
                                    ):
                                        Config.secure("testuser")
                                        chown.assert_called_once_with(
                                            "testuser", "/etc/nixos"
                                        )


class TestConfigUpdate:
    def test_basic_update(self, mock_shell):
        Config.sh = mock_shell
        with patch.object(Config, "create_secrets"):
            with patch.object(Config, "secure"):
                with patch.object(mock_shell, "exists", return_value=True):
                    with patch.object(mock_shell, "whoami",
                                      return_value="root"):
                        with patch.object(mock_shell, "realpath",
                                          return_value="/etc/nixos"):
                            with patch.object(Config, "get_host",
                                              return_value="host"):
                                with patch.object(Config, "get_target",
                                                  return_value="Standard-Boot"):
                                    with patch.object(mock_shell, "run") as run:
                                        with patch.object(
                                            Interactive, "ask_to_reboot"
                                        ):
                                            Config.update()
                                            assert any(
                                                "nixos-rebuild" in str(c)
                                                for c in run.call_args_list
                                            )

    def test_update_with_delete_cache(self, mock_shell):
        Config.sh = mock_shell
        with patch.object(Config, "create_secrets"):
            with patch.object(Config, "secure"):
                with patch.object(mock_shell, "exists", return_value=True):
                    with patch.object(mock_shell, "whoami",
                                      return_value="root"):
                        with patch.object(mock_shell, "realpath",
                                          return_value="/etc/nixos"):
                            with patch.object(Config, "get_host",
                                              return_value="host"):
                                with patch.object(Config, "get_target",
                                                  return_value="Standard-Boot"):
                                    with patch.object(mock_shell, "run") as run:
                                        with patch.object(mock_shell, "rm"):
                                            with patch.object(
                                                Interactive, "ask_to_reboot"
                                            ):
                                                Config.update(
                                                    delete_cache=True
                                                )
                                                assert any(
                                                    "nix-collect-garbage" in
                                                    str(c)
                                                    for c in
                                                    run.call_args_list
                                                )

    def test_update_with_upgrade(self, mock_shell):
        Config.sh = mock_shell
        with patch.object(Config, "create_secrets"):
            with patch.object(Config, "secure"):
                with patch.object(mock_shell, "exists", return_value=True):
                    with patch.object(mock_shell, "whoami",
                                      return_value="root"):
                        with patch.object(mock_shell, "realpath",
                                          return_value="/etc/nixos"):
                            with patch.object(Config, "get_host",
                                              return_value="host"):
                                with patch.object(Config, "get_target",
                                                  return_value="Standard-Boot"):
                                    with patch.object(mock_shell, "run") as run:
                                        with patch.object(mock_shell, "rm"):
                                            with patch.object(
                                                Interactive, "ask_to_reboot"
                                            ):
                                                Config.update(upgrade=True)
                                                assert any(
                                                    "nix flake update" in
                                                    str(c)
                                                    for c in
                                                    run.call_args_list
                                                )

    def test_update_with_reboot(self, mock_shell):
        Config.sh = mock_shell
        with patch.object(Config, "create_secrets"):
            with patch.object(Config, "secure"):
                with patch.object(mock_shell, "exists", return_value=True):
                    with patch.object(mock_shell, "whoami",
                                      return_value="root"):
                        with patch.object(mock_shell, "realpath",
                                          return_value="/etc/nixos"):
                            with patch.object(Config, "get_host",
                                              return_value="host"):
                                with patch.object(Config, "get_target",
                                                  return_value="Standard-Boot"):
                                    with patch.object(mock_shell, "run"):
                                        with patch.object(
                                            Utils, "reboot"
                                        ) as reboot:
                                            Config.update(reboot=True)
                                            reboot.assert_called_once()

    def test_update_missing_config(self, mock_shell):
        Config.sh = mock_shell
        with patch.object(Config, "create_secrets"):
            with patch.object(Config, "secure"):
                with patch.object(mock_shell, "exists", return_value=False):
                    with patch.object(mock_shell, "whoami",
                                      return_value="root"):
                        with patch.object(Utils, "print_error"):
                            with patch.object(
                                Interactive, "ask_for_host_path",
                                return_value="/path/host.nix"
                            ):
                                with patch.object(Config, "reset_config"):
                                    with patch.object(mock_shell, "realpath",
                                                      return_value="/etc/nixos"):
                                        with patch.object(
                                            Config, "get_host",
                                            return_value="host"
                                        ):
                                            with patch.object(
                                                Config, "get_target",
                                                return_value="Standard-Boot"
                                            ):
                                                with patch.object(
                                                    mock_shell, "run"
                                                ):
                                                    with patch.object(
                                                        Interactive,
                                                        "ask_to_reboot"
                                                    ):
                                                        Config.update()

    def test_update_rebuild_file_system(self, mock_shell):
        Config.sh = mock_shell
        with patch.object(Config, "create_secrets"):
            with patch.object(Config, "secure"):
                with patch.object(mock_shell, "exists", return_value=True):
                    with patch.object(mock_shell, "whoami",
                                      return_value="root"):
                        with patch.object(mock_shell, "realpath",
                                          return_value="/etc/nixos"):
                            with patch.object(Config, "get_host",
                                              return_value="host"):
                                with patch.object(Config, "get_target",
                                                  return_value="Standard-Boot"):
                                    with patch.object(mock_shell, "run") as run:
                                        with patch.object(
                                            Interactive, "ask_to_reboot"
                                        ):
                                            Config.update(
                                                rebuild_file_system=True
                                            )
                                            assert any(
                                                "NIXOS_INSTALL_BOOTLOADER=1"
                                                in str(c)
                                                for c in run.call_args_list
                                            )


class TestConfigMetadata:
    def test_metadata(self, mock_shell):
        Config.sh = mock_shell
        with patch.object(mock_shell, "run") as run:
            with patch.object(mock_shell, "realpath",
                              return_value="/etc/nixos"):
                run.return_value.stdout = '{"locked": {"rev": "abc123"}}'
                result = Config.metadata("disko")
                assert result["locked"]["rev"] == "abc123"
