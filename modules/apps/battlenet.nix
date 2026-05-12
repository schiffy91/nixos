{ config, pkgs, lib, ... }:
let
  user = config.settings.user.admin.username;
  home = "/home/${user}";
  prefix = "${home}/Games/Battle.net/prefix";
  proton = "${home}/.local/share/Steam/compatibilitytools.d/GE-Proton10-34";
  exe = "${prefix}/drive_c/Program Files (x86)/Battle.net/Battle.net Launcher.exe";
  iconPath = "${home}/.local/share/icons/hicolor/256x256/apps/battlenet.png";
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
  desktopItem = pkgs.makeDesktopItem {
    name = "battlenet";
    desktopName = "Battle.net";
    exec = "${battlenet}/bin/battlenet";
    icon = "battlenet";
    categories = [ "Game" ];
    comment = "Battle.net via Proton (native Wayland + HDR)";
    terminal = false;
    startupWMClass = "battle.net.exe";
  };
in {
  environment.systemPackages = [ battlenet desktopItem ];
  system.activationScripts.battlenetIcon = lib.stringAfter [ "users" ] ''
    if [ -f "${exe}" ] && [ ! -s "${iconPath}" ]; then
      ${pkgs.coreutils}/bin/install -d -o ${user} "$(dirname ${iconPath})"
      tmp=$(${pkgs.coreutils}/bin/mktemp --suffix=.ico)
      if ${pkgs.icoutils}/bin/wrestool -x -t 14 -o "$tmp" "${exe}" 2>/dev/null; then
        for w in 256 128 64 48 32; do
          if ${pkgs.icoutils}/bin/icotool -x -w $w -o "${iconPath}" "$tmp" 2>/dev/null && [ -s "${iconPath}" ]; then
            ${pkgs.coreutils}/bin/chown ${user} "${iconPath}"
            break
          fi
        done
      fi
      ${pkgs.coreutils}/bin/rm -f "$tmp"
    fi
  '';
}
