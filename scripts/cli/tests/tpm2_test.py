from unittest.mock import patch

import pytest

from cli.tpm2 import (
    tpm2_exists, disk_encrypted,
    enable_tpm2, disable_tpm2, main,
)


class TestTpm2Helpers:
    def test_tpm2_exists_true(self, mock_shell,
                              mock_config_eval):  # noqa: ARG002
        with patch("cli.tpm2.sh", mock_shell):
            with patch.object(mock_shell, "exists", return_value=True):
                with patch.object(mock_shell, "file_read", return_value="2"):
                    assert tpm2_exists() is True

    def test_tpm2_exists_false(self, mock_shell,
                               mock_config_eval):  # noqa: ARG002
        with patch("cli.tpm2.sh", mock_shell):
            with patch.object(mock_shell, "exists", return_value=False):
                assert tpm2_exists() is False

    def test_disk_encrypted_true(self, mock_shell, mock_subprocess,
                                 mock_config_eval):  # noqa: ARG002
        mock_subprocess.return_value.returncode = 0
        with patch("cli.tpm2.sh", mock_shell):
            assert disk_encrypted() is True

    def test_disk_encrypted_false(self, mock_shell, mock_subprocess,
                                  mock_config_eval):  # noqa: ARG002
        mock_subprocess.return_value.returncode = 1
        with patch("cli.tpm2.sh", mock_shell):
            assert disk_encrypted() is False


class TestTpm2Enable:
    def test_enable_success(self, mock_shell):
        with patch("cli.tpm2.sh", mock_shell):
            with patch("cli.tpm2.tpm2_exists", return_value=True):
                with patch("cli.tpm2.disk_encrypted", return_value=True):
                    with patch("cli.tpm2.enroll_tpm2", return_value=True):
                        enable_tpm2()

    def test_enable_no_tpm(self, mock_shell):
        with patch("cli.tpm2.sh", mock_shell):
            with patch("cli.tpm2.tpm2_exists", return_value=False):
                with pytest.raises(SystemExit):
                    enable_tpm2()

    def test_enable_not_encrypted(self, mock_shell,
                                  mock_config_eval):  # noqa: ARG002
        with patch("cli.tpm2.sh", mock_shell):
            with patch("cli.tpm2.tpm2_exists", return_value=True):
                with patch("cli.tpm2.disk_encrypted", return_value=False):
                    with pytest.raises(SystemExit):
                        enable_tpm2()


class TestTpm2Disable:
    def test_disable_success(self, mock_shell, mock_subprocess,
                             mock_config_eval):  # noqa: ARG002
        mock_subprocess.return_value.returncode = 0
        with patch("cli.tpm2.sh", mock_shell):
            with patch("cli.tpm2.tpm2_exists", return_value=True):
                disable_tpm2()

    def test_disable_no_tpm(self, mock_shell):
        with patch("cli.tpm2.sh", mock_shell):
            with patch("cli.tpm2.tpm2_exists", return_value=False):
                with pytest.raises(SystemExit):
                    disable_tpm2()


class TestTpm2Main:
    def test_main_enable(self, monkeypatch):
        monkeypatch.setattr("sys.argv", ["tpm2.py", "enable"])
        with patch("cli.tpm2.enable_tpm2") as m:
            main()
            m.assert_called_once()

    def test_main_disable(self, monkeypatch):
        monkeypatch.setattr("sys.argv", ["tpm2.py", "disable"])
        with patch("cli.tpm2.disable_tpm2") as m:
            main()
            m.assert_called_once()
