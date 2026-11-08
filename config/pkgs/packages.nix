{ config, lib, ... }:
let apps = config.settings.apps;
in {
  environment.systemPackages = lib.mkIf apps.enable (
    apps.packages
    ++ lib.optionals apps.dev.enable apps.dev.packages
    ++ lib.optionals apps.gaming.enable apps.gaming.packages
    ++ lib.optionals apps.utils.enable apps.utils.packages
  );
}
