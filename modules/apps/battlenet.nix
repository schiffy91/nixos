{ config, pkgs, lib, ... }:
let
  prefix   = "${config.home.homeDirectory}/Games/Battle.net/prefix";
  proton   = "${config.home.homeDirectory}/.local/share/Steam/compatibilitytools.d/GE-Proton10-34";
  exe      = "${prefix}/drive_c/Program Files (x86)/Battle.net/Battle.net Launcher.exe";
  iconPath = "${config.home.homeDirectory}/.local/share/icons/hicolor/256x256/apps/battlenet.png";
  battlenet = pkgs.writeShellApplication {
    name = "battlenet";
    runtimeInputs = [ pkgs.umu-launcher ];
    text = ''
      mkdir -p "${prefix}"
      EXE="''${1:-${exe}}"
      cd "$HOME"
      exec env \
        WINEPREFIX="${prefix}" \
        GAMEID=umu-battlenet \
        PROTONPATH="${proton}" \
        PROTON_ENABLE_WAYLAND=1 \
        PROTON_ENABLE_HDR=1 \
        DXVK_HDR=1 \
        ENABLE_HDR_WSI=1 \
        WINE_SIMULATE_WRITECOPY=1 \
        umu-run "$EXE"
    '';
  };
in {
  home.packages = [ battlenet ];
  home.activation.battlenetIcon = lib.hm.dag.entryAfter [ "writeBoundary" ] ''
    if [ -f "${exe}" ] && [ ! -s "${iconPath}" ]; then
      mkdir -p "$(dirname ${iconPath})"
      tmp=$(mktemp --suffix=.ico)
      if ${pkgs.icoutils}/bin/wrestool -x -t 14 -o "$tmp" "${exe}" 2>/dev/null; then
        for w in 256 128 64 48 32; do
          if ${pkgs.icoutils}/bin/icotool -x -w $w -o "${iconPath}" "$tmp" 2>/dev/null && [ -s "${iconPath}" ]; then
            break
          fi
        done
      fi
      rm -f "$tmp"
    fi
  '';

  xdg.desktopEntries.battlenet = {
    name = "Battle.net";
    exec = "${battlenet}/bin/battlenet";
    icon = "battlenet";
    categories = [ "Game" ];
    comment = "Battle.net via Proton (native Wayland + HDR)";
    terminal = false;
    settings.StartupWMClass = "battle.net.exe";
  };
}
