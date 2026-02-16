from pathlib import Path

from core.tests.helpers import compile_python_file

CLI_DIR = Path(__file__).parent.parent
EXPECTED_SCRIPTS = [
    "install.py", "update.py", "diff.py", "secure_boot.py",
    "tpm2.py", "eval.py", "change_password.py", "gpu_vfio.py",
]


class TestCLIStructure:
    def test_cli_directory_exists(self):
        assert CLI_DIR.exists() and CLI_DIR.is_dir()

    def test_all_scripts_compile(self):
        for script in CLI_DIR.glob("*.py"):
            if script.name == "__init__.py":
                continue
            assert compile_python_file(script), f"{script.name} failed"

    def test_scripts_have_shebang(self):
        for script in CLI_DIR.glob("*.py"):
            if script.name == "__init__.py":
                continue
            first_line = script.read_text().split("\n")[0]
            assert first_line.startswith(
                "#!"), f"{script.name} missing shebang"

    def test_expected_scripts_exist(self):
        for name in EXPECTED_SCRIPTS:
            assert (CLI_DIR / name).exists(), f"{name} missing"

    def test_scripts_have_main(self):
        for script in CLI_DIR.glob("*.py"):
            if script.name == "__init__.py":
                continue
            content = script.read_text()
            assert "def main()" in content, f"{script.name} missing main()"
            assert '__name__' in content, f"{
                script.name} missing __name__ guard"
