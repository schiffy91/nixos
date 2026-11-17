{ inputs ? (builtins.getFlake "git+file://${toString ../../..}").inputs
, system ? builtins.currentSystem
}:
let
  pkgs = import inputs.nixpkgs { inherit system; };
  programs = import ../../../config/pkgs/utils/drive/package.nix { inherit pkgs inputs; };
  contract = (inputs.btrc.lib.buildPackage pkgs) {
    name = "drive-store-contract";
    src = (import ../../lib/test-source.nix { inherit pkgs inputs; }) {
      root = ../../..;
      fileset = pkgs.lib.fileset.unions [
        ./Contract.btrc ../btrc.toml (pkgs.lib.fileset.fileFilter (file: file.hasExt "btrc" || file.hasExt "h" || file.name == "btrc.toml" || file.name == "btrc.lock") ../../../config/pkgs/utils/drive)
      ];
    };
    programs.drive-store-contract = "tests/drive/store/Contract.btrc";
  };
in (import ../../lib/vm-test.nix { inherit pkgs inputs; }) {
  name = "drive-store";
  nodes.machine = { ... }: { environment.systemPackages = [ programs.fake contract pkgs.curl ]; };
  scenario = ./Main.btrc;
}
