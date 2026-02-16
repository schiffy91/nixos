from core.shell import Shell

_sh = Shell()


def compile_python_file(file_path):
    result = _sh.run(
        f"python3 -m py_compile '{file_path}'",
        sudo=False, check=False,
    )
    return result.returncode == 0
