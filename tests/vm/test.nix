{ pkgs, inputs }:
{ scenario, extraFiles ? [], settings ? {}, graphical ? false, ... }@definition:
let
  inherit (pkgs) lib;
  testing = import (pkgs.path + "/nixos/lib/testing") { inherit lib; };
  evaluated = (testing.evalTest {
    imports = [ (removeAttrs definition [ "scenario" "extraFiles" "settings" "graphical" ]) ];
    hostPkgs = pkgs;
    node.pkgs = pkgs;
    testScript = "";
    skipLint = true;
    skipTypeCheck = true;
  }).config;
  descriptor = pkgs.writeText "vm-lab.json" (builtins.toJSON {
    settings = if builtins.isFunction settings then settings evaluated.nodes else settings;
    inherit (evaluated.driverConfiguration) vlans;
    vms = lib.mapAttrs (name: vm: vm // {
      headless = !lib.any (option: lib.hasPrefix "-display " option) evaluated.nodes.${name}.virtualisation.qemu.options;
    }) evaluated.driverConfiguration.vms;
  });
  program = (import ../../config/lib/btrc.nix { inherit pkgs inputs; }) {
    name = "${evaluated.name}-scenario";
    src = lib.fileset.toSource {
      root = ../..;
      fileset = lib.fileset.unions ([
        ./. ../lib ../../config/lib/BackgroundProcess.btrc
        ../../ci/Main.btrc ../../ci/lib ../../ci/native.h ../../ci/btrc.toml ../../ci/btrc.lock
        scenario
      ] ++ extraFiles);
    };
    programs.scenario = lib.removePrefix (toString ../.. + "/") (toString scenario);
    buildInputs = lib.optionals graphical [ pkgs.libvncserver pkgs.zlib ];
    linkFlags = lib.optional graphical "-lvncclient";
  };
  runner = pkgs.writeShellApplication {
    name = "btrc-vm-test";
    runtimeInputs = [ pkgs.socat pkgs.vde2 pkgs.coreutils pkgs.bash ] ++ lib.optional graphical pkgs.tesseract;
    text = ''
      export out="$1"
      mkdir -p "$out"
      exec ${program}/bin/scenario ${descriptor} "$out"
    '';
  };
in pkgs.runCommand evaluated.name {
  nativeBuildInputs = [ pkgs.socat pkgs.vde2 pkgs.coreutils pkgs.bash ];
  requiredSystemFeatures = [ "kvm" ];
  passthru = { inherit descriptor program runner; nodes = evaluated.nodes; };
} ''
  mkdir -p "$out"
  ${runner}/bin/btrc-vm-test "$out"
''
