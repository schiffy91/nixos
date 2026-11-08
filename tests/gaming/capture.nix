{ pkgs, inputs }:
let program = (import ../../config/lib/btrc.nix { inherit pkgs inputs; }) {
  name = "capture-evidence-tests";
  src = pkgs.lib.fileset.toSource {
    root = ../..;
    fileset = pkgs.lib.fileset.unions [ ./CaptureTests.btrc ../lib/Check.btrc ../../config/pkgs/gaming/sunshine/hdr-screencast-test/Capture.btrc ];
  };
  programs.capture-tests = "tests/gaming/CaptureTests.btrc";
};
in pkgs.runCommand "capture-evidence-check" {} ''
  ${program}/bin/capture-tests
  touch "$out"
''
