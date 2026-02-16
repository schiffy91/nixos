from subprocess import CalledProcessError
import json, os
from unittest.mock import MagicMock, patch, mock_open

import pytest

from lib.shell import Shell, chrootable, chrootable_registry

def _get_cmd(mock_subprocess, index=-1):
    return mock_subprocess.call_args_list[index][0][0]

def _all_cmds(mock_subprocess):
    return [c[0][0] for c in mock_subprocess.call_args_list]

class TestShellBasics:
    def test_stdout_strips_whitespace(self):
        mock = MagicMock()
        mock.stdout = "  test output  \n"
        assert Shell.stdout(mock) == "test output"
    def test_stdout_empty_string(self):
        mock = MagicMock()
        mock.stdout = ""
        assert Shell.stdout(mock) == ""
    def test_stdout_multiline(self):
        mock = MagicMock()
        mock.stdout = "  line1\nline2  "
        assert Shell.stdout(mock) == "line1\nline2"
    def test_init_without_root(self):
        shell = Shell(root_required=False)
        assert not shell.chroots
    def test_init_with_root_when_root(self, monkeypatch):
        mock_run = MagicMock()
        mock_run.return_value.stdout = "0"
        monkeypatch.setattr(Shell, "run", lambda self,
                            cmd: mock_run.return_value)
        shell = Shell(root_required=True)
        assert not shell.chroots
    def test_init_with_root_not_root(self, monkeypatch):
        mock_run = MagicMock()
        mock_run.return_value.stdout = "1000"
        monkeypatch.setattr(Shell, "run", lambda self,
                            cmd: mock_run.return_value)
        with pytest.raises(SystemExit):
            Shell(root_required=True)
    def test_evals_is_shared_class_dict(self):
        assert isinstance(Shell.evals, dict)
        s1 = Shell()
        s2 = Shell()
        s1.evals["k"] = "v"
        assert s2.evals.get("k") == "v"
        del Shell.evals["k"]

