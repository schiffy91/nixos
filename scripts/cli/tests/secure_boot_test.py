from unittest.mock import patch

import pytest

from core.config import Config
from cli.secure_boot import (
    are_keys_enrolled, are_keys_signed,
    enable_secure_boot, disable_secure_boot, main,
)


class TestSecureBootHelpers:
    def test_are_keys_enrolled_true(self, mock_shell, mock_subprocess):
        mock_subprocess.return_value.stdout = "Secure Boot: ✓ Enabled"
        with patch("cli.secure_boot.sh", mock_shell):
            assert are_keys_enrolled() is True

    def test_are_keys_enrolled_false(self, mock_shell, mock_subprocess):
        mock_subprocess.return_value.stdout = "Secure Boot: ✗ Disabled"
        with patch("cli.secure_boot.sh", mock_shell):
            assert are_keys_enrolled() is False

    def test_are_keys_signed_true(self, mock_shell, mock_subprocess):
        mock_subprocess.return_value.stdout = "file.efi is signed"
        with patch("cli.secure_boot.sh", mock_shell):
            assert are_keys_signed() is True

    def test_are_keys_signed_false(self, mock_shell, mock_subprocess):
        mock_subprocess.return_value.stdout = "file.efi is not signed"
        with patch("cli.secure_boot.sh", mock_shell):
            assert are_keys_signed() is False


class TestSecureBootEnable:
    def test_enable(self, mock_shell):
        with patch("cli.secure_boot.sh", mock_shell):
            with patch("cli.secure_boot.remove_old_efi_entries"):
                with patch("cli.secure_boot.create_keys"):
                    with patch("cli.secure_boot.are_keys_enrolled",
                               return_value=True):
                        with patch.object(Config, "set_target"):
                            with patch.object(Config, "update"):
                                with patch(
                                    "cli.secure_boot."
                                    "require_signed_boot_loader"
                                ):
                                    enable_secure_boot()

    def test_disable(self, mock_shell):
        with patch("cli.secure_boot.sh", mock_shell):
            with patch("cli.secure_boot.remove_old_efi_entries"):
                with patch.object(Config, "set_target") as st:
                    with patch.object(Config, "update"):
                        disable_secure_boot()
                        st.assert_called_once_with("Standard-Boot")


class TestSecureBootMain:
    def test_main_enable(self, monkeypatch):
        monkeypatch.setattr("sys.argv", ["secure_boot.py", "enable"])
        with patch("cli.secure_boot.enable_secure_boot") as m:
            main()
            m.assert_called_once()

    def test_main_disable(self, monkeypatch):
        monkeypatch.setattr("sys.argv", ["secure_boot.py", "disable"])
        with patch("cli.secure_boot.disable_secure_boot") as m:
            main()
            m.assert_called_once()

    def test_main_invalid(self, mock_shell, monkeypatch):
        monkeypatch.setattr("sys.argv", ["secure_boot.py", "invalid"])
        with patch("core.utils.Utils.sh", mock_shell):
            with patch("cli.secure_boot.disable_secure_boot"):
                with pytest.raises(SystemExit):
                    main()
