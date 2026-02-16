from unittest.mock import patch

import pytest

from cli.gpu_vfio import (
    get_driver, check_iommu,
    detach, attach, status, main,
)


class TestGpuVfioHelpers:
    def test_get_driver_nvidia(self, mock_shell, mock_subprocess):
        mock_subprocess.return_value.returncode = 0
        mock_subprocess.return_value.stdout = "nvidia"
        with patch("cli.gpu_vfio.sh", mock_shell):
            with patch.object(mock_shell, "is_symlink", return_value=True):
                with patch.object(mock_shell, "realpath",
                                  return_value="/sys/nvidia"):
                    with patch.object(mock_shell, "basename",
                                      return_value="nvidia"):
                        assert get_driver("0000:01:00.0") == "nvidia"

    def test_get_driver_none(self, mock_shell):
        with patch("cli.gpu_vfio.sh", mock_shell):
            with patch.object(mock_shell, "is_symlink", return_value=False):
                assert get_driver("0000:01:00.0") == "none"

    def test_check_iommu_enabled(self, mock_shell, mock_subprocess):
        mock_subprocess.return_value.returncode = 0
        with patch("cli.gpu_vfio.sh", mock_shell):
            check_iommu()

    def test_check_iommu_disabled(self, mock_shell, mock_subprocess):
        mock_subprocess.return_value.returncode = 1
        with patch("cli.gpu_vfio.sh", mock_shell):
            with pytest.raises(SystemExit):
                check_iommu()


class TestGpuVfioDetach:
    def test_detach(self, mock_shell, mock_subprocess):
        mock_subprocess.return_value.returncode = 0
        mock_subprocess.return_value.stdout = "nvidia"
        with patch("cli.gpu_vfio.sh", mock_shell):
            with patch("cli.gpu_vfio.check_iommu"):
                with patch("cli.gpu_vfio.get_driver",
                           side_effect=["nvidia", "vfio-pci", "vfio-pci"]):
                    with patch("cli.gpu_vfio.unbind_device"):
                        with patch("cli.gpu_vfio.bind_device"):
                            detach()

    def test_detach_already_vfio(self, mock_shell):
        with patch("cli.gpu_vfio.sh", mock_shell):
            with patch("cli.gpu_vfio.check_iommu"):
                with patch("cli.gpu_vfio.get_driver",
                           return_value="vfio-pci"):
                    detach()


class TestGpuVfioAttach:
    def test_attach(self, mock_shell, mock_subprocess):
        mock_subprocess.return_value.returncode = 0
        with patch("cli.gpu_vfio.sh", mock_shell):
            with patch("cli.gpu_vfio.get_driver",
                       side_effect=["vfio-pci", "nvidia", "snd_hda_intel"]):
                with patch("cli.gpu_vfio.unbind_device"):
                    with patch("cli.gpu_vfio.bind_device"):
                        attach()

    def test_attach_already_nvidia(self, mock_shell):
        with patch("cli.gpu_vfio.sh", mock_shell):
            with patch("cli.gpu_vfio.get_driver", return_value="nvidia"):
                attach()


class TestGpuVfioStatus:
    def test_status_vfio(self, mock_shell, capsys):
        with patch("cli.gpu_vfio.sh", mock_shell):
            with patch("cli.gpu_vfio.get_driver", return_value="vfio-pci"):
                status()
                output = capsys.readouterr().out
                assert "Detached" in output

    def test_status_nvidia(self, mock_shell, capsys):
        with patch("cli.gpu_vfio.sh", mock_shell):
            with patch("cli.gpu_vfio.get_driver", return_value="nvidia"):
                status()
                output = capsys.readouterr().out
                assert "Attached" in output


class TestGpuVfioMain:
    def test_main_detach(self, mock_shell,  # noqa: ARG002
                         monkeypatch):
        monkeypatch.setattr("sys.argv", ["gpu_vfio.py", "detach"])
        with patch("cli.gpu_vfio.detach") as m:
            main()
            m.assert_called_once()

    def test_main_attach(self, mock_shell,  # noqa: ARG002
                         monkeypatch):
        monkeypatch.setattr("sys.argv", ["gpu_vfio.py", "attach"])
        with patch("cli.gpu_vfio.attach") as m:
            main()
            m.assert_called_once()

    def test_main_status(self, mock_shell,  # noqa: ARG002
                         monkeypatch):
        monkeypatch.setattr("sys.argv", ["gpu_vfio.py", "status"])
        with patch("cli.gpu_vfio.status") as m:
            main()
            m.assert_called_once()

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
