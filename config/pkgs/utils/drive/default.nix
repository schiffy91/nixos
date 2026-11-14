{ config, lib, pkgs, inputs, ... }:
let
  settings = config.settings.apps.utils.drive;
  user = config.settings.users.admin.username;
  home = config.settings.users.admin.homeDirectory;
  programs = import ./package.nix { inherit pkgs inputs; };
  drive = "${programs.drive}/bin/drive";
  runtime = "/run/drive";
  credentials = "${home}/.config/drive/credentials.json";
  places = lib.concatStringsSep " " (lib.mapAttrsToList (path: location: "--place ${lib.escapeShellArg "${path}=${location}"}") settings.placements
    ++ lib.mapAttrsToList (location: device: "--place-device ${lib.escapeShellArg "${location}=${device}"}") settings.placementDevices);
  deny = pkgs.writeText "drive-readers.deny" (builtins.readFile ./readers.deny + lib.concatMapStrings (name: "${name}\n") settings.readers.deny);
  engine = lib.concatStringsSep " " ([
    "${drive} engine --store ${lib.escapeShellArg settings.store} --runtime ${runtime} --credentials ${lib.escapeShellArg credentials}"
    "--machine ${lib.escapeShellArg config.networking.hostName} --quiet ${toString settings.quietSeconds} --follow ${toString settings.followSeconds}"
    "--deny-file ${deny} --budget-files ${toString settings.readers.budgetFiles} --ask ${programs.askSession}/bin/drive-ask-session"
    "--cache-bytes ${toString settings.cacheBytes} --free-floor ${toString settings.freeFloorBytes}"
    places
  ] ++ map (path: "--pin ${lib.escapeShellArg path}") settings.pinned
    ++ map (name: "--ignore ${lib.escapeShellArg name}") settings.ignore
    ++ lib.optional (settings.api != null) "--api ${lib.escapeShellArg settings.api}");
  # A door that died without unmounting leaves an endpoint that answers "not connected".
  prepare = pkgs.writeShellScript "drive-door-prepare" ''
    if ! ${pkgs.coreutils}/bin/stat ${lib.escapeShellArg settings.mountPoint} >/dev/null 2>&1; then
      /run/wrappers/bin/umount -l ${lib.escapeShellArg settings.mountPoint} >/dev/null 2>&1 || true
    fi
  '';
  # System units, not user ones: sleep freezes user.slice first, and a program waiting on a
  # mount whose server is already frozen can never freeze, so a suspend would fail.
  service = description: extra: lib.recursiveUpdate {
    inherit description;
    wantedBy = [ "multi-user.target" ];
    unitConfig.RequiresMountsFor = [ home settings.store ];
    unitConfig.StartLimitIntervalSec = 0; # keep trying; a unit that gave up would leave ~/Drive missing until someone noticed
    serviceConfig = {
      User = user;
      RuntimeDirectory = "drive";
      RuntimeDirectoryMode = "0700";
      RuntimeDirectoryPreserve = true;
      Restart = "always";
      RestartSec = 1;
    };
  } extra;
in lib.mkIf settings.enable {
  assertions = [ {
    assertion = !config.settings.apps.utils.rclone.enable;
    message = "settings.apps.utils.drive and settings.apps.utils.rclone both serve ~/Drive; enable one.";
  } ];
  environment.systemPackages = [ programs.drive ];
  settings.apps.utils.drive.ignore = [ "*.part" "*.crdownload" "*.kate-swp" ".~lock.*" ".goutputstream-*" ".#*" ];  # other modules add their programs' names
  environment.sessionVariables = { DRIVE_RUNTIME = runtime; DRIVE_MOUNT = settings.mountPoint; };
  settings.disk.immutability.persist.paths = [ "/var/lib/drive" "${home}/.config/drive" ];
  systemd.tmpfiles.rules = [ "d /var/lib/drive 0755 root root -" "d ${settings.store} 0700 ${user} users -" ];

  # The door: the mount itself. Small, no network. It needs the one capability that
  # mounting and handing opened files to the kernel take, and nothing else.
  systemd.services.drive-door = service "Drive: the mount at ${settings.mountPoint}" {
    serviceConfig = {
      ExecStartPre = [ "+${prepare}" "${pkgs.coreutils}/bin/mkdir -p ${lib.escapeShellArg settings.mountPoint}" ];
      ExecStart = "${drive} door --store ${lib.escapeShellArg settings.store} --mount ${lib.escapeShellArg settings.mountPoint} --runtime ${runtime} ${places}";
      AmbientCapabilities = [ "CAP_SYS_ADMIN" ];
      CapabilityBoundingSet = [ "CAP_SYS_ADMIN" ];
      LimitNOFILE = 1048576; # one descriptor for every file the kernel remembers
    };
  };

  # The engine: everything that talks to Google. It may stop and start without the mount going away.
  systemd.services.drive-engine = service "Drive: sync with Google Drive" {
    after = [ "network-online.target" "drive-door.service" ];
    wants = [ "network-online.target" "drive-door.service" ];
    unitConfig.ConditionPathExists = credentials; # nothing to do until `drive auth`
    environment.DRIVE_RUNTIME = runtime; # for the window that asks about a program
    serviceConfig = {
      ExecStart = engine;
      Nice = 5;
    };
  };

  systemd.services.drive-push = lib.mkIf settings.push (service "Drive: wait for the Drive to announce changes" {
    after = [ "network-online.target" "drive-engine.service" ];
    wants = [ "network-online.target" ];
    unitConfig.ConditionPathExists = credentials;
    serviceConfig = {
      ExecStart = "${drive} push listen --quiet --runtime ${runtime} --credentials ${lib.escapeShellArg credentials} --machine ${lib.escapeShellArg config.networking.hostName}";
      RestartSec = 30;
    };
  });
}
