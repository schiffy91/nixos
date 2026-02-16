import contextlib
from unittest.mock import patch

import pytest

from lib.config import Config
from bin.change_password import (
    ask_for_old_password, ask_for_new_password,
    change_luks_password, change_user_password,
    update_tpm2, main,
)

class TestChangePasswordHelpers:
    def test_ask_for_old_password(self, mock_shell):  # noqa: ARG002
        with patch("getpass.getpass", return_value="oldpw"):
            assert ask_for_old_password() == "oldpw"
    def test_ask_for_new_password_match(self, mock_shell):  # noqa: ARG002
        with patch("getpass.getpass", side_effect=["newpw", "newpw"]):
            assert ask_for_new_password() == "newpw"
    def test_ask_for_new_password_mismatch_then_match(
        self, mock_shell, capsys  # noqa: ARG002
    ):
        with patch("getpass.getpass", side_effect=["a", "b", "c", "c"]):
            assert ask_for_new_password() == "c"
            assert "do not match" in capsys.readouterr().err
    def test_change_luks_password(self, mock_shell, mock_subprocess,
                                  mock_config_eval):  # noqa: ARG002
        with patch("bin.change_password.sh", mock_shell):
            change_luks_password("old", "new")
            cmd = mock_subprocess.call_args[0][0]
            assert "cryptsetup luksChangeKey" in cmd
    def test_change_luks_password_uses_printf_not_echo(
        self, mock_shell, mock_subprocess,
        mock_config_eval  # noqa: ARG002
    ):
        with patch("bin.change_password.sh", mock_shell):
            change_luks_password("old", "new")
            cmd = mock_subprocess.call_args[0][0]
            assert "printf '%s\\n%s'" in cmd
            assert "echo -e" not in cmd
    def test_change_luks_password_fails(self, mock_shell, mock_subprocess,
                                        mock_config_eval):  # noqa: ARG002
        mock_subprocess.return_value.returncode = 1
        with patch("bin.change_password.sh", mock_shell):
            with pytest.raises(SystemExit):
                change_luks_password("old", "new")
    def test_change_user_password(self, mock_shell, mock_subprocess,
                                  mock_config_eval):  # noqa: ARG002
        mock_subprocess.return_value.stdout = "$6$hash"
        with contextlib.ExitStack() as stack:
            stack.enter_context(patch("bin.change_password.sh", mock_shell))
            stack.enter_context(patch.object(mock_shell, "file_write"))
            stack.enter_context(patch.object(Config, "update"))
            change_user_password("newpw")
    def test_change_user_password_calls_mkpasswd_with_shlex_quote(
        self, mock_shell, mock_subprocess,
        mock_config_eval  # noqa: ARG002
    ):
        mock_subprocess.return_value.stdout = "$6$hash"
        with contextlib.ExitStack() as stack:
            stack.enter_context(patch("bin.change_password.sh", mock_shell))
            stack.enter_context(patch.object(mock_shell, "file_write"))
            stack.enter_context(patch.object(Config, "update"))
            change_user_password("test'pw")
            cmd = mock_subprocess.call_args[0][0]
            assert "mkpasswd -m sha-512" in cmd
    def test_password_with_single_quotes_in_luks_cmd(
        self, mock_shell, mock_subprocess,
        mock_config_eval  # noqa: ARG002
    ):
        with patch("bin.change_password.sh", mock_shell):
            change_luks_password("old'pw", "new'pw")
            cmd = mock_subprocess.call_args[0][0]
            assert "printf '%s\\n%s'" in cmd
            assert "cryptsetup luksChangeKey" in cmd
    def test_password_with_backslash_chars(
        self, mock_shell, mock_subprocess,
        mock_config_eval  # noqa: ARG002
    ):
        with patch("bin.change_password.sh", mock_shell):
            change_luks_password("old\\npw", "new\\tpw")
            cmd = mock_subprocess.call_args[0][0]
            assert "printf '%s\\n%s'" in cmd
            assert "cryptsetup luksChangeKey" in cmd
    def test_password_with_special_characters(
        self, mock_shell, mock_subprocess,
        mock_config_eval  # noqa: ARG002
    ):
        with patch("bin.change_password.sh", mock_shell):
            change_luks_password("p@$$w0rd!&|;", "n3w#p*ss()")
            cmd = mock_subprocess.call_args[0][0]
            assert "cryptsetup luksChangeKey" in cmd

class TestUpdateTpm2:
    def test_update_tpm2_success(self, mock_shell, mock_subprocess,
                                 mock_config_eval):  # noqa: ARG002
        mock_subprocess.return_value.returncode = 0
        with patch("bin.change_password.sh", mock_shell):
            update_tpm2()
    def test_update_tpm2_failure(self, mock_shell, mock_subprocess,
                                 mock_config_eval, capsys):  # noqa: ARG002
        mock_subprocess.return_value.returncode = 1
        with patch("bin.change_password.sh", mock_shell):
            update_tpm2()
            assert "failed" in capsys.readouterr().err.lower()

class TestChangePasswordMain:
    def test_main_conflicting_args(self, mock_shell,  # noqa: ARG002
                                   monkeypatch):
        monkeypatch.setattr("sys.argv", [
            "change_password.py",
            "--full-disk-encryption-only", "--user-account-only",
        ])
        with pytest.raises(SystemExit):
            main()
    def test_main_both_reuses_new_password_for_user(
        self, mock_shell, monkeypatch  # noqa: ARG002
    ):
        monkeypatch.setattr("sys.argv", ["change_password.py"])
        with contextlib.ExitStack() as stack:
            stack.enter_context(patch(
                "bin.change_password.ask_for_old_password",
                return_value="old"))
            stack.enter_context(patch(
                "bin.change_password.ask_for_new_password",
                return_value="shared"))
            stack.enter_context(patch(
                "bin.change_password.change_luks_password"))
            user = stack.enter_context(patch(
                "bin.change_password.change_user_password"))
            main()
            user.assert_called_once_with("shared")