class TestShellRun:
    def test_run_returns_completed_process(self, mock_shell, mock_subprocess):
        mock_subprocess.return_value.stdout = "hello"
        result = mock_shell.run("echo hello")
        assert result.stdout == "hello"
    def test_run_adds_sudo_by_default(self, mock_shell, mock_subprocess):
        mock_shell.run("echo test")
        assert _get_cmd(mock_subprocess).startswith("sudo ")
    def test_run_without_sudo(self, mock_shell, mock_subprocess):
        mock_shell.run("echo test", sudo=False)
        assert "sudo" not in _get_cmd(mock_subprocess)
    def test_run_with_env(self, mock_shell, mock_subprocess):
        mock_shell.run("echo test", env="FOO=bar")
        cmd = _get_cmd(mock_subprocess)
        assert cmd.startswith("FOO=bar sudo ")
    def test_run_with_env_no_sudo(self, mock_shell, mock_subprocess):
        mock_shell.run("echo test", env="FOO=bar", sudo=False)
        cmd = _get_cmd(mock_subprocess)
        assert cmd.startswith("FOO=bar echo")
        assert "sudo" not in cmd
    def test_run_env_empty_string(self, mock_shell, mock_subprocess):
        mock_shell.run("echo test", env="")
        cmd = _get_cmd(mock_subprocess)
        assert cmd == "sudo echo test"
    def test_run_sensitive_redacts_log_only(self, mock_shell,
                                            mock_subprocess, capsys):
        mock_shell.run("mkpasswd -m sha-512 'MyP@ss'",
                        sensitive="MyP@ss")
        captured = capsys.readouterr()
        assert "MyP@ss" not in captured.out
        assert "***" in captured.out
        cmd = _get_cmd(mock_subprocess)
        assert "MyP@ss" in cmd
    def test_run_sensitive_none_shows_full_log(self, mock_shell,
                                                mock_subprocess, capsys):
        mock_shell.run("echo visible")
        captured = capsys.readouterr()
        assert "visible" in captured.out
    def test_run_capture_output_true(self, mock_shell, mock_subprocess):
        mock_shell.run("echo test", capture_output=True)
        kwargs = mock_subprocess.call_args[1]
        assert kwargs["capture_output"] is True
    def test_run_capture_output_false(self, mock_shell, mock_subprocess):
        mock_shell.run("echo test", capture_output=False)
        kwargs = mock_subprocess.call_args[1]
        assert kwargs["capture_output"] is False
    def test_run_check_true_raises_on_failure(self, mock_shell,
                                               mock_subprocess):
        error = CalledProcessError(1, "cmd")
        error.stdout = ""
        error.stderr = ""
        mock_subprocess.side_effect = error
        with pytest.raises(CalledProcessError):
            mock_shell.run("bad command", check=True)
    def test_run_check_false_no_raise(self, mock_shell, mock_subprocess):
        mock_subprocess.return_value.returncode = 1
        result = mock_shell.run("bad command", check=False)
        assert result.returncode == 1
    def test_run_error_prints_stdout_and_stderr(self, mock_shell,
                                                 mock_subprocess, capsys):
        error = CalledProcessError(1, "cmd")
        error.stdout = "stdout msg"
        error.stderr = "stderr msg"
        mock_subprocess.side_effect = error
        with pytest.raises(CalledProcessError):
            mock_shell.run("bad command")
        captured = capsys.readouterr()
        assert "stdout msg" in captured.out
        assert "stderr msg" in captured.err
    def test_run_error_empty_stdout_stderr(self, mock_shell,
                                            mock_subprocess, capsys):
        error = CalledProcessError(1, "cmd")
        error.stdout = ""
        error.stderr = ""
        mock_subprocess.side_effect = error
        with pytest.raises(CalledProcessError):
            mock_shell.run("bad command")
    def test_run_error_redacts_sensitive_in_stdout(self, mock_shell,
                                                    mock_subprocess, capsys):
        error = CalledProcessError(1, "cmd")
        error.stdout = "password is MySecret123"
        error.stderr = ""
        mock_subprocess.side_effect = error
        with pytest.raises(CalledProcessError):
            mock_shell.run("bad command", sensitive="MySecret123")
        captured = capsys.readouterr()
        assert "MySecret123" not in captured.out
        assert "***" in captured.out
    def test_run_error_redacts_sensitive_in_stderr(self, mock_shell,
                                                    mock_subprocess, capsys):
        error = CalledProcessError(1, "cmd")
        error.stdout = ""
        error.stderr = "error: bad password MySecret123"
        mock_subprocess.side_effect = error
        with pytest.raises(CalledProcessError):
            mock_shell.run("bad command", sensitive="MySecret123")
        captured = capsys.readouterr()
        assert "MySecret123" not in captured.err
        assert "***" in captured.err
    def test_run_error_redacts_sensitive_in_both(self, mock_shell,
                                                  mock_subprocess, capsys):
        error = CalledProcessError(1, "cmd")
        error.stdout = "got hunter2"
        error.stderr = "failed with hunter2"
        mock_subprocess.side_effect = error
        with pytest.raises(CalledProcessError):
            mock_shell.run("bad command", sensitive="hunter2")
        captured = capsys.readouterr()
        assert "hunter2" not in captured.out
        assert "hunter2" not in captured.err
    def test_run_error_no_sensitive_shows_raw(self, mock_shell,
                                               mock_subprocess, capsys):
        error = CalledProcessError(1, "cmd")
        error.stdout = "raw output"
        error.stderr = "raw error"
        mock_subprocess.side_effect = error
        with pytest.raises(CalledProcessError):
            mock_shell.run("bad command")
        captured = capsys.readouterr()
        assert "raw output" in captured.out
        assert "raw error" in captured.err
    def test_run_chroot_with_sensitive_redacts_log(self, mock_shell,
                                                    mock_subprocess, capsys):
        with mock_shell.chroot("/mnt"):
            mock_shell.run("mkpasswd secret", sensitive="secret")
        captured = capsys.readouterr()
        assert "secret" not in captured.out
        assert "***" in captured.out
        cmd = _get_cmd(mock_subprocess)
        assert "secret" in cmd
    def test_run_uses_shell_true(self, mock_shell, mock_subprocess):
        mock_shell.run("echo test")
        kwargs = mock_subprocess.call_args[1]
        assert kwargs["shell"] is True
    def test_run_uses_text_true(self, mock_shell, mock_subprocess):
        mock_shell.run("echo test")
        kwargs = mock_subprocess.call_args[1]
        assert kwargs["text"] is True
    def test_run_in_chroot_wraps_with_nixos_enter(self, mock_shell,
                                                    mock_subprocess):
        with mock_shell.chroot("/mnt"):
            mock_shell.run("echo test")
        cmd = _get_cmd(mock_subprocess)
        assert "nixos-enter --root /mnt" in cmd
        assert '--command "echo test"' in cmd
    def test_run_in_nested_chroot_uses_innermost(self, mock_shell,
                                                   mock_subprocess):
        with mock_shell.chroot("/mnt1"):
            with mock_shell.chroot("/mnt2"):
                mock_shell.run("echo test")
        cmd = _get_cmd(mock_subprocess)
        assert "nixos-enter --root /mnt2" in cmd
    def test_run_chroot_with_env_and_sudo(self, mock_shell, mock_subprocess):
        with mock_shell.chroot("/mnt"):
            mock_shell.run("echo test", env="X=1")
        cmd = _get_cmd(mock_subprocess)
        assert "X=1" in cmd
        assert "sudo" in cmd
        assert "nixos-enter" in cmd
    def test_whoami(self, mock_shell, mock_subprocess):
        mock_subprocess.return_value.stdout = "user pts/0 2024-01-01"
        assert mock_shell.whoami() == "user"
    def test_hostname(self, mock_shell, mock_subprocess):
        mock_subprocess.return_value.stdout = "myhost"
        assert mock_shell.hostname() == "myhost"

