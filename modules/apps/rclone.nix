{ config, pkgs, lib, ... }:
let
  user = config.settings.user.admin.username;
  home = "/home/${user}";
  mountPoint = "${home}/Drive";
  remote = "gdrive:";
  configFile = "${home}/.config/rclone/rclone.conf";
in {
  environment.systemPackages = [ pkgs.rclone ];

  systemd.user.services.rclone-drive = {
    description = "rclone mount: Google Drive at ~/Drive";
    after = [ "network-online.target" ];
    wants = [ "network-online.target" ];
    wantedBy = [ "default.target" ];
    unitConfig.ConditionPathExists = configFile; # skip until `rclone config` is done
    environment.PATH = lib.mkForce "/run/wrappers/bin"; # need setuid fusermount3
    serviceConfig = {
      Type = "notify";
      ExecStartPre = "${pkgs.coreutils}/bin/mkdir -p ${mountPoint}";
      ExecStart = ''
        ${pkgs.rclone}/bin/rclone mount ${remote} ${mountPoint} \
          --config ${configFile} \
          --vfs-cache-mode full \
          --vfs-cache-max-age 24h \
          --dir-cache-time 1h \
          --umask 0022
      '';
      ExecStop = "${pkgs.fuse}/bin/fusermount -u ${mountPoint}";
      Restart = "on-failure";
      RestartSec = 10;
    };
  };
}
