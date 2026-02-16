import contextlib
from unittest.mock import patch

import pytest

from lib.config import Config
from lib.utils import Utils
from bin.secure_boot import (
    are_keys_enrolled, are_keys_signed, remove_old_efi_entries,
    create_keys, enroll_keys,
    enable_secure_boot, disable_secure_boot,
    require_signed_boot_loader, main,
)

class TestSecureBootHelpers:
    def test_are_keys_enrolled_true(self, mock_shell, mock_subprocess):
        mock_subprocess.return_value.stdout = "Secure Boot: \u2713 Enabled"
        with patch("bin.secure_boot.sh", mock_shell):
            assert are_keys_enrolled() is True
    def test_are_keys_enrolled_false(self, mock_shell, mock_subprocess):
        mock_subprocess.return_value.stdout = "Secure Boot: \u2717 Disabled"
        with patch("bin.secure_boot.sh", mock_shell):
            assert are_keys_enrolled() is False
    def test_are_keys_enrolled_empty_output(self, mock_shell, mock_subprocess):
        mock_subprocess.return_value.stdout = ""
        with patch("bin.secure_boot.sh", mock_shell):
            assert are_keys_enrolled() is False
    def test_are_keys_signed_true(self, mock_shell, mock_subprocess):
        mock_subprocess.return_value.stdout = "file.efi is signed"
        with patch("bin.secure_boot.sh", mock_shell):
            assert are_keys_signed() is True
    def test_are_keys_signed_false(self, mock_shell, mock_subprocess):
        mock_subprocess.return_value.stdout = "file.efi is not signed"
        with patch("bin.secure_boot.sh", mock_shell):
            assert are_keys_signed() is False
    def test_are_keys_signed_no_output(self, mock_shell, mock_subprocess):
        mock_subprocess.return_value.stdout = ""
        with patch("bin.secure_boot.sh", mock_shell):
            assert are_keys_signed() is False
    def test_are_keys_enrolled_case_insensitive(self, mock_shell,
                                                 mock_subprocess):
        mock_subprocess.return_value.stdout = "SECURE BOOT: \u2713 ENABLED"
        with patch("bin.secure_boot.sh", mock_shell):
            assert are_keys_enrolled() is True
    def test_are_keys_signed_mixed_output(self, mock_shell, mock_subprocess):
        mock_subprocess.return_value.stdout = (
            "file1.efi is signed\nfile2.efi is signed")
        with patch("bin.secure_boot.sh", mock_shell):
            assert are_keys_signed() is True

class TestSecureBootCommands:
    def test_remove_old_efi_entries(self, mock_shell):
        with contextlib.ExitStack() as s:
            s.enter_context(patch("bin.secure_boot.sh", mock_shell))
            mkdir = s.enter_context(patch.object(mock_shell, "mkdir"))
            rm = s.enter_context(patch.object(mock_shell, "rm"))
            remove_old_efi_entries()
            mkdir.assert_called_once_with("/boot/EFI/Linux", "/var/lib/sbctl")
            rm.assert_called_once_with(
                "/boot/EFI/Linux/linux-*.efi", "/etc/secureboot")
    def test_create_keys_runs_sbctl(self, mock_shell):
        with contextlib.ExitStack() as s:
            s.enter_context(patch("bin.secure_boot.sh", mock_shell))
            run = s.enter_context(patch.object(mock_shell, "run"))
            create_keys()
            cmds = [str(c) for c in run.call_args_list]
            assert any("sbctl reset" in c for c in cmds)
            assert any("sbctl create-keys" in c for c in cmds)
    def test_enroll_keys_uses_microsoft_flag(self, mock_shell):
        with contextlib.ExitStack() as s:
            s.enter_context(patch("bin.secure_boot.sh", mock_shell))
            run = s.enter_context(patch.object(mock_shell, "run"))
            enroll_keys()
            cmd = run.call_args[0][0]
            assert "--microsoft" in cmd

class TestSecureBootEnable:
    def test_enable_calls_update_with_rebuild(self, mock_shell):
        with contextlib.ExitStack() as s:
            s.enter_context(patch("bin.secure_boot.sh", mock_shell))
            s.enter_context(patch("bin.secure_boot.remove_old_efi_entries"))
            s.enter_context(patch("bin.secure_boot.create_keys"))
            s.enter_context(patch("bin.secure_boot.are_keys_enrolled",
                                  return_value=True))
            s.enter_context(patch.object(Config, "set_target"))
            update = s.enter_context(patch.object(Config, "update"))
            s.enter_context(patch(
                "bin.secure_boot.require_signed_boot_loader"))
            enable_secure_boot()
            update.assert_called_once_with(
                rebuild_file_system=True, delete_cache=True)
    def test_disable(self, mock_shell):
        with contextlib.ExitStack() as s:
            s.enter_context(patch("bin.secure_boot.sh", mock_shell))
            s.enter_context(patch("bin.secure_boot.remove_old_efi_entries"))
            st = s.enter_context(patch.object(Config, "set_target"))
            s.enter_context(patch.object(Config, "update"))
            disable_secure_boot()
            st.assert_called_once_with("Standard-Boot")
    def test_disable_calls_update_with_rebuild(self, mock_shell):
        with contextlib.ExitStack() as s:
            s.enter_context(patch("bin.secure_boot.sh", mock_shell))
            s.enter_context(patch("bin.secure_boot.remove_old_efi_entries"))
            s.enter_context(patch.object(Config, "set_target"))
            update = s.enter_context(patch.object(Config, "update"))
            disable_secure_boot()
            update.assert_called_once_with(
                rebuild_file_system=True, delete_cache=True)
    def test_require_signed_boot_loader_signed(self, mock_shell):
        with contextlib.ExitStack() as s:
            s.enter_context(patch("bin.secure_boot.sh", mock_shell))
            s.enter_context(patch("bin.secure_boot.are_keys_signed",
                                  return_value=True))
            disable = s.enter_context(patch(
                "bin.secure_boot.disable_secure_boot"))
            require_signed_boot_loader()
            disable.assert_not_called()
    def test_require_signed_boot_loader_unsigned(self, mock_shell):
        with contextlib.ExitStack() as s:
            s.enter_context(patch("bin.secure_boot.sh", mock_shell))
            s.enter_context(patch("bin.secure_boot.are_keys_signed",
                                  return_value=False))
            disable = s.enter_context(patch(
                "bin.secure_boot.disable_secure_boot"))
            require_signed_boot_loader()
            disable.assert_called_once()

class TestSecureBootMain:
    def test_main_invalid(self, mock_shell, monkeypatch):
        monkeypatch.setattr("sys.argv", ["secure_boot.py", "invalid"])
        with contextlib.ExitStack() as s:
            s.enter_context(patch("lib.utils.Utils.sh", mock_shell))
            s.enter_context(patch("bin.secure_boot.disable_secure_boot"))
            with pytest.raises(SystemExit):
                main()
    def test_main_sets_log_info_false(self, monkeypatch):
        monkeypatch.setattr("sys.argv", ["secure_boot.py", "enable"])
        with patch("bin.secure_boot.enable_secure_boot"):
            main()
            assert Utils.LOG_INFO is False
