import sys
from pathlib import Path
from unittest.mock import MagicMock

sys.path.insert(0, str(Path(__file__).parent.parent.parent))

# Patch subprocess.run before CLI modules are imported so that
# Shell(root_required=True) at module level doesn't call sys.exit(1).
import subprocess  # noqa: E402

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

# Preload all CLI modules while subprocess.run is patched
import cli.install  # noqa: E402, F401
import cli.update  # noqa: E402, F401
import cli.diff  # noqa: E402, F401
import cli.eval  # noqa: E402, F401
import cli.secure_boot  # noqa: E402, F401
import cli.tpm2  # noqa: E402, F401
import cli.change_password  # noqa: E402, F401
import cli.gpu_vfio  # noqa: E402, F401

# Restore original subprocess.run
subprocess.run = _original_run  # type: ignore[assignment]

from core.tests.fixtures import *  # noqa: E402, F403
