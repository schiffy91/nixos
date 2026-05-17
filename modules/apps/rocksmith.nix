{ config, pkgs, lib, steam, ... }:
let
  user = config.settings.user.admin.username;
  home = "/home/${user}";
  sampleSize = config.settings.rocksmith.sampleSize;
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
    src = ./pkg-overrides/rocksmith;
    installPhase = ''
      mkdir -p $out
      cp $src/RS_ASIO.dll $src/avrt.dll $out/
      cp ${rsAsioIni} $out/RS_ASIO.ini
      cp $src/xinput1_3.dll $src/RSMods.ini $out/
      cp $src/wineasio32.dll $src/wineasio32.dll.so $out/
    '';
  };
  steamPath = "${home}/.local/share/Steam";
  steamAppsPath = "${steamPath}/steamapps";
  gamePath = "${steamAppsPath}/common/Rocksmith2014";
  protonPath = "${steamPath}/compatibilitytools.d/${steam.proton.name}/files";
  prefixPath = "${steamAppsPath}/compatdata/221680/pfx";
in {
  system.activationScripts.rocksmith = lib.stringAfter [ "users" ] ''
    export PATH="${pkgs.coreutils}/bin:${pkgs.gnused}/bin:${pkgs.util-linux}/bin:$PATH"
    if [ -d "${gamePath}" ]; then
      cp -f ${mods}/RS_ASIO.dll "${gamePath}/"
      cp -f ${mods}/avrt.dll "${gamePath}/"
      cp -f ${mods}/RS_ASIO.ini "${gamePath}/"
      cp -f ${mods}/xinput1_3.dll "${gamePath}/"
      cp -f ${mods}/RSMods.ini "${gamePath}/"

      if [ -d "${protonPath}/lib/wine" ]; then
        cp -f ${mods}/wineasio32.dll "${protonPath}/lib/wine/i386-windows/"
        cp -f ${mods}/wineasio32.dll.so "${protonPath}/lib/wine/i386-unix/wineasio32.dll.so"
      fi

      if [ -f "${prefixPath}/user.reg" ]; then
        sed -i '/"wineasio32"="native"/d' "${prefixPath}/user.reg" 2>/dev/null || true
      fi

      if [ -f "${gamePath}/Rocksmith.ini" ]; then
        sed -i 's/^ExclusiveMode=.*/ExclusiveMode=1/' "${gamePath}/Rocksmith.ini"
        sed -i 's/^Win32UltraLowLatencyMode=.*/Win32UltraLowLatencyMode=1/' "${gamePath}/Rocksmith.ini"
      fi
    fi
  '';
}
