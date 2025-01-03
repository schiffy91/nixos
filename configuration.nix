{ lib, ...}:
{
  imports = lib.filter (path: lib.hasSuffix ".nix" path) (lib.filesystem.listFilesRecursive ./system);
  nix.settings.experimental-features = [ "nix-command" "flakes" ];
  system.stateVersion = "24.11";
}