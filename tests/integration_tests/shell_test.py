import os
from unittest.mock import patch

from lib.shell import Shell

class TestShellRealFilesystem:
    def _make_shell(self):
        sh = Shell()
        sh.exists = os.path.exists
        return sh
    def test_file_read_returns_content(self, tmp_path):
        path = str(tmp_path / "test.txt")
        (tmp_path / "test.txt").write_text("hello world")
        sh = self._make_shell()
        assert sh.file_read(path) == "hello world"
    def test_file_read_nonexistent_returns_empty(self, tmp_path):
        sh = self._make_shell()
        assert sh.file_read(str(tmp_path / "missing.txt")) == ""
    def test_file_write_creates_file(self, tmp_path):
        path = str(tmp_path / "out.txt")
        sh = self._make_shell()
        with patch.object(sh, "rm"):
            with patch.object(sh, "mkdir"):
                with patch.object(sh, "dirname", return_value=str(tmp_path)):
                    sh.file_write(path, "content here")
        assert (tmp_path / "out.txt").read_text() == "content here"
    def test_file_write_sensitive_not_in_file(self, tmp_path):
        path = str(tmp_path / "secret.txt")
        sh = self._make_shell()
        with patch.object(sh, "rm"):
            with patch.object(sh, "mkdir"):
                with patch.object(sh, "dirname", return_value=str(tmp_path)):
                    sh.file_write(path, "my_password", sensitive="my_password")
        assert (tmp_path / "secret.txt").read_text() == "my_password"
    def test_json_roundtrip(self, tmp_path):
        path = str(tmp_path / "data.json")
        sh = self._make_shell()
        (tmp_path / "data.json").write_text("{}")
        with patch.object(sh, "rm"):
            with patch.object(sh, "mkdir"):
                with patch.object(sh, "dirname", return_value=str(tmp_path)):
                    sh.json_write(path, "key", "value")
        assert sh.json_read(path) == {"key": "value"}
    def test_json_write_preserves_existing_keys(self, tmp_path):
        path = str(tmp_path / "data.json")
        (tmp_path / "data.json").write_text('{"existing": 1}')
        sh = self._make_shell()
        with patch.object(sh, "rm"):
            with patch.object(sh, "mkdir"):
                with patch.object(sh, "dirname", return_value=str(tmp_path)):
                    sh.json_write(path, "new", 2)
        result = sh.json_read(path)
        assert result == {"existing": 1, "new": 2}
    def test_json_read_invalid_json_returns_empty_dict(self, tmp_path):
        path = str(tmp_path / "bad.json")
        (tmp_path / "bad.json").write_text("not json at all")
        sh = self._make_shell()
        assert sh.json_read(path) == {}
    def test_json_read_empty_file_returns_empty_dict(self, tmp_path):
        path = str(tmp_path / "empty.json")
        (tmp_path / "empty.json").write_text("")
        sh = self._make_shell()
        assert sh.json_read(path) == {}
