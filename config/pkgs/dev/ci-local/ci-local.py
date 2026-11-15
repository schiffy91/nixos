import json
import os
import platform
import re
import socket
import subprocess
import sys
import time
from pathlib import Path

USAGE = """ci-local: let a commit's local test passes stand in for the same jobs in CI.

  ci-local run [JOB...]   run this platform's jobs from ci/local.json on a clean tree; record each pass
  ci-local status         what is recorded for HEAD, and what has been published
  ci-local publish        post the recorded passes to GitHub as commit statuses (the commit must be there)
  ci-local push [ARGS]    put the commit on GitHub, post its passes, then `git push ARGS`

A pass is recorded only for the exact commit: the tree must be clean before and after, with HEAD unchanged.
The status is `local/<platform>/<job>`; a workflow skips a job whose status the repository's owner posted."""


class Local:
    @classmethod
    def git(cls, *arguments, check=True):
        result = subprocess.run(["git", *arguments], capture_output=True, text=True)
        if check and result.returncode != 0: sys.exit(f"ci-local: git {' '.join(arguments)}: {result.stderr.strip()}")
        return result.stdout.strip()

    @classmethod
    def platform(cls): return f"{'macos' if platform.system() == 'Darwin' else platform.system().lower()}-{platform.machine().lower()}"

    @classmethod
    def root(cls): return Path(cls.git("rev-parse", "--show-toplevel"))

    @classmethod
    def head(cls): return cls.git("rev-parse", "HEAD")

    @classmethod
    def clean(cls): return cls.git("status", "--porcelain") == ""

    @classmethod
    def jobs(cls):
        table = cls.root() / "ci" / "local.json"
        if not table.exists(): sys.exit(f"ci-local: {table} does not exist; it names the jobs a local pass may stand in for")
        return json.loads(table.read_text())["jobs"]

    @classmethod
    def context(cls, job, where): return f"local/{where}/{job}"

    @classmethod
    def receipts(cls, commit): return Path(cls.git("rev-parse", "--path-format=absolute", "--git-common-dir")) / "ci-local" / commit

    @classmethod
    def receipt(cls, commit, job): return cls.receipts(commit) / (re.sub(r"[^A-Za-z0-9._-]+", "_", job) + ".json")

    @classmethod
    def recorded(cls, commit): return [json.loads(path.read_text()) for path in sorted(cls.receipts(commit).glob("*.json"))]

    @classmethod
    def repository(cls):
        match = re.search(r"github\.com[:/]([^/]+/[^/]+?)(\.git)?$", cls.git("config", "--get", "remote.origin.url"))  # as written, before any insteadOf
        if not match: sys.exit("ci-local: origin is not a GitHub repository")
        return match.group(1)

    @classmethod
    def post(cls, entry, state, description):
        command = ["gh", "api", "--method", "POST", f"repos/{cls.repository()}/statuses/{entry['commit']}", "-f", f"state={state}", "-f", f"context={entry['context']}", "-f", f"description={description[:140]}"]
        result = subprocess.run(command, capture_output=True, text=True)
        if result.returncode != 0: sys.exit(f"ci-local: could not post {entry['context']}: {result.stderr.strip()}")

    @classmethod
    def run(cls, names):
        jobs, here, commit, failed = cls.jobs(), cls.platform(), cls.head(), []
        for name in names:
            if name not in jobs: sys.exit(f"ci-local: ci/local.json names no job {name}; it names: {', '.join(jobs)}")
            if jobs[name]["platform"] != here: sys.exit(f"ci-local: {name} is a {jobs[name]['platform']} job and this is {here}; a pass here would not stand in for it")
        if not cls.clean(): sys.exit("ci-local: the tree has changes or untracked files; a pass is recorded for a commit, so commit or remove them first")
        for name in names or [name for name, job in jobs.items() if job["platform"] == here]:
            print(f"ci-local: {name}: {jobs[name]['run']}", flush=True)
            started = time.time()
            code = subprocess.run(jobs[name]["run"], shell=True, cwd=cls.root()).returncode
            path = cls.receipt(commit, name)
            if code == 0 and (cls.head() != commit or not cls.clean()): sys.exit(f"ci-local: {name} passed but left the tree changed or HEAD moved; nothing recorded")
            if code != 0:
                earlier = json.loads(path.read_text()) if path.exists() else None
                if earlier and earlier.get("published"): cls.post(earlier, "failure", f"failed on {socket.gethostname()} after an earlier local pass")
                path.unlink(missing_ok=True)
                failed.append(name)
                print(f"ci-local: {name} FAILED (exit {code}); nothing recorded", flush=True)
                continue
            path.parent.mkdir(parents=True, exist_ok=True)
            entry = {"commit": commit, "job": name, "platform": here, "context": cls.context(name, here), "host": socket.gethostname(), "run": jobs[name]["run"], "finished": int(time.time()), "seconds": int(time.time() - started)}
            path.write_text(json.dumps(entry, indent=2) + "\n")
            print(f"ci-local: {name} passed in {entry['seconds']} s; recorded for {commit[:12]}", flush=True)
        return 1 if failed else 0

    @classmethod
    def status(cls):
        commit = cls.head()
        entries = cls.recorded(commit)
        if not entries: print(f"Nothing is recorded for {commit[:12]}.")
        for entry in entries: print(f"{entry['context']}  passed on {entry['host']} in {entry['seconds']} s  {'published' if entry.get('published') else 'not published'}")
        return 0

    @classmethod
    def publish(cls):
        commit = cls.head()
        for entry in cls.recorded(commit):
            minutes = max(1, round(entry["seconds"] / 60))
            cls.post(entry, "success", f"passed on {entry['host']} in {minutes} min")
            entry["published"] = int(time.time())
            cls.receipt(commit, entry["job"]).write_text(json.dumps(entry, indent=2) + "\n")
            print(f"ci-local: posted {entry['context']}")
        return 0

    @classmethod
    def push(cls, arguments):
        commit = cls.head()
        if not cls.recorded(commit): print(f"ci-local: nothing is recorded for {commit[:12]}; pushing as it is")
        else:
            held = f"refs/ci-local/{commit}"  # not a branch: no workflow starts until the statuses are there
            cls.git("push", "--quiet", "origin", f"{commit}:{held}")
            try: cls.publish()
            finally: cls.git("push", "--quiet", "origin", f":{held}", check=False)
        return subprocess.run(["git", "push", *arguments]).returncode

    @classmethod
    def main(cls, arguments):
        if not arguments or arguments[0] in ("-h", "--help", "help"):
            print(USAGE)
            return 0 if arguments else 1
        os.chdir(cls.root())
        command, rest = arguments[0], arguments[1:]
        if command == "run": return cls.run(rest)
        if command == "status": return cls.status()
        if command == "publish": return cls.publish()
        if command == "push": return cls.push(rest)
        sys.exit(f"ci-local: unknown command {command}\n\n{USAGE}")


if __name__ == "__main__": sys.exit(Local.main(sys.argv[1:]))
