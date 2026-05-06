{ settings, config, pkgs, lib, ... }:
let
  steam = import ../../lib/steam.nix { inherit pkgs; };
  sampleSize = settings.rocksmith.sampleSize;
  sampleRate = settings.rocksmith.sampleRate;
  rsAsioIni = pkgs.writeText "RS_ASIO.ini" ''
    [Config]
    EnableWasapiOutputs=0
    EnableWasapiInputs=0
    EnableAsio=1

    [Asio]
    BufferSizeMode=custom
    CustomBufferSize=${toString sampleSize}

    [Asio.Output]
    Driver=wineasio-rsasio
    BaseChannel=0
    AltBaseChannel=
    EnableSoftwareEndpointVolumeControl=1
    EnableSoftwareMasterVolumeControl=1
    SoftwareMasterVolumePercent=100
    EnableRefCountHack=

    [Asio.Input.0]
    Driver=wineasio-rsasio
    Channel=0
    EnableSoftwareEndpointVolumeControl=1
    EnableSoftwareMasterVolumeControl=1
    SoftwareMasterVolumePercent=100
    EnableRefCountHack=

    [Asio.Input.1]
    Driver=
    Channel=1
    EnableSoftwareEndpointVolumeControl=1
    EnableSoftwareMasterVolumeControl=1
    SoftwareMasterVolumePercent=100
    EnableRefCountHack=

    [Asio.Input.Mic]
    Driver=
    Channel=1
    EnableSoftwareEndpointVolumeControl=1
    EnableSoftwareMasterVolumeControl=1
    SoftwareMasterVolumePercent=100
    EnableRefCountHack=
  '';
  mods = pkgs.stdenvNoCC.mkDerivation {
    name = "rocksmith-mods";
    src = ./.;
    installPhase = ''
      mkdir -p $out
      cp $src/RS_ASIO.dll $src/avrt.dll $out/
      cp ${rsAsioIni} $out/RS_ASIO.ini
      cp $src/xinput1_3.dll $src/RSMods.ini $out/
      cp $src/wineasio32.dll $src/wineasio32.dll.so $out/
    '';
  };
  launchOptions = "LD_PRELOAD=/usr/lib32/libjack.so PIPEWIRE_LATENCY=${toString sampleSize}/${toString sampleRate} %command%";
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

      # Set Steam launch options
      ${steam.setLaunchOptions}/bin/set-steam-launch-options \
        --steam-path "${steamPath}" \
        --launch-options "${launchOptions}" \
        --app-id "221680"
    fi
  '';
}
