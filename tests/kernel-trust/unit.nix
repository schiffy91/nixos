{ flake ? builtins.getFlake "git+file://${toString ../..}", inputs ? flake.inputs, system ? builtins.currentSystem }:
let
  pkgs = import inputs.nixpkgs { inherit system; };
  verifier = import ../../config/system/nixos/boot/kernel-trust/package.nix { inherit pkgs inputs; };
in pkgs.runCommand "module-trust-signature-tests" {
  nativeBuildInputs = with pkgs; [ python3 openssl zstd ];
} ''
  python3 ${./verify.py} ${verifier}/bin/check-module-trust
  touch "$out"
''
