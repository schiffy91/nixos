from unittest.mock import patch

from core.config import Config
from core.interactive import Interactive
from cli.install import Installer, main


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
    def test_mount_disk(self, mock_shell):
        Installer.sh = mock_shell
        with patch.object(Installer, "run_disko") as m:
            Installer.mount_disk()
            m.assert_called_once_with("mount")

    def test_erase_and_mount_disk(self, mock_shell):
        Installer.sh = mock_shell
        with patch.object(Installer, "run_disko") as m:
            Installer.erase_and_mount_disk()
            m.assert_called_once_with(
                "destroy,format,mount", "--yes-wipe-all-disks"
            )

    def test_run_disko(self, mock_shell, mock_subprocess):
        Installer.sh = mock_shell
        with patch.object(Config, "metadata", return_value={
            "locked": {"rev": "abc123"},
        }):
            with patch.object(Config, "get_host", return_value="VM-TEST"):
                with patch.object(
                    Config, "get_disk_operation_target",
                    return_value="Disk-Operation",
                ):
                    Installer.run_disko("mount")
                    cmd = mock_subprocess.call_args[0][0]
                    assert "disko" in cmd
                    assert "abc123" in cmd
                    assert "VM-TEST-Disk-Operation" in cmd


class TestInstallerInstall:
    def test_install_nixos(self, mock_shell, mock_subprocess):
        Installer.sh = mock_shell
        mock_subprocess.return_value.stdout = "/etc/nixos"
        with patch.object(Config, "get_host", return_value="VM-TEST"):
            with patch.object(
                Config, "get_target", return_value="Standard-Boot"
            ):
                with patch.object(mock_shell, "cpdir"):
                    with patch.object(Installer, "permission_nixos"):
                        Installer.install_nixos()
                        calls = [
                            str(c) for c in mock_subprocess.call_args_list
                        ]
                        assert any("nixos-install" in c for c in calls)

    def test_permission_nixos(self, mock_shell, mock_subprocess):
        Installer.sh = mock_shell
        mock_subprocess.return_value.stdout = "testuser"
        with patch.object(Config, "secure"):
            with patch("cli.install.Snapshot") as mock_snap:
                with patch.object(
                    Installer, "get_username", return_value="testuser"
                ):
                    Installer.permission_nixos()
                    mock_snap.create_initial_snapshots.assert_called_once()


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


class TestInstallerMain:
    def test_main_format_and_install(self, mock_shell, monkeypatch,
                                     mock_config_eval):  # noqa: ARG002
        Installer.sh = mock_shell
        monkeypatch.setattr("sys.argv", ["install.py"])
        with patch("cli.install.Utils") as mock_utils:
            with patch.object(mock_shell, "exists", return_value=True):
                with patch.object(Config, "create_secrets"):
                    with patch.object(
                        Interactive, "confirm",
                        side_effect=[True, True],
                    ):
                        with patch.object(Installer, "erase_and_mount_disk"):
                            with patch.object(Installer, "install_nixos"):
                                with patch.object(
                                    Interactive, "ask_to_reboot"
                                ):
                                    main()
                                    mock_utils.require_root\
                                        .assert_called_once()

    def test_main_mount_only(self, mock_shell, monkeypatch,
                             mock_config_eval):  # noqa: ARG002
        Installer.sh = mock_shell
        monkeypatch.setattr("sys.argv", ["install.py"])
        with patch("cli.install.Utils"):
            with patch.object(mock_shell, "exists", return_value=True):
                with patch.object(Config, "create_secrets"):
                    with patch.object(
                        Interactive, "confirm",
                        side_effect=[False, False, False],
                    ):
                        with patch.object(Installer, "mount_disk"):
                            with patch.object(
                                Interactive, "ask_to_reboot"
                            ):
                                main()
