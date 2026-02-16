import contextlib
from unittest.mock import patch

import pytest

from bin.gpu_vfio import (
    get_driver, check_iommu,
    detach, attach, status, main,
)

class TestGpuVfioHelpers:
    def test_get_driver_nvidia(self, mock_shell, mock_subprocess):
        mock_subprocess.return_value.returncode = 0
        mock_subprocess.return_value.stdout = "nvidia"
        with contextlib.ExitStack() as stack:
            stack.enter_context(patch("bin.gpu_vfio.sh", mock_shell))
            stack.enter_context(patch.object(mock_shell, "is_symlink",
                                             return_value=True))
            stack.enter_context(patch.object(mock_shell, "realpath",
                                             return_value="/sys/nvidia"))
            stack.enter_context(patch.object(mock_shell, "basename",
                                             return_value="nvidia"))
            assert get_driver("0000:01:00.0") == "nvidia"
    def test_get_driver_vfio_pci(self, mock_shell, mock_subprocess):
        mock_subprocess.return_value.returncode = 0
        mock_subprocess.return_value.stdout = "vfio-pci"
        with contextlib.ExitStack() as stack:
            stack.enter_context(patch("bin.gpu_vfio.sh", mock_shell))
            stack.enter_context(patch.object(mock_shell, "is_symlink",
                                             return_value=True))
            stack.enter_context(patch.object(mock_shell, "realpath",
                                             return_value="/sys/vfio-pci"))
            stack.enter_context(patch.object(mock_shell, "basename",
                                             return_value="vfio-pci"))
            assert get_driver("0000:01:00.0") == "vfio-pci"
    def test_get_driver_none(self, mock_shell):
        with patch("bin.gpu_vfio.sh", mock_shell):
            with patch.object(mock_shell, "is_symlink", return_value=False):
                assert get_driver("0000:01:00.0") == "none"
    def test_check_iommu_enabled(self, mock_shell, mock_subprocess):
        mock_subprocess.return_value.returncode = 0
        with patch("bin.gpu_vfio.sh", mock_shell):
            check_iommu()
    def test_check_iommu_disabled(self, mock_shell, mock_subprocess):
        mock_subprocess.return_value.returncode = 1
        with patch("bin.gpu_vfio.sh", mock_shell):
            with pytest.raises(SystemExit):
                check_iommu()

class TestGpuVfioDetach:
    def test_detach(self, mock_shell, mock_subprocess):
        mock_subprocess.return_value.returncode = 0
        mock_subprocess.return_value.stdout = "nvidia"
        with contextlib.ExitStack() as stack:
            stack.enter_context(patch("bin.gpu_vfio.sh", mock_shell))
            stack.enter_context(patch("bin.gpu_vfio.check_iommu"))
            stack.enter_context(patch("bin.gpu_vfio.get_driver",
                                      side_effect=["nvidia", "vfio-pci",
                                                    "vfio-pci"]))
            stack.enter_context(patch("bin.gpu_vfio.unbind_device"))
            stack.enter_context(patch("bin.gpu_vfio.bind_device"))
            detach()
    def test_detach_already_vfio(self, mock_shell):
        with contextlib.ExitStack() as stack:
            stack.enter_context(patch("bin.gpu_vfio.sh", mock_shell))
            stack.enter_context(patch("bin.gpu_vfio.check_iommu"))
            stack.enter_context(patch("bin.gpu_vfio.get_driver",
                                      return_value="vfio-pci"))
            detach()

class TestGpuVfioAttach:
    def test_attach(self, mock_shell, mock_subprocess):
        mock_subprocess.return_value.returncode = 0
        with contextlib.ExitStack() as stack:
            stack.enter_context(patch("bin.gpu_vfio.sh", mock_shell))
            stack.enter_context(patch("bin.gpu_vfio.get_driver",
                                      side_effect=["vfio-pci", "nvidia",
                                                    "snd_hda_intel"]))
            stack.enter_context(patch("bin.gpu_vfio.unbind_device"))
            stack.enter_context(patch("bin.gpu_vfio.bind_device"))
            attach()
    def test_attach_already_nvidia(self, mock_shell):
        with patch("bin.gpu_vfio.sh", mock_shell):
            with patch("bin.gpu_vfio.get_driver", return_value="nvidia"):
                attach()

class TestGpuVfioStatus:
    def test_status_vfio(self, mock_shell, capsys):
        with patch("bin.gpu_vfio.sh", mock_shell):
            with patch("bin.gpu_vfio.get_driver", return_value="vfio-pci"):
                status()
                output = capsys.readouterr().out
                assert "Detached" in output
    def test_status_nvidia(self, mock_shell, capsys):
        with patch("bin.gpu_vfio.sh", mock_shell):
            with patch("bin.gpu_vfio.get_driver", return_value="nvidia"):
                status()
                output = capsys.readouterr().out
                assert "Attached" in output
    def test_status_unknown(self, mock_shell, capsys):
        with patch("bin.gpu_vfio.sh", mock_shell):
            with patch("bin.gpu_vfio.get_driver", return_value="other"):
                status()
                output = capsys.readouterr().out
                assert "Unknown" in output

class TestGpuVfioMain:
    def test_main_invalid(self, mock_shell,  # noqa: ARG002
                          monkeypatch):
        monkeypatch.setattr("sys.argv", ["gpu_vfio.py", "invalid"])
        with pytest.raises(SystemExit):
            main()
    def test_main_no_args(self, mock_shell,  # noqa: ARG002
                          monkeypatch):
        monkeypatch.setattr("sys.argv", ["gpu_vfio.py"])
        with pytest.raises(SystemExit):
            main()
