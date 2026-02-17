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
  steamDir = "${config.home.homeDirectory}/.local/share/Steam/steamapps";
  gameDir = "${steamDir}/common/Rocksmith2014";
  protonDir = "${steamDir}/common/Proton - Experimental/files";
  prefixDir = "${steamDir}/compatdata/221680/pfx";
in {
  home.activation.rocksmith = lib.hm.dag.entryAfter [ "writeBoundary" ] ''
    if [ -d "${gameDir}" ]; then
      # Deploy RS_ASIO + RSMods to game directory
      cp -f ${rocksmithMods}/RS_ASIO.dll "${gameDir}/"
      cp -f ${rocksmithMods}/avrt.dll "${gameDir}/"
      cp -f ${rocksmithMods}/RS_ASIO.ini "${gameDir}/"
      cp -f ${rocksmithMods}/xinput1_3.dll "${gameDir}/"
      cp -f ${rocksmithMods}/RSMods.ini "${gameDir}/"

      # Deploy wineasio as builtin Wine DLL (into Proton's own lib dirs)
      if [ -d "${protonDir}/lib/wine" ]; then
        cp -f ${rocksmithMods}/wineasio32.dll "${protonDir}/lib/wine/i386-windows/"
        cp -f ${rocksmithMods}/wineasio32.dll.so "${protonDir}/lib/wine/i386-unix/wineasio32.dll.so"
      fi

      # Remove stale "native" override that prevents builtin loading
      if [ -f "${prefixDir}/user.reg" ]; then
        sed -i '/"wineasio32"="native"/d' "${prefixDir}/user.reg" 2>/dev/null || true
      fi

      # Configure Rocksmith.ini
      if [ -f "${gameDir}/Rocksmith.ini" ]; then
        sed -i 's/^ExclusiveMode=.*/ExclusiveMode=1/' "${gameDir}/Rocksmith.ini"
        sed -i 's/^Win32UltraLowLatencyMode=.*/Win32UltraLowLatencyMode=1/' "${gameDir}/Rocksmith.ini"
      fi
    fi
  '';
}
