from unittest.mock import patch, MagicMock, mock_open

import pytest

from core import immutability


def mock_run(return_code=0):
    return MagicMock(returncode=return_code, stdout="", stderr="")


class TestLogging:
    def test_log(self, capsys):
        immutability._depth = 0
        immutability.log("hello")
        assert "hello" in capsys.readouterr().out

    def test_log_indented(self, capsys):
        immutability._depth = 2
        immutability.log("hello")
        assert "    hello" in capsys.readouterr().out
        immutability._depth = 0

    def test_warn(self, capsys):
        immutability.warn("danger")
        assert "WRN danger" in capsys.readouterr().err

    def test_error(self, capsys):
        immutability.error("fail")
        assert "ERR fail" in capsys.readouterr().err


class TestRun:
    def test_success(self):
        with patch("subprocess.run", return_value=mock_run()):
            assert immutability.run("echo hi") == 0

    def test_failure(self):
        r = mock_run(1)
        r.stderr = "bad\n"
        with patch("subprocess.run", return_value=r):
            assert immutability.run("false") == 1

    def test_logs_stdout(self, capsys):
        r = mock_run()
        r.stdout = "output line\n"
        with patch("subprocess.run", return_value=r):
            immutability.run("cmd")
            assert "output line" in capsys.readouterr().out

    def test_depth_tracking(self):
        immutability._depth = 0
        with patch("subprocess.run", return_value=mock_run()):
            immutability.run("cmd")
            assert immutability._depth == 0


class TestAbort:
    def test_abort_exits(self):
        with patch("subprocess.run", return_value=mock_run()):
            with pytest.raises(SystemExit):
                immutability.abort("test error")

    def test_abort_unmounts(self):
        with patch("subprocess.run", return_value=mock_run()) as m:
            with pytest.raises(SystemExit):
                immutability.abort("test error")
            cmds = [c[0][0] for c in m.call_args_list]
            assert any("umount" in c for c in cmds)


class TestRequire:
    def test_require_passes(self):
        with patch("subprocess.run", return_value=mock_run()):
            immutability.require("-d /tmp")

    def test_require_fails(self):
        with patch("subprocess.run", return_value=mock_run(1)):
            with pytest.raises(SystemExit):
                immutability.require("-d /nonexistent")


class TestBtrfs:
    def test_sync(self):
        with patch("subprocess.run", return_value=mock_run()) as m:
            immutability.btrfs_sync("/mnt")
            cmd = m.call_args[0][0]
            assert "btrfs filesystem sync /mnt" in cmd

    def test_delete_success(self):
        with patch("subprocess.run", return_value=mock_run()) as m:
            immutability.btrfs_delete("/mnt/@root")
            cmds = [c[0][0] for c in m.call_args_list]
            assert any("btrfs subvolume delete" in c for c in cmds)

    def test_delete_failure(self):
        with patch("subprocess.run", return_value=mock_run(1)):
            with pytest.raises(SystemExit):
                immutability.btrfs_delete("/mnt/@root")

    def test_delete_recursively_nonexistent(self):
        with patch("os.path.isdir", return_value=False):
            immutability.btrfs_delete_recursively("/nonexistent")

    def test_delete_recursively_with_children(self):
        with patch("os.path.isdir", return_value=True):
            list_result = mock_run()
            list_result.stdout = (
                "ID 256 gen 10 top level 5 path @root/sub1\n"
            )
            delete_result = mock_run()
            with patch("subprocess.run", side_effect=[
                list_result, delete_result, delete_result,
                delete_result, delete_result, delete_result,
            ]):
                with patch("os.path.isdir",
                           side_effect=[True, True, False, True]):
                    immutability.btrfs_delete_recursively("/mnt/@root")

    def test_delete_recursively_joins_path_fields(self):
        with patch("os.path.isdir", return_value=True):
            list_result = mock_run()
            list_result.stdout = (
                "ID 256 gen 10 top level 5 path @root/sub with space\n"
            )
            delete_result = mock_run()
            with patch("subprocess.run", side_effect=[
                list_result, delete_result, delete_result,
                delete_result, delete_result, delete_result,
            ]):
                with patch("os.path.isdir",
                           side_effect=[True, True, False, True]):
                    immutability.btrfs_delete_recursively("/mnt/@root")

    def test_snapshot_success(self):
        with patch("subprocess.run", return_value=mock_run()):
            with patch("os.path.isdir", return_value=False):
                immutability.btrfs_snapshot("/mnt/src", "/mnt/dst")

    def test_snapshot_failure(self):
        results = [mock_run(), mock_run(1), mock_run(), mock_run()]
        with patch("subprocess.run", side_effect=results):
            with patch("os.path.isdir", return_value=False):
                with pytest.raises(SystemExit):
                    immutability.btrfs_snapshot("/mnt/src", "/mnt/dst")

    def test_set_rw_success(self):
        with patch("subprocess.run", return_value=mock_run()) as m:
            immutability.btrfs_set_rw("/mnt/@root")
            cmd = m.call_args[0][0]
            assert "btrfs property set" in cmd

    def test_set_rw_failure(self):
        with patch("subprocess.run", return_value=mock_run(1)):
            with pytest.raises(SystemExit):
                immutability.btrfs_set_rw("/mnt/@root")