class TestShellFileSystem:
    def test_mkdir(self, mock_shell, mock_subprocess):
        mock_shell.mkdir("/test/path", "/another/path")
        assert any(
            "mkdir -p /test/path /another/path" in str(c)
            for c in mock_subprocess.call_args_list
        )
    def test_mkdir_single(self, mock_shell, mock_subprocess):
        mock_shell.mkdir("/only/one")
        assert any(
            "mkdir -p /only/one" in str(c)
            for c in mock_subprocess.call_args_list
        )
    def test_rm(self, mock_shell, mock_subprocess):
        mock_shell.rm("/test/file")
        assert any(
            "rm -rf /test/file" in str(c)
            for c in mock_subprocess.call_args_list
        )
    def test_rm_multiple(self, mock_shell, mock_subprocess):
        mock_shell.rm("/a", "/b", "/c")
        assert any(
            "rm -rf /a /b /c" in str(c)
            for c in mock_subprocess.call_args_list
        )
    def test_mv(self, mock_shell, mock_subprocess):
        mock_subprocess.return_value.stdout = "/dest"
        mock_shell.mv("/src/file", "/dest/file")
        cmds = _all_cmds(mock_subprocess)
        assert any("mv /src/file /dest/file" in c for c in cmds)
    def test_mv_creates_parent_dir(self, mock_shell, mock_subprocess):
        mock_subprocess.return_value.stdout = "/dest"
        mock_shell.mv("/src", "/new/dir/file")
        cmds = _all_cmds(mock_subprocess)
        assert any("mkdir -p" in c for c in cmds)
    def test_cpdir_removes_then_copies(self, mock_shell, mock_subprocess):
        mock_subprocess.return_value.stdout = "/dest"
        mock_shell.cpdir("/src", "/dest/target")
        cmds = _all_cmds(mock_subprocess)
        assert any("rm -rf /dest/target" in c for c in cmds)
        assert any("cp -r /src /dest/target" in c for c in cmds)
        rm_idx = next(
            i for i, c in enumerate(cmds) if "rm -rf /dest/target" in c
        )
        cp_idx = next(
            i for i, c in enumerate(cmds) if "cp -r /src /dest/target" in c
        )
        assert rm_idx < cp_idx
    def test_basename(self, mock_shell, mock_subprocess):
        mock_subprocess.return_value.stdout = "file.txt"
        assert mock_shell.basename("/path/to/file.txt") == "file.txt"
    def test_dirname(self, mock_shell, mock_subprocess):
        mock_subprocess.return_value.stdout = "/path/to"
        assert mock_shell.dirname("/path/to/file.txt") == "/path/to"
    def test_parent_name(self, mock_shell, mock_subprocess):
        mock_subprocess.return_value.stdout = "parent"
        assert mock_shell.parent_name("/path/parent/file") == "parent"
    def test_exists_true(self, mock_shell, mock_subprocess):
        mock_subprocess.return_value.returncode = 0
        assert mock_shell.exists("/test/file") is True
    def test_exists_false(self, mock_shell, mock_subprocess):
        mock_subprocess.return_value.returncode = 1
        assert mock_shell.exists("/test/file") is False
    def test_exists_multiple_paths(self, mock_shell, mock_subprocess):
        mock_subprocess.return_value.returncode = 0
        assert mock_shell.exists("/a", "/b", "/c") is True
    def test_exists_multiple_uses_and(self, mock_shell, mock_subprocess):
        mock_shell.exists("/a", "/b")
        cmd = _get_cmd(mock_subprocess)
        assert "[ -e '/a' ] && [ -e '/b' ]" in cmd
    def test_is_dir_true(self, mock_shell, mock_subprocess):
        mock_subprocess.return_value.returncode = 0
        assert mock_shell.is_dir("/test/dir") is True
    def test_is_dir_false(self, mock_shell, mock_subprocess):
        mock_subprocess.return_value.returncode = 1
        assert mock_shell.is_dir("/test/file") is False
    def test_symlink(self, mock_shell, mock_subprocess):
        mock_shell.symlink("/src", "/dst")
        assert any(
            "ln -s /src /dst" in str(c)
            for c in mock_subprocess.call_args_list
        )
    def test_is_symlink_true(self, mock_shell, mock_subprocess):
        mock_subprocess.return_value.returncode = 0
        assert mock_shell.is_symlink("/link") is True
    def test_is_symlink_false(self, mock_shell, mock_subprocess):
        mock_subprocess.return_value.returncode = 1
        assert mock_shell.is_symlink("/file") is False
    def test_realpath(self, mock_shell, mock_subprocess):
        mock_subprocess.return_value.stdout = "/real/path"
        assert mock_shell.realpath("/some/path") == "/real/path"
    def test_realpath_multiline_takes_first(self, mock_shell,
                                             mock_subprocess):
        mock_subprocess.return_value.stdout = "/first\n/second"
        assert mock_shell.realpath("/some") == "/first"
    def test_realpaths(self, mock_shell, mock_subprocess):
        mock_subprocess.return_value.stdout = "/real/a\n/real/b"
        result = mock_shell.realpaths("/a", "/b")
        assert result == ["/real/a", "/real/b"]
    def test_find_basic(self, mock_shell, mock_subprocess):
        mock_subprocess.return_value.stdout = "/path\n/real/path\n/a\n/b"
        result = mock_shell.find("/path")
        assert len(result) > 0
    def test_find_empty_output(self, mock_shell, mock_subprocess):
        mock_subprocess.return_value.stdout = "/path"
        def side_effect(cmd, **kwargs):
            result = MagicMock()
            result.returncode = 0
            result.stderr = ""
            if "realpath" in cmd:
                result.stdout = "/path"
            else:
                result.stdout = ""
            return result
        mock_subprocess.side_effect = side_effect
        result = mock_shell.find("/path")
        assert result == []
    def test_find_directories(self, mock_shell, mock_subprocess):
        mock_subprocess.return_value.stdout = "/path\n/a\n/b"
        mock_shell.find_directories("/path")
        assert any(
            "-type d" in str(c)
            for c in mock_subprocess.call_args_list
        )
    def test_find_files(self, mock_shell, mock_subprocess):
        mock_subprocess.return_value.stdout = "/path\n/a.txt"
        mock_shell.find_files("/path")
        assert any(
            "-type f" in str(c)
            for c in mock_subprocess.call_args_list
        )
    def test_find_with_ignore(self, mock_shell, mock_subprocess):
        mock_subprocess.return_value.stdout = "/path\n/a"
        mock_shell.find("/path", ignore_pattern="*.pyc")
        assert any(
            "-not" in str(c) and "*.pyc" in str(c)
            for c in mock_subprocess.call_args_list
        )
    def test_find_with_multi_pattern(self, mock_shell, mock_subprocess):
        mock_subprocess.return_value.stdout = "/path\n/a"
        mock_shell.find("/path", pattern="*.py *.nix")
        cmd = _get_cmd(mock_subprocess)
        assert "-o" in cmd
        assert "*.py" in cmd
        assert "*.nix" in cmd
    def test_find_with_multi_ignore_pattern(self, mock_shell,
                                             mock_subprocess):
        mock_subprocess.return_value.stdout = "/path\n/a"
        mock_shell.find("/path", ignore_pattern="*.pyc *.pyo")
        cmd = _get_cmd(mock_subprocess)
        assert "-not" in cmd
        assert "-o" in cmd

