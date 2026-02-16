from subprocess import CalledProcessError
from unittest.mock import MagicMock, patch, mock_open

import pytest

from core.shell import Shell, chrootable, chrootable_registry


class TestShellBasics:
    def test_stdout_strips_whitespace(self):
        mock = MagicMock()
        mock.stdout = "  test output  \n"
        assert Shell.stdout(mock) == "test output"

    def test_stdout_empty_string(self):
        mock = MagicMock()
        mock.stdout = ""
        assert Shell.stdout(mock) == ""

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


class TestShellExecution:
    def test_run_basic(self, mock_shell, mock_subprocess):
        mock_subprocess.return_value.stdout = "output"
        mock_shell.run("echo test")
        assert mock_subprocess.called

    def test_run_with_env(self, mock_shell, mock_subprocess):
        mock_shell.run("echo test", env="FOO=bar")
        cmd = mock_subprocess.call_args[0][0]
        assert "FOO=bar" in cmd

    def test_run_without_sudo(self, mock_shell, mock_subprocess):
        mock_shell.run("echo test", sudo=False)
        cmd = mock_subprocess.call_args[0][0]
        assert "sudo" not in cmd

    def test_run_with_sensitive(self, mock_shell, capsys):
        mock_shell.run("echo secret123", sensitive="secret123")
        captured = capsys.readouterr()
        assert "secret123" not in captured.out
        assert "***" in captured.out

    def test_run_error_handling(self, mock_shell, mock_subprocess):
        error = CalledProcessError(1, "cmd")
        error.stdout = "stdout msg"
        error.stderr = "stderr msg"
        mock_subprocess.side_effect = error
        with pytest.raises(CalledProcessError):
            mock_shell.run("bad command")

    def test_run_in_chroot(self, mock_shell, mock_subprocess):
        with mock_shell.chroot("/mnt"):
            mock_shell.run("echo test")
        cmd = mock_subprocess.call_args[0][0]
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

    def test_rm(self, mock_shell, mock_subprocess):
        mock_shell.rm("/test/file")
        assert any(
            "rm -rf /test/file" in str(c)
            for c in mock_subprocess.call_args_list
        )

    def test_mv(self, mock_shell, mock_subprocess):
        mock_subprocess.return_value.stdout = "/dest"
        mock_shell.mv("/src/file", "/dest/file")
        assert any(
            "mv /src/file /dest/file" in str(c)
            for c in mock_subprocess.call_args_list
        )

    def test_cpdir(self, mock_shell, mock_subprocess):
        mock_subprocess.return_value.stdout = "/dest"
        mock_shell.cpdir("/src", "/dest/target")
        assert any(
            "cp -r /src /dest/target" in str(c)
            for c in mock_subprocess.call_args_list
        )

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

    def test_realpaths(self, mock_shell, mock_subprocess):
        mock_subprocess.return_value.stdout = "/real/a\n/real/b"
        result = mock_shell.realpaths("/a", "/b")
        assert result == ["/real/a", "/real/b"]

    def test_find_basic(self, mock_shell, mock_subprocess):
        mock_subprocess.return_value.stdout = "/path\n/real/path\n/a\n/b"
        result = mock_shell.find("/path")
        assert len(result) > 0

    def test_find_empty(self, mock_shell, mock_subprocess):
        mock_subprocess.return_value.stdout = "/path\n"
        result = mock_shell.find("/path")
        assert isinstance(result, list)

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
            "-not" in str(c)
            for c in mock_subprocess.call_args_list
        )

    def test_find_with_multi_pattern(self, mock_shell, mock_subprocess):
        mock_subprocess.return_value.stdout = "/path\n/a"
        mock_shell.find("/path", pattern="*.py *.nix")
        assert any(
            "-o" in str(c)
            for c in mock_subprocess.call_args_list
        )


