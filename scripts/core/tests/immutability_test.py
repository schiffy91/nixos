from unittest.mock import patch, MagicMock, mock_open

import pytest

from core import immutability


def mock_run(return_code=0):
    return MagicMock(returncode=return_code, stdout="", stderr="")


class TestLogging:
    def test_log(self, capsys):
        immutability.log("hello")
        assert "hello" in capsys.readouterr().out

    def test_warn(self, capsys):
        immutability.warn("danger")
        assert "WRN danger" in capsys.readouterr().err

    def test_error(self, capsys):
        immutability.error("fail")
        assert "ERR fail" in capsys.readouterr().err


class TestRun:
    def test_success_with_string(self):
        with patch("subprocess.run", return_value=mock_run()):
            assert immutability.run("echo hi") == 0

    def test_success_with_list(self):
        with patch("subprocess.run", return_value=mock_run()) as m:
            assert immutability.run(["echo", "hi"]) == 0
            m.assert_called_once_with(
                ["echo", "hi"], capture_output=True, text=True,
                check=False,
            )

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

    def test_no_shell_true(self):
        with patch("subprocess.run", return_value=mock_run()) as m:
            immutability.run("btrfs filesystem sync /mnt")
            args, kwargs = m.call_args
            assert "shell" not in kwargs
            assert args[0] == ["btrfs", "filesystem", "sync", "/mnt"]


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
            assert any("umount" in str(c) for c in cmds)


class TestBtrfs:
    def test_sync(self):
        with patch("subprocess.run", return_value=mock_run()) as m:
            immutability.btrfs_sync("/mnt")
            m.assert_called_once_with(
                ["btrfs", "filesystem", "sync", "/mnt"],
                capture_output=True, text=True, check=False,
            )

    def test_delete_success(self):
        with patch("subprocess.run", return_value=mock_run()) as m:
            immutability.btrfs_delete("/mnt/@root")
            args = m.call_args[0][0]
            assert args == ["btrfs", "subvolume", "delete",
                            "/mnt/@root", "--commit-after"]

    def test_delete_failure(self):
        with patch("subprocess.run", return_value=mock_run(1)):
            with pytest.raises(SystemExit):
                immutability.btrfs_delete("/mnt/@root")

    def test_delete_no_per_op_sync(self):
        """Batched sync: delete no longer calls btrfs_sync."""
        with patch("subprocess.run", return_value=mock_run()) as m:
            immutability.btrfs_delete("/mnt/@root")
            cmds = [c[0][0] for c in m.call_args_list]
            assert not any(
                "sync" in str(c) for c in cmds
            ), "delete should not sync per-operation"

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
                delete_result, delete_result,
            ]):
                with patch("os.path.isdir",
                           side_effect=[True, True, False, True]):
                    immutability.btrfs_delete_recursively("/mnt/@root")

    def test_snapshot_success(self):
        with patch("subprocess.run", return_value=mock_run()):
            with patch("os.path.isdir",
                       side_effect=lambda p: "src" in p):
                immutability.btrfs_snapshot("/mnt/src", "/mnt/dst")

    def test_snapshot_aborts_missing_src(self):
        with patch("os.path.isdir", return_value=False):
            with patch("subprocess.run", return_value=mock_run()):
                with pytest.raises(SystemExit):
                    immutability.btrfs_snapshot("/mnt/src", "/mnt/dst")

    def test_snapshot_failure(self):
        with patch("subprocess.run", return_value=mock_run(1)):
            with patch("os.path.isdir",
                       side_effect=lambda p: "src" in p):
                with pytest.raises(SystemExit):
                    immutability.btrfs_snapshot("/mnt/src", "/mnt/dst")

    def test_snapshot_no_per_op_sync(self):
        """Batched sync: snapshot no longer syncs src and dst."""
        with patch("subprocess.run", return_value=mock_run()) as m:
            with patch("os.path.isdir",
                       side_effect=lambda p: "src" in p):
                immutability.btrfs_snapshot("/mnt/src", "/mnt/dst")
                cmds = [c[0][0] for c in m.call_args_list]
                sync_cmds = [c for c in cmds if "sync" in str(c)]
                assert len(sync_cmds) == 0

    def test_set_rw_success(self):
        with patch("subprocess.run", return_value=mock_run()) as m:
            immutability.btrfs_set_rw("/mnt/@root")
            args = m.call_args[0][0]
            assert args == ["btrfs", "property", "set", "-ts",
                            "/mnt/@root", "ro", "false"]

    def test_set_rw_failure(self):
        with patch("subprocess.run", return_value=mock_run(1)):
            with pytest.raises(SystemExit):
                immutability.btrfs_set_rw("/mnt/@root")


