import subprocess
from pathlib import Path

CLI_DIR = Path(__file__).resolve().parent.parent.parent / "bin"
EXPECTED_SCRIPTS = [
    "install.py", "update.py", "diff.py", "secure_boot.py",
    "tpm2.py", "eval.py", "change_password.py",
]

def _scripts(): return [p for p in CLI_DIR.glob("*.py") if p.name != "__init__.py"]
def _compiles(p): return subprocess.run(["python3", "-m", "py_compile", str(p)]).returncode == 0

class TestCLIStructure:
    def test_cli_directory_exists(self):
        assert CLI_DIR.exists() and CLI_DIR.is_dir()
    def test_all_scripts_compile(self):
        for s in _scripts(): assert _compiles(s), f"{s.name} failed"
    def test_scripts_have_shebang(self):
        for s in _scripts(): assert s.read_text().splitlines()[0].startswith("#!"), f"{s.name} missing shebang"
    def test_expected_scripts_exist(self):
        for name in EXPECTED_SCRIPTS: assert (CLI_DIR / name).exists(), f"{name} missing"
    def test_scripts_have_main(self):
        for s in _scripts():
            content = s.read_text()
            assert "def main()" in content, f"{s.name} missing main()"
            assert '__name__' in content, f"{s.name} missing __name__ guard"