class TestMount:
    def test_mount_subvolumes(self):
        with patch("subprocess.run", return_value=mock_run()) as m:
            immutability.mount_subvolumes("/dev/sda")
            cmds = [c[0][0] for c in m.call_args_list]
            assert any("subvolid=5" in c for c in cmds)

    def test_unmount_subvolumes(self):
        with patch("subprocess.run", return_value=mock_run()) as m:
            immutability.unmount_subvolumes()
            cmds = [c[0][0] for c in m.call_args_list]
            assert any("umount" in c for c in cmds)
            assert any("rm -rf" in c for c in cmds)


class TestRecovery:
    def test_recovery_needed_incomplete(self):
        with patch("os.path.isdir", return_value=True):
            with patch("os.path.isfile", return_value=False):
                assert immutability.check_recovery_needed("/snap") is True

    def test_recovery_not_needed_complete(self):
        with patch("os.path.isdir", return_value=True):
            with patch("os.path.isfile", return_value=True):
                assert immutability.check_recovery_needed("/snap") is False

    def test_recovery_not_needed_missing(self):
        with patch("os.path.isdir", return_value=False):
            assert immutability.check_recovery_needed("/snap") is False

    def test_create_sentinel(self):
        with patch("builtins.open", mock_open()):
            with patch("subprocess.run", return_value=mock_run()):
                immutability.create_sentinel("/snap")


class TestReadPathsFile:
    def test_reads_lines(self):
        content = "/etc/nixos\n/var/lib/nixos\n/home/user/.config\n"
        with patch("builtins.open", mock_open(read_data=content)):
            result = immutability.read_paths_file("/paths.txt")
            assert result == ["/etc/nixos", "/var/lib/nixos",
                              "/home/user/.config"]

    def test_skips_blank_lines(self):
        content = "/etc/nixos\n\n/var/lib/nixos\n\n"
        with patch("builtins.open", mock_open(read_data=content)):
            result = immutability.read_paths_file("/paths.txt")
            assert result == ["/etc/nixos", "/var/lib/nixos"]


