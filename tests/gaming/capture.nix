{ pkgs, inputs }:
let program = (inputs.btrc.lib.buildPackage pkgs) {
  name = "capture-evidence-tests";
  src = (import ../lib/test-source.nix { inherit pkgs inputs; }) {
    root = ../..;
    fileset = pkgs.lib.fileset.unions [ ./CaptureTests.btrc ../../config/pkgs/gaming/sunshine/hdr-screencast-test/Capture.btrc ];
  };
  programs.capture-tests = "tests/gaming/CaptureTests.btrc";
};
in pkgs.runCommand "capture-evidence-check" {} ''
  ${program}/bin/capture-tests
  touch "$out"
''
