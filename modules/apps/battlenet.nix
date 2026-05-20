{ config, pkgs, lib, steam, ... }:
let
  user = config.settings.user.admin.username;
  home = "/home/${user}";
  prefix = "${home}/Games/Battle.net/prefix";
  proton = "${home}/.local/share/Steam/compatibilitytools.d/${steam.proton.scwhineName}";
  exe = "${prefix}/drive_c/Program Files (x86)/Battle.net/Battle.net Launcher.exe";
  legacyDxvkConfig = "${prefix}/drive_c/Program Files (x86)/Battle.net/dxvk.conf";
  iconPath = "${home}/.local/share/icons/hicolor/256x256/apps/battlenet.png";
  primary = lib.findFirst (o: o.primary) null config.settings.desktop.outputs;
  scaleFactor = if primary == null then 1.0 else primary.scaleFactor;
  logPixels = builtins.floor (96.0 * scaleFactor + 0.5);
  mkLauncher = { name, label, waylandHdr }: rec {
    launcher = pkgs.writeShellApplication {
      inherit name;
      runtimeInputs = [ pkgs.coreutils pkgs.gawk pkgs.umu-launcher pkgs.gnused ];
      text = ''
        mkdir -p "${prefix}"
        EXE="''${1:-${exe}}"
        LOG_PIXELS="''${BATTLE_NET_LOG_PIXELS:-${toString logPixels}}"

        # Stop a stale prefix wineserver before editing user.reg; otherwise Wine
        # may keep the previous DPI in its registry cache and rewrite the file.
        WINE_SERVER="${proton}/files/bin-wow64/wineserver"
        if [ -x "$WINE_SERVER" ]; then
          WINEPREFIX="${prefix}" "$WINE_SERVER" -k >/dev/null 2>&1 || true
        fi

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
        SCALE_FACTOR="''${BATTLE_NET_FORCE_SCALE:-$(${pkgs.gawk}/bin/awk -v dpi="$LOG_PIXELS" 'BEGIN { if (dpi <= 0) dpi = 96; printf "%.6g", dpi / 96 }')}"
        EXTRA_ARGS=(--high-dpi-support=1 --force-device-scale-factor="$SCALE_FACTOR")
        ${lib.optionalString waylandHdr ''
        ANGLE_BACKEND="''${BATTLE_NET_ANGLE_BACKEND:-}"
        if [ -n "$ANGLE_BACKEND" ]; then
          EXTRA_ARGS+=(--use-angle="$ANGLE_BACKEND")
        fi
        if [ -n "''${BATTLE_NET_DISABLE_GPU_COMPOSITING:-}" ]; then
          EXTRA_ARGS+=(--disable-gpu-compositing)
        fi
        ''}
        exec env \
          WINEPREFIX="${prefix}" \
          GAMEID=umu-battlenet \
          PROTONPATH="${proton}" \
          PROTON_USE_WOW64=1 \
          WINE_SIMULATE_WRITECOPY=1 \
          WINE_WAYLAND_HACKS=1 \
          WINE_SNI_ICON_NAME=battlenet \
          ${lib.optionalString waylandHdr ''
          PROTON_ENABLE_WAYLAND=1 \
          PROTON_ENABLE_HDR=1 \
          DXVK_HDR=1 \
          ENABLE_HDR_WSI=1 \
        ''}umu-run "$EXE" "''${EXTRA_ARGS[@]}"
      '';
      # Wine reads LogPixels from user.reg, keeping Qt, Win32, and CEF on one
      # DPI source. The matching Chromium scale flag is also passed so the
      # login, interstitial, and launcher CEF processes cannot fall back to 1x.
      # BATTLE_NET_FORCE_SCALE is left as an escape hatch for diagnostics.
      #
      # The default intentionally leaves Chromium/ANGLE on its D3D11 path so
      # the patched DComp/DXGI/Wayland bridge is exercised. BATTLE_NET_ANGLE_BACKEND
      # remains as a diagnostic override.
      #
      # BATTLE_NET_DISABLE_GPU_COMPOSITING is left as a fallback for diagnosing
      # CEF compositor regressions. The default keeps CEF's GPU compositor
      # enabled; games still use their normal Proton, DXVK, winevulkan, and HDR
      # paths.
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
