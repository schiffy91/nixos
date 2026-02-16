import contextlib
from unittest.mock import patch

import pytest

from lib.config import Config
from lib.interactive import Interactive
from bin.install import Installer, main

class TestInstallerPaths:
    def test_get_mount_point(self):
        assert Installer.get_mount_point() == "/mnt"
    def test_get_username(self, mock_config_eval):  # noqa: ARG002
        assert Installer.get_username() == "testuser"
    def test_get_installation_disk(self, mock_config_eval):  # noqa: ARG002
        assert Installer.get_installation_disk() == "/dev/sda"
    def test_get_plain_text_password_path_encrypted(self, mock_shell):
        Config.sh = mock_shell
        with patch.object(Config, "eval", side_effect=[
            True, "/secrets/password.txt",
        ]):
            result = Installer.get_plain_text_password_path()
            assert result == "/secrets/password.txt"
    def test_get_plain_text_password_path_unencrypted(self, mock_shell):
        Config.sh = mock_shell
        with patch.object(Config, "eval", return_value=False):
            assert Installer.get_plain_text_password_path() is None

class TestInstallerDisko:
    def test_run_disko_command_format(self, mock_shell, mock_subprocess):
        Installer.sh = mock_shell
        with contextlib.ExitStack() as stack:
            stack.enter_context(patch.object(Config, "metadata",
                                             return_value={"locked": {"rev": "abc123"}}))
            stack.enter_context(patch.object(Config, "get_host",
                                             return_value="VM-TEST"))
            stack.enter_context(patch.object(Config, "get_disk_operation_target",
                                             return_value="Disk-Operation"))
            Installer.run_disko("mount")
            cmd = mock_subprocess.call_args[0][0]
            assert "disko" in cmd
            assert "abc123" in cmd
            assert "VM-TEST-Disk-Operation" in cmd
    def test_run_disko_includes_mode(self, mock_shell, mock_subprocess):
        Installer.sh = mock_shell
        with contextlib.ExitStack() as stack:
            stack.enter_context(patch.object(Config, "metadata",
                                             return_value={"locked": {"rev": "def456"}}))
            stack.enter_context(patch.object(Config, "get_host",
                                             return_value="HOST"))
            stack.enter_context(patch.object(Config, "get_disk_operation_target",
                                             return_value="Disk-Operation"))
            Installer.run_disko("destroy,format,mount",
                                "--yes-wipe-all-disks")
            cmd = mock_subprocess.call_args[0][0]
            assert "--mode destroy,format,mount" in cmd
            assert "--yes-wipe-all-disks" in cmd
    def test_run_disko_includes_root_mountpoint(
        self, mock_shell, mock_subprocess
    ):
        Installer.sh = mock_shell
        with contextlib.ExitStack() as stack:
            stack.enter_context(patch.object(Config, "metadata",
                                             return_value={"locked": {"rev": "r1"}}))
            stack.enter_context(patch.object(Config, "get_host",
                                             return_value="H"))
            stack.enter_context(patch.object(Config, "get_disk_operation_target",
                                             return_value="D"))
            Installer.run_disko("mount")
            cmd = mock_subprocess.call_args[0][0]
            assert "--root-mountpoint /mnt" in cmd

class TestInstallerInstall:
    def test_install_nixos(self, mock_shell, mock_subprocess):
        Installer.sh = mock_shell
        mock_subprocess.return_value.stdout = "/etc/nixos"
        with contextlib.ExitStack() as stack:
            stack.enter_context(patch.object(Config, "get_host",
                                             return_value="VM-TEST"))
            stack.enter_context(patch.object(Config, "get_target",
                                             return_value="Standard-Boot"))
            stack.enter_context(patch.object(mock_shell, "cpdir"))
            stack.enter_context(patch.object(Installer, "permission_nixos"))
            Installer.install_nixos()
            calls = [str(c) for c in mock_subprocess.call_args_list]
            assert any("nixos-install" in c for c in calls)
    def test_install_nixos_includes_flake_and_root(
        self, mock_shell, mock_subprocess
    ):
        Installer.sh = mock_shell
        mock_subprocess.return_value.stdout = "/etc/nixos"
        with contextlib.ExitStack() as stack:
            stack.enter_context(patch.object(Config, "get_host",
                                             return_value="VM-TEST"))
            stack.enter_context(patch.object(Config, "get_target",
                                             return_value="Standard-Boot"))
            stack.enter_context(patch.object(mock_shell, "cpdir"))
            stack.enter_context(patch.object(Installer, "permission_nixos"))
            Installer.install_nixos()
            calls = [str(c) for c in mock_subprocess.call_args_list]
            install_call = [c for c in calls if "nixos-install" in c][0]
            assert "--root /mnt" in install_call
            assert "--flake" in install_call

