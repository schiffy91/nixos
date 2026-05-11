from unittest.mock import patch, MagicMock

import pytest

from lib.interactive import Interactive

class TestConfirm:
    def test_y(self, monkeypatch):
        monkeypatch.setattr("builtins.input", lambda _: "y")
        assert Interactive.confirm("Test?") is True
    def test_yes(self, monkeypatch):
        monkeypatch.setattr("builtins.input", lambda _: "yes")
        assert Interactive.confirm("Test?") is True
    def test_n(self, monkeypatch):
        monkeypatch.setattr("builtins.input", lambda _: "n")
        assert Interactive.confirm("Test?") is False
    def test_no(self, monkeypatch):
        monkeypatch.setattr("builtins.input", lambda _: "no")
        assert Interactive.confirm("Test?") is False
    def test_yes_case_insensitive(self, monkeypatch):
        monkeypatch.setattr("builtins.input", lambda _: "YES")
        assert Interactive.confirm("Test?") is True
    def test_invalid_then_valid(self, monkeypatch):
        inputs = iter(["invalid", "maybe", "y"])
        monkeypatch.setattr("builtins.input", lambda _: next(inputs))
        assert Interactive.confirm("Test?") is True
    def test_multiple_invalid_then_y(self, monkeypatch, capsys):
        inputs = iter(["bad", "nope", "wrong", "y"])
        monkeypatch.setattr("builtins.input", lambda _: next(inputs))
        assert Interactive.confirm("Test?") is True
        captured = capsys.readouterr()
        assert captured.out.count("Invalid input") == 3

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
    def test_empty_password_accepted(self):
        with patch("getpass.getpass", side_effect=["", ""]):
            assert Interactive.ask_for_password() == ""
    def test_special_characters_preserved(self):
        special = "p@$$w0rd!#%^&*()"
        with patch("getpass.getpass", side_effect=[special, special]):
            assert Interactive.ask_for_password() == special

def _stub_shell(mock_shell):
    mock_shell.basename = lambda p: p.split("/")[-1]
    mock_shell.dirname = lambda p: "/".join(p.split("/")[:-1])
    mock_shell.parent_name = lambda p: p.split("/")[-2] if "/" in p else ""


class TestHostPath:
    def test_valid_selection_first(self, mock_shell, monkeypatch):
        Interactive.sh = mock_shell
        _stub_shell(mock_shell)
        paths = ["/etc/nixos/modules/hosts/x86_64/host1/host1.nix"]
        with patch.object(mock_shell, "find_files", return_value=paths):
            monkeypatch.setattr("builtins.input", lambda _: "1")
            result = Interactive.ask_for_host_path("/etc/nixos/modules/hosts")
            assert result == paths[0]
    def test_second_selection_two_hosts(self, mock_shell, monkeypatch):
        Interactive.sh = mock_shell
        _stub_shell(mock_shell)
        paths = [
            "/etc/nixos/modules/hosts/x86_64/host1/host1.nix",
            "/etc/nixos/modules/hosts/aarch64/host2/host2.nix",
        ]
        with patch.object(mock_shell, "find_files", return_value=paths):
            monkeypatch.setattr("builtins.input", lambda _: "2")
            result = Interactive.ask_for_host_path("/etc/nixos/modules/hosts")
            assert result == paths[1]
    def test_invalid_index_then_valid(self, mock_shell, monkeypatch, capsys):
        Interactive.sh = mock_shell
        _stub_shell(mock_shell)
        paths = ["/etc/nixos/modules/hosts/x86_64/host1/host1.nix"]
        inputs = iter(["99", "invalid", "1"])
        with patch.object(mock_shell, "find_files", return_value=paths):
            monkeypatch.setattr("builtins.input", lambda _: next(inputs))
            result = Interactive.ask_for_host_path("/etc/nixos/modules/hosts")
            assert result == paths[0]
            assert "Invalid choice" in capsys.readouterr().err
    def test_keyboard_interrupt_exits(self, mock_shell, monkeypatch):
        Interactive.sh = mock_shell
        _stub_shell(mock_shell)
        paths = ["/etc/nixos/modules/hosts/x86_64/host1/host1.nix"]
        with patch.object(mock_shell, "find_files", return_value=paths):
            monkeypatch.setattr(
                "builtins.input",
                lambda _: (_ for _ in ()).throw(KeyboardInterrupt),
            )
            with pytest.raises(SystemExit):
                Interactive.ask_for_host_path("/etc/nixos/modules/hosts")
    def test_multiple_hosts_displayed_correctly(
        self, mock_shell, monkeypatch, capsys
    ):
        Interactive.sh = mock_shell
        _stub_shell(mock_shell)
        paths = [
            "/etc/nixos/modules/hosts/x86_64/desktop/desktop.nix",
            "/etc/nixos/modules/hosts/aarch64/laptop/laptop.nix",
            "/etc/nixos/modules/hosts/x86_64/server/server.nix",
        ]
        with patch.object(mock_shell, "find_files", return_value=paths):
            monkeypatch.setattr("builtins.input", lambda _: "1")
            Interactive.ask_for_host_path("/etc/nixos/modules/hosts")
            captured = capsys.readouterr()
            assert "1)" in captured.out
            assert "2)" in captured.out
            assert "3)" in captured.out
    def test_filters_non_host_nix_files(self, mock_shell, monkeypatch):
        Interactive.sh = mock_shell
        _stub_shell(mock_shell)
        paths = [
            "/etc/nixos/modules/hosts/x86_64/HOST/HOST.nix",
            "/etc/nixos/modules/hosts/x86_64/HOST/audio.nix",
            "/etc/nixos/modules/hosts/x86_64/HOST/graphics.nix",
        ]
        with patch.object(mock_shell, "find_files", return_value=paths):
            monkeypatch.setattr("builtins.input", lambda _: "1")
            result = Interactive.ask_for_host_path("/etc/nixos/modules/hosts")
            assert result == paths[0]

class TestReboot:
    def test_yes_triggers_reboot(self, monkeypatch):
        monkeypatch.setattr("builtins.input", lambda _: "y")
        with patch("lib.interactive.Utils.reboot") as m:
            Interactive.ask_to_reboot()
            m.assert_called_once()
    def test_no_returns_false(self, monkeypatch):
        monkeypatch.setattr("builtins.input", lambda _: "n")
        with patch("lib.interactive.Utils.reboot") as m:
            result = Interactive.ask_to_reboot()
            m.assert_not_called()
            assert result is False
    def test_reboot_return_value(self, monkeypatch):
        monkeypatch.setattr("builtins.input", lambda _: "y")
        sentinel = MagicMock()
        with patch("lib.interactive.Utils.reboot", return_value=sentinel):
            result = Interactive.ask_to_reboot()
            assert result is sentinel
