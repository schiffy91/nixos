{ config, pkgs, lib, ... }:
let
  user = config.settings.users.admin.username;
  home = config.settings.users.admin.homeDirectory;
  mountPoint = "${home}/Drive";
  remote = "gdrive:";
  configFile = "${home}/.config/rclone/rclone.conf";
  fusermount3 = "/run/wrappers/bin/fusermount3";
  prepareMount = pkgs.writeShellScript "rclone-drive-prepare" ''
    set -eu

    # A failed rclone stop can leave a stale FUSE endpoint that makes mkdir/stat
    # report "Transport endpoint is not connected" during the next activation.
    if ! ${pkgs.coreutils}/bin/stat ${lib.escapeShellArg mountPoint} >/dev/null 2>&1; then
      ${fusermount3} -uz ${lib.escapeShellArg mountPoint} >/dev/null 2>&1 || true
    fi

    ${pkgs.coreutils}/bin/mkdir -p ${lib.escapeShellArg mountPoint}
  '';
  cleanupMount = pkgs.writeShellScript "rclone-drive-cleanup" ''
    set -eu

    ${fusermount3} -uz ${lib.escapeShellArg mountPoint} >/dev/null 2>&1 || true
  '';
  # An offline folder is a full local copy kept equal to its Drive folder by a two-way sync.
  # The first run has no listings to compare, so it copies what either side lacks (the newer
  # file wins) and deletes nothing; later runs carry changes and deletions both ways, keep
  # both versions of a file changed on both sides, and stop if a tenth of a side vanished.
  offline = config.settings.apps.utils.rclone.offline;
  offlineSync = name: folder: pkgs.writeShellScript "rclone-offline-${name}" ''
    set -eu
    state="$HOME/.cache/rclone/bisync/${name}"
    ${pkgs.coreutils}/bin/mkdir -p "$state"
    first=""
    if ! ${pkgs.coreutils}/bin/ls "$state"/*.lst >/dev/null 2>&1; then first="--resync --resync-mode newer"; fi
    exec ${pkgs.rclone}/bin/rclone bisync ${lib.escapeShellArg folder.local} ${lib.escapeShellArg "${remote}${folder.remote}"} \
      --config ${configFile} --workdir "$state" \
      --filter-from ${pkgs.writeText "rclone-offline-${name}-filters" (lib.concatMapStrings (pattern: "- ${pattern}\n") folder.exclude)} \
      --compare size,modtime --conflict-resolve newer --conflict-loser num --conflict-suffix conflict \
      --resilient --recover --max-lock 2m --max-delete 10 --create-empty-src-dirs \
      --fast-list --drive-skip-gdocs --transfers 4 --log-level INFO $first
  '';
in lib.mkIf (config.settings.apps.utils.rclone.enable) (lib.mkMerge [ {
  settings.disk.immutability.persist.paths = [ "${home}/.config/rclone" ];
  users.users.${user}.packages = [ pkgs.rclone ];

  systemd.user.services.rclone-drive = {
    description = "rclone mount: Google Drive at ~/Drive";
    after = [ "network-online.target" ];
    wants = [ "network-online.target" ];
    wantedBy = [ "default.target" ];
    unitConfig.ConditionUser = user;
    unitConfig.ConditionPathExists = configFile; # skip until `rclone config` is done
    environment.PATH = lib.mkForce "/run/wrappers/bin"; # need setuid fusermount3
    serviceConfig = {
      Type = "notify";
      ExecStartPre = "${prepareMount}";
      ExecStart = ''
        ${pkgs.rclone}/bin/rclone mount ${remote} ${mountPoint} \
          --config ${configFile} \
          --vfs-cache-mode full \
          --vfs-cache-max-age 24h \
          --dir-cache-time 1h \
          --umask 0022
      '';
      ExecStop = "${cleanupMount}";
      ExecStopPost = "${cleanupMount}";
      Restart = "on-failure";
      RestartSec = 10;
    };
  };
} {
  systemd.user.services = lib.mapAttrs' (name: folder: lib.nameValuePair "rclone-offline-${name}" {
    description = "rclone sync: ${folder.local} with Drive ${folder.remote}";
    after = [ "network-online.target" ];
    wants = [ "network-online.target" ];
    unitConfig.ConditionUser = user;
    unitConfig.ConditionPathExists = [ configFile ] ++ lib.optional (folder.marker != "") "${folder.local}/${folder.marker}";
    serviceConfig = {
      Type = "oneshot";
      ExecStart = "${offlineSync name folder}";
      Nice = 10;
      IOSchedulingClass = "idle";
    };
  }) offline;

  systemd.user.timers = lib.mapAttrs' (name: folder: lib.nameValuePair "rclone-offline-${name}" {
    description = "Keep ${folder.local} in sync with Drive";
    wantedBy = [ "timers.target" ];
    unitConfig.ConditionUser = user;
    timerConfig = {
      OnCalendar = folder.interval;
      OnStartupSec = "3min";
      RandomizedDelaySec = "5min";
      Persistent = true;
    };
  }) offline;
} ])
