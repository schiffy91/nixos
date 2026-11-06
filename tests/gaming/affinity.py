import json
import os
import subprocess
import sys

launcher = os.environ["GAME_RUN"]
available = sorted(os.sched_getaffinity(0))
cpu = available[0]
probe = "import json,os,sys; print(json.dumps([sorted(os.sched_getaffinity(0)), sys.argv[1:], os.getenv('GAME_TEST')]))"


def run(affinity):
    env = dict(os.environ, GAME_CPU_AFFINITY=affinity)
    result = subprocess.run(
        [launcher, "env", "GAME_TEST=value with spaces", sys.executable, "-c", probe, "one argument", "$(literal)"],
        env=env, check=True, capture_output=True, text=True,
    )
    return json.loads(result.stdout)


assert run(str(cpu)) == [[cpu], ["one argument", "$(literal)"], "value with spaces"]
assert run("all")[0] == available
assert run("")[0] == available
env = dict(os.environ, GAME_CPU_AFFINITY=str(cpu))
assert subprocess.run([launcher, sys.executable, "-c", "raise SystemExit(37)"], env=env).returncode == 37
assert subprocess.run([launcher], capture_output=True).returncode == 2
print("PASS: real CPU affinity, opt-out, environment/argv preservation, and exit status")
