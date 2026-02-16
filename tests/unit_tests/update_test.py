from unittest.mock import patch

from lib.config import Config
from lib.shell import Shell
from bin.update import main

class TestUpdateMain:
    def test_no_flags_defaults_to_all_false(self, monkeypatch):
        monkeypatch.setattr("sys.argv", ["update.py"])
        with patch.object(Shell, "require_root"):
            with patch.object(Config, "update") as update:
                main()
                kwargs = update.call_args[1]
                assert not any(kwargs.values())
    def test_clean_implies_delete_cache(self, monkeypatch):
        monkeypatch.setattr("sys.argv", ["update.py", "--clean"])
        with patch.object(Shell, "require_root"):
            with patch.object(Config, "update") as update:
                main()
                kwargs = update.call_args[1]
                assert kwargs["delete_cache"] is True
                assert kwargs["upgrade"] is False
    def test_upgrade_implies_delete_cache_and_upgrade(self, monkeypatch):
        monkeypatch.setattr("sys.argv", ["update.py", "--upgrade"])
        with patch.object(Shell, "require_root"):
            with patch.object(Config, "update") as update:
                main()
                kwargs = update.call_args[1]
                assert kwargs["delete_cache"] is True
                assert kwargs["upgrade"] is True
