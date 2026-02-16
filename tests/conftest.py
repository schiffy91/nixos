import subprocess, sys
from pathlib import Path
from unittest.mock import MagicMock
import pytest

sys.path.insert(0, str(Path(__file__).parent.parent))

# Patch subprocess.run before CLI modules are imported so that
# Shell(root_required=True) at module level doesn't call sys.exit(1).
_original_run = subprocess.run

def _mock_run(*args, **kwargs):
    cmd = args[0] if args else kwargs.get("cmd", "")
    if isinstance(cmd, str) and "id -u" in cmd:
        mock_result = MagicMock()
        mock_result.stdout = "0"
        mock_result.stderr = ""
        mock_result.returncode = 0
        return mock_result
    return _original_run(*args, **kwargs)

subprocess.run = _mock_run  # type: ignore[assignment]

import bin.install, bin.update, bin.diff, bin.eval  # noqa: E402, F401, E401
import bin.secure_boot, bin.tpm2, bin.change_password, bin.gpu_vfio  # noqa: E402, F401, E401

subprocess.run = _original_run  # type: ignore[assignment]

from lib.shell import Shell  # noqa: E402
from lib.config import Config  # noqa: E402

@pytest.fixture
def mock_subprocess():
    mock = MagicMock()
    mock.return_value.returncode = 0
    mock.return_value.stdout = ""
    mock.return_value.stderr = ""
    return mock

@pytest.fixture
def mock_shell(monkeypatch, mock_subprocess):
    monkeypatch.setattr("subprocess.run", mock_subprocess)
    return Shell()

@pytest.fixture
def mock_config_eval(monkeypatch):
    evals = {
        "config.settings.secrets.path": "/etc/nixos/secrets",
        "config.settings.secrets.hashedPasswordFile": "hashed_password.txt",
        "config.settings.disk.device": "/dev/sda",
        "config.settings.disk.by.partlabel.root":
            "/dev/disk/by-partlabel/disk-main-root",
        "config.settings.disk.subvolumes.snapshots.mountPoint": "/.snapshots",
        "config.settings.disk.immutability.persist.snapshots.cleanName":
            "CLEAN",
        "config.settings.disk.immutability.mode": "reset",
        "config.settings.disk.immutability.persist.paths":
            "/etc/nixos /var/lib/nixos",
        "config.settings.disk.subvolumes.nameMountPointPairs.resetOnBoot":
            "@root=/ @home=/home",
        "config.settings.user.admin.username": "testuser",
        "config.settings.tpm.device": "/dev/tpmrm0",
        "config.settings.tpm.versionPath":
            "/sys/class/tpm/tpm0/tpm_version_major",
    }
    monkeypatch.setattr(
        Config, "eval",
        classmethod(lambda cls, attr: evals.get(attr, ""))
    )