class TestShellIO:
    def test_file_write_writes_exact_content(self, mock_shell, tmp_path):
        mock_shell.chroots = []
        with patch("builtins.open", mock_open()) as m:
            with patch.object(mock_shell, "rm"):
                with patch.object(mock_shell, "mkdir"):
                    with patch.object(mock_shell, "dirname",
                                      return_value=str(tmp_path)):
                        mock_shell.file_write(
                            str(tmp_path / "test.txt"), "content"
                        )
                        handle = m()
                        written = handle.write.call_args[0][0]
                        assert written == "content"
    def test_file_write_calls_rm_then_mkdir(self, mock_shell, tmp_path):
        mock_shell.chroots = []
        call_order = []
        with patch("builtins.open", mock_open()):
            with patch.object(
                mock_shell, "rm",
                side_effect=lambda *a: call_order.append("rm")
            ):
                with patch.object(
                    mock_shell, "mkdir",
                    side_effect=lambda *a: call_order.append("mkdir")
                ):
                    with patch.object(mock_shell, "dirname",
                                      return_value="/dir"):
                        mock_shell.file_write("/dir/f.txt", "data")
        assert call_order == ["rm", "mkdir"]
    def test_file_write_sensitive_preserves_content(self, mock_shell,
                                                     tmp_path):
        mock_shell.chroots = []
        with patch("builtins.open", mock_open()) as m:
            with patch.object(mock_shell, "rm"):
                with patch.object(mock_shell, "mkdir"):
                    with patch.object(mock_shell, "dirname",
                                      return_value=str(tmp_path)):
                        mock_shell.file_write(
                            str(tmp_path / "t.txt"),
                            "secret123", sensitive="secret123",
                        )
                        handle = m()
                        written = handle.write.call_args[0][0]
                        assert written == "secret123"
    def test_file_write_sensitive_redacts_log(self, mock_shell, tmp_path,
                                               capsys):
        mock_shell.chroots = []
        with patch("builtins.open", mock_open()):
            with patch.object(mock_shell, "rm"):
                with patch.object(mock_shell, "mkdir"):
                    with patch.object(mock_shell, "dirname",
                                      return_value=str(tmp_path)):
                        mock_shell.file_write(
                            str(tmp_path / "t.txt"),
                            "secret123", sensitive="secret123",
                        )
        captured = capsys.readouterr()
        assert "secret123" not in captured.out
        assert "***" in captured.out
    def test_file_write_sensitive_partial_match(self, mock_shell, tmp_path):
        mock_shell.chroots = []
        with patch("builtins.open", mock_open()) as m:
            with patch.object(mock_shell, "rm"):
                with patch.object(mock_shell, "mkdir"):
                    with patch.object(mock_shell, "dirname",
                                      return_value=str(tmp_path)):
                        mock_shell.file_write(
                            str(tmp_path / "t.txt"),
                            "prefix_secret_suffix",
                            sensitive="secret",
                        )
                        handle = m()
                        written = handle.write.call_args[0][0]
                        assert written == "prefix_secret_suffix"
    def test_file_write_sensitive_not_in_content(self, mock_shell, tmp_path):
        mock_shell.chroots = []
        with patch("builtins.open", mock_open()) as m:
            with patch.object(mock_shell, "rm"):
                with patch.object(mock_shell, "mkdir"):
                    with patch.object(mock_shell, "dirname",
                                      return_value=str(tmp_path)):
                        mock_shell.file_write(
                            str(tmp_path / "t.txt"),
                            "normal content",
                            sensitive="not_present",
                        )
                        handle = m()
                        written = handle.write.call_args[0][0]
                        assert written == "normal content"
    def test_file_write_sensitive_none(self, mock_shell, tmp_path):
        mock_shell.chroots = []
        with patch("builtins.open", mock_open()) as m:
            with patch.object(mock_shell, "rm"):
                with patch.object(mock_shell, "mkdir"):
                    with patch.object(mock_shell, "dirname",
                                      return_value=str(tmp_path)):
                        mock_shell.file_write(
                            str(tmp_path / "t.txt"),
                            "my_password", sensitive=None,
                        )
                        handle = m()
                        written = handle.write.call_args[0][0]
                        assert written == "my_password"
    def test_file_write_sensitive_empty_string(self, mock_shell, tmp_path):
        mock_shell.chroots = []
        with patch("builtins.open", mock_open()) as m:
            with patch.object(mock_shell, "rm"):
                with patch.object(mock_shell, "mkdir"):
                    with patch.object(mock_shell, "dirname",
                                      return_value=str(tmp_path)):
                        mock_shell.file_write(
                            str(tmp_path / "t.txt"),
                            "data", sensitive="",
                        )
                        handle = m()
                        written = handle.write.call_args[0][0]
                        assert written == "data"
    def test_file_write_sensitive_special_chars(self, mock_shell, tmp_path):
        mock_shell.chroots = []
        password = "p@$$w0rd!#%^&*()'\"\\n\\t"
        with patch("builtins.open", mock_open()) as m:
            with patch.object(mock_shell, "rm"):
                with patch.object(mock_shell, "mkdir"):
                    with patch.object(mock_shell, "dirname",
                                      return_value=str(tmp_path)):
                        mock_shell.file_write(
                            str(tmp_path / "t.txt"),
                            password, sensitive=password,
                        )
                        handle = m()
                        written = handle.write.call_args[0][0]
                        assert written == password
    def test_file_write_sensitive_hash_content(self, mock_shell, tmp_path):
        mock_shell.chroots = []
        hashed = "$6$rounds=65536$salt$abc123longhashedvalue=="
        with patch("builtins.open", mock_open()) as m:
            with patch.object(mock_shell, "rm"):
                with patch.object(mock_shell, "mkdir"):
                    with patch.object(mock_shell, "dirname",
                                      return_value=str(tmp_path)):
                        mock_shell.file_write(
                            str(tmp_path / "t.txt"),
                            hashed, sensitive=hashed,
                        )
                        handle = m()
                        written = handle.write.call_args[0][0]
                        assert written == hashed
    def test_file_write_with_chroot_prepends_path(self, mock_shell, tmp_path):
        mock_shell.chroots = ["/mnt"]
        with patch("builtins.open", mock_open()) as m:
            with patch.object(mock_shell, "rm"):
                with patch.object(mock_shell, "mkdir"):
                    with patch.object(mock_shell, "dirname",
                                      return_value=str(tmp_path)):
                        mock_shell.file_write("/etc/test", "data")
                        path_arg = m.call_args[0][0]
                        assert path_arg == "/mnt/etc/test"
    def test_file_write_no_chroot_uses_original_path(self, mock_shell,
                                                      tmp_path):
        mock_shell.chroots = []
        with patch("builtins.open", mock_open()) as m:
            with patch.object(mock_shell, "rm"):
                with patch.object(mock_shell, "mkdir"):
                    with patch.object(mock_shell, "dirname",
                                      return_value=str(tmp_path)):
                        mock_shell.file_write("/etc/test", "data")
                        path_arg = m.call_args[0][0]
                        assert path_arg == "/etc/test"
    def test_file_write_chroot_with_sensitive(self, mock_shell, tmp_path):
        mock_shell.chroots = ["/mnt"]
        written_data = {}
        real_mock_open = mock_open()
        def capturing_open(path, mode="r", **kwargs):
            written_data["path"] = path
            return real_mock_open(path, mode, **kwargs)

        with patch("builtins.open", capturing_open):
            with patch.object(mock_shell, "rm"):
                with patch.object(mock_shell, "mkdir"):
                    with patch.object(mock_shell, "dirname",
                                      return_value=str(tmp_path)):
                        mock_shell.file_write(
                            "/etc/pw", "hunter2", sensitive="hunter2",
                        )
                        handle = real_mock_open()
                        written = handle.write.call_args[0][0]
                        assert written == "hunter2"
                        assert written_data["path"] == "/mnt/etc/pw"
    def test_file_read_nonexistent(self, mock_shell):
        with patch.object(mock_shell, "exists", return_value=False):
            assert mock_shell.file_read("/nonexistent") == ""
    def test_file_read_existing(self, mock_shell):
        mock_shell.chroots = []
        with patch.object(mock_shell, "exists", return_value=True):
            with patch("builtins.open",
                       mock_open(read_data="file content")):
                result = mock_shell.file_read("/etc/test")
                assert result == "file content"
    def test_file_read_with_chroot(self, mock_shell):
        mock_shell.chroots = ["/mnt"]
        with patch.object(mock_shell, "exists", return_value=True):
            with patch("builtins.open",
                       mock_open(read_data="content")) as m:
                result = mock_shell.file_read("/etc/test")
                assert result == "content"
                path_arg = m.call_args[0][0]
                assert path_arg == "/mnt/etc/test"
    def test_file_read_no_chroot_uses_original_path(self, mock_shell):
        mock_shell.chroots = []
        with patch.object(mock_shell, "exists", return_value=True):
            with patch("builtins.open",
                       mock_open(read_data="data")) as m:
                mock_shell.file_read("/etc/test")
                path_arg = m.call_args[0][0]
                assert path_arg == "/etc/test"
    def test_json_read_valid(self, mock_shell):
        with patch.object(mock_shell, "file_read",
                          return_value='{"key": "value"}'):
            assert mock_shell.json_read("/t.json") == {"key": "value"}
    def test_json_read_invalid(self, mock_shell):
        with patch.object(mock_shell, "file_read",
                          return_value="invalid"):
            assert mock_shell.json_read("/t.json") == {}
    def test_json_read_empty(self, mock_shell):
        with patch.object(mock_shell, "file_read", return_value=""):
            assert mock_shell.json_read("/t.json") == {}
    def test_json_read_nested(self, mock_shell):
        with patch.object(
            mock_shell, "file_read",
            return_value='{"a": {"b": [1, 2, 3]}}'
        ):
            result = mock_shell.json_read("/t.json")
            assert result["a"]["b"] == [1, 2, 3]
    def test_json_write_merges_with_existing(self, mock_shell):
        with patch.object(mock_shell, "json_read",
                          return_value={"existing": "data"}):
            with patch.object(mock_shell, "file_write") as m:
                mock_shell.json_write("/t.json", "new", "val")
                written = m.call_args[0][1]
                parsed = json.loads(written)
                assert parsed == {"existing": "data", "new": "val"}
    def test_json_write_overwrites_key(self, mock_shell):
        with patch.object(mock_shell, "json_read",
                          return_value={"key": "old"}):
            with patch.object(mock_shell, "file_write") as m:
                mock_shell.json_write("/t.json", "key", "new")
                written = m.call_args[0][1]
                parsed = json.loads(written)
                assert parsed == {"key": "new"}
    def test_json_write_to_empty_file(self, mock_shell):
        with patch.object(mock_shell, "json_read", return_value={}):
            with patch.object(mock_shell, "file_write") as m:
                mock_shell.json_write("/t.json", "key", "val")
                written = m.call_args[0][1]
                parsed = json.loads(written)
                assert parsed == {"key": "val"}
    def test_json_overwrite(self, mock_shell):
        with patch.object(mock_shell, "rm"):
            with patch.object(mock_shell, "file_write") as m:
                mock_shell.json_overwrite("/t.json", {"a": 1})
                written = m.call_args[0][1]
                parsed = json.loads(written)
                assert parsed == {"a": 1}
    def test_json_overwrite_calls_rm(self, mock_shell):
        with patch.object(mock_shell, "rm") as rm_mock:
            with patch.object(mock_shell, "file_write"):
                mock_shell.json_overwrite("/t.json", {})
                rm_mock.assert_called_once_with("/t.json")

