{ lib, hosts, publicHosts, systems, diskOperations, homes }:
{ pkgs }:
let
  expectedHosts = [ "FRACTAL-NORTH" "STEAM-DECK-OLED" "QEMU-x86_64" "QEMU-aarch64" ];
  expectedLinux = lib.concatMap (name: map (target: "${name}-${target}") systems.bootableTargets)
    (expectedHosts ++ [ "FRACTAL-NORTH-VM" "STEAM-DECK-OLED-VM" ]);
  publicNames = map (host: host.outputName) publicHosts;
  testNames = map (host: hosts.hostOutputName host.module) hosts.simulated;
  homeOf = host: systems.darwinConfigurations.${host.name}.config.home-manager.users;
  failures =
    lib.optional (!(lib.all (name: builtins.elem name publicNames) expectedHosts))
      "host picker lost a physical Linux host"
    ++ lib.optional (lib.length publicNames != lib.length (lib.unique publicNames))
      "host picker output names are ambiguous"
    ++ lib.optional (!(lib.all (host:
      lib.hasSuffix "-linux" host.system && !(builtins.elem host.outputName testNames)) publicHosts))
      "host picker includes a Darwin or test-only host"
    ++ lib.optional (!(lib.all (host:
      lib.hasSuffix "/default.nix" host.path
      && builtins.pathExists (../../config/hosts + "/${host.path}")) publicHosts))
      "host picker paths do not resolve to default.nix entries"
    ++ lib.optional (!(lib.all (name: builtins.hasAttr name systems.nixosConfigurations) expectedLinux))
      "a public NixOS output was renamed or removed"
    ++ lib.optional (!(lib.all (name:
      builtins.hasAttr "${name}-Disk-Operation" diskOperations.diskoConfigurations) expectedHosts))
      "a public disk-operation output was renamed or removed"
    ++ lib.optional (!(systems.darwinConfigurations ? mbp21 && homes ? "alexanderschiffhauer@mbp21"))
      "the Mac system or standalone home output was renamed or removed"
    ++ lib.concatMap (host:
      let
        username = systems.darwinConfigurations.${host.name}.config.settings.users.admin.username;
        standalone = homes."${username}@${host.name}".activationPackage;
        integrated = (homeOf host).${username}.home.activationPackage;
      in lib.optional (standalone.drvPath != integrated.drvPath)
        "${host.name}: standalone Home Manager differs from its system declaration") hosts.darwin;
in {
  name = "host-layout";
  value = pkgs.runCommand "host-layout" { } (
    if failures == [ ] then "touch $out"
    else throw (lib.concatStringsSep "\n" failures));
}
