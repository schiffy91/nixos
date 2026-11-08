{ config, inputs, lib, pkgs, ... }:
let
  user = config.settings.users.admin.username;
  helper = import ./sudo/package.nix { inherit pkgs inputs; admin = user; };
in {
  assertions = [{
    assertion = builtins.match "[a-zA-Z_][a-zA-Z0-9_-]*" user != null;
    message = "The administrator name must be safe as a literal sudoers user.";
  }];
  environment.systemPackages = [ helper ];
  security.sudo.wheelNeedsPassword = lib.mkDefault true;
  systemd.tmpfiles.rules = [ "d /run/nixosctl-sudo 0755 root root -" ];
  security.sudo.extraConfig = lib.mkAfter ''
    @includedir /run/nixosctl-sudo
    # Last match wins: grants cannot make their own renewal passwordless.
    ${user} ALL=(root) PASSWD: /run/current-system/sw/bin/nixosctl-sudo
    ${user} ALL=(root) NOPASSWD: /run/current-system/sw/bin/nixosctl-sudo status
    Defaults!/run/current-system/sw/bin/nixosctl-sudo timestamp_timeout=0
  '';
}