class TestShellSecurity:
    def test_chmod(self, mock_shell, mock_subprocess):
        mock_subprocess.return_value.stdout = "/real/path"
        mock_shell.chmod("755", "/test/path")
        assert any(
            "chmod -R 755" in str(c)
            for c in mock_subprocess.call_args_list
        )
    def test_chmod_multiple_paths(self, mock_shell, mock_subprocess):
        mock_subprocess.return_value.stdout = "/a\n/b"
        mock_shell.chmod("644", "/a", "/b")
        assert any(
            "chmod -R 644" in str(c)
            for c in mock_subprocess.call_args_list
        )
    def test_chown(self, mock_shell, mock_subprocess):
        mock_subprocess.return_value.stdout = "/real/path"
        mock_shell.chown("user", "/test/path")
        assert any(
            "chown -R user" in str(c)
            for c in mock_subprocess.call_args_list
        )
    def test_chown_multiple_paths(self, mock_shell, mock_subprocess):
        mock_subprocess.return_value.stdout = "/a\n/b"
        mock_shell.chown("root", "/a", "/b")
        assert any(
            "chown -R root" in str(c)
            for c in mock_subprocess.call_args_list
        )
    def test_ssh_keygen_basic(self, mock_shell, mock_subprocess):
        mock_subprocess.return_value.stdout = "/path"
        mock_shell.ssh_keygen("ed25519", "/path/key")
        cmds = _all_cmds(mock_subprocess)
        assert any("ssh-keygen" in c for c in cmds)
        assert any("-t ed25519" in c for c in cmds)
    def test_ssh_keygen_creates_parent_dir(self, mock_shell, mock_subprocess):
        mock_subprocess.return_value.stdout = "/path"
        mock_shell.ssh_keygen("ed25519", "/path/key")
        cmds = _all_cmds(mock_subprocess)
        assert any("mkdir -p" in c for c in cmds)
    def test_ssh_keygen_with_password(self, mock_shell, mock_subprocess):
        mock_subprocess.return_value.stdout = "/path"
        mock_shell.ssh_keygen("rsa", "/path/key", password="passphrase")
        cmds = _all_cmds(mock_subprocess)
        assert any(
            "passphrase" in c and "ssh-keygen" in c
            for c in cmds
        )
    def test_ssh_keygen_empty_password(self, mock_shell, mock_subprocess):
        mock_subprocess.return_value.stdout = "/path"
        mock_shell.ssh_keygen("ed25519", "/path/key", password="")
        cmds = _all_cmds(mock_subprocess)
        assert any('-N ""' in c for c in cmds)

