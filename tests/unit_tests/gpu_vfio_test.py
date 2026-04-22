from unittest.mock import patch
import pytest
from bin.gpu_vfio import vm_state, vm_defined, gpu_driver, status, main


class TestVmState:
    def test_running(self, mock_shell, mock_subprocess):
        mock_subprocess.return_value.returncode = 0
        mock_subprocess.return_value.stdout = "running\n"
        with patch("bin.gpu_vfio.sh", mock_shell):
            assert vm_state() == "running"
    def test_undefined(self, mock_shell, mock_subprocess):
        mock_subprocess.return_value.returncode = 1
        with patch("bin.gpu_vfio.sh", mock_shell):
            assert vm_state() == "undefined"


class TestVmDefined:
    def test_yes(self, mock_shell, mock_subprocess):
        mock_subprocess.return_value.returncode = 0
        with patch("bin.gpu_vfio.sh", mock_shell):
            assert vm_defined() is True
    def test_no(self, mock_shell, mock_subprocess):
        mock_subprocess.return_value.returncode = 1
        with patch("bin.gpu_vfio.sh", mock_shell):
            assert vm_defined() is False


class TestGpuDriver:
    def test_nvidia(self):
        with patch("pathlib.Path.is_symlink", return_value=True):
            with patch("pathlib.Path.resolve") as r:
                r.return_value.name = "nvidia"
                assert gpu_driver() == "nvidia"
    def test_none(self):
        with patch("pathlib.Path.is_symlink", return_value=False):
            assert gpu_driver() == "none"


class TestStatus:
    def test_prints(self, mock_shell, capsys):
        with patch("bin.gpu_vfio.sh", mock_shell):
            with patch("bin.gpu_vfio.vm_state", return_value="shut off"):
                with patch("bin.gpu_vfio.gpu_driver", return_value="nvidia"):
                    with patch("pathlib.Path.is_char_device", return_value=True):
                        with patch("pathlib.Path.exists", return_value=True):
                            status()
                            assert "VFIO Status" in capsys.readouterr().out


class TestMain:
    def test_invalid(self, mock_shell, monkeypatch):  # noqa: ARG002
        monkeypatch.setattr("sys.argv", ["gpu_vfio.py", "invalid"])
        with pytest.raises(SystemExit):
            main()
    def test_no_args(self, mock_shell, monkeypatch):  # noqa: ARG002
        monkeypatch.setattr("sys.argv", ["gpu_vfio.py"])
        with pytest.raises(SystemExit):
            main()
