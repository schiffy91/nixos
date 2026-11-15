{ config, lib, pkgs, ... }: lib.mkIf (config.settings.apps.dev.git.enable) {
  programs.git.enable = true;
  environment.systemPackages = [
    pkgs.gh  # GitHub's own tool: CI logs and artifacts, pull requests
    (import ./ci-local/package.nix { inherit pkgs; })  # records a commit's local test passes for CI
  ];
  # Its sign-in: the account and host are here, the token is in the desktop's wallet where there is one.
  settings.disk.immutability.persist.paths = [ "${config.settings.users.admin.homeDirectory}/.config/gh" ];
}
