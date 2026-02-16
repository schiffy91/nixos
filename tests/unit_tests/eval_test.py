from unittest.mock import patch

import pytest

from lib.config import Config
from bin.eval import main

class TestEvalMain:
    def test_eval_expression(self, mock_shell,  # noqa: ARG002
                             monkeypatch, capsys):
        monkeypatch.setattr("sys.argv", ["eval.py", "config.test.attr"])
        with patch.object(Config, "eval", return_value="test-value"):
            main()
            assert "test-value" in capsys.readouterr().out
    def test_eval_error(self, mock_shell,  # noqa: ARG002
                        monkeypatch):
        monkeypatch.setattr("sys.argv", ["eval.py", "bad.attr"])
        with patch.object(Config, "eval", side_effect=RuntimeError("fail")):
            with pytest.raises(RuntimeError):
                main()
    def test_missing_expression(self, mock_shell,  # noqa: ARG002
                                monkeypatch):
        monkeypatch.setattr("sys.argv", ["eval.py"])
        with pytest.raises(SystemExit):
            main()
    def test_eval_boolean(self, mock_shell,  # noqa: ARG002
                          monkeypatch, capsys):
        monkeypatch.setattr("sys.argv", ["eval.py", "config.settings.flag"])
        with patch.object(Config, "eval", return_value=True):
            main()
            assert "True" in capsys.readouterr().out
    def test_eval_empty_string(self, mock_shell,  # noqa: ARG002
                               monkeypatch, capsys):
        monkeypatch.setattr("sys.argv", ["eval.py", "config.settings.empty"])
        with patch.object(Config, "eval", return_value=""):
            main()
            output = capsys.readouterr().out
            assert output.strip() == ""
