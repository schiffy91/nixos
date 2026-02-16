from unittest.mock import patch

import pytest

from lib.snapshot import Snapshot

@pytest.mark.usefixtures("mock_config_eval")
class TestSnapshotPaths:
    def test_get_snapshots_path(self):
        assert Snapshot.get_snapshots_path() == "/.snapshots"
    def test_get_clean_snapshot_name(self):
        assert Snapshot.get_clean_snapshot_name() == "CLEAN"
    def test_get_subvolumes_to_reset_on_boot(self):
        result = Snapshot.get_subvolumes_to_reset_on_boot()
        assert result == [["@root", "/"], ["@home", "/home"]]
    def test_get_clean_snapshot_path_root(self):
        result = Snapshot.get_clean_snapshot_path("@root")
        assert result == "/.snapshots/@root/CLEAN"
    def test_get_clean_snapshot_path_home(self):
        result = Snapshot.get_clean_snapshot_path("@home")
        assert result == "/.snapshots/@home/CLEAN"
    def test_single_subvolume_parsing(self, monkeypatch):
        from lib.config import Config
        monkeypatch.setattr(
            Config, "eval",
            classmethod(lambda cls, attr: (
                "@data=/data"
                if "resetOnBoot" in attr else
                "/.snapshots"
                if "mountPoint" in attr else
                "CLEAN"
            )),
        )
        result = Snapshot.get_subvolumes_to_reset_on_boot()
        assert result == [["@data", "/data"]]

@pytest.mark.usefixtures("mock_config_eval")
class TestSnapshotCreation:
    def test_create_handles_exception_without_crashing(
        self, mock_shell, capsys
    ):
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
    def test_create_verifies_btrfs_command_format(self, mock_shell):
        Snapshot.sh = mock_shell
        with patch.object(mock_shell, "rm"):
            with patch.object(mock_shell, "mkdir"):
                with patch.object(mock_shell, "run") as run:
                    with patch.object(mock_shell, "dirname",
                                      return_value="/.snapshots/@root"):
                        Snapshot.create_initial_snapshots()
                        for call_args in run.call_args_list:
                            cmd = call_args[0][0]
                            assert cmd.startswith(
                                "btrfs subvolume snapshot -r"
                            )
    def test_create_processes_all_subvolumes(self, mock_shell):
        Snapshot.sh = mock_shell
        processed = []
        with patch.object(mock_shell, "rm",
                          side_effect=lambda p: processed.append(p)):
            with patch.object(mock_shell, "mkdir"):
                with patch.object(mock_shell, "run"):
                    with patch.object(mock_shell, "dirname",
                                      return_value="/.snapshots/@root"):
                        Snapshot.create_initial_snapshots()
                        assert len(processed) == 2
                        assert "/.snapshots/@root/CLEAN" in processed
                        assert "/.snapshots/@home/CLEAN" in processed
    def test_create_continues_after_first_failure(self, mock_shell, capsys):
        Snapshot.sh = mock_shell
        call_count = {"rm": 0}
        def counting_rm(path):
            call_count["rm"] += 1
            if call_count["rm"] == 1:
                raise Exception("first subvolume failed")

        with patch.object(mock_shell, "rm", side_effect=counting_rm):
            with patch.object(mock_shell, "mkdir"):
                with patch.object(mock_shell, "run"):
                    with patch.object(mock_shell, "dirname",
                                      return_value="/.snapshots/@root"):
                        Snapshot.create_initial_snapshots()
                        assert call_count["rm"] == 2
                        captured = capsys.readouterr()
                        assert "Failed to create" in captured.err
    def test_create_snapshot_command_includes_mount_point(self, mock_shell):
        Snapshot.sh = mock_shell
        with patch.object(mock_shell, "rm"):
            with patch.object(mock_shell, "mkdir"):
                with patch.object(mock_shell, "run") as run:
                    with patch.object(mock_shell, "dirname",
                                      return_value="/.snapshots/@root"):
                        Snapshot.create_initial_snapshots()
                        cmds = [c[0][0] for c in run.call_args_list]
                        assert any(" / " in c for c in cmds)
                        assert any(" /home " in c for c in cmds)
    def test_create_snapshot_command_includes_clean_path(self, mock_shell):
        Snapshot.sh = mock_shell
        with patch.object(mock_shell, "rm"):
            with patch.object(mock_shell, "mkdir"):
                with patch.object(mock_shell, "run") as run:
                    with patch.object(mock_shell, "dirname",
                                      return_value="/.snapshots/@root"):
                        Snapshot.create_initial_snapshots()
                        cmds = [c[0][0] for c in run.call_args_list]
                        assert any(
                            "/.snapshots/@root/CLEAN" in c for c in cmds
                        )
                        assert any(
                            "/.snapshots/@home/CLEAN" in c for c in cmds
                        )
