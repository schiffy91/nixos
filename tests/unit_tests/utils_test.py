from unittest.mock import MagicMock, patch

import pytest

from lib.utils import Utils

class TestUtilsParseArgs:
    def test_single_match(self):
        assert Utils.parse_args(["enable"], "enable", "disable") == ["enable"]
    def test_multiple_matches(self):
        result = Utils.parse_args(
            ["enable", "verbose"], "enable", "disable", "verbose"
        )
        assert sorted(result) == ["enable", "verbose"]
    def test_no_matches(self):
        assert Utils.parse_args(["foo"], "enable", "disable") == []
    def test_empty_argv(self):
        assert Utils.parse_args([], "enable") == []
    def test_none_argv(self):
        assert Utils.parse_args(None, "enable") == []
    def test_no_accepted_args(self):
        assert Utils.parse_args(["enable"]) == []
    def test_extra_unrecognized_args_filtered_out(self):
        result = Utils.parse_args(
            ["enable", "bogus", "unknown", "disable"],
            "enable", "disable",
        )
        assert result == ["enable", "disable"]

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

class TestUtilsToggle:
    def test_enable_calls_on_enable(self):
        called = []
        Utils.toggle(["enable"], on_enable=lambda: called.append("e"))
        assert called == ["e"]
    def test_disable_calls_on_disable(self):
        called = []
        Utils.toggle(["disable"], on_disable=lambda: called.append("d"))
        assert called == ["d"]
    def test_invalid_shows_usage_and_exits(self, mock_shell):
        Utils.sh = mock_shell
        with patch.object(mock_shell, "basename", return_value="test.py"):
            with pytest.raises(SystemExit):
                Utils.toggle(["invalid"])
    def test_exception_calls_on_exception_then_re_raises(self):
        exception_called = []
        def on_enable():
            raise ValueError("test error")
        def on_exception():
            exception_called.append(True)

        with pytest.raises(ValueError):
            Utils.toggle(
                ["enable"],
                on_enable=on_enable,
                on_exception=on_exception,
            )
        assert exception_called
    def test_exception_without_handler_still_re_raises(self):
        with pytest.raises(ValueError):
            Utils.toggle(
                ["enable"],
                on_enable=lambda: (_ for _ in ()).throw(ValueError("err")),
            )
    def test_enable_with_none_on_enable_hits_default_case(self, mock_shell):
        Utils.sh = mock_shell
        with patch.object(mock_shell, "basename", return_value="test.py"):
            with pytest.raises(SystemExit):
                Utils.toggle(["enable"], on_enable=None)
    def test_toggle_with_empty_argv(self, mock_shell):
        Utils.sh = mock_shell
        with patch.object(mock_shell, "basename", return_value="test.py"):
            with pytest.raises(SystemExit):
                Utils.toggle([])
