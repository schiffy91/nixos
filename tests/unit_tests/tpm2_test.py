from unittest.mock import patch

import pytest

from bin.tpm2 import require_tpm2, require_luks, enroll, wipe, enable, disable

class TestTpm2Checks:
    def test_require_tpm2_exists(self, mock_shell,
                                 mock_config_eval):  # noqa: ARG002
        with patch("bin.tpm2.sh", mock_shell):
            with patch.object(mock_shell, "exists", return_value=True):
                with patch.object(mock_shell, "file_read", return_value="2"):
                    require_tpm2()
    def test_require_tpm2_missing(self, mock_shell,
                                   mock_config_eval):  # noqa: ARG002
        with patch("bin.tpm2.sh", mock_shell):
            with patch.object(mock_shell, "exists", return_value=False):
                with pytest.raises(SystemExit):
                    require_tpm2()
    def test_require_tpm2_wrong_version(self, mock_shell,
                                         mock_config_eval):  # noqa: ARG002
        with patch("bin.tpm2.sh", mock_shell):
            with patch.object(mock_shell, "exists", return_value=True):
                with patch.object(mock_shell, "file_read", return_value="1"):
                    with pytest.raises(SystemExit):
                        require_tpm2()
    def test_require_luks_encrypted(self, mock_shell, mock_subprocess,
                                     mock_config_eval):  # noqa: ARG002
        mock_subprocess.return_value.returncode = 0
        with patch("bin.tpm2.sh", mock_shell):
            require_luks()
    def test_require_luks_not_encrypted(self, mock_shell, mock_subprocess,
                                         mock_config_eval):  # noqa: ARG002
        mock_subprocess.return_value.returncode = 1
        with patch("bin.tpm2.sh", mock_shell):
            with pytest.raises(SystemExit):
                require_luks()

class TestTpm2Enroll:
    def test_enroll_success(self, mock_shell, mock_subprocess,
                             mock_config_eval):  # noqa: ARG002
        mock_subprocess.return_value.returncode = 0
        with patch("bin.tpm2.sh", mock_shell):
            enroll()
    def test_enroll_failure(self, mock_shell, mock_subprocess,
                             mock_config_eval):  # noqa: ARG002
        mock_subprocess.return_value.returncode = 1
        with patch("bin.tpm2.sh", mock_shell):
            with pytest.raises(SystemExit):
                enroll()

class TestTpm2Wipe:
    def test_wipe_success(self, mock_shell, mock_subprocess,
                           mock_config_eval):  # noqa: ARG002
        mock_subprocess.return_value.returncode = 0
        with patch("bin.tpm2.sh", mock_shell):
            wipe()
    def test_wipe_failure(self, mock_shell, mock_subprocess,
                           mock_config_eval):  # noqa: ARG002
        mock_subprocess.return_value.returncode = 1
        with patch("bin.tpm2.sh", mock_shell):
            with pytest.raises(SystemExit):
                wipe()

class TestTpm2Enable:
    def test_enable_success(self, mock_shell, mock_subprocess,
                             mock_config_eval):  # noqa: ARG002
        mock_subprocess.return_value.returncode = 0
        with patch("bin.tpm2.sh", mock_shell):
            with patch.object(mock_shell, "exists", return_value=True):
                with patch.object(mock_shell, "file_read", return_value="2"):
                    enable()
    def test_enable_no_tpm(self, mock_shell,
                            mock_config_eval):  # noqa: ARG002
        with patch("bin.tpm2.sh", mock_shell):
            with patch.object(mock_shell, "exists", return_value=False):
                with pytest.raises(SystemExit):
                    enable()

class TestTpm2Disable:
    def test_disable_success(self, mock_shell, mock_subprocess,
                              mock_config_eval):  # noqa: ARG002
        mock_subprocess.return_value.returncode = 0
        with patch("bin.tpm2.sh", mock_shell):
            with patch.object(mock_shell, "exists", return_value=True):
                with patch.object(mock_shell, "file_read", return_value="2"):
                    disable()
    def test_disable_no_tpm(self, mock_shell,
                             mock_config_eval):  # noqa: ARG002
        with patch("bin.tpm2.sh", mock_shell):
            with patch.object(mock_shell, "exists", return_value=False):
                with pytest.raises(SystemExit):
                    disable()
    def test_disable_wipe_fails(self, mock_shell, mock_subprocess,
                                 mock_config_eval):  # noqa: ARG002
        mock_subprocess.return_value.returncode = 1
        with patch("bin.tpm2.sh", mock_shell):
            with patch.object(mock_shell, "exists", return_value=True):
                with patch.object(mock_shell, "file_read", return_value="2"):
                    with pytest.raises(SystemExit):
                        disable()
