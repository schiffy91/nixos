{ pkgs, inputs }:
{
  configureSteamApps = import ../package.nix { inherit pkgs inputs; };
}