class TestBuildRsyncFilter:
    def test_root_mount_point(self):
        paths = ["/etc/nixos", "/var/lib/nixos"]
        with patch("os.path.isdir", return_value=True):
            with patch("os.path.exists", return_value=True):
                lines = immutability.build_rsync_filter(
                    "/", paths, "/mnt/@snapshots/@root/PREVIOUS",
                )
        assert "+ */" in lines[0]
        assert "+ /etc/nixos/" in lines
        assert "+ /etc/nixos/**" in lines
        assert "+ /var/lib/nixos/" in lines
        assert "+ /var/lib/nixos/**" in lines
        assert "- *" in lines

    def test_home_mount_point_strips_prefix(self):
        paths = ["/home/user/.config/Code", "/etc/nixos"]
        with patch("os.path.isdir", return_value=True):
            with patch("os.path.exists", return_value=True):
                lines = immutability.build_rsync_filter(
                    "/home", paths,
                    "/mnt/@snapshots/@home/PREVIOUS",
                )
        assert "+ /user/.config/Code/" in lines
        assert "+ /user/.config/Code/**" in lines
        assert "/etc/nixos" not in " ".join(lines)

    def test_skips_nonexistent_paths(self, capsys):
        paths = ["/etc/nixos", "/etc/ghost"]
        with patch("os.path.isdir", return_value=False):
            with patch("os.path.exists", return_value=False):
                lines = immutability.build_rsync_filter(
                    "/", paths, "/mnt/@snapshots/@root/PREVIOUS",
                )
        has_includes = any(
            l.startswith("+ /") and l != "+ */" for l in lines
        )
        assert not has_includes

    def test_includes_files(self):
        paths = ["/etc/machine-id"]
        with patch("os.path.isdir", return_value=False):
            with patch("os.path.exists", return_value=True):
                lines = immutability.build_rsync_filter(
                    "/", paths, "/mnt/@snapshots/@root/PREVIOUS",
                )
        assert "+ /etc/machine-id" in lines
        assert "+ /etc/machine-id/**" not in lines

    def test_empty_relative_skipped(self):
        paths = ["/home"]
        with patch("os.path.isdir", return_value=True):
            with patch("os.path.exists", return_value=True):
                lines = immutability.build_rsync_filter(
                    "/home", paths,
                    "/mnt/@snapshots/@home/PREVIOUS",
                )
        has_includes = any(
            l.startswith("+ /") and l != "+ */" for l in lines
        )
        assert not has_includes


class TestCopyPersistentFiles:
    def test_copy_builds_filter_and_runs_rsync(self):
        paths = ["/etc/nixos"]
        with patch("os.path.isdir", return_value=True):
            with patch("os.path.exists", return_value=True):
                with patch("builtins.open", mock_open()):
                    with patch("subprocess.run",
                               return_value=mock_run()) as m:
                        immutability.copy_persistent_files(
                            "/", paths, "/prev", "/curr",
                        )
                        cmds = [c[0][0] for c in m.call_args_list]
                        assert any("rsync" in c for c in cmds)

    def test_copy_skips_when_no_matches(self, capsys):
        paths = ["/home/user/.config"]
        with patch("os.path.isdir", return_value=False):
            with patch("os.path.exists", return_value=False):
                immutability.copy_persistent_files(
                    "/", paths, "/prev", "/curr",
                )
        output = capsys.readouterr().out
        assert "skip" in output or "No paths matched" in output

    def test_copy_aborts_on_rsync_failure(self):
        paths = ["/etc/nixos"]
        with patch("os.path.isdir", return_value=True):
            with patch("os.path.exists", return_value=True):
                with patch("builtins.open", mock_open()):
                    with patch("subprocess.run",
                               return_value=mock_run(1)):
                        with pytest.raises(SystemExit):
                            immutability.copy_persistent_files(
                                "/", paths, "/prev", "/curr",
                            )


