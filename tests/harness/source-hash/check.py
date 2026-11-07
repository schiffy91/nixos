"""Verify source-sensitive graph caching through the real CLI and state store."""
import json
import os
from pathlib import Path
import shlex
import subprocess
import tempfile


root = Path.cwd()
extensions = ("cpp", "cc", "cxx", "hpp", "hh", "hxx", "js")

with tempfile.TemporaryDirectory(prefix="nixos-source-hash-") as temporary:
    directory = Path(temporary)
    fixtures = directory / "fixtures with spaces"
    fixtures.mkdir()
    for extension in extensions:
        declaration = "const revision" if extension == "js" else "constexpr int revision"
        (fixtures / f"capture.{extension}").write_text(f"{declaration} = 1;\n")
    state = directory / ".vm"
    state.mkdir()
    executions = state / "executions"
    spec = {
        "name": "source-probe", "state": "source-probe",
        "stateMaterial": "source-probe:{{sourceHash}}",
        "stateRoot": str(state / "chain"), "workDir": str(state / "work"),
        "operations": [
            {"op": "host", "command": "printf 'run\\n' >> " + shlex.quote(str(executions))},
            {"op": "record-state"},
        ],
    }
    (directory / "probe.json").write_text(json.dumps(spec))
    graph = directory / "graph.json"
    graph.write_text(json.dumps({
        "name": "source-hash-regression", "workspaceRoot": ".",
        "nodes": [{"id": "probe", "spec": "probe.json"}],
    }))

    def run(expected_executions):
        process = subprocess.run(
            [str(root / "build/nixosctl"), "graph", str(graph), "run", "probe"],
            cwd=directory, env={**os.environ, "NIXOS_CONFIG_ROOT": str(root)},
            stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, timeout=20,
        )
        assert process.returncode == 0, process.stdout
        actual = executions.read_text().splitlines()
        assert actual == ["run"] * expected_executions, process.stdout + repr(actual)
        return (state / "chain/source-probe/hash").read_text()

    previous = run(1)
    assert run(1) == previous, "Unchanged sources did not reuse the ready state"
    for expected, extension in enumerate(extensions, start=2):
        print(f"Checking content/cache behavior for .{extension}", flush=True)
        fixture = fixtures / f"capture.{extension}"
        fixture.write_text(fixture.read_text().replace("= 1", "= 2"))
        current = run(expected)
        assert current != previous, f"Changing .{extension} did not invalidate the state hash"
        assert run(expected) == current, f"Unchanged .{extension} did not reuse the ready state"
        fixture.touch()
        assert run(expected) == current, f"An mtime-only .{extension} change invalidated content cache"
        previous = current

    # Generated C++ and prior test results remain outside source identity.
    for excluded in (directory / "build", state):
        excluded.mkdir(exist_ok=True)
        (excluded / "generated.cpp").write_text("constexpr int generated = 1;\n")
    assert run(1 + len(extensions)) == previous, "Generated outputs invalidated source identity"

print("PASS: C++ and JavaScript content invalidates graph cache; unchanged content reuses it")
