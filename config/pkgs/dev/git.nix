{ config, inputs, lib, pkgs, ... }: lib.mkIf (config.settings.apps.dev.git.enable) {
  programs.git.enable = true;
  environment.systemPackages = [
    pkgs.gh  # GitHub's own tool: CI logs and artifacts, pull requests
    inputs.ci-local.packages.${pkgs.stdenv.hostPlatform.system}.default  # records a commit's local test passes for CI
  ];
  settings.disk.immutability.persist.paths = [
    "${config.settings.users.admin.homeDirectory}/.config/gh"  # its sign-in: account and host; the token is in the wallet
    "${config.settings.users.admin.homeDirectory}/Projects"  # working clones, the product repositories among them
  ];
}
