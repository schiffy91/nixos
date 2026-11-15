flake:
let
  derivations = output: builtins.concatLists (builtins.attrValues (builtins.mapAttrs (system: attrs:
    map (name: "${output}.${system}.${name}.drvPath") (builtins.attrNames attrs)) (flake.${output} or { })));
in map (name: "nixosConfigurations.${name}.config.system.build.toplevel.type") (builtins.attrNames flake.nixosConfigurations)  # what `nix flake check` forces
  ++ derivations "packages" ++ derivations "checks"
