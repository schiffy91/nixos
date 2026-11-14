{ pkgs, inputs }:
let
  build = import ../../../lib/btrc.nix { inherit pkgs inputs; };
  src = pkgs.lib.fileset.toSource {
    root = ./.;
    fileset = pkgs.lib.fileset.fileFilter (file: file.hasExt "btrc" || file.hasExt "h" || file.name == "btrc.toml" || file.name == "btrc.lock") ./.;
  };
in {
  drive = build {
    name = "drive";
    inherit src;
    programs.drive = "Main.btrc";
    runtimeInputs = [ pkgs.curl pkgs.util-linux ];  # every request is one curl process; setsid starts the question
    postInstall = "install -Dm644 ${./readers.deny} $out/share/drive/readers.deny";  # the default list of programs that never download
  };
  fake = build {  # the Drive API from memory, for tests
    name = "drive-fake";
    inherit src;
    programs.drive-fake = "testing/FakeDrive.btrc";
  };
}