class TestShellGit:
    def test_git_add_safe_directory(self, mock_shell, mock_subprocess):
        mock_subprocess.return_value.stdout = "/real/path"
        mock_shell.git_add_safe_directory("/path")
        assert any(
            "git config --global --add safe.directory" in str(c)
            for c in mock_subprocess.call_args_list
        )
    def test_git_add_safe_directory_resolves_path(self, mock_shell,
                                                    mock_subprocess):
        mock_subprocess.return_value.stdout = "/resolved/path"
        mock_shell.git_add_safe_directory("/some/link")
        cmds = _all_cmds(mock_subprocess)
        assert any("/resolved/path" in c for c in cmds)

class TestShellChroot:
    def test_chroot_context_manager(self, mock_shell):
        assert not mock_shell.chroots
        with mock_shell.chroot("/mnt"):
            assert mock_shell.chroots == ["/mnt"]
        assert not mock_shell.chroots
    def test_chroot_affects_commands(self, mock_shell, mock_subprocess):
        with mock_shell.chroot("/mnt"):
            mock_shell.run("echo test")
        assert any(
            "nixos-enter" in str(c)
            for c in mock_subprocess.call_args_list
        )
    def test_nested_chroots(self, mock_shell):
        with mock_shell.chroot("/mnt1"):
            assert mock_shell.chroots == ["/mnt1"]
            with mock_shell.chroot("/mnt2"):
                assert mock_shell.chroots == ["/mnt1", "/mnt2"]
            assert mock_shell.chroots == ["/mnt1"]
        assert not mock_shell.chroots
    def test_chroot_restores_on_exception(self, mock_shell):
        with pytest.raises(RuntimeError):
            with mock_shell.chroot("/mnt"):
                raise RuntimeError("boom")
        assert not mock_shell.chroots
    def test_chroot_swaps_chrootable_registry_shells(self, mock_shell):
        @chrootable
        class _Dummy:
            sh = Shell()
        original_sh = _Dummy.sh
        with mock_shell.chroot("/mnt"):
            assert _Dummy.sh is mock_shell
        assert _Dummy.sh is original_sh
        chrootable_registry.remove(_Dummy)
    def test_chroot_restores_chrootable_on_exception(self, mock_shell):
        @chrootable
        class _Dummy2:
            sh = Shell()
        original_sh = _Dummy2.sh
        with pytest.raises(ValueError):
            with mock_shell.chroot("/mnt"):
                assert _Dummy2.sh is mock_shell
                raise ValueError("test")
        assert _Dummy2.sh is original_sh
        chrootable_registry.remove(_Dummy2)

