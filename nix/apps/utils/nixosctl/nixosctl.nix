{ pkgs, config, lib, inputs, ... }:
let
  btrcpy = inputs.btrc.packages.${pkgs.stdenv.hostPlatform.system}.btrcpy;
  nixosctlBin = import ./package.nix {
    inherit pkgs lib btrcpy;
    inherit (config.settings.apps.nixosctl) configPath;
  };
in lib.mkIf (config.settings.apps.enable && config.settings.apps.utils.enable && config.settings.apps.nixosctl.enable) {
  environment.systemPackages = [ nixosctlBin ];
  # Rebuilds run as root, but fix-permissions gives this repository to the admin.
  # Root's mutable Git config disappears when the immutable root is reset.
  programs.git = {
    enable = true;
    config.safe.directory = [ "/etc/nixos" ];
  };
}
