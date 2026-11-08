{ lib }:
let
  platforms = { "aarch64/mbp21" = "aarch64-darwin"; };
  directories = root: builtins.attrNames (lib.filterAttrs (_: type: type == "directory") (builtins.readDir root));
  discover = root: lib.concatMap (architecture:
    lib.concatMap (name:
      let module = root + "/${architecture}/${name}/default.nix";
      in lib.optional (builtins.pathExists module) {
        inherit name module;
        system = platforms."${architecture}/${name}" or "${architecture}-linux";
      }) (directories (root + "/${architecture}"))) (directories root);
  registered = discover ../hosts;
  physical = lib.filter (host: lib.hasSuffix "-linux" host.system) registered;
  darwin = lib.filter (host: lib.hasSuffix "-darwin" host.system) registered;
  simulated = discover ../../tests/hosts;
  linux = physical ++ simulated;
  hostOf = module: lib.findFirst (host: host.module == module)
    (throw "Host module is not registered: ${toString module}") (linux ++ darwin);
  hostSystem = module: (hostOf module).system;
  hostOutputName = module:
    let
      host = hostOf module;
      duplicates = lib.filter (candidate: candidate.name == host.name) linux;
      architecture = lib.removeSuffix "-linux" host.system;
    in if lib.length duplicates > 1 then "${host.name}-${architecture}" else host.name;
in {
  inherit physical simulated linux darwin hostOf hostSystem hostOutputName;
  systems = lib.unique (map (host: host.system) linux);
}
