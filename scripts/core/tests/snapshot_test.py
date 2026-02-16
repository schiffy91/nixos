from unittest.mock import patch

import pytest

from core.snapshot import Snapshot


@pytest.mark.usefixtures("mock_config_eval")
class TestSnapshotPaths:
    def test_get_snapshots_path(self):
        assert Snapshot.get_snapshots_path() == "/.snapshots"

    def test_get_clean_snapshot_name(self):
        assert Snapshot.get_clean_snapshot_name() == "CLEAN"

    def test_get_subvolumes_to_reset_on_boot(self):
        result = Snapshot.get_subvolumes_to_reset_on_boot()
        assert result == [["@root", "/"], ["@home", "/home"]]

    def test_get_clean_snapshot_path(self):
        result = Snapshot.get_clean_snapshot_path("@root")
        assert result == "/.snapshots/@root/CLEAN"


@pytest.mark.usefixtures("mock_config_eval")
class TestSnapshotCreation:
    def test_create_success(self, mock_shell):
        Snapshot.sh = mock_shell
        with patch.object(mock_shell, "rm"):
            with patch.object(mock_shell, "mkdir"):
                with patch.object(mock_shell, "run") as run:
                    with patch.object(mock_shell, "dirname",
                                      return_value="/.snapshots/@root"):
                        Snapshot.create_initial_snapshots()
                        assert run.call_count == 2
                        calls = [str(c) for c in run.call_args_list]
                        assert any(
                            "btrfs subvolume snapshot" in c for c in calls
                        )

    def test_create_handles_errors(self, mock_shell, capsys):
        Snapshot.sh = mock_shell
        with patch.object(mock_shell, "rm"):
            with patch.object(mock_shell, "mkdir"):
                with patch.object(mock_shell, "dirname",
                                  return_value="/.snapshots/@root"):
                    with patch.object(mock_shell, "run",
                                      side_effect=Exception("btrfs error")):
                        Snapshot.create_initial_snapshots()
                        captured = capsys.readouterr()
                        assert "Failed to create" in captured.err
