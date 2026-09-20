{ config, pkgs, lib, protonCustom, winePrefixDpi, ... }:
let
  enabled = config.settings.apps.enable
    && config.settings.apps.gaming.enable
    && config.settings.apps.battlenet.enable
    && config.settings.apps.steam.enable
    && pkgs.stdenv.hostPlatform.isx86_64;
  user = config.settings.users.admin.username;
  group = config.users.users.${user}.group;
  home = "/home/${user}";
  prefix = "${home}/Games/Battle.net/prefix";
  proton = protonCustom.path;
  exe = "${prefix}/drive_c/Program Files (x86)/Battle.net/Battle.net Launcher.exe";
  iconPath = "${home}/.local/share/icons/hicolor/256x256/apps/battlenet.png";
  launcher = pkgs.writeShellApplication {
    name = "battlenet";
    runtimeInputs = [ pkgs.coreutils pkgs.procps pkgs.gnugrep pkgs.umu-launcher winePrefixDpi.package ];
    text = ''
        mkdir -p "${prefix}"
        EXE="''${1:-${exe}}"
        PROTON_RUNTIME_LOG_DIR="''${XDG_RUNTIME_DIR:-/tmp}/battlenet-proton"
        mkdir -p "$PROTON_RUNTIME_LOG_DIR"
        rm -f "$PROTON_RUNTIME_LOG_DIR"/steam-battlenet.log "$PROTON_RUNTIME_LOG_DIR"/battlenet-wrapper.log

        # Plasma starts desktop entries without a terminal. Keep stdout/stderr
        # open on a file so umu/pressure-vessel do not inherit a dead pipe
        # during Battle.net's CEF handoff.
        if ! [ -t 1 ]; then
          exec >> "$PROTON_RUNTIME_LOG_DIR/battlenet-wrapper.log" 2>&1
        fi

        # One session per prefix: a previous launch's Agent keeps a wineserver
        # alive inside its own runtime container, where no host socket reaches
        # it, and a second server on the same prefix clobbers the registry.
        for pid in $(pgrep -u "$(id -u)"); do  # umu appends /pfx to the prefix it is given
          if tr '\0' '\n' 2>/dev/null < "/proc/$pid/environ" | grep -qE "^(WINEPREFIX|STEAM_COMPAT_DATA_PATH)=${prefix}(/pfx/?)?$"; then
            kill "$pid" 2>/dev/null || true
          fi
        done

        wine-prefix-dpi set --dpi ${toString winePrefixDpi.dpi} "${prefix}"
        cd "$HOME"
        EXTRA_ARGS=()  # CEF reads the prefix DPI like any Windows app; no scale flags
        # ENABLE_HDR_WSI=1 (vk_hdr_layer) intentionally absent: nvidia 610 does HDR natively and the
        # layer mis-tags winewayland's scRGB (D2R/WoW DX12 too dark). Re-add next to DXVK_HDR to undo.
        exec env \
          WINEPREFIX="${prefix}" \
          TMPDIR=/tmp \
          GAMEID=umu-battlenet \
          PROTONPATH="${proton}" \
          PROTON_LOG=1 \
          PROTON_LOG_DIR="$PROTON_RUNTIME_LOG_DIR" \
          WINEDEBUG="''${WINEDEBUG:--all}" \
          PROTON_USE_WOW64=1 \
          WINE_SIMULATE_WRITECOPY=1 \
          WINE_WAYLAND_HACKS=1 \
          PROTON_ENABLE_WAYLAND=1 \
          PROTON_ENABLE_HDR=1 \
          DXVK_HDR=1 \
          DXVK_LOG_LEVEL="''${DXVK_LOG_LEVEL:-none}" \
          umu-run "$EXE" "''${EXTRA_ARGS[@]}"  # games inherit this env, so HDR stays on for Diablo IV
    '';
  };
  desktop = pkgs.writeTextFile {
    name = "battlenet-desktop";
    destination = "/share/applications/battlenet.desktop";
    text = ''
      [Desktop Entry]
      Type=Application
      Version=1.5
      Name=Battle.net
      Comment=Play this game
      Exec=battlenet
      Icon=battlenet
      Terminal=false
      Categories=Game;
      StartupWMClass=battle.net.exe
    '';
  };
  captureHelper = pkgs.stdenv.mkDerivation {
    pname = "battlenet-capture-window";
    version = "1";
    src = ./tests/battlenet-capture-window.c;
    dontUnpack = true;
    nativeBuildInputs = [ pkgs.pkg-config ];
    buildInputs = [ pkgs.glib ];
    buildPhase = ''
      runHook preBuild
      cc "$src" -o battlenet-capture-window $(
        pkg-config --cflags --libs gio-2.0 gio-unix-2.0 glib-2.0
      )
      runHook postBuild
    '';
    installPhase = ''
      runHook preInstall
      install -Dm755 battlenet-capture-window "$out/libexec/battlenet-capture-window"
      install -Dm644 /dev/stdin "$out/share/applications/battlenet-capture-window.desktop" <<EOF
      [Desktop Entry]
      Type=Application
      Name=Battle.net Window Capture
      Exec=$out/libexec/battlenet-capture-window
      Icon=battlenet
      NoDisplay=true
      StartupNotify=false
      X-KDE-DBUS-Restricted-Interfaces=org.kde.kwin.Screenshot,org.kde.KWin.ScreenShot2
      X-KDE-Wayland-Interfaces=org_kde_plasma_window_management,zkde_screencast_unstable_v1
      EOF
      runHook postInstall
    '';
  };
in lib.mkIf enabled {
  users.users.${user}.packages = [ launcher desktop captureHelper ];
  system.activationScripts.battlenetIcon = lib.stringAfter [ "users" ] ''
    if [ -f "${exe}" ]; then
      ${pkgs.coreutils}/bin/install -d -o ${user} -g ${group} "$(dirname ${iconPath})"
      ${pkgs.coreutils}/bin/install -d -m 1777 /tmp
      scratch=$(${pkgs.coreutils}/bin/mktemp -d /tmp/battlenet-icon.XXXXXX)
      tmp="$scratch/icon.ico"
      png="$scratch/icon.png"
      if ${pkgs.icoutils}/bin/wrestool -x -t 14 -o "$tmp" "${exe}" 2>/dev/null; then
        for w in 256 128 64 48 32; do
          if ${pkgs.icoutils}/bin/icotool -x -w $w -o "$png" "$tmp" 2>/dev/null && [ -s "$png" ]; then
            ${pkgs.coreutils}/bin/install -m644 -o ${user} -g ${group} "$png" "${iconPath}"
            break
          fi
        done
      fi
      ${pkgs.coreutils}/bin/rm -rf "$scratch"
    fi
  '';
}