class TestResetSubvolume:
    def setup_method(self):
        immutability._mount_point = "/mnt"
        immutability._depth = 0

    def test_reset_full_flow(self):
        paths = ["/etc/nixos"]
        with patch("subprocess.run", return_value=mock_run()):
            with patch("os.path.isdir", return_value=True):
                with patch("os.path.isfile", return_value=True):
                    with patch("os.path.exists", return_value=True):
                        with patch("builtins.open", mock_open()):
                            immutability.reset_subvolume(
                                "@root", "/", "@snapshots",
                                "CLEAN", paths,
                            )

    def test_reset_initializes_missing_snapshots(self):
        paths = ["/etc/nixos"]
        dirs_exist = {
            "/mnt/@snapshots/@root/PENULTIMATE": False,
            "/mnt/@snapshots/@root/PREVIOUS": False,
        }
        with patch("subprocess.run", return_value=mock_run()):
            with patch("os.path.isdir",
                       side_effect=lambda p: dirs_exist.get(p, True)):
                with patch("os.path.isfile", return_value=True):
                    with patch("os.path.exists", return_value=True):
                        with patch("builtins.open", mock_open()):
                            immutability.reset_subvolume(
                                "@root", "/", "@snapshots",
                                "CLEAN", paths,
                            )

    def test_reset_with_crash_recovery(self):
        paths = ["/etc/nixos"]
        with patch("subprocess.run", return_value=mock_run()):
            with patch("os.path.isdir", return_value=True):
                with patch("os.path.isfile",
                           side_effect=lambda p:
                           ".boot-ready" not in p):
                    with patch("os.path.exists", return_value=True):
                        with patch("builtins.open", mock_open()):
                            immutability.reset_subvolume(
                                "@root", "/", "@snapshots",
                                "CLEAN", paths,
                            )

    def test_reset_home_uses_mount_point(self):
        paths = ["/home/user/.config/Code", "/etc/nixos"]
        with patch("subprocess.run", return_value=mock_run()) as m:
            with patch("os.path.isdir", return_value=True):
                with patch("os.path.isfile", return_value=True):
                    with patch("os.path.exists", return_value=True):
                        with patch("builtins.open", mock_open()):
                            immutability.reset_subvolume(
                                "@home", "/home", "@snapshots",
                                "CLEAN", paths,
                            )
                            cmds = [c[0][0] for c in m.call_args_list]
                            rsync_cmds = [c for c in cmds
                                          if "rsync" in c]
                            assert len(rsync_cmds) > 0


class TestRestoreSubvolume:
    def setup_method(self):
        immutability._mount_point = "/mnt"
        immutability._depth = 0

    def test_restore_previous(self):
        with patch("subprocess.run", return_value=mock_run()):
            with patch("os.path.isdir", return_value=True):
                immutability.restore_subvolume(
                    "@root", "@snapshots", "PREVIOUS",
                )

    def test_restore_penultimate(self):
        with patch("subprocess.run", return_value=mock_run()):
            with patch("os.path.isdir", return_value=True):
                immutability.restore_subvolume(
                    "@root", "@snapshots", "PENULTIMATE",
                )

    def test_restore_aborts_if_missing(self):
        with patch("os.path.isdir", return_value=False):
            with patch("subprocess.run", return_value=mock_run()):
                with pytest.raises(SystemExit):
                    immutability.restore_subvolume(
                        "@root", "@snapshots", "PREVIOUS",
                    )


class TestSnapshotOnly:
    def setup_method(self):
        immutability._mount_point = "/mnt"
        immutability._depth = 0

    def test_snapshot_only_flow(self):
        with patch("subprocess.run", return_value=mock_run()):
            with patch("os.path.isdir", return_value=True):
                immutability.snapshot_only(
                    "@root", "@snapshots", "CLEAN",
                )

    def test_snapshot_only_initializes_missing(self):
        dirs_exist = {
            "/mnt/@snapshots/@root/PENULTIMATE": False,
            "/mnt/@snapshots/@root/PREVIOUS": False,
        }
        with patch("subprocess.run", return_value=mock_run()):
            with patch("os.path.isdir",
                       side_effect=lambda p: dirs_exist.get(p, True)):
                immutability.snapshot_only(
                    "@root", "@snapshots", "CLEAN",
                )


