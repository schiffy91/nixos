from unittest.mock import patch

from cli.diff import (
    get_tmp_snapshot_path, get_paths_to_keep,
    sha256sum, diff_file, _mount_cache,
)


class TestDiffHelpers:
    def test_get_tmp_snapshot_path(self, mock_config_eval):  # noqa: ARG002
        assert get_tmp_snapshot_path("@root") == "/.snapshots/@root/tmp"

    def test_get_paths_to_keep(self, mock_shell,
                               mock_config_eval):  # noqa: ARG002
        with patch("cli.diff.sh", mock_shell):
            result = get_paths_to_keep()
            assert "/etc/nixos" in result
            assert "/var/lib/nixos" in result

    def test_sha256sum_nonexistent(self, mock_shell):
        with patch("cli.diff.sh", mock_shell):
            with patch.object(mock_shell, "exists", return_value=False):
                assert sha256sum("/nonexistent") == "N/A"

    def test_sha256sum_directory(self, mock_shell):
        with patch("cli.diff.sh", mock_shell):
            with patch.object(mock_shell, "exists", return_value=True):
                with patch.object(mock_shell, "is_dir", return_value=True):
                    assert sha256sum("/some/dir") == "N/A"

    def test_sha256sum_file(self, mock_shell):
        with patch("cli.diff.sh", mock_shell):
            with patch.object(mock_shell, "exists", return_value=True):
                with patch.object(mock_shell, "is_dir", return_value=False):
                    with patch.object(mock_shell, "is_symlink",
                                      return_value=False):
                        with patch.object(mock_shell, "file_read",
                                          return_value="content"):
                            result = sha256sum("/some/file")
                            assert result != "N/A"
                            assert len(result) == 64


class TestDiffFile:
    def test_nonexistent(self, mock_shell):
        with patch("cli.diff.sh", mock_shell):
            with patch.object(mock_shell, "exists", return_value=False):
                assert "DOES NOT EXIST" in diff_file("/nonexistent")

    def test_directory(self, mock_shell):
        with patch("cli.diff.sh", mock_shell):
            with patch.object(mock_shell, "exists", return_value=True):
                with patch.object(mock_shell, "is_dir", return_value=True):
                    assert "DIRECTORY" in diff_file("/some/dir")

    def test_symlink(self, mock_shell):
        with patch("cli.diff.sh", mock_shell):
            with patch.object(mock_shell, "exists", return_value=True):
                with patch.object(mock_shell, "is_dir", return_value=False):
                    with patch.object(mock_shell, "is_symlink",
                                      return_value=True):
                        assert "LINK" in diff_file("/some/link")

    def test_new_file(self, mock_shell):
        _mount_cache.clear()
        with patch("cli.diff.sh", mock_shell):
            with patch.object(mock_shell, "exists", return_value=True):
                with patch.object(mock_shell, "is_dir", return_value=False):
                    with patch.object(mock_shell, "is_symlink",
                                      return_value=False):
                        with patch.object(mock_shell, "file_read",
                                          return_value="new content"):
                            with patch("cli.diff.get_mount_cache",
                                       return_value={}):
                                result = diff_file("/new/file")
                                assert result == "new content"