class TestMount:
    def test_mount_subvolumes(self):
        with patch("os.path.exists", return_value=True):
            with patch("subprocess.run", return_value=mock_run()) as m:
                immutability.mount_subvolumes("/dev/sda")
                cmds = [c[0][0] for c in m.call_args_list]
                assert any("subvolid=5" in str(c) for c in cmds)

    def test_mount_aborts_missing_device(self):
        with patch("os.path.exists", return_value=False):
            with patch("subprocess.run", return_value=mock_run()):
                with pytest.raises(SystemExit):
                    immutability.mount_subvolumes("/dev/missing")

    def test_mount_aborts_on_failure(self):
        with patch("os.path.exists", return_value=True):
            results = [mock_run(), mock_run(1), mock_run(), mock_run()]
            with patch("subprocess.run", side_effect=results):
                with pytest.raises(SystemExit):
                    immutability.mount_subvolumes("/dev/sda")

    def test_unmount_subvolumes(self):
        with patch("subprocess.run", return_value=mock_run()) as m:
            immutability.unmount_subvolumes()
            cmds = [c[0][0] for c in m.call_args_list]
            assert any("umount" in str(c) for c in cmds)
            assert any("rm" in str(c) for c in cmds)


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
            immutability.create_sentinel("/snap")


class TestCopyPersistentFiles:
    def test_copy_runs_rsync_with_precomputed_filter(self):
        with patch("subprocess.run", return_value=mock_run()) as m:
            immutability.copy_persistent_files(
                "/prev", "/curr", "/nix/store/abc-filter",
            )
            args = m.call_args[0][0]
            assert "rsync" in args
            assert any("abc-filter" in str(a) for a in args)
            assert "/prev/" in args
            assert "/curr" in args

    def test_copy_aborts_on_rsync_failure(self):
        with patch("subprocess.run", return_value=mock_run(1)):
            with pytest.raises(SystemExit):
                immutability.copy_persistent_files(
                    "/prev", "/curr", "/filter.txt",
                )

    def test_copy_uses_list_args(self):
        with patch("subprocess.run", return_value=mock_run()) as m:
            immutability.copy_persistent_files(
                "/prev", "/curr", "/filter.txt",
            )
            args, kwargs = m.call_args
            assert isinstance(args[0], list)
            assert "shell" not in kwargs


class TestResetSubvolume:
    def setup_method(self):
        immutability._mount_point = "/mnt"

    def test_reset_full_flow(self):
        with patch("subprocess.run", return_value=mock_run()):
            with patch("os.path.isdir", return_value=True):
                with patch("os.path.isfile", return_value=True):
                    with patch("builtins.open", mock_open()):
                        immutability.reset_subvolume(
                            "@root", "/", "@snapshots",
                            "CLEAN", "/filter.txt",
                        )

    def test_reset_initializes_missing_snapshots(self):
        created = set()

        def isdir(p):
            if p in created:
                return True
            if "PENULTIMATE" in p or "PREVIOUS" in p:
                return False
            return True

        def fake_run(args, **kw):
            if isinstance(args, list) and "snapshot" in args:
                created.add(args[-1])
            return mock_run()

        with patch("subprocess.run", side_effect=fake_run):
            with patch("os.path.isdir", side_effect=isdir):
                with patch("os.path.isfile", return_value=True):
                    with patch("builtins.open", mock_open()):
                        immutability.reset_subvolume(
                            "@root", "/", "@snapshots",
                            "CLEAN", "/filter.txt",
                        )

    def test_reset_aborts_missing_clean(self):
        with patch("os.path.isdir", return_value=False):
            with patch("os.path.isfile", return_value=True):
                with patch("subprocess.run", return_value=mock_run()):
                    with pytest.raises(SystemExit):
                        immutability.reset_subvolume(
                            "@root", "/", "@snapshots",
                            "CLEAN", "/filter.txt",
                        )

    def test_reset_with_crash_recovery(self):
        with patch("subprocess.run", return_value=mock_run()):
            with patch("os.path.isdir", return_value=True):
                with patch("os.path.isfile",
                           side_effect=lambda p:
                           ".boot-ready" not in p):
                    with patch("builtins.open", mock_open()):
                        immutability.reset_subvolume(
                            "@root", "/", "@snapshots",
                            "CLEAN", "/filter.txt",
                        )

    def test_reset_ends_with_single_sync(self):
        runs = []

        def tracking_run(args, **kw):
            runs.append(args)
            return mock_run()
        with patch("subprocess.run", side_effect=tracking_run):
            with patch("os.path.isdir", return_value=True):
                with patch("os.path.isfile", return_value=True):
                    with patch("builtins.open", mock_open()):
                        immutability.reset_subvolume(
                            "@root", "/", "@snapshots",
                            "CLEAN", "/filter.txt",
                        )
        sync_calls = [r for r in runs
                      if isinstance(r, list) and "sync" in r]
        assert len(sync_calls) == 1
        assert "/mnt/@root" in sync_calls[0]


