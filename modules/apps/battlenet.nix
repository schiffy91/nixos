{ config, pkgs, lib, ... }:
let
  user = config.settings.user.admin.username;
  home = "/home/${user}";
  prefix = "${home}/Games/Battle.net/prefix";
  proton = "${home}/.local/share/Steam/compatibilitytools.d/GE-Proton10-34";
  exe = "${prefix}/drive_c/Program Files (x86)/Battle.net/Battle.net Launcher.exe";
  iconPath = "${home}/.local/share/icons/hicolor/256x256/apps/battlenet.png";
  primary = lib.findFirst (o: o.primary) null config.settings.desktop.outputs;
  scaleFactor = if primary == null then 1.0 else primary.scaleFactor;
  logPixels = builtins.floor (96.0 * scaleFactor + 0.5);
  mkLauncher = { name, label, waylandHdr }: rec {
    launcher = pkgs.writeShellApplication {
      inherit name;
      runtimeInputs = [ pkgs.umu-launcher pkgs.gnused ];
      text = ''
        mkdir -p "${prefix}"
        EXE="''${1:-${exe}}"
        cd "$HOME"
        if [ -f "${prefix}/user.reg" ]; then
          DPI_HEX=$(printf '%08x' ${toString logPixels})
          sed -i -E 's|"LogPixels"=dword:[0-9a-f]+|"LogPixels"=dword:'"$DPI_HEX"'|g' "${prefix}/user.reg"
        fi
        exec env \
          WINEPREFIX="${prefix}" \
          GAMEID=umu-battlenet \
          PROTONPATH="${proton}" \
          PROTON_USE_WOW64=0 \
          WINE_SIMULATE_WRITECOPY=1 \
          ${lib.optionalString waylandHdr ''
          PROTON_ENABLE_WAYLAND=1 \
          PROTON_ENABLE_HDR=1 \
          DXVK_HDR=1 \
          ENABLE_HDR_WSI=1 \
        ''}umu-run "$EXE" --force-device-scale-factor=${toString scaleFactor} --high-dpi-support=1
      '';
    };
    desktop = pkgs.makeDesktopItem {
      inherit name;
      desktopName = label;
      exec = "${launcher}/bin/${name}";
      icon = "battlenet";
      categories = [ "Game" ];
      comment = "Battle.net via Proton${lib.optionalString waylandHdr " (native Wayland + HDR)"}";
      terminal = false;
      startupWMClass = "battle.net.exe";
    };
  };
  wayland = mkLauncher { name = "battlenet"; label = "Battle.net"; waylandHdr = true; };
  x11 = mkLauncher { name = "battlenet-x11"; label = "Battle.net (X11)"; waylandHdr = false; };
in {
  environment.systemPackages = [ wayland.launcher wayland.desktop x11.launcher x11.desktop ];
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
