#!/usr/bin/env nix-shell
#!nix-shell -i python3 -p python3 gh git
import json
import re
import subprocess
import sys
from pathlib import Path

USAGE = """btrc-pin: one compiler revision in every project, and only one that passed its tests.

  btrc-pin status         every lock that pins btrc: its revision, how far behind main, what hosted CI said of it
  btrc-pin bump [REV]     set every lock to REV; without REV, to the newest commit on main that passed

A revision is taken only if every required workflow passed on it (--require, default: CI and Strict stdlib).
Locks are found under --root (default: /etc/nixos and every directory in ~/Projects). Nothing is committed:
each project is then tested and pushed in the usual way."""


class Pins:
    repository, required, roots = "schiffy91/btrc", ["CI", "Strict stdlib"], [Path("/etc/nixos"), *sorted(Path.home().glob("Projects/*"))]
    skipped = {".vm", "build", "node_modules", ".git", "result"}

    @classmethod
    def run(cls, *command, cwd=None, check=True):
        result = subprocess.run(command, cwd=cwd, capture_output=True, text=True)
        if check and result.returncode != 0: sys.exit(f"btrc-pin: {' '.join(command)}: {result.stderr.strip()}")
        return result.stdout.strip()

    @classmethod
    def locks(cls):
        found = []
        for root in cls.roots:
            for path in sorted(root.glob("flake.lock")) + sorted(root.glob("*/flake.lock")) + sorted(root.glob("*/*/flake.lock")) + sorted(root.glob("*/*/*/flake.lock")):
                if cls.skipped & set(path.relative_to(root).parts): continue
                node = json.loads(path.read_text()).get("nodes", {}).get("btrc", {})
                locked = node.get("locked", {})
                if f"{locked.get('owner')}/{locked.get('repo')}" == cls.repository: found.append((path, locked["rev"], "rev" in node.get("original", {})))
        return found

    @classmethod
    def verdicts(cls, revision):
        runs = json.loads(cls.run("gh", "run", "list", "--repo", cls.repository, "--commit", revision, "--limit", "100", "--json", "name,status,conclusion,createdAt"))
        latest = {}
        for run in sorted(runs, key=lambda run: run["createdAt"]): latest[run["name"]] = run["conclusion"] or run["status"]
        return latest

    @classmethod
    def passed(cls, revision):
        verdicts = cls.verdicts(revision)
        return all(verdicts.get(name) == "success" for name in cls.required), verdicts

    @classmethod
    def words(cls, verdicts): return ", ".join(f"{name} {verdicts.get(name, 'not run')}" for name in cls.required) + "".join(f"; {name} {state}" for name, state in sorted(verdicts.items()) if name not in cls.required and state != "success")

    @classmethod
    def main_commits(cls, count=40): return cls.run("gh", "api", f"repos/{cls.repository}/commits?sha=main&per_page={count}", "--jq", ".[].sha").split()

    @classmethod
    def status(cls):
        commits, seen = cls.main_commits(100), {}
        for path, revision, fixed in cls.locks():
            if revision not in seen: seen[revision] = cls.passed(revision)
            behind = commits.index(revision) if revision in commits else None
            print(f"{revision[:8]}  {'main' if behind == 0 else f'{behind} behind' if behind is not None else 'over 100 behind':16s} {'passed' if seen[revision][0] else 'NOT PASSED':10s} {path}{'  (revision written in flake.nix)' if fixed else ''}")
        for revision, (ok, verdicts) in seen.items(): print(f"  {revision[:8]}: {cls.words(verdicts)}")
        print(f"{len(seen)} revision{'s' if len(seen) != 1 else ''} in {len(cls.locks())} locks")
        return 0 if len(seen) == 1 and all(ok for ok, verdicts in seen.values()) else 1

    @classmethod
    def newest_passed(cls):
        for revision in cls.main_commits():
            if cls.passed(revision)[0]: return revision
        sys.exit(f"btrc-pin: none of the newest 40 commits on main passed {' and '.join(cls.required)}")

    @classmethod
    def bump(cls, revision):
        revision = cls.run("gh", "api", f"repos/{cls.repository}/commits/{revision}", "--jq", ".sha") if revision else cls.newest_passed()
        ok, verdicts = cls.passed(revision)
        if not ok: sys.exit(f"btrc-pin: {revision[:8]} did not pass: {cls.words(verdicts)}")
        print(f"{revision[:8]} passed: {cls.words(verdicts)}")
        for path, current, fixed in cls.locks():
            directory = path.parent
            if current == revision:
                print(f"  already there   {path}")
                continue
            if cls.run("git", "status", "--porcelain", "--", "flake.lock", "flake.nix", cwd=directory): sys.exit(f"btrc-pin: {directory} has uncommitted changes to its flake; nothing more was touched")
            if fixed:
                flake = directory / "flake.nix"
                flake.write_text(re.sub(rf"(github:{re.escape(cls.repository)}/)[0-9a-f]{{40}}", rf"\g<1>{revision}", flake.read_text()))
                cls.run("nix", "flake", "update", "btrc", cwd=directory)
            else: cls.run("nix", "flake", "lock", "--override-input", "btrc", f"github:{cls.repository}/{revision}", cwd=directory)
            print(f"  {current[:8]} -> {revision[:8]}  {path}")
        return 0

    @classmethod
    def main(cls, arguments):
        while arguments and arguments[0] in ("--require", "--root", "--repository"):
            if len(arguments) < 2: sys.exit(USAGE)
            if arguments[0] == "--require": cls.required = [name.strip() for name in arguments[1].split(",") if name.strip()]
            elif arguments[0] == "--root": cls.roots = [Path(root).expanduser() for root in arguments[1].split(",")]
            else: cls.repository = arguments[1]
            arguments = arguments[2:]
        if arguments[:1] == ["status"] and len(arguments) == 1: return cls.status()
        if arguments[:1] == ["bump"] and len(arguments) <= 2: return cls.bump(arguments[1] if len(arguments) == 2 else "")
        print(USAGE)
        return 0 if arguments[:1] in (["help"], ["--help"], ["-h"]) else 1


if __name__ == "__main__": sys.exit(Pins.main(sys.argv[1:]))
