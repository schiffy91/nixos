{ inputs ? (builtins.getFlake "git+file://${toString ../../..}").inputs
, system ? builtins.currentSystem
}:
# Each safety rule is cut out of the client's source, one at a time, and the scenario that covers it is run
# against that client. The scenario has to fail, and at the step that is there for the rule.
let
  pkgs = import inputs.nixpkgs { inherit system; };
  inherit (pkgs) lib;
  cut = file: from: to: "substituteInPlace ${file} --replace-fail ${lib.escapeShellArg from} ${lib.escapeShellArg to}\n";
  mutants = [
    {
      rule = "a not-downloaded file is never handed to the kernel as it is";
      scenario = ../door;
      mutation = cut "door/Door.btrc" ''probe.attribute("state").equals("cloud") && !self.unguarded;'' "false;";
      at = "A read fetches its own range and nothing else";
      sees = "FAIL: machine: [ \"$(head -c 100 /mnt/drive/Media/big.bin | sha256sum)\"";
    }
    {
      rule = "a program reads only so much without a yes";
      scenario = ../readers;
      mutation = cut "engine/Readers.btrc" ''return use.files.len > self.budgetFiles || use.bytes > self.budgetBytes ? "ask" : "ok";'' ''return "ok";'';
      at = "With nobody to ask, a program that reads past its allowance is stopped there";
      sees = "FAIL: machine: expected failure, exit 0: cat /mnt/drive/Quiet/f*.bin";
    }
    {
      rule = "a file changed on both sides is kept twice";
      scenario = ../writes;
      mutation = cut "engine/Uploader.btrc" "action == PLAN_KEEP_BOTH && self.overwriteConflicts" "action == PLAN_KEEP_BOTH"
        + cut "engine/Mirror.btrc" "uploader != null && !uploader.overwriteConflicts &&" "false &&";
      at = "A file changed on both sides is kept twice";
      sees = "timed out waiting for ls /mnt/drive | grep -q 'conflict TEST'";
    }
    {
      rule = "many deletions made here wait for a yes";
      scenario = ../writes;
      mutation = cut "engine/Uploader.btrc" "if (!self.allowed && Planner.tooManyDeletions(" "if (false && Planner.tooManyDeletions(";
      at = "Many deletions at once wait for a yes, here";
      sees = "timed out waiting for drive status --runtime /run/drive-test | grep -q 'deletions held'";
    }
    {
      rule = "many deletions made on the Drive wait for a yes";
      scenario = ../writes;
      mutation = cut "engine/Mirror.btrc" "if (!self.allowDeletions && deletions > 0 && Planner.tooManyDeletions(" "if (false && Planner.tooManyDeletions(";
      at = "And on the Drive, however few changes it reports at a time";
      sees = "timed out waiting for drive status --runtime /run/drive-test | grep -q 'deletions held'";
    }
    {
      rule = "deletions held here are written down for the engine that starts next";
      scenario = ../held;
      mutation = cut "engine/Uploader.btrc" ''for id in self.removals { lines.push(f"removed {id}"); }'' "";
      at = "The engine is killed and started again, and both batches still wait";
      sees = "timed out waiting for drive held --runtime /run/drive-test | grep -q '  here$'";
    }
    {
      rule = "a batch the Drive deletes is counted on across a restart";
      scenario = ../held;
      mutation = cut "engine/Mirror.btrc" "self.streak = store.streak();" "self.streak = 0;";
      at = "The engine is killed and started again, and both batches still wait";
      sees = "timed out waiting for drive held --runtime /run/drive-test | grep -q 'on the Drive$'";
    }
  ];
  failed = mutant: pkgs.testers.testBuildFailure (import mutant.scenario { inherit inputs system; inherit (mutant) mutation; });
in pkgs.runCommand "drive-mutations" { passthru.runs = map failed mutants; } (''
  mkdir -p $out
'' + lib.concatMapStrings (mutant: ''
  log=${failed mutant}/testBuildFailure.log
  step="$(grep -F -B1 -- ${lib.escapeShellArg mutant.sees} "$log" | head -1)"  # the step the scenario was in when it failed
  if [ "$step" != ${lib.escapeShellArg mutant.at} ] || grep -q '^PASS' "$log"; then
    echo ${lib.escapeShellArg "Without the rule '${mutant.rule}' the scenario did not fail where it should:"}
    tail -30 "$log"
    exit 1
  fi
  printf '%s\n  in the step: %s\n  %s\n' ${lib.escapeShellArg "Without '${mutant.rule}' the scenario fails"} "$step" "$(grep -F -- ${lib.escapeShellArg mutant.sees} "$log" | head -1 | cut -c1-200)" | tee -a $out/report
'') mutants)