class TestRestoreSubvolume:
    def setup_method(self):
        immutability._mount_point = "/mnt"

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

    def test_restore_syncs_once(self):
        runs = []

        def tracking_run(args, **kw):
            runs.append(args)
            return mock_run()
        with patch("subprocess.run", side_effect=tracking_run):
            with patch("os.path.isdir", return_value=True):
                immutability.restore_subvolume(
                    "@root", "@snapshots", "PREVIOUS",
                )
        sync_calls = [r for r in runs
                      if isinstance(r, list) and "sync" in r]
        assert len(sync_calls) == 1


class TestSnapshotOnly:
    def setup_method(self):
        immutability._mount_point = "/mnt"

    def test_snapshot_only_flow(self):
        with patch("subprocess.run", return_value=mock_run()):
            with patch("os.path.isdir", return_value=True):
                immutability.snapshot_only(
                    "@root", "@snapshots", "CLEAN",
                )

    def test_snapshot_only_initializes_missing(self):
        created = set()

        def isdir(p):
            if p in created:
                return True
            if "PENULTIMATE" in p or "PREVIOUS" in p:
                return False
            return True

        def fake_run(args, **kw):
            if isinstance(args, list) and "snapshot" in args:
                created.add(args[-1])
            return mock_run()

        with patch("subprocess.run", side_effect=fake_run):
            with patch("os.path.isdir", side_effect=isdir):
                immutability.snapshot_only(
                    "@root", "@snapshots", "CLEAN",
                )

    def test_snapshot_only_aborts_missing_clean(self):
        with patch("os.path.isdir", return_value=False):
            with patch("subprocess.run", return_value=mock_run()):
                with pytest.raises(SystemExit):
                    immutability.snapshot_only(
                        "@root", "@snapshots", "CLEAN",
                    )

    def test_snapshot_only_syncs_once(self):
        runs = []

        def tracking_run(args, **kw):
            runs.append(args)
            return mock_run()
        with patch("subprocess.run", side_effect=tracking_run):
            with patch("os.path.isdir", return_value=True):
                immutability.snapshot_only(
                    "@root", "@snapshots", "CLEAN",
                )
        sync_calls = [r for r in runs
                      if isinstance(r, list) and "sync" in r]
        assert len(sync_calls) == 1


class TestProcessSubvolume:
    def setup_method(self):
        immutability._mount_point = "/mnt"

    def test_process_reset(self):
        with patch.object(immutability, "reset_subvolume") as reset:
            immutability.process_subvolume(
                "@root=/", "reset", "@snapshots", "CLEAN",
                {"@root": "/filter.txt"},
            )
            reset.assert_called_once_with(
                "@root", "/", "@snapshots", "CLEAN", "/filter.txt",
            )

    def test_process_snapshot_only(self):
        with patch.object(immutability, "snapshot_only") as snap:
            immutability.process_subvolume(
                "@root=/", "snapshot-only", "@snapshots", "CLEAN", {},
            )
            snap.assert_called_once_with("@root", "@snapshots", "CLEAN")

    def test_process_restore_previous(self):
        with patch.object(immutability, "restore_subvolume") as rest:
            immutability.process_subvolume(
                "@root=/", "restore-previous", "@snapshots", "CLEAN", {},
            )
            rest.assert_called_once_with("@root", "@snapshots", "PREVIOUS")

    def test_process_restore_penultimate(self):
        with patch.object(immutability, "restore_subvolume") as rest:
            immutability.process_subvolume(
                "@root=/", "restore-penultimate", "@snapshots", "CLEAN", {},
            )
            rest.assert_called_once_with(
                "@root", "@snapshots", "PENULTIMATE",
            )

    def test_process_unknown_mode(self):
        with patch("subprocess.run", return_value=mock_run()):
            with pytest.raises(SystemExit):
                immutability.process_subvolume(
                    "@root=/", "bogus", "@snapshots", "CLEAN", {},
                )

    def test_process_default_mount_point(self):
        with patch.object(immutability, "reset_subvolume") as reset:
            immutability.process_subvolume(
                "@root", "reset", "@snapshots", "CLEAN",
                {"@root": "/filter.txt"},
            )
            assert reset.call_args[0][1] == "/"

    def test_process_reset_empty_filter(self):
        with patch.object(immutability, "reset_subvolume") as reset:
            immutability.process_subvolume(
                "@root=/", "reset", "@snapshots", "CLEAN", {},
            )
            assert reset.call_args[0][4] == ""


