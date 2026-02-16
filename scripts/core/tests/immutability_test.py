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


class TestCopyPersistentFiles:
    def test_copy_uses_rsync(self):
        with patch("subprocess.run", return_value=mock_run()) as m:
            immutability.copy_persistent_files(
                "/prev", "/curr", "/filter.txt",
            )
            cmd = m.call_args[0][0]
            assert "rsync" in cmd
            assert "/filter.txt" in cmd

    def test_copy_aborts_on_rsync_failure(self):
        with patch("subprocess.run", return_value=mock_run(1)):
            with pytest.raises(SystemExit):
                immutability.copy_persistent_files(
                    "/prev", "/curr", "/filter.txt",
                )


class TestResetSubvolume:
    def setup_method(self):
        immutability._mount_point = "/mnt"
        immutability._depth = 0

    def test_reset_full_flow(self):
        with patch("subprocess.run", return_value=mock_run()):
            with patch("os.path.isdir", return_value=True):
                with patch("os.path.isfile", return_value=True):
                    with patch("builtins.open", mock_open()):
                        immutability.reset_subvolume(
                            "@root", "@snapshots", "CLEAN",
                            "/filter.txt",
                        )

    def test_reset_initializes_missing_snapshots(self):
        dirs_exist = {
            "/mnt/@snapshots/@root/PENULTIMATE": False,
            "/mnt/@snapshots/@root/PREVIOUS": False,
        }
        with patch("subprocess.run", return_value=mock_run()):
            with patch("os.path.isdir",
                       side_effect=lambda p: dirs_exist.get(p, True)):
                with patch("os.path.isfile", return_value=True):
                    with patch("builtins.open", mock_open()):
                        immutability.reset_subvolume(
                            "@root", "@snapshots", "CLEAN",
                            "/filter.txt",
                        )

    def test_reset_with_crash_recovery(self):
        with patch("subprocess.run", return_value=mock_run()):
            with patch("os.path.isdir", return_value=True):
                with patch("os.path.isfile",
                           side_effect=lambda p: ".boot-ready" not in p):
                    with patch("builtins.open", mock_open()):
                        immutability.reset_subvolume(
                            "@root", "@snapshots", "CLEAN",
                            "/filter.txt",
                        )


class TestMain:
    def setup_method(self):
        immutability._depth = 0

    def test_missing_args(self, monkeypatch):
        monkeypatch.setattr("sys.argv", ["immutability"])
        with pytest.raises(SystemExit):
            immutability.main()

    def test_full_run(self, monkeypatch):
        monkeypatch.setattr("sys.argv", [
            "immutability", "/dev/sda", "@snapshots", "CLEAN",
            "/filter.txt", "@root=/", "@home=/home",
        ])
        with patch("core.immutability.mount_subvolumes"):
            with patch("core.immutability.reset_subvolume") as reset:
                with patch("core.immutability.unmount_subvolumes"):
                    immutability.main()
                    assert reset.call_count == 2
                    reset.assert_any_call(
                        "@root", "@snapshots", "CLEAN", "/filter.txt",
                    )
                    reset.assert_any_call(
                        "@home", "@snapshots", "CLEAN", "/filter.txt",
                    )

    def test_single_subvolume(self, monkeypatch):
        monkeypatch.setattr("sys.argv", [
            "immutability", "/dev/sda", "@snapshots", "CLEAN",
            "/filter.txt", "@root=/",
        ])
        with patch("core.immutability.mount_subvolumes") as mount:
            with patch("core.immutability.reset_subvolume"):
                with patch("core.immutability.unmount_subvolumes"):
                    immutability.main()
                    mount.assert_called_once_with("/dev/sda")
