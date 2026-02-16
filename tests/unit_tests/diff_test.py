import contextlib
from unittest.mock import patch

from bin.diff import (
    get_tmp_snapshot_path, get_paths_to_keep,
    sha256sum, diff_file, diff_files,
    get_mount_cache, _mount_cache,
)

class TestDiffHelpers:
    def test_get_tmp_snapshot_path(self, mock_config_eval):  # noqa: ARG002
        assert get_tmp_snapshot_path("@root") == "/.snapshots/@root/tmp"
    def test_get_tmp_snapshot_path_home(self, mock_config_eval):  # noqa: ARG002
        assert get_tmp_snapshot_path("@home") == "/.snapshots/@home/tmp"
    def test_get_paths_to_keep(self, mock_shell,
                               mock_config_eval):  # noqa: ARG002
        with patch("bin.diff.sh", mock_shell):
            result = get_paths_to_keep()
            assert "/etc/nixos" in result
            assert "/var/lib/nixos" in result
    def test_sha256sum_nonexistent(self, mock_shell):
        with patch("bin.diff.sh", mock_shell):
            with patch.object(mock_shell, "exists", return_value=False):
                assert sha256sum("/nonexistent") == "N/A"
    def test_sha256sum_directory(self, mock_shell):
        with patch("bin.diff.sh", mock_shell):
            with patch.object(mock_shell, "exists", return_value=True):
                with patch.object(mock_shell, "is_dir", return_value=True):
                    assert sha256sum("/some/dir") == "N/A"
    def test_sha256sum_symlink(self, mock_shell):
        with contextlib.ExitStack() as stack:
            stack.enter_context(patch("bin.diff.sh", mock_shell))
            stack.enter_context(patch.object(mock_shell, "exists",
                                             return_value=True))
            stack.enter_context(patch.object(mock_shell, "is_dir",
                                             return_value=False))
            stack.enter_context(patch.object(mock_shell, "is_symlink",
                                             return_value=True))
            assert sha256sum("/some/link") == "N/A"
    def test_sha256sum_file(self, mock_shell):
        with contextlib.ExitStack() as stack:
            stack.enter_context(patch("bin.diff.sh", mock_shell))
            stack.enter_context(patch.object(mock_shell, "exists",
                                             return_value=True))
            stack.enter_context(patch.object(mock_shell, "is_dir",
                                             return_value=False))
            stack.enter_context(patch.object(mock_shell, "is_symlink",
                                             return_value=False))
            stack.enter_context(patch.object(mock_shell, "file_read",
                                             return_value="content"))
            result = sha256sum("/some/file")
            assert result != "N/A"
            assert len(result) == 64
    def test_sha256sum_produces_consistent_hash(self, mock_shell):
        with contextlib.ExitStack() as stack:
            stack.enter_context(patch("bin.diff.sh", mock_shell))
            stack.enter_context(patch.object(mock_shell, "exists",
                                             return_value=True))
            stack.enter_context(patch.object(mock_shell, "is_dir",
                                             return_value=False))
            stack.enter_context(patch.object(mock_shell, "is_symlink",
                                             return_value=False))
            stack.enter_context(patch.object(mock_shell, "file_read",
                                             return_value="same content"))
            hash1 = sha256sum("/file1")
            hash2 = sha256sum("/file2")
            assert hash1 == hash2

class TestDiffFile:
    def test_nonexistent(self, mock_shell):
        with patch("bin.diff.sh", mock_shell):
            with patch.object(mock_shell, "exists", return_value=False):
                assert "DOES NOT EXIST" in diff_file("/nonexistent")
    def test_directory(self, mock_shell):
        with patch("bin.diff.sh", mock_shell):
            with patch.object(mock_shell, "exists", return_value=True):
                with patch.object(mock_shell, "is_dir", return_value=True):
                    assert "DIRECTORY" in diff_file("/some/dir")
    def test_symlink(self, mock_shell):
        with contextlib.ExitStack() as stack:
            stack.enter_context(patch("bin.diff.sh", mock_shell))
            stack.enter_context(patch.object(mock_shell, "exists",
                                             return_value=True))
            stack.enter_context(patch.object(mock_shell, "is_dir",
                                             return_value=False))
            stack.enter_context(patch.object(mock_shell, "is_symlink",
                                             return_value=True))
            assert "LINK" in diff_file("/some/link")
    def test_new_file_no_mount_cache(self, mock_shell):
        _mount_cache.clear()
        with contextlib.ExitStack() as stack:
            stack.enter_context(patch("bin.diff.sh", mock_shell))
            stack.enter_context(patch.object(mock_shell, "exists",
                                             return_value=True))
            stack.enter_context(patch.object(mock_shell, "is_dir",
                                             return_value=False))
            stack.enter_context(patch.object(mock_shell, "is_symlink",
                                             return_value=False))
            stack.enter_context(patch.object(mock_shell, "file_read",
                                             return_value="new content"))
            stack.enter_context(patch("bin.diff.get_mount_cache",
                                      return_value={}))
            result = diff_file("/new/file")
            assert result == "new content"
    def test_existing_binary_file(self, mock_shell):
        _mount_cache.clear()
        with contextlib.ExitStack() as stack:
            stack.enter_context(patch("bin.diff.sh", mock_shell))
            stack.enter_context(patch.object(mock_shell, "exists",
                                             return_value=True))
            stack.enter_context(patch.object(mock_shell, "is_dir",
                                             return_value=False))
            stack.enter_context(patch.object(mock_shell, "is_symlink",
                                             return_value=False))
            stack.enter_context(patch.object(mock_shell, "file_read",
                                             side_effect=UnicodeDecodeError(
                                                 "utf-8", b"", 0, 1, "err")))
            stack.enter_context(patch("bin.diff.get_mount_cache",
                                      return_value={"/": "/.snapshots/@root/CLEAN"}))
            result = diff_file("/some/binary")
            assert "BINARY" in result

class TestDiffFiles:
    def test_progress_output(self, mock_shell, capsys):
        with patch("bin.diff.sh", mock_shell):
            with patch("bin.diff.diff_file", return_value="content"):
                result = diff_files(["/file1", "/file2"])
                assert len(result) == 2
                output = capsys.readouterr().out
                assert "Progress" in output
    def test_empty_list(self, mock_shell):
        with patch("bin.diff.sh", mock_shell):
            result = diff_files([])
            assert result == {}

class TestGetMountCache:
    def test_populates_on_first_call(self, mock_shell,
                                     mock_config_eval):  # noqa: ARG002
        _mount_cache.clear()
        with patch("bin.diff.sh", mock_shell):
            result = get_mount_cache()
            assert "/" in result
            assert "/home" in result
    def test_returns_cached_after(self, mock_shell,
                                  mock_config_eval):  # noqa: ARG002
        _mount_cache.clear()
        with patch("bin.diff.sh", mock_shell):
            result1 = get_mount_cache()
            result2 = get_mount_cache()
            assert result1 is result2