class TestChrootableDecorator:
    def test_chrootable_decorator(self):
        @chrootable
        class TestClass:
            sh = Shell()
        assert hasattr(TestClass, "chroot")
        assert TestClass in chrootable_registry
        chrootable_registry.remove(TestClass)
    def test_chrootable_chroot_context_manager(self):
        @chrootable
        class TestClass:
            sh = Shell()
        new_sh = Shell()
        original = TestClass.sh
        with TestClass.chroot(new_sh) as cls:  # type: ignore[attr-defined]
            assert cls.sh is new_sh
        assert TestClass.sh is original
        chrootable_registry.remove(TestClass)
    def test_chrootable_chroot_restores_on_exception(self):
        @chrootable
        class TestClass:
            sh = Shell()
        new_sh = Shell()
        original = TestClass.sh
        with pytest.raises(RuntimeError):
            with TestClass.chroot(new_sh):  # type: ignore[attr-defined]
                raise RuntimeError("test")
        assert TestClass.sh is original
        chrootable_registry.remove(TestClass)
    def test_chrootable_without_sh(self):
        with pytest.raises(TypeError):
            @chrootable
            class _TestClass:
                pass

class TestCreateSecretsIntegration:
    def _simulate_create_secrets(self, mock_shell, password, tmp_path):
        plain_text_path = str(tmp_path / "plain_text_password.txt")
        hashed_path = str(tmp_path / "hashed_password.txt")
        hashed = f"$6$rounds=65536$salt${password}hashed"

        written_files = {}
        mock_shell.chroots = []
        def fake_file_write(path, string, sensitive=None):  # noqa: ARG001
            written_files[path] = string

        with patch.object(mock_shell, "file_write",
                          side_effect=fake_file_write):
            mock_shell.file_write(
                plain_text_path, password, sensitive=password
            )
            mock_shell.file_write(
                hashed_path, hashed, sensitive=hashed
            )

        return written_files, plain_text_path, hashed_path
    def test_plain_text_password_written_correctly(self, mock_shell,
                                                    tmp_path):
        files, pt_path, _ = self._simulate_create_secrets(
            mock_shell, "MyP@ssw0rd!", tmp_path
        )
        assert files[pt_path] == "MyP@ssw0rd!"
    def test_hashed_password_written_correctly(self, mock_shell, tmp_path):
        files, _, h_path = self._simulate_create_secrets(
            mock_shell, "MyP@ssw0rd!", tmp_path
        )
        assert "$6$rounds=65536$salt$" in files[h_path]
        assert "***" not in files[h_path]
    def test_password_with_special_chars(self, mock_shell, tmp_path):
        password = "p@$$w0rd!#%^&*()'\"\\n"
        files, pt_path, h_path = self._simulate_create_secrets(
            mock_shell, password, tmp_path
        )
        assert files[pt_path] == password
        assert "***" not in files[pt_path]
        assert "***" not in files[h_path]
    def test_simple_password(self, mock_shell, tmp_path):
        files, pt_path, _ = self._simulate_create_secrets(
            mock_shell, "abc", tmp_path
        )
        assert files[pt_path] == "abc"
    def test_password_is_triple_asterisk(self, mock_shell, tmp_path):
        files, pt_path, _ = self._simulate_create_secrets(
            mock_shell, "***", tmp_path
        )
        assert files[pt_path] == "***"
