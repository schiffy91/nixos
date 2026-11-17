# What lint evaluates. This system's checks are left out: ci/checks.sh builds them, which evaluates them.
flake:
let
  derivations = output: builtins.concatLists (builtins.attrValues (builtins.mapAttrs (system: attrs:
    if output == "checks" && system == builtins.currentSystem then [ ]
    else map (name: "${output}.${system}.${name}.drvPath") (builtins.attrNames attrs)) (flake.${output} or { })));
in map (name: "nixosConfigurations.${name}.config.system.build.toplevel.type") (builtins.attrNames flake.nixosConfigurations)  # what `nix flake check` forces
  ++ derivations "packages" ++ derivations "checks"
