{ config, pkgs, lib, ... }:
let
  cfg    = config.settings.sudolessAllowlist;
  pkgMap = { tcpdump = pkgs.tcpdump; ethtool = pkgs.ethtool; python3 = pkgs.python3; moonlightQt = pkgs.moonlight-qt; };
  active = tbl: lib.mapAttrsToList (k: _: k) (lib.filterAttrs (_: en: en) tbl);
in
lib.mkIf cfg.enable {
  environment.systemPackages = map (name: pkgMap.${name}) (active cfg.packages);
  security.sudo.extraRules = [{
    users    = [ config.settings.user.admin.username ];
    commands = map (cmd: { command = "/run/current-system/sw/bin/${cmd}"; options = [ "NOPASSWD" ]; }) (active cfg.nopasswd);
  }];
}
