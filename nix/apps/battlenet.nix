{ config, pkgs, lib, steam, ... }:
let
  enabled = config.settings.apps.enable
    && config.settings.apps.battlenet.enable
    && config.settings.apps.steam.enable
    && pkgs.stdenv.hostPlatform.isx86_64;
  user = config.settings.users.admin.username;
  home = "/home/${user}";
  prefix = "${home}/Games/Battle.net/prefix";
  proton = "${home}/.local/share/Steam/compatibilitytools.d/${steam.proton.customName}";
  exe = "${prefix}/drive_c/Program Files (x86)/Battle.net/Battle.net Launcher.exe";
  legacyDxvkConfig = "${prefix}/drive_c/Program Files (x86)/Battle.net/dxvk.conf";
  iconPath = "${home}/.local/share/icons/hicolor/256x256/apps/battlenet.png";
  primary = lib.findFirst (o: o.primary) null config.settings.desktop.outputs;
  scaleFactor = if primary == null then 1.0 else primary.scaleFactor;
  logPixels = builtins.floor (96.0 * scaleFactor + 0.5);
  launcher = pkgs.writeShellApplication {
    name = "battlenet";
    runtimeInputs = [ pkgs.coreutils pkgs.gawk pkgs.umu-launcher ];
    text = ''
        mkdir -p "${prefix}"
        EXE="''${1:-${exe}}"
        LOG_PIXELS="${toString logPixels}"
        PROTON_RUNTIME_LOG_DIR="''${XDG_RUNTIME_DIR:-/tmp}/battlenet-proton"
        mkdir -p "$PROTON_RUNTIME_LOG_DIR"
        rm -f "$PROTON_RUNTIME_LOG_DIR"/steam-battlenet.log "$PROTON_RUNTIME_LOG_DIR"/battlenet-wrapper.log

        # Plasma starts desktop entries without a terminal. Keep stdout/stderr
        # open on a file so umu/pressure-vessel do not inherit a dead pipe
        # during Battle.net's CEF handoff.
        if ! [ -t 1 ]; then
          exec >> "$PROTON_RUNTIME_LOG_DIR/battlenet-wrapper.log" 2>&1
        fi

        # Stop a stale prefix wineserver before editing user.reg; otherwise Wine
        # may keep the previous DPI in its registry cache and rewrite the file.
        WINE_SERVER="${proton}/files/bin-wow64/wineserver"
        if [ -x "$WINE_SERVER" ]; then
          WINEPREFIX="${prefix}" "$WINE_SERVER" -k >/dev/null 2>&1 || true
        fi

        sync_builtin() {
          ARCH="$1"
          WINDOWS_DIR="$2"
          DLL="$3"
          SRC="${proton}/files/lib/wine/$ARCH-windows/$DLL"
          DST="${prefix}/drive_c/windows/$WINDOWS_DIR/$DLL"
          if [ -e "$SRC" ]; then
            install -Dm644 "$SRC" "$DST"
          fi
        }
        for DLL in dcomp.dll dxgi.dll ntdll.dll secur32.dll winevulkan.dll win32u.dll winewayland.drv explorer.exe; do
          sync_builtin x86_64 system32 "$DLL"
          sync_builtin i386 syswow64 "$DLL"
        done

        rm -f "${legacyDxvkConfig}"
        cd "$HOME"
        set_reg_dword() {
          REG_FILE="$1"
          REG_SECTION="$2"
          REG_VALUE="$3"
          REG_HEX="$4"
          [ -f "$REG_FILE" ] || return 0
          REG_TMP=$(mktemp)
          REG_SECTION="$REG_SECTION" REG_VALUE="$REG_VALUE" REG_HEX="$REG_HEX" \
          awk '
            BEGIN {
              section = "[" ENVIRON["REG_SECTION"] "]"
              name = "\"" ENVIRON["REG_VALUE"] "\""
              line = name "=dword:" ENVIRON["REG_HEX"]
            }
            $0 == section {
              in_section = 1
              updated = 0
              saw_section = 1
              print
              next
            }
            in_section && /^\[/ {
              if (!updated) print line
              in_section = 0
            }
            in_section && index($0, name "=dword:") == 1 {
              print line
              updated = 1
              next
            }
            { print }
            END {
              if (in_section && !updated) print line
              if (!saw_section) {
                print ""
                print section
                print line
              }
            }
          ' "$REG_FILE" > "$REG_TMP" && mv "$REG_TMP" "$REG_FILE"
        }
        DPI_HEX=$(printf '%08x' "$LOG_PIXELS")
        set_reg_dword "${prefix}/user.reg" "Control Panel\\\\Desktop" LogPixels "$DPI_HEX"
        set_reg_dword "${prefix}/user.reg" "Software\\\\Wine\\\\Fonts" LogPixels "$DPI_HEX"
        set_reg_dword "${prefix}/system.reg" "System\\\\ControlSet001\\\\Hardware Profiles\\\\Current\\\\Software\\\\Fonts" LogPixels "$DPI_HEX"
        EXTRA_ARGS=(
          --high-dpi-support=1
        )
        CEF_SCALE="${toString scaleFactor}"
        if [ "$CEF_SCALE" != "1" ] && [ "$CEF_SCALE" != "1.0" ]; then
          EXTRA_ARGS+=(--force-device-scale-factor="$CEF_SCALE")
        fi
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
          WINE_SNI_ICON_NAME=battlenet \
          PROTON_ENABLE_WAYLAND=1 \
          PROTON_ENABLE_HDR=1 \
          DXVK_HDR=1 \
          DXVK_LOG_LEVEL="''${DXVK_LOG_LEVEL:-none}" \
          ENABLE_HDR_WSI=1 \
          umu-run "$EXE" "''${EXTRA_ARGS[@]}"
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
    src = ./battlenet-tests/battlenet-capture-window.c;
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
      ${pkgs.coreutils}/bin/install -d -o ${user} "$(dirname ${iconPath})"
      tmp=$(${pkgs.coreutils}/bin/mktemp --suffix=.ico)
      png=$(${pkgs.coreutils}/bin/mktemp --suffix=.png)
      if ${pkgs.icoutils}/bin/wrestool -x -t 14 -o "$tmp" "${exe}" 2>/dev/null; then
        for w in 256 128 64 48 32; do
          if ${pkgs.icoutils}/bin/icotool -x -w $w -o "$png" "$tmp" 2>/dev/null && [ -s "$png" ]; then
            ${pkgs.coreutils}/bin/install -m644 -o ${user} "$png" "${iconPath}"
            break
          fi
        done
      fi
      ${pkgs.coreutils}/bin/rm -f "$tmp" "$png"
    fi
  '';
}
