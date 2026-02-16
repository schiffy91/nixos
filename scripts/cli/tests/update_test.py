from unittest.mock import patch

from core.config import Config
from core.shell import Shell
from cli.update import main


class TestUpdateMain:
    def test_basic_update(self, monkeypatch):
        monkeypatch.setattr("sys.argv", ["update.py"])
        with patch.object(Shell, "require_root"):
            with patch.object(Config, "update") as update:
                main()
                update.assert_called_once_with(
                    rebuild_file_system=False,
                    reboot=False,
                    delete_cache=False,
                    upgrade=False,
                )

    def test_with_reboot(self, monkeypatch):
        monkeypatch.setattr("sys.argv", ["update.py", "--reboot"])
        with patch.object(Shell, "require_root"):
            with patch.object(Config, "update") as update:
                main()
                update.assert_called_once_with(
                    rebuild_file_system=False,
                    reboot=True,
                    delete_cache=False,
                    upgrade=False,
                )

    def test_with_upgrade(self, monkeypatch):
        monkeypatch.setattr("sys.argv", ["update.py", "--upgrade"])
        with patch.object(Shell, "require_root"):
            with patch.object(Config, "update") as update:
                main()
                update.assert_called_once_with(
                    rebuild_file_system=False,
                    reboot=False,
                    delete_cache=True,
                    upgrade=True,
                )

    def test_with_clean(self, monkeypatch):
        monkeypatch.setattr("sys.argv", ["update.py", "--clean"])
        with patch.object(Shell, "require_root"):
            with patch.object(Config, "update") as update:
                main()
                update.assert_called_once_with(
                    rebuild_file_system=False,
                    reboot=False,
                    delete_cache=True,
                    upgrade=False,
                )

    def test_rebuild_filesystem(self, monkeypatch):
        monkeypatch.setattr("sys.argv", [
            "update.py", "--rebuild-filesystem",
        ])
        with patch.object(Shell, "require_root"):
            with patch.object(Config, "update") as update:
                main()
                update.assert_called_once_with(
                    rebuild_file_system=True,
                    reboot=False,
                    delete_cache=False,
                    upgrade=False,
                )