class TestMain:
    def setup_method(self):
        immutability._depth = 0

    def test_missing_args(self, monkeypatch):
        monkeypatch.setattr("sys.argv", ["immutability"])
        with pytest.raises(SystemExit):
            immutability.main()

    def test_full_run_reset(self, monkeypatch):
        monkeypatch.setattr("sys.argv", [
            "immutability", "/dev/sda", "@snapshots", "CLEAN",
            "reset", "/paths.txt", "@root=/", "@home=/home",
        ])
        with patch("core.immutability.mount_subvolumes"):
            with patch("core.immutability.read_paths_file",
                       return_value=["/etc/nixos"]):
                with patch("core.immutability.reset_subvolume") as reset:
                    with patch("core.immutability.unmount_subvolumes"):
                        immutability.main()
                        assert reset.call_count == 2
                        reset.assert_any_call(
                            "@root", "/", "@snapshots",
                            "CLEAN", ["/etc/nixos"],
                        )
                        reset.assert_any_call(
                            "@home", "/home", "@snapshots",
                            "CLEAN", ["/etc/nixos"],
                        )

    def test_single_subvolume(self, monkeypatch):
        monkeypatch.setattr("sys.argv", [
            "immutability", "/dev/sda", "@snapshots", "CLEAN",
            "reset", "/paths.txt", "@root=/",
        ])
        with patch("core.immutability.mount_subvolumes") as mount:
            with patch("core.immutability.read_paths_file",
                       return_value=[]):
                with patch("core.immutability.reset_subvolume"):
                    with patch("core.immutability.unmount_subvolumes"):
                        immutability.main()
                        mount.assert_called_once_with("/dev/sda")

    def test_snapshot_only_mode(self, monkeypatch):
        monkeypatch.setattr("sys.argv", [
            "immutability", "/dev/sda", "@snapshots", "CLEAN",
            "snapshot-only", "/paths.txt", "@root=/",
        ])
        with patch("core.immutability.mount_subvolumes"):
            with patch("core.immutability.read_paths_file",
                       return_value=[]):
                with patch("core.immutability.snapshot_only") as snap:
                    with patch("core.immutability.unmount_subvolumes"):
                        immutability.main()
                        snap.assert_called_once_with(
                            "@root", "@snapshots", "CLEAN",
                        )

    def test_restore_previous_mode(self, monkeypatch):
        monkeypatch.setattr("sys.argv", [
            "immutability", "/dev/sda", "@snapshots", "CLEAN",
            "restore-previous", "/paths.txt", "@root=/",
        ])
        with patch("core.immutability.mount_subvolumes"):
            with patch("core.immutability.read_paths_file",
                       return_value=[]):
                with patch(
                    "core.immutability.restore_subvolume"
                ) as rest:
                    with patch("core.immutability.unmount_subvolumes"):
                        immutability.main()
                        rest.assert_called_once_with(
                            "@root", "@snapshots", "PREVIOUS",
                        )

    def test_restore_penultimate_mode(self, monkeypatch):
        monkeypatch.setattr("sys.argv", [
            "immutability", "/dev/sda", "@snapshots", "CLEAN",
            "restore-penultimate", "/paths.txt", "@root=/",
        ])
        with patch("core.immutability.mount_subvolumes"):
            with patch("core.immutability.read_paths_file",
                       return_value=[]):
                with patch(
                    "core.immutability.restore_subvolume"
                ) as rest:
                    with patch("core.immutability.unmount_subvolumes"):
                        immutability.main()
                        rest.assert_called_once_with(
                            "@root", "@snapshots", "PENULTIMATE",
                        )

    def test_disabled_mode(self, monkeypatch):
        monkeypatch.setattr("sys.argv", [
            "immutability", "/dev/sda", "@snapshots", "CLEAN",
            "disabled", "/paths.txt", "@root=/",
        ])
        with patch("core.immutability.mount_subvolumes"):
            with patch("core.immutability.reset_subvolume") as reset:
                with patch("core.immutability.unmount_subvolumes"):
                    immutability.main()
                    reset.assert_not_called()

    def test_unknown_mode_aborts(self, monkeypatch):
        monkeypatch.setattr("sys.argv", [
            "immutability", "/dev/sda", "@snapshots", "CLEAN",
            "bogus", "/paths.txt", "@root=/",
        ])
        with patch("core.immutability.mount_subvolumes"):
            with patch("core.immutability.read_paths_file",
                       return_value=[]):
                with patch("core.immutability.unmount_subvolumes"):
                    with pytest.raises(SystemExit):
                        immutability.main()
