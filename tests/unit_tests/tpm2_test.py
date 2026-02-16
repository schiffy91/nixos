import contextlib
from unittest.mock import patch

import pytest

from bin.tpm2 import (
    tpm2_exists, disk_encrypted, get_enrolled_tpm2_devices,
    enroll_tpm2, enable_tpm2, disable_tpm2,
)

class TestTpm2Helpers:
    def test_tpm2_exists_true(self, mock_shell,
                              mock_config_eval):  # noqa: ARG002
        with patch("bin.tpm2.sh", mock_shell):
            with patch.object(mock_shell, "exists", return_value=True):
                with patch.object(mock_shell, "file_read", return_value="2"):
                    assert tpm2_exists() is True
    def test_tpm2_exists_false(self, mock_shell,
                               mock_config_eval):  # noqa: ARG002
        with patch("bin.tpm2.sh", mock_shell):
            with patch.object(mock_shell, "exists", return_value=False):
                assert tpm2_exists() is False
    def test_tpm2_exists_wrong_version(self, mock_shell,
                                       mock_config_eval):  # noqa: ARG002
        with patch("bin.tpm2.sh", mock_shell):
            with patch.object(mock_shell, "exists", return_value=True):
                with patch.object(mock_shell, "file_read", return_value="1"):
                    assert tpm2_exists() is False
    def test_disk_encrypted_true(self, mock_shell, mock_subprocess,
                                 mock_config_eval):  # noqa: ARG002
        mock_subprocess.return_value.returncode = 0
        with patch("bin.tpm2.sh", mock_shell):
            assert disk_encrypted() is True
    def test_disk_encrypted_false(self, mock_shell, mock_subprocess,
                                  mock_config_eval):  # noqa: ARG002
        mock_subprocess.return_value.returncode = 1
        with patch("bin.tpm2.sh", mock_shell):
            assert disk_encrypted() is False
    def test_get_enrolled_tpm2_devices(self, mock_shell, mock_subprocess):
        mock_subprocess.return_value.stdout = "/dev/tpmrm0"
        with patch("bin.tpm2.sh", mock_shell):
            result = get_enrolled_tpm2_devices()
            assert "/dev/tpmrm0" in result
    def test_enroll_tpm2_success(self, mock_shell, mock_subprocess,
                                 mock_config_eval):  # noqa: ARG002
        mock_subprocess.return_value.returncode = 0
        with patch("bin.tpm2.sh", mock_shell):
            assert enroll_tpm2() is True
    def test_enroll_tpm2_failure(self, mock_shell, mock_subprocess,
                                 mock_config_eval):  # noqa: ARG002
        mock_subprocess.return_value.returncode = 1
        with patch("bin.tpm2.sh", mock_shell):
            assert enroll_tpm2() is False

class TestTpm2Enable:
    def test_enable_success(self, mock_shell):
        with contextlib.ExitStack() as stack:
            stack.enter_context(patch("bin.tpm2.sh", mock_shell))
            stack.enter_context(patch("bin.tpm2.tpm2_exists",
                                      return_value=True))
            stack.enter_context(patch("bin.tpm2.disk_encrypted",
                                      return_value=True))
            stack.enter_context(patch("bin.tpm2.enroll_tpm2",
                                      return_value=True))
            enable_tpm2()
    def test_enable_no_tpm(self, mock_shell):
        with patch("bin.tpm2.sh", mock_shell):
            with patch("bin.tpm2.tpm2_exists", return_value=False):
                with pytest.raises(SystemExit):
                    enable_tpm2()
    def test_enable_not_encrypted(self, mock_shell,
                                  mock_config_eval):  # noqa: ARG002
        with contextlib.ExitStack() as stack:
            stack.enter_context(patch("bin.tpm2.sh", mock_shell))
            stack.enter_context(patch("bin.tpm2.tpm2_exists",
                                      return_value=True))
            stack.enter_context(patch("bin.tpm2.disk_encrypted",
                                      return_value=False))
            with pytest.raises(SystemExit):
                enable_tpm2()
    def test_enable_enroll_fails(self, mock_shell):
        with contextlib.ExitStack() as stack:
            stack.enter_context(patch("bin.tpm2.sh", mock_shell))
            stack.enter_context(patch("bin.tpm2.tpm2_exists",
                                      return_value=True))
            stack.enter_context(patch("bin.tpm2.disk_encrypted",
                                      return_value=True))
            stack.enter_context(patch("bin.tpm2.enroll_tpm2",
                                      return_value=False))
            with pytest.raises(SystemExit):
                enable_tpm2()

class TestTpm2Disable:
    def test_disable_success(self, mock_shell, mock_subprocess,
                             mock_config_eval):  # noqa: ARG002
        mock_subprocess.return_value.returncode = 0
        with patch("bin.tpm2.sh", mock_shell):
            with patch("bin.tpm2.tpm2_exists", return_value=True):
                disable_tpm2()
    def test_disable_no_tpm(self, mock_shell):
        with patch("bin.tpm2.sh", mock_shell):
            with patch("bin.tpm2.tpm2_exists", return_value=False):
                with pytest.raises(SystemExit):
                    disable_tpm2()
    def test_disable_wipe_fails(self, mock_shell, mock_subprocess,
                                mock_config_eval):  # noqa: ARG002
        mock_subprocess.return_value.returncode = 1
        with patch("bin.tpm2.sh", mock_shell):
            with patch("bin.tpm2.tpm2_exists", return_value=True):
                with pytest.raises(SystemExit):
                    disable_tpm2()
