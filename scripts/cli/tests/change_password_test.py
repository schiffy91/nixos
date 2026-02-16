from unittest.mock import patch

import pytest

from core.config import Config
from cli.change_password import (
    ask_for_old_password, ask_for_new_password,
    change_luks_password, change_user_password, main,
)


class TestChangePasswordHelpers:
    def test_ask_for_old_password(self, mock_shell):  # noqa: ARG002
        with patch("getpass.getpass", return_value="oldpw"):
            assert ask_for_old_password() == "oldpw"

    def test_ask_for_new_password_match(self, mock_shell):  # noqa: ARG002
        with patch("getpass.getpass", side_effect=["newpw", "newpw"]):
            assert ask_for_new_password() == "newpw"

    def test_ask_for_new_password_mismatch(self, mock_shell,  # noqa: ARG002
                                           capsys):
        with patch("getpass.getpass", side_effect=[
            "a", "b", "c", "c",
        ]):
            assert ask_for_new_password() == "c"
            assert "do not match" in capsys.readouterr().err

    def test_change_luks_password(self, mock_shell, mock_subprocess,
                                  mock_config_eval):  # noqa: ARG002
        with patch("cli.change_password.sh", mock_shell):
            change_luks_password("old", "new")
            assert any(
                "cryptsetup" in str(c)
                for c in mock_subprocess.call_args_list
            )

    def test_change_luks_password_fails(self, mock_shell, mock_subprocess,
                                        mock_config_eval):  # noqa: ARG002
        mock_subprocess.return_value.returncode = 1
        with patch("cli.change_password.sh", mock_shell):
            with pytest.raises(SystemExit):
                change_luks_password("old", "new")

    def test_change_user_password(self, mock_shell, mock_subprocess,
                                  mock_config_eval):  # noqa: ARG002
        mock_subprocess.return_value.stdout = "$6$hash"
        with patch("cli.change_password.sh", mock_shell):
            with patch.object(mock_shell, "file_write"):
                with patch.object(Config, "update"):
                    change_user_password("newpw")


class TestChangePasswordMain:
    def test_main_both(self, mock_shell,  # noqa: ARG002
                       monkeypatch):
        monkeypatch.setattr("sys.argv", ["change_password.py"])
        with patch("cli.change_password.ask_for_old_password",
                   return_value="old"):
            with patch("cli.change_password.ask_for_new_password",
                       return_value="new"):
                with patch("cli.change_password.change_luks_password"):
                    with patch("cli.change_password.change_user_password"):
                        main()

    def test_main_fde_only(self, mock_shell,  # noqa: ARG002
                           monkeypatch):
        monkeypatch.setattr("sys.argv", [
            "change_password.py", "--full-disk-encryption-only",
        ])
        with patch("cli.change_password.ask_for_old_password",
                   return_value="old"):
            with patch("cli.change_password.ask_for_new_password",
                       return_value="new"):
                with patch("cli.change_password.change_luks_password") as luks:
                    with patch(
                        "cli.change_password.change_user_password"
                    ) as user:
                        main()
                        luks.assert_called_once()
                        user.assert_not_called()

    def test_main_user_only(self, mock_shell,  # noqa: ARG002
                            monkeypatch):
        monkeypatch.setattr("sys.argv", [
            "change_password.py", "--user-account-only",
        ])
        with patch("cli.change_password.ask_for_new_password",
                   return_value="new"):
            with patch("cli.change_password.change_luks_password") as luks:
                with patch(
                    "cli.change_password.change_user_password"
                ) as user:
                    main()
                    luks.assert_not_called()
                    user.assert_called_once()

    def test_main_conflicting_args(self, mock_shell,  # noqa: ARG002
                                   monkeypatch):
        monkeypatch.setattr("sys.argv", [
            "change_password.py",
            "--full-disk-encryption-only", "--user-account-only",
        ])
        with pytest.raises(SystemExit):
            main()