class TestMain:
    def test_missing_args(self, monkeypatch):
        monkeypatch.setattr("sys.argv", ["immutability"])
        with pytest.raises(SystemExit):
            immutability.main()

    def test_parses_filter_from_pair_args(self, monkeypatch):
        monkeypatch.setattr("sys.argv", [
            "immutability", "/dev/sda", "@snapshots", "CLEAN",
            "reset", "@root=/:/filter-root", "@home=/home:/filter-home",
        ])
        with patch("core.immutability.mount_subvolumes"):
            with patch("core.immutability.process_subvolume") as proc:
                with patch("core.immutability.unmount_subvolumes"):
                    immutability.main()
                    assert proc.call_count == 2

    def test_full_run_reset_parallel(self, monkeypatch):
        monkeypatch.setattr("sys.argv", [
            "immutability", "/dev/sda", "@snapshots", "CLEAN",
            "reset", "@root=/:/f1", "@home=/home:/f2",
        ])
        with patch("core.immutability.mount_subvolumes"):
            with patch("core.immutability.process_subvolume") as proc:
                with patch("core.immutability.unmount_subvolumes"):
                    immutability.main()
                    assert proc.call_count == 2
                    names = {c[0][0] for c in proc.call_args_list}
                    assert names == {"@root=/", "@home=/home"}

    def test_single_subvolume_no_threads(self, monkeypatch):
        monkeypatch.setattr("sys.argv", [
            "immutability", "/dev/sda", "@snapshots", "CLEAN",
            "reset", "@root=/:/f1",
        ])
        with patch("core.immutability.mount_subvolumes"):
            with patch("core.immutability.process_subvolume") as proc:
                with patch("core.immutability.unmount_subvolumes"):
                    with patch("threading.Thread") as thread_cls:
                        immutability.main()
                        thread_cls.assert_not_called()
                        proc.assert_called_once()

    def test_disabled_mode(self, monkeypatch):
        monkeypatch.setattr("sys.argv", [
            "immutability", "/dev/sda", "@snapshots", "CLEAN",
            "disabled", "@root=/",
        ])
        with patch("core.immutability.mount_subvolumes"):
            with patch("core.immutability.process_subvolume") as proc:
                with patch("core.immutability.unmount_subvolumes"):
                    immutability.main()
                    proc.assert_not_called()

    def test_unknown_mode_aborts(self, monkeypatch):
        monkeypatch.setattr("sys.argv", [
            "immutability", "/dev/sda", "@snapshots", "CLEAN",
            "bogus", "@root=/",
        ])
        with patch("core.immutability.mount_subvolumes"):
            with patch("core.immutability.unmount_subvolumes"):
                with pytest.raises(SystemExit):
                    immutability.main()

    def test_pair_without_filter(self, monkeypatch):
        monkeypatch.setattr("sys.argv", [
            "immutability", "/dev/sda", "@snapshots", "CLEAN",
            "snapshot-only", "@root=/",
        ])
        with patch("core.immutability.mount_subvolumes"):
            with patch("core.immutability.process_subvolume") as proc:
                with patch("core.immutability.unmount_subvolumes"):
                    immutability.main()
                    filter_files = proc.call_args[0][4]
                    assert filter_files == {}

    def test_thread_error_propagation(self, monkeypatch):
        monkeypatch.setattr("sys.argv", [
            "immutability", "/dev/sda", "@snapshots", "CLEAN",
            "reset", "@root=/:/f1", "@home=/home:/f2",
        ])

        def failing_process(pair, *a):
            if "@root" in pair:
                raise SystemExit(1)
        with patch("core.immutability.mount_subvolumes"):
            with patch("core.immutability.process_subvolume",
                       side_effect=failing_process):
                with patch("core.immutability.unmount_subvolumes"):
                    with pytest.raises(SystemExit):
                        immutability.main()
