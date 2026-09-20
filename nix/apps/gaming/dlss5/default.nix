# DLSS 5 neural rendering for Proton games: upstream DLSS5VKLayer packaged as-is,
# its Windows helper run under proton-custom through umu, toggled from the tray.
# The layer is fail-open, so games carry VKLayer_DLSS5=1 permanently and the
# helper service decides whether frames are processed. Delete this folder and
# the settings switch to remove it entirely.
{ config, pkgs, lib, inputs, protonCustom, ... }:
let
  enabled = config.settings.apps.enable
    && config.settings.apps.gaming.enable
    && config.settings.apps.steam.enable
    && config.settings.apps.dlss5.enable
    && config.settings.desktop.enable
    && pkgs.stdenv.hostPlatform.isx86_64;
  user = config.settings.users.admin.username;
  home = "/home/${user}";
  dlss5 = pkgs.callPackage ./package.nix { };
  stateDir = "${home}/.local/state/dlssnr";
  shm = "${stateDir}/shm.bin";  # under $HOME: pressure-vessel gives every game a private /tmp
  binaries = "${home}/.local/share/dlssnr/binaries";  # user-supplied nvngx_dlssnr.dll, never packaged
  launchEnv = "VKLayer_DLSS5=1 DLSSNR_SHM=${shm} DLSSNR_LOG=${stateDir}/layer.log";
  runner = pkgs.writeShellApplication {  # proton-custom's wine is GE's build, so it needs the runtime container
    name = "dlss5-helper-runner";
    runtimeInputs = [ pkgs.umu-launcher ];
    text = ''
      cd "$HOME"  # bwrap chdirs into the caller's cwd, which the container may not have
      exec env GAMEID=umu-dlssnr PROTONPATH="${protonCustom.path}" \
        PROTON_ENABLE_NVAPI=1 DLSSNR_SKIP_NVAPI=1 TMPDIR=/tmp umu-run "$@"
    '';
  };
  helperConfig = pkgs.writeText "dlssnr-config.ini" ''
    runner_type=custom
    runner_path=${runner}/bin/dlss5-helper-runner
    binaries=${binaries}
    shm=${shm}
    log=${stateDir}/helper.log
  '';
  ctl = pkgs.writeShellApplication {
    name = "dlss5ctl";
    runtimeInputs = [ pkgs.systemd pkgs.gnugrep dlss5 pkgs.kdePackages.konsole ];
    text = ''
      case "''${1:-}" in
        status)   systemctl --user is-active --quiet dlss5-helper ;;
        start)    systemctl --user start dlss5-helper ;;
        stop)     systemctl --user stop dlss5-helper ;;
        toggle)   if systemctl --user is-active --quiet dlss5-helper; then systemctl --user stop dlss5-helper; else systemctl --user start dlss5-helper; fi ;;
        style)    dlssnr-shmctl "${shm}" set style "$2" ;;
        style-is) dlssnr-shmctl "${shm}" status 2>/dev/null | grep -qx "style=$2" ;;
        set)      dlssnr-shmctl "${shm}" set "$2" "$3" ;;
        settings) dlssnr-shmctl "${shm}" status ;;
        log)      konsole -e tail -n 200 -f "${stateDir}/helper.log" ;;
        launch-env) echo "${launchEnv}" ;;
        *) echo "usage: dlss5ctl status|start|stop|toggle|style N|style-is N|set KEY V|settings|log|launch-env" >&2; exit 2 ;;
      esac
    '';
  };
  btrcpy = inputs.btrc.packages.${pkgs.stdenv.hostPlatform.system}.btrcpy;
  tray = pkgs.stdenv.mkDerivation {
    name = "dlss5-tray";
    src = ./.;
    nativeBuildInputs = [ btrcpy pkgs.pkg-config pkgs.makeWrapper ];
    buildInputs = [ pkgs.dbus ];  # Library.Tray binds libdbus-1 for its Linux provider
    dontConfigure = true;
    buildPhase = ''
      btrcpy --strict-imports tray.btrc -o dlss5-tray.c
      $CC -std=c11 -O2 dlss5-tray.c $(pkg-config --cflags --libs dbus-1) -lm -lpthread -o dlss5-tray
    '';
    installPhase = ''
      mkdir -p $out/bin
      cp dlss5-tray $out/bin/dlss5-tray
      wrapProgram $out/bin/dlss5-tray \
        --set DLSS5_TRAY_ICON "${pkgs.papirus-icon-theme}/share/icons/Papirus/64x64/apps/nvidia-settings.svg" \
        --prefix PATH : ${lib.makeBinPath [ ctl ]}
    '';
  };
in lib.mkMerge [
  {
    _module.args.dlss5 = { inherit launchEnv; package = dlss5; };  # games.nix opts a title in with launchEnv
  }
  (lib.mkIf enabled {
    hardware.graphics.extraPackages = [ dlss5 ];  # implicit-layer manifest lands in /run/opengl-driver
    users.users.${user}.packages = [ dlss5 ctl tray ];
    systemd.user.tmpfiles.users.${user}.rules = [ "d ${stateDir} 0700 - - -" ];  # the layer refuses a shm dir others can read
    systemd.user.services.dlss5-helper = {
      description = "DLSS 5 neural rendering helper (off until the tray starts it)";
      after = [ "graphical-session.target" ];
      partOf = [ "graphical-session.target" ];
      unitConfig.ConditionUser = user;
      serviceConfig = {
        Type = "forking";
        WorkingDirectory = home;
        PIDFile = "/tmp/dlssnr-%U/helper.pid";  # the upstream script pins its pid file there
        ExecStartPre = [
          "${pkgs.coreutils}/bin/install -d -m 700 ${stateDir}"
          "${pkgs.coreutils}/bin/install -Dm644 ${helperConfig} %h/.config/dlssnr/config.ini"
        ];
        ExecStart = "${dlss5}/bin/dlssnr-helper start";
        ExecStop = "${dlss5}/bin/dlssnr-helper stop";
        KillMode = "mixed";
        TimeoutStartSec = 60;
      };
    };
    systemd.user.services.dlss5-tray = {
      description = "DLSS 5 system tray";
      wantedBy = [ "graphical-session.target" ];
      partOf = [ "graphical-session.target" ];
      after = [ "graphical-session.target" ];
      unitConfig.ConditionUser = user;
      serviceConfig = {
        ExecStart = "${tray}/bin/dlss5-tray";
        Restart = "on-failure";
        RestartSec = 3;
      };
    };
  })
]
