{ inputs ? (builtins.getFlake "git+file://${toString ../../..}").inputs
, system ? builtins.currentSystem
}:
let
  pkgs = import inputs.nixpkgs { inherit system; };
  programs = import ../../../config/pkgs/utils/drive/package.nix { inherit pkgs inputs; };
  contract = (import ../../../config/lib/btrc.nix { inherit pkgs inputs; }) {
    name = "drive-remote-contract";
    src = pkgs.lib.fileset.toSource {
      root = ../../..;
      fileset = pkgs.lib.fileset.unions [
        ./Main.btrc ../../lib/Check.btrc
        (pkgs.lib.fileset.fileFilter (file: file.hasExt "btrc") ../../../config/pkgs/utils/drive/remote)
      ];
    };
    programs.drive-remote-contract = "tests/drive/remote/Main.btrc";
  };
in pkgs.runCommand "drive-remote" { nativeBuildInputs = [ pkgs.curl ]; } ''
  ${programs.fake}/bin/drive-fake 18473 > fake.log &
  fake=$!
  for attempt in $(seq 50); do grep -q READY fake.log && break; sleep 0.1; done
  mkdir scratch
  ${contract}/bin/drive-remote-contract 18473 "$PWD/scratch" || { kill $fake; exit 1; }
  kill $fake
  touch $out
''
