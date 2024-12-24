{ inputs, lib, ...}:
{
  imports = [ inputs.disko.nixosModules.disko ]
          ++ lib.filter (p: lib.hasSuffix ".nix" p) (lib.filesystem.listFilesRecursive ./modules);
  nix.settings = {
    warn-dirty = false;
    experimental-features = [ "nix-command" "flakes" ];
  };
  system.stateVersion = "24.11";
}