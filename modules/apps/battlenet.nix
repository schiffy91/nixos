{ config, pkgs, lib, steam, ... }:
let
  user = config.settings.user.admin.username;
  home = "/home/${user}";
  prefix = "${home}/Games/Battle.net/prefix";
  proton = "${home}/.local/share/Steam/compatibilitytools.d/${steam.proton.scwhineName}";
  exe = "${prefix}/drive_c/Program Files (x86)/Battle.net/Battle.net Launcher.exe";
  iconPath = "${home}/.local/share/icons/hicolor/256x256/apps/battlenet.png";
  primary = lib.findFirst (o: o.primary) null config.settings.desktop.outputs;
  scaleFactor = if primary == null then 1.0 else primary.scaleFactor;
  logPixels = builtins.floor (96.0 * scaleFactor + 0.5);
  # scwhine: DXVK config — enableDummyCompositionSwapchain lets CEF's
  # IDXGIFactory2::CreateSwapChainForComposition return a real swap chain
  # instead of E_NOTIMPL, so BNet's launcher keeps the DComp path alive.
  dxvkConf = pkgs.writeText "scwhine-dxvk.conf" ''
    dxgi.enableDummyCompositionSwapchain = True
  '';
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
          PROTON_USE_WOW64=1 \
          WINE_SIMULATE_WRITECOPY=1 \
          DXVK_CONFIG_FILE="${dxvkConf}" \
          ${lib.optionalString waylandHdr ''
          PROTON_ENABLE_WAYLAND=1 \
          PROTON_ENABLE_HDR=1 \
          DXVK_HDR=1 \
          ENABLE_HDR_WSI=1 \
        ''}umu-run "$EXE" --high-dpi-support=1 --force-device-scale-factor=${toString scaleFactor}
      '';
      # CEF reads --force-device-scale-factor (and the matching LogPixels we
      # set in user.reg above) to size its DComp swap chain at full device-
      # pixel resolution. With LogPixels=240 (96*2.5) + scale=2.5, BNet's wine
      # HWND is created at 3000x2000 device pixels = 1200x800 logical, and
      # CEF renders the launcher at 2.5x detail. My subsurface viewport then
      # maps the 3000x2000 buffer to 1200x800 surface units, which the
      # compositor displays at 1:1 device pixels — crisp, no bilinear.
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
