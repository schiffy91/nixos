{ config, pkgs, lib, ... }:
let
  cfg    = config.settings.sudolessAllowlist;
  active = tbl: lib.mapAttrsToList (k: _: k) (lib.filterAttrs (_: en: en) tbl);
in
lib.mkIf cfg.enable {
  environment.systemPackages = map (name: pkgs.${name}) (active cfg.packages);
  security.sudo.extraRules = [{
    users    = [ config.settings.user.admin.username ];
    commands = map (cmd: { command = "/run/current-system/sw/bin/${cmd}"; options = [ "NOPASSWD" ]; }) (active cfg.nopasswd);
  }];
}
