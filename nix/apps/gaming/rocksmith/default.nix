{ config, pkgs, lib, steam, ... }:
let
  enabled = config.settings.apps.enable
    && config.settings.apps.gaming.enable
    && config.settings.apps.rocksmith.enable
    && config.settings.apps.steam.enable
    && pkgs.stdenv.hostPlatform.isx86_64;
  user = config.settings.users.admin.username;
  home = "/home/${user}";
  sampleSize = config.settings.rocksmith.sampleSize;
  cdlcPath = config.settings.rocksmith.cdlcPath;
  slopsmithConfigPath = config.settings.rocksmith.slopsmith.configPath;
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
    src = ./assets;
    installPhase = ''
      mkdir -p $out
      cp $src/RS_ASIO.dll $src/avrt.dll $out/
      cp ${rsAsioIni} $out/RS_ASIO.ini
      cp $src/xinput1_3.dll $src/RSMods.ini $out/
    '';
  };
  steamPath = "${home}/.local/share/Steam";
  steamAppsPath = "${steamPath}/steamapps";
  gamePath = "${steamAppsPath}/common/Rocksmith2014";
  dlcPath = "${gamePath}/dlc";
  protonPath = "${steamPath}/compatibilitytools.d/${steam.proton.customName}/files";
  prefixPath = "${steamAppsPath}/compatdata/221680/pfx";
  slopsmith = pkgs.callPackage ../slopsmith/package.nix {
    rocksmithDlc = dlcPath;
    configDir = slopsmithConfigPath;
    port = config.settings.rocksmith.slopsmith.port;
  };
in lib.mkIf enabled {
  virtualisation.podman.enable = true;
  environment.systemPackages = [ slopsmith ];

  system.activationScripts.rocksmith = lib.stringAfter [ "users" "protonCustomCompatTool" ] ''
    export PATH="${pkgs.coreutils}/bin:${pkgs.findutils}/bin:${pkgs.gnused}/bin:${pkgs.gnugrep}/bin:${pkgs.util-linux}/bin:$PATH"
    install -d -o ${user} -g users "${cdlcPath}" "${slopsmithConfigPath}"

    if [ -d "${gamePath}" ]; then
      install -d -o ${user} -g users "${dlcPath}"
      cp -f ${mods}/RS_ASIO.dll "${gamePath}/"
      cp -f ${mods}/avrt.dll "${gamePath}/"
      cp -f ${mods}/RS_ASIO.ini "${gamePath}/"
      cp -f ${mods}/xinput1_3.dll "${gamePath}/"
      cp -f ${mods}/RSMods.ini "${gamePath}/"

      find "${dlcPath}" -maxdepth 1 -type l -lname "${cdlcPath}/*" -delete
      while IFS= read -r -d "" song; do
        ln -sfn "$song" "${dlcPath}/$(basename "$song")"
      done < <(find "${cdlcPath}" -maxdepth 1 -type f -iname '*.psarc' -print0)

      if [ -d "${protonPath}/lib/wine" ]; then
        cp -f "${protonPath}/lib/wine/i386-windows/wineasio32.dll" "${gamePath}/wineasio32.dll"

        if [ -d "${prefixPath}/drive_c/windows/syswow64" ]; then
          cp -f "${protonPath}/lib/wine/i386-windows/wineasio32.dll" \
            "${prefixPath}/drive_c/windows/syswow64/wineasio32.dll"
        fi
        if [ -d "${prefixPath}/drive_c/windows/system32" ] && [ -f "${protonPath}/lib/wine/x86_64-windows/wineasio64.dll" ]; then
          cp -f "${protonPath}/lib/wine/x86_64-windows/wineasio64.dll" \
            "${prefixPath}/drive_c/windows/system32/wineasio64.dll"
        fi
      fi

      if [ -f "${prefixPath}/system.reg" ] && ! grep -q 'Software\\\\ASIO\\\\WineASIO' "${prefixPath}/system.reg"; then
        cat >> "${prefixPath}/system.reg" <<'EOF'

[Software\\ASIO\\WineASIO]
"CLSID"="{48D0C522-BFCC-45CC-8B84-17F25F33E6E8}"
"Description"="WineASIO Driver"

[Software\\Classes\\CLSID\\{48D0C522-BFCC-45CC-8B84-17F25F33E6E8}]
@="WineASIO Object"

[Software\\Classes\\CLSID\\{48D0C522-BFCC-45CC-8B84-17F25F33E6E8}\\InprocServer32]
@="wineasio32.dll"
"ThreadingModel"="Apartment"
EOF
      fi

      if [ -f "${prefixPath}/user.reg" ]; then
        sed -i '/"wineasio32"="native"/d' "${prefixPath}/user.reg" 2>/dev/null || true
        if ! grep -q 'Software\\\\Wine\\\\WineASIO' "${prefixPath}/user.reg"; then
          cat >> "${prefixPath}/user.reg" <<EOF

[Software\\Wine\\WineASIO]
"Autostart server"=dword:00000000
"Connect to hardware"=dword:00000001
"Fixed buffersize"=dword:00000001
"Number of inputs"=dword:00000010
"Number of outputs"=dword:00000010
"Preferred buffersize"=dword:$(printf '%08x' ${toString sampleSize})
EOF
        fi
      fi

      if [ -f "${gamePath}/Rocksmith.ini" ]; then
        sed -i 's/^ExclusiveMode=.*/ExclusiveMode=1/' "${gamePath}/Rocksmith.ini"
        sed -i 's/^Win32UltraLowLatencyMode=.*/Win32UltraLowLatencyMode=1/' "${gamePath}/Rocksmith.ini"
      fi
    fi
  '';
}
