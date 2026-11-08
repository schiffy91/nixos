{ pkgs, config, lib, inputs, ... }:
let
  btrcpy = inputs.btrc.packages.${pkgs.stdenv.hostPlatform.system}.btrcpy;
  nixosctlBin = import ./package.nix {
    inherit pkgs lib btrcpy;
    inherit (config.settings.apps.utils.nixosctl) configPath;
  };
in {
  imports = [ ./ui ];
  config = lib.mkMerge [
    {
      settings.apps.utils.nixosctl.configPath = lib.mkIf (config.settings.apps.utils.nixosctl.settings != {})
        (lib.mkDefault (toString ((pkgs.formats.toml {}).generate "nixosctl.toml" config.settings.apps.utils.nixosctl.settings)));
    }
    (lib.mkIf (config.settings.apps.utils.nixosctl.enable) {
      environment.systemPackages = [ nixosctlBin ];
      # Rebuilds run as root, but fix-permissions gives this repository to the admin.
      # Root's mutable Git config disappears when the immutable root is reset.
      programs.git = {
        enable = true;
        config.safe.directory = [ "/etc/nixos" ];
      };
    })
  ];
}
