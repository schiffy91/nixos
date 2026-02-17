{ config, pkgs, lib, ... }:
let
  rocksmithMods = pkgs.stdenvNoCC.mkDerivation {
    name = "rocksmith-mods";
    src = ./.;
    installPhase = ''
      mkdir -p $out
      cp $src/RS_ASIO.dll $src/avrt.dll $src/RS_ASIO.ini $out/
      cp $src/xinput1_3.dll $src/RSMods.ini $out/
      cp $src/wineasio32.dll $src/wineasio32.dll.so $out/
    '';
  };
  gameDir = "${config.home.homeDirectory}/.local/share/Steam/steamapps/common/Rocksmith2014";
  prefixDir = "${config.home.homeDirectory}/.local/share/Steam/steamapps/compatdata/221680/pfx";
in {
  home.activation.rocksmith = lib.hm.dag.entryAfter [ "writeBoundary" ] ''
    if [ -d "${gameDir}" ]; then
      # Deploy RS_ASIO + RSMods to game directory
      cp -f ${rocksmithMods}/RS_ASIO.dll "${gameDir}/"
      cp -f ${rocksmithMods}/avrt.dll "${gameDir}/"
      cp -f ${rocksmithMods}/RS_ASIO.ini "${gameDir}/"
      cp -f ${rocksmithMods}/xinput1_3.dll "${gameDir}/"
      cp -f ${rocksmithMods}/RSMods.ini "${gameDir}/"

      # Deploy wineasio to Proton prefix
      if [ -d "${prefixDir}" ]; then
        cp -f ${rocksmithMods}/wineasio32.dll "${prefixDir}/drive_c/windows/syswow64/"
        mkdir -p "${config.home.homeDirectory}/.local/lib/wine/i386-unix"
        cp -f ${rocksmithMods}/wineasio32.dll.so "${config.home.homeDirectory}/.local/lib/wine/i386-unix/"

        # Register wineasio in Wine registry
        if ! grep -q "WineASIO" "${prefixDir}/system.reg" 2>/dev/null; then
          cat >> "${prefixDir}/system.reg" << 'REGEOF'

[Software\\ASIO\\WineASIO]
"CLSID"="{48D0C522-BFCC-45CC-8B84-17F25F33E6E8}"
"Description"="WineASIO Driver"

[Software\\Classes\\CLSID\\{48D0C522-BFCC-45CC-8B84-17F25F33E6E8}]
@="WineASIO Object"

[Software\\Classes\\CLSID\\{48D0C522-BFCC-45CC-8B84-17F25F33E6E8}\\InprocServer32]
@="wineasio32.dll"
"ThreadingModel"="Apartment"
REGEOF
        fi

        # Set DLL override for wineasio32
        if ! grep -q "wineasio32" "${prefixDir}/user.reg" 2>/dev/null; then
          sed -i '/\[Software\\\\Wine\\\\DllOverrides\]/a "wineasio32"="native"' "${prefixDir}/user.reg" 2>/dev/null || true
        fi
      fi

      # Configure Rocksmith.ini
      if [ -f "${gameDir}/Rocksmith.ini" ]; then
        sed -i 's/^ExclusiveMode=.*/ExclusiveMode=1/' "${gameDir}/Rocksmith.ini"
        sed -i 's/^Win32UltraLowLatencyMode=.*/Win32UltraLowLatencyMode=1/' "${gameDir}/Rocksmith.ini"
      fi
    fi
  '';
}