class TestShellIO:
    def test_file_write(self, mock_shell, tmp_path):
        mock_shell.chroots = []
        with patch("builtins.open", mock_open()):
            with patch.object(mock_shell, "rm"):
                with patch.object(mock_shell, "mkdir"):
                    with patch.object(mock_shell, "dirname",
                                      return_value=str(tmp_path)):
                        mock_shell.file_write(
                            str(tmp_path / "test.txt"), "content"
                        )

    def test_file_write_with_sensitive(self, mock_shell, tmp_path):
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
                        assert "secret123" not in written

    def test_file_write_with_chroot(self, mock_shell, tmp_path):
        mock_shell.chroots = ["/mnt"]
        with patch("builtins.open", mock_open()) as m:
            with patch.object(mock_shell, "rm"):
                with patch.object(mock_shell, "mkdir"):
                    with patch.object(mock_shell, "dirname",
                                      return_value=str(tmp_path)):
                        mock_shell.file_write("/etc/test", "data")
                        path_arg = m.call_args[0][0]
                        assert path_arg.startswith("/mnt")

    def test_file_read_nonexistent(self, mock_shell):
        with patch.object(mock_shell, "exists", return_value=False):
            assert mock_shell.file_read("/nonexistent") == ""

    def test_file_read_with_chroot(self, mock_shell):
        mock_shell.chroots = ["/mnt"]
        with patch.object(mock_shell, "exists", return_value=True):
            with patch("builtins.open",
                       mock_open(read_data="content")):
                result = mock_shell.file_read("/etc/test")
                assert result == "content"

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

    def test_json_write(self, mock_shell):
        with patch.object(mock_shell, "json_read",
                          return_value={"existing": "data"}):
            with patch.object(mock_shell, "file_write") as m:
                mock_shell.json_write("/t.json", "new", "val")
                written = m.call_args[0][1]
                assert "existing" in written
                assert "new" in written

    def test_json_overwrite(self, mock_shell):
        with patch.object(mock_shell, "rm"):
            with patch.object(mock_shell, "file_write") as m:
                mock_shell.json_overwrite("/t.json", {"a": 1})
                written = m.call_args[0][1]
                assert '"a"' in written


class TestShellSecurity:
    def test_chmod(self, mock_shell, mock_subprocess):
        mock_subprocess.return_value.stdout = "/real/path"
        mock_shell.chmod("755", "/test/path")
        assert any(
            "chmod -R 755" in str(c)
            for c in mock_subprocess.call_args_list
        )

    def test_chown(self, mock_shell, mock_subprocess):
        mock_subprocess.return_value.stdout = "/real/path"
        mock_shell.chown("user", "/test/path")
        assert any(
            "chown -R user" in str(c)
            for c in mock_subprocess.call_args_list
        )

    def test_ssh_keygen(self, mock_shell, mock_subprocess):
        mock_subprocess.return_value.stdout = "/path"
        mock_shell.ssh_keygen("ed25519", "/path/key")
        assert any(
            "ssh-keygen" in str(c)
            for c in mock_subprocess.call_args_list
        )


class TestShellGit:
    def test_git_add_safe_directory(self, mock_shell, mock_subprocess):
        mock_subprocess.return_value.stdout = "/real/path"
        mock_shell.git_add_safe_directory("/path")
        assert any(
            "git config --global --add safe.directory" in str(c)
            for c in mock_subprocess.call_args_list
        )


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


class TestChrootableDecorator:
    def test_chrootable_decorator(self):
        @chrootable
        class TestClass:
            sh = Shell()
        assert hasattr(TestClass, "chroot")
        assert TestClass in chrootable_registry

    def test_chrootable_chroot_context_manager(self):
        @chrootable
        class TestClass:
            sh = Shell()
        new_sh = Shell()
        original = TestClass.sh
        with TestClass.chroot(new_sh) as cls:  # type: ignore[attr-defined]
            assert cls.sh is new_sh
        assert TestClass.sh is original

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

    def test_chrootable_without_sh(self):
        with pytest.raises(TypeError):
            @chrootable
            class _TestClass:
                pass
