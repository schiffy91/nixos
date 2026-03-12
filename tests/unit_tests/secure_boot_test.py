import contextlib, json
from unittest.mock import patch

import pytest

from lib.config import Config
from bin.secure_boot import (
    verify, remove_old_efi_entries, create_keys, enroll_keys,
    enable_secure_boot, disable_secure_boot, main,
)

class TestSecureBootVerify:
    def test_verify_all_signed(self, mock_shell, mock_subprocess, capsys):
        mock_subprocess.return_value.stdout = json.dumps({
            "/boot/a.efi": {"is_signed": True},
            "/boot/b.efi": {"is_signed": True}})
        with patch("bin.secure_boot.sh", mock_shell):
            verify()
            assert "signed" in capsys.readouterr().out.lower()
    def test_verify_unsigned(self, mock_shell, mock_subprocess, capsys):
        mock_subprocess.return_value.stdout = json.dumps({
            "/boot/a.efi": {"is_signed": False}})
        with patch("bin.secure_boot.sh", mock_shell):
            verify()
            output = capsys.readouterr()
            assert "NOT signed" in output.err
    def test_verify_null(self, mock_shell, mock_subprocess, capsys):
        mock_subprocess.return_value.stdout = "null"
        with patch("bin.secure_boot.sh", mock_shell):
            verify()
            output = capsys.readouterr()
            assert "No EFI" in output.err
    def test_verify_empty(self, mock_shell, mock_subprocess, capsys):
        mock_subprocess.return_value.stdout = ""
        with patch("bin.secure_boot.sh", mock_shell):
            verify()
            output = capsys.readouterr()
            assert "No EFI" in output.err
    def test_verify_does_not_rollback(self, mock_shell, mock_subprocess):
        mock_subprocess.return_value.stdout = json.dumps({
            "/boot/a.efi": {"is_signed": False}})
        with patch("bin.secure_boot.sh", mock_shell):
            with patch("bin.secure_boot.disable_secure_boot") as disable:
                verify()
                disable.assert_not_called()

class TestSecureBootCommands:
    def test_remove_old_efi_entries(self, mock_shell):
        with contextlib.ExitStack() as s:
            s.enter_context(patch("bin.secure_boot.sh", mock_shell))
            mkdir = s.enter_context(patch.object(mock_shell, "mkdir"))
            s.enter_context(patch.object(mock_shell, "find_files",
                                         return_value=["/boot/EFI/Linux/linux-old.efi"]))
            rm = s.enter_context(patch.object(mock_shell, "rm"))
            remove_old_efi_entries()
            mkdir.assert_called_once_with("/boot/EFI/Linux", "/var/lib/sbctl")
            rm.assert_any_call("/boot/EFI/Linux/linux-old.efi")
            rm.assert_any_call("/etc/secureboot")
    def test_create_keys_runs_sbctl(self, mock_shell):
        with contextlib.ExitStack() as s:
            s.enter_context(patch("bin.secure_boot.sh", mock_shell))
            run = s.enter_context(patch.object(mock_shell, "run"))
            create_keys()
            cmds = [str(c) for c in run.call_args_list]
            assert any("sbctl create-keys" in c for c in cmds)
    def test_enroll_keys_no_microsoft(self, mock_shell):
        with contextlib.ExitStack() as s:
            s.enter_context(patch("bin.secure_boot.sh", mock_shell))
            run = s.enter_context(patch.object(mock_shell, "run"))
            enroll_keys(microsoft=False)
            cmd = run.call_args[0][0]
            assert "--yes-this-might-brick-my-machine" in cmd
    def test_enroll_keys_microsoft(self, mock_shell):
        with contextlib.ExitStack() as s:
            s.enter_context(patch("bin.secure_boot.sh", mock_shell))
            run = s.enter_context(patch.object(mock_shell, "run"))
            enroll_keys(microsoft=True)
            cmd = run.call_args[0][0]
            assert "--microsoft" in cmd

class TestSecureBootEnable:
    def test_enable_always_enrolls(self, mock_shell):
        with contextlib.ExitStack() as s:
            s.enter_context(patch("bin.secure_boot.sh", mock_shell))
            s.enter_context(patch("bin.secure_boot.remove_old_efi_entries"))
            s.enter_context(patch("bin.secure_boot.create_keys"))
            enroll = s.enter_context(patch("bin.secure_boot.enroll_keys"))
            s.enter_context(patch.object(Config, "set_target"))
            s.enter_context(patch.object(Config, "update"))
            s.enter_context(patch("bin.secure_boot.verify"))
            enable_secure_boot()
            enroll.assert_called_once_with(microsoft=False)
    def test_enable_calls_update_with_rebuild(self, mock_shell):
        with contextlib.ExitStack() as s:
            s.enter_context(patch("bin.secure_boot.sh", mock_shell))
            s.enter_context(patch("bin.secure_boot.remove_old_efi_entries"))
            s.enter_context(patch("bin.secure_boot.create_keys"))
            s.enter_context(patch("bin.secure_boot.enroll_keys"))
            s.enter_context(patch.object(Config, "set_target"))
            update = s.enter_context(patch.object(Config, "update"))
            s.enter_context(patch("bin.secure_boot.verify"))
            enable_secure_boot()
            update.assert_called_once_with(
                rebuild_file_system=True, delete_cache=True)
    def test_enable_does_not_auto_rollback(self, mock_shell):
        with contextlib.ExitStack() as s:
            s.enter_context(patch("bin.secure_boot.sh", mock_shell))
            s.enter_context(patch("bin.secure_boot.remove_old_efi_entries"))
            s.enter_context(patch("bin.secure_boot.create_keys"))
            s.enter_context(patch("bin.secure_boot.enroll_keys"))
            s.enter_context(patch.object(Config, "set_target"))
            s.enter_context(patch.object(Config, "update"))
            s.enter_context(patch("bin.secure_boot.verify"))
            disable = s.enter_context(patch(
                "bin.secure_boot.disable_secure_boot"))
            enable_secure_boot()
            disable.assert_not_called()
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

class TestSecureBootMain:
    def test_main_invalid(self, mock_shell, monkeypatch):
        monkeypatch.setattr("sys.argv", ["secure_boot.py", "invalid"])
        with contextlib.ExitStack() as s:
            s.enter_context(patch("lib.utils.Utils.sh", mock_shell))
            s.enter_context(patch("bin.secure_boot.disable_secure_boot"))
            with pytest.raises(SystemExit):
                main()
    def test_main_enable(self, monkeypatch):
        monkeypatch.setattr("sys.argv", ["secure_boot.py", "enable"])
        with patch("bin.secure_boot.enable_secure_boot") as enable:
            main()
            enable.assert_called_once_with(microsoft=False)
    def test_main_enable_microsoft(self, monkeypatch):
        monkeypatch.setattr("sys.argv",
                            ["secure_boot.py", "enable", "--microsoft"])
        with patch("bin.secure_boot.enable_secure_boot") as enable:
            main()
            enable.assert_called_once_with(microsoft=True)
    def test_main_status(self, monkeypatch):
        monkeypatch.setattr("sys.argv", ["secure_boot.py", "status"])
        with patch("bin.secure_boot.status") as st:
            main()
            st.assert_called_once()
