{ inputs ? (builtins.getFlake "git+file://${toString ../../..}").inputs
, system ? builtins.currentSystem
}:
let
  pkgs = import inputs.nixpkgs { inherit system; };
  programs = import ../../../config/pkgs/utils/drive/package.nix { inherit pkgs inputs; };
  contract = (import ../../../config/lib/btrc.nix { inherit pkgs inputs; }) {
    name = "drive-store-contract";
    src = pkgs.lib.fileset.toSource {
      root = ../../..;
      fileset = pkgs.lib.fileset.unions [
        ./Contract.btrc ../btrc.toml ../../lib/Check.btrc
        (pkgs.lib.fileset.fileFilter (file: file.hasExt "btrc" || file.hasExt "h" || file.name == "btrc.toml" || file.name == "btrc.lock") ../../../config/pkgs/utils/drive)
      ];
    };
    programs.drive-store-contract = "tests/drive/store/Contract.btrc";
  };
in (import ../../vm/test.nix { inherit pkgs inputs; }) {
  name = "drive-store";
  nodes.machine = { ... }: { environment.systemPackages = [ programs.fake contract pkgs.curl ]; };
  scenario = ./Main.btrc;
}
