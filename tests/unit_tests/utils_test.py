from unittest.mock import MagicMock

import pytest

from lib.utils import Utils

class TestUtilsParseArgs:
    def test_parses_command(self, monkeypatch):
        monkeypatch.setattr("sys.argv", ["prog", "enable"])
        args = Utils.parse_args({"enable": [], "disable": []})
        assert args.command == "enable"
    def test_parses_flag(self, monkeypatch):
        monkeypatch.setattr("sys.argv", ["prog", "enable", "--microsoft"])
        args = Utils.parse_args({"enable": ["--microsoft"], "disable": []})
        assert args.command == "enable"
        assert args.microsoft is True
    def test_flag_defaults_false(self, monkeypatch):
        monkeypatch.setattr("sys.argv", ["prog", "enable"])
        args = Utils.parse_args({"enable": ["--microsoft"], "disable": []})
        assert args.microsoft is False
    def test_missing_command_exits(self, monkeypatch):
        monkeypatch.setattr("sys.argv", ["prog"])
        with pytest.raises(SystemExit):
            Utils.parse_args({"enable": [], "disable": []})
    def test_invalid_command_exits(self, monkeypatch):
        monkeypatch.setattr("sys.argv", ["prog", "bogus"])
        with pytest.raises(SystemExit):
            Utils.parse_args({"enable": [], "disable": []})
    def test_boolean_flags(self, monkeypatch):
        monkeypatch.setattr("sys.argv", ["prog", "--clean"])
        args = Utils.parse_args(["--rebuild-filesystem", "--clean"])
        assert args.clean is True
        assert args.rebuild_filesystem is False
    def test_typed_arg(self, monkeypatch):
        monkeypatch.setattr("sys.argv", ["prog", "--depth", "3"])
        args = Utils.parse_args([("--depth", int)])
        assert args.depth == 3
    def test_typed_arg_defaults_none(self, monkeypatch):
        monkeypatch.setattr("sys.argv", ["prog"])
        args = Utils.parse_args([("--depth", int)])
        assert args.depth is None
    def test_positional_arg(self, monkeypatch):
        monkeypatch.setattr("sys.argv", ["prog", "some.expression"])
        args = Utils.parse_args(["expression"])
        assert args.expression == "some.expression"

class TestUtilsLogging:
    def test_log_enabled(self, capsys):
        Utils.LOG_INFO = True
        Utils.log("test message")
        captured = capsys.readouterr()
        assert "LOG: test message" in captured.out
        assert Utils.GRAY in captured.out
    def test_log_disabled(self, capsys):
        original = Utils.LOG_INFO
        Utils.LOG_INFO = False
        Utils.log("test message")
        captured = capsys.readouterr()
        assert captured.out == ""
        Utils.LOG_INFO = original
    def test_log_error_with_color(self, capsys):
        Utils.log_error("error message")
        captured = capsys.readouterr()
        assert "ERROR: error message" in captured.err
        assert Utils.ORANGE in captured.err
    def test_print(self, capsys):
        Utils.print("test")
        assert "test" in capsys.readouterr().out
    def test_print_inline_format(self, capsys):
        Utils.print_inline("progress")
        captured = capsys.readouterr()
        assert "\rprogress" in captured.out
        assert not captured.out.endswith("\n")
    def test_print_error_color(self, capsys):
        Utils.print_error("error")
        captured = capsys.readouterr()
        assert "error" in captured.err
        assert Utils.RED in captured.err

class TestUtilsAbort:
    def test_abort_with_message_prints_error_and_exits(self, capsys):
        with pytest.raises(SystemExit) as exc:
            Utils.abort("fatal error")
        assert exc.value.code == 1
        captured = capsys.readouterr()
        assert "fatal error" in captured.err
    def test_abort_without_message_just_exits(self):
        with pytest.raises(SystemExit) as exc:
            Utils.abort()
        assert exc.value.code == 1
    def test_abort_message_appears_in_stderr(self, capsys):
        with pytest.raises(SystemExit):
            Utils.abort("stderr check")
        captured = capsys.readouterr()
        assert "stderr check" in captured.err
        assert captured.out == "" or "stderr check" not in captured.out

class TestUtilsSystem:
    def test_reboot_calls_shutdown(self, mock_shell, monkeypatch):
        Utils.sh = mock_shell
        mock_run = MagicMock()
        monkeypatch.setattr(mock_shell, "run", mock_run)
        Utils.reboot()
        mock_run.assert_called_once_with("shutdown -r now")
    def test_require_root_delegates(self, mock_shell, monkeypatch):
        Utils.sh = mock_shell
        mock_rr = MagicMock()
        monkeypatch.setattr(mock_shell, "require_root", mock_rr)
        Utils.require_root()
        mock_rr.assert_called_once()

