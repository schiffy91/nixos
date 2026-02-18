{ config, pkgs, lib, ... }:
let
  mods = pkgs.stdenvNoCC.mkDerivation {
    name = "rocksmith-mods";
    src = ./.;
    installPhase = ''
      mkdir -p $out
      cp $src/RS_ASIO.dll $src/avrt.dll $src/RS_ASIO.ini $out/
      cp $src/xinput1_3.dll $src/RSMods.ini $out/
      cp $src/wineasio32.dll $src/wineasio32.dll.so $out/
    '';
  };
  bufferSize = 64;
  sampleRate = 48000;
  launchOptions = "LD_PRELOAD=/usr/lib32/libjack.so PIPEWIRE_LATENCY=${toString bufferSize}/${toString sampleRate} %command%";
  setLaunchOptions = pkgs.writers.writePython3Bin "set-launch-options" {
    libraries = [ pkgs.python3Packages.vdf ];
  } ''
    import glob
    import sys
    import vdf

    APP_ID, OPTS = "221680", sys.argv[1]
    for path in glob.glob(
        sys.argv[2] + "/userdata/*/config/localconfig.vdf"
    ):
        with open(path) as f:
            config = vdf.load(f)
        sd = config.setdefault
        root = sd("UserLocalConfigStore", {})
        steam = root.setdefault("Software", {}) \
            .setdefault("Valve", {}) \
            .setdefault("Steam", {})
        apps = steam.get("apps", steam.get("Apps"))
        if apps is None:
            apps = steam.setdefault("apps", {})
        app = apps.setdefault(APP_ID, {})
        if app.get("LaunchOptions") == OPTS:
            continue
        app["LaunchOptions"] = OPTS
        with open(path, "w") as f:
            vdf.dump(config, f, pretty=True)
  '';
  steamPath = "${config.home.homeDirectory}/.local/share/Steam";
  steamAppsPath = "${steamPath}/steamapps";
  gamePath = "${steamAppsPath}/common/Rocksmith2014";
  protonPath = "${steamAppsPath}/common/Proton - Experimental/files";
  prefixPath = "${steamAppsPath}/compatdata/221680/pfx";
in {
  home.activation.rocksmith = lib.hm.dag.entryAfter [ "writeBoundary" ] ''
    if [ -d "${gamePath}" ]; then
      # Deploy RS_ASIO + RSMods to game directory
      cp -f ${mods}/RS_ASIO.dll "${gamePath}/"
      cp -f ${mods}/avrt.dll "${gamePath}/"
      cp -f ${mods}/RS_ASIO.ini "${gamePath}/"
      cp -f ${mods}/xinput1_3.dll "${gamePath}/"
      cp -f ${mods}/RSMods.ini "${gamePath}/"

      # Deploy wineasio as builtin Wine DLL (into Proton's own lib dirs)
      if [ -d "${protonPath}/lib/wine" ]; then
        cp -f ${mods}/wineasio32.dll "${protonPath}/lib/wine/i386-windows/"
        cp -f ${mods}/wineasio32.dll.so "${protonPath}/lib/wine/i386-unix/wineasio32.dll.so"
      fi

      # Remove stale "native" override that prevents builtin loading
      if [ -f "${prefixPath}/user.reg" ]; then
        sed -i '/"wineasio32"="native"/d' "${prefixPath}/user.reg" 2>/dev/null || true
      fi

      # Configure Rocksmith.ini
      if [ -f "${gamePath}/Rocksmith.ini" ]; then
        sed -i 's/^ExclusiveMode=.*/ExclusiveMode=1/' "${gamePath}/Rocksmith.ini"
        sed -i 's/^Win32UltraLowLatencyMode=.*/Win32UltraLowLatencyMode=1/' "${gamePath}/Rocksmith.ini"
      fi

      # Set Steam launch options (skip if Steam is running — it overwrites on exit)
      if ! ${pkgs.procps}/bin/pgrep -x steam > /dev/null 2>&1; then
        ${setLaunchOptions}/bin/set-launch-options "${launchOptions}" "${steamPath}"
      else
        echo "Rocksmith 2014: Steam is running — close it and nixos-rebuild switch to set launch options" >&2
      fi
    fi
  '';
}
