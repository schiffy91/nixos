{ pkgs }:
# ci-local against a repository of its own and a stand-in for `gh` that writes down what it is asked to post.
pkgs.runCommand "ci-local" { nativeBuildInputs = [ pkgs.git pkgs.python3 ]; } ''
  set -eu
  export HOME=$TMPDIR/home FLAG=$TMPDIR/flag POSTED=$TMPDIR/posted ORIGIN=$TMPDIR/origin.git
  mkdir -p "$HOME" $TMPDIR/bin
  tool() { python3 -P ${../../config/pkgs/dev/ci-local/ci-local.py} "$@"; }
  fail() { echo "FAIL: $1" >&2; exit 1; }
  cat > $TMPDIR/bin/gh <<'GH'
  #!/bin/sh
  commit=$(echo "$4" | sed 's|.*/||')
  git --git-dir "$ORIGIN" rev-parse -q --verify "refs/ci-local/$commit" > /dev/null && held=held || held=absent
  git --git-dir "$ORIGIN" rev-parse -q --verify refs/heads/main > /dev/null && branch=$(git --git-dir "$ORIGIN" rev-parse refs/heads/main) || branch=none
  echo "$* | $held | $branch" >> "$POSTED"
  GH
  chmod +x $TMPDIR/bin/gh
  export PATH=$TMPDIR/bin:$PATH
  git config --global user.name tester && git config --global user.email tester@example.invalid && git config --global init.defaultBranch main
  git config --global url."$ORIGIN".insteadOf "git@github.com:owner/project.git"
  git init -q --bare "$ORIGIN"
  git init -q $TMPDIR/work && cd $TMPDIR/work
  git remote add origin git@github.com:owner/project.git
  here=$(python3 -c 'import platform; print(("macos" if platform.system() == "Darwin" else platform.system().lower()) + "-" + platform.machine().lower())')
  mkdir ci
  cat > ci/local.json <<JSON
  { "jobs": {
    "passes": { "platform": "$here", "run": "true" },
    "fails": { "platform": "$here", "run": "false" },
    "elsewhere": { "platform": "another-platform", "run": "true" },
    "litters": { "platform": "$here", "run": "touch stray" },
    "turns": { "platform": "$here", "run": "test ! -e $FLAG" }
  } }
  JSON
  git add ci && git commit -qm first
  first=$(git rev-parse HEAD)

  echo "A pass is for a commit: nothing is recorded on a tree with anything else in it"
  touch extra
  tool run passes 2> $TMPDIR/refused && fail "ran on a dirty tree"
  grep -q "commit or remove them first" $TMPDIR/refused || fail "the refusal does not say why"
  rm extra

  echo "A pass is recorded, a failure is not, and another platform's job is refused"
  tool run passes turns
  test -s .git/ci-local/$first/passes.json && test -s .git/ci-local/$first/turns.json || fail "passes were not recorded"
  tool run fails && fail "a failing job reported success"
  test ! -e .git/ci-local/$first/fails.json || fail "a failure was recorded"
  tool run elsewhere 2> $TMPDIR/refused && fail "ran another platform's job"
  grep -q "would not stand in for it" $TMPDIR/refused || fail "the refusal does not name the platform"
  tool run litters 2> $TMPDIR/refused && fail "a job that changed the tree was recorded"
  test ! -e .git/ci-local/$first/litters.json && grep -q "left the tree changed" $TMPDIR/refused || fail "a job that changed the tree was recorded"
  rm stray
  tool status | grep -q "local/$here/passes .*not published" || fail "the status does not list the pass"

  echo "A push posts the passes before the branch moves, for that commit, and leaves nothing behind"
  tool push -q origin main
  test "$(git --git-dir "$ORIGIN" rev-parse refs/heads/main)" = "$first" || fail "the branch was not pushed"
  test "$(wc -l < "$POSTED")" = 2 || fail "expected two statuses: $(cat "$POSTED")"
  grep -q "^api --method POST repos/owner/project/statuses/$first -f state=success -f context=local/$here/passes -f description=passed on .* | held | none$" "$POSTED" || fail "the pass was not posted as expected: $(cat "$POSTED")"
  grep -q "context=local/$here/turns " "$POSTED" || fail "the second pass was not posted"
  test -z "$(git --git-dir "$ORIGIN" for-each-ref refs/ci-local)" || fail "the holding ref was left behind"
  tool status | grep -q "local/$here/passes .* published" || fail "the status does not say published"

  echo "A job that fails after its pass was published takes the pass back"
  touch "$FLAG"
  tool run turns && fail "the turned job reported success"
  test ! -e .git/ci-local/$first/turns.json || fail "the stale pass was kept"
  tail -1 "$POSTED" | grep -q "statuses/$first -f state=failure -f context=local/$here/turns " || fail "the failure was not posted: $(tail -1 "$POSTED")"

  echo "A commit with nothing recorded is pushed as it is"
  echo more > file && git add file && git commit -qm second
  before=$(wc -l < "$POSTED")
  tool push -q origin main 2>&1 | grep -q "nothing is recorded" || fail "did not say nothing is recorded"
  test "$(git --git-dir "$ORIGIN" rev-parse refs/heads/main)" = "$(git rev-parse HEAD)" && test "$(wc -l < "$POSTED")" = "$before" || fail "pushed wrongly or posted something"

  echo "The workflow skips exactly what the table names, and runs the same commands"
  table=${../../ci/local.json} workflow=${../../.github/workflows/checks.yml}
  for job in lint checks; do
    run=$(python3 -c 'import json, sys; print(json.load(open(sys.argv[1]))["jobs"][sys.argv[2]]["run"])' $table $job)
    where=$(python3 -c 'import json, sys; print(json.load(open(sys.argv[1]))["jobs"][sys.argv[2]]["platform"])' $table $job)
    grep -qF "run: $run" $workflow || fail "the workflow does not run '$run' for $job"
    test "$(grep -cF "'local/$where/$job'" $workflow)" = 2 || fail "the workflow does not skip $job on local/$where/$job at the step and at the job"
  done
  touch $out
''
