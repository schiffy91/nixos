from unittest.mock import patch

import pytest

from core.interactive import Interactive


class TestConfirm:
    def test_yes(self, monkeypatch):
        monkeypatch.setattr("builtins.input", lambda _: "y")
        assert Interactive.confirm("Test?") is True

    def test_yes_full(self, monkeypatch):
        monkeypatch.setattr("builtins.input", lambda _: "yes")
        assert Interactive.confirm("Test?") is True

    def test_no(self, monkeypatch):
        monkeypatch.setattr("builtins.input", lambda _: "n")
        assert Interactive.confirm("Test?") is False

    def test_no_full(self, monkeypatch):
        monkeypatch.setattr("builtins.input", lambda _: "no")
        assert Interactive.confirm("Test?") is False

    def test_invalid_then_valid(self, monkeypatch):
        inputs = iter(["invalid", "maybe", "y"])
        monkeypatch.setattr("builtins.input", lambda _: next(inputs))
        assert Interactive.confirm("Test?") is True


class TestPassword:
    def test_match(self):
        with patch("getpass.getpass",
                   side_effect=["password123", "password123"]):
            assert Interactive.ask_for_password() == "password123"

    def test_mismatch_then_match(self, capsys):
        with patch("getpass.getpass",
                   side_effect=["p1", "wrong", "p1", "p1"]):
            assert Interactive.ask_for_password() == "p1"
            assert "do not match" in capsys.readouterr().err


class TestHostPath:
    def test_valid_selection(self, mock_shell, monkeypatch):
        Interactive.sh = mock_shell
        paths = ["/etc/nixos/modules/hosts/x86_64/host1.nix"]
        with patch("glob.glob", return_value=paths):
            with patch.object(mock_shell, "basename",
                              side_effect=lambda p: p.split("/")[-1]):
                with patch.object(mock_shell, "parent_name",
                                  return_value="x86_64"):
                    monkeypatch.setattr("builtins.input", lambda _: "1")
                    result = Interactive.ask_for_host_path(
                        "/etc/nixos/modules/hosts"
                    )
                    assert result == paths[0]

    def test_invalid_then_valid(self, mock_shell, monkeypatch, capsys):
        Interactive.sh = mock_shell
        paths = ["/etc/nixos/modules/hosts/x86_64/host1.nix"]
        inputs = iter(["99", "invalid", "1"])
        with patch("glob.glob", return_value=paths):
            with patch.object(mock_shell, "basename",
                              side_effect=lambda p: p.split("/")[-1]):
                with patch.object(mock_shell, "parent_name",
                                  return_value="x86_64"):
                    monkeypatch.setattr(
                        "builtins.input", lambda _: next(inputs)
                    )
                    result = Interactive.ask_for_host_path(
                        "/etc/nixos/modules/hosts"
                    )
                    assert result == paths[0]
                    assert "Invalid choice" in capsys.readouterr().err

    def test_keyboard_interrupt(self, mock_shell, monkeypatch):
        Interactive.sh = mock_shell
        paths = ["/etc/nixos/modules/hosts/x86_64/host1.nix"]
        with patch("glob.glob", return_value=paths):
            with patch.object(mock_shell, "basename",
                              return_value="host1.nix"):
                with patch.object(mock_shell, "parent_name",
                                  return_value="x86_64"):
                    monkeypatch.setattr(
                        "builtins.input",
                        lambda _: (_ for _ in ()).throw(KeyboardInterrupt),
                    )
                    with pytest.raises(SystemExit):
                        Interactive.ask_for_host_path(
                            "/etc/nixos/modules/hosts"
                        )


class TestReboot:
    def test_yes(self, monkeypatch):
        monkeypatch.setattr("builtins.input", lambda _: "y")
        with patch("core.interactive.Utils.reboot") as m:
            Interactive.ask_to_reboot()
            m.assert_called_once()

    def test_no(self, monkeypatch):
        monkeypatch.setattr("builtins.input", lambda _: "n")
        with patch("core.interactive.Utils.reboot") as m:
            result = Interactive.ask_to_reboot()
            m.assert_not_called()
            assert result is False