class TestInstallerParseArgs:
    def test_no_args(self, mock_shell, monkeypatch):
        Installer.sh = mock_shell
        monkeypatch.setattr("sys.argv", ["install.py"])
        with patch.object(mock_shell, "run"):
            Installer.parse_args()
    def test_collect_garbage(self, mock_shell, monkeypatch):
        Installer.sh = mock_shell
        monkeypatch.setattr("sys.argv", [
            "install.py", "--collect-garbage",
        ])
        with patch.object(mock_shell, "run") as run:
            Installer.parse_args()
            assert any(
                "nix-collect-garbage" in str(c)
                for c in run.call_args_list
            )
    def test_debug_mode(self, mock_shell, monkeypatch):
        Installer.sh = mock_shell
        monkeypatch.setattr("sys.argv", ["install.py", "--debug"])
        with contextlib.ExitStack() as stack:
            run = stack.enter_context(patch.object(mock_shell, "run"))
            stack.enter_context(patch.object(Config, "get_nixos_path",
                                             return_value="/etc/nixos"))
            with pytest.raises(SystemExit):
                Installer.parse_args()
            calls = [str(c) for c in run.call_args_list]
            assert any("vscodium" in c.lower() for c in calls)

class TestInstallerMain:
    def test_main_no_config_triggers_reset(
        self, mock_shell, monkeypatch,
        mock_config_eval  # noqa: ARG002
    ):
        Installer.sh = mock_shell
        monkeypatch.setattr("sys.argv", ["install.py"])
        with contextlib.ExitStack() as stack:
            stack.enter_context(patch("bin.install.Utils"))
            stack.enter_context(patch.object(mock_shell, "exists",
                                             return_value=False))
            reset = stack.enter_context(patch.object(Config, "reset_config"))
            stack.enter_context(patch.object(Interactive, "ask_for_host_path",
                                             return_value="/hosts/test.nix"))
            stack.enter_context(patch.object(Config, "get_standard_flake_target",
                                             return_value="Standard-Boot"))
            stack.enter_context(patch.object(Config, "create_secrets"))
            stack.enter_context(patch.object(Interactive, "confirm",
                                             side_effect=[False, False, False]))
            stack.enter_context(patch.object(Installer, "mount_disk"))
            stack.enter_context(patch.object(Interactive, "ask_to_reboot"))
            main()
            reset.assert_called_once_with(
                "/hosts/test.nix", "Standard-Boot")
    def test_main_permission_only(self, mock_shell, monkeypatch,
                                  mock_config_eval):  # noqa: ARG002
        Installer.sh = mock_shell
        monkeypatch.setattr("sys.argv", ["install.py"])
        with contextlib.ExitStack() as stack:
            stack.enter_context(patch("bin.install.Utils"))
            stack.enter_context(patch.object(mock_shell, "exists",
                                             return_value=True))
            stack.enter_context(patch.object(Config, "create_secrets"))
            stack.enter_context(patch.object(Interactive, "confirm",
                                             side_effect=[False, False, True]))
            stack.enter_context(patch.object(Installer, "mount_disk"))
            perm = stack.enter_context(patch.object(Installer, "permission_nixos"))
            stack.enter_context(patch.object(Interactive, "ask_to_reboot"))
            main()
            perm.assert_called_once()
