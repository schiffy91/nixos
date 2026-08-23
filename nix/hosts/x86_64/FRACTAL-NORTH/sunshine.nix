{ config, pkgs, lib, ... }:
let
  enabled = config.settings.apps.enable && config.settings.apps.gaming.enable && config.settings.apps.sunshine.enable;
  virtualDisplay = config.settings.apps.sunshine.virtualDisplay;
  primary = lib.findFirst (output: output.primary) null config.settings.desktop.outputs;
  primaryOutput =
    if virtualDisplay.primaryOutput != ""
    then virtualDisplay.primaryOutput
    else if primary != null
    then primary.name
    else "DP-1";
  virtualOutput = "Virtual-${virtualDisplay.name}";
  startVirtualDisplay = pkgs.writeShellApplication {
    name = "sunshine-start-virtual-display";
    runtimeInputs = with pkgs; [
      coreutils
      jq
      kdePackages.krfb
      kdePackages.libkscreen
    ];
    text = ''
      set -euo pipefail

      export QT_QPA_PLATFORM=wayland

      name="${virtualDisplay.name}"
      output="${virtualOutput}"
      primary="${primaryOutput}"
      state_dir="''${XDG_RUNTIME_DIR:-/tmp}/sunshine-virtual-display"
      state_file="$state_dir/state.json"
      pid_file="$state_dir/krfb-virtualmonitor.pid"

      width="''${SUNSHINE_CLIENT_WIDTH:-${toString virtualDisplay.defaultWidth}}"
      height="''${SUNSHINE_CLIENT_HEIGHT:-${toString virtualDisplay.defaultHeight}}"
      fps="''${SUNSHINE_CLIENT_FPS:-${toString virtualDisplay.defaultFps}}"
      fps="''${fps%.*}"
      fps="''${fps:-${toString virtualDisplay.defaultFps}}"
      refresh_mhz=$((fps * 1000))
      hdr="''${SUNSHINE_CLIENT_HDR:-false}"
      case "''${hdr,,}" in
        true|1|yes|on) hdr_enabled=true ;;
        *) hdr_enabled=false ;;
      esac

      mkdir -p "$state_dir"

      kscreen-doctor --json \
        | jq --arg primary "$primary" --arg output "$output" '{
            primary: $primary,
            outputs: [
              .outputs[]
              | select(.enabled and .name != $output)
              | {
                  name,
                  currentModeId,
                  pos,
                  scale,
                  hdr,
                  wcg,
                  priority
                }
            ]
          }' > "$state_file"

      if [ -s "$pid_file" ]; then
        kill "$(cat "$pid_file")" 2>/dev/null || true
        rm -f "$pid_file"
      fi

      password="$(od -An -tx1 -N16 /dev/urandom | tr -d ' \n')"
      krfb-virtualmonitor \
        --resolution "''${width}x''${height}" \
        --name "$name" \
        --password "$password" \
        --scale "${virtualDisplay.scale}" \
        --port "${toString virtualDisplay.port}" &
      echo "$!" > "$pid_file"

      for _ in $(seq 1 ${toString virtualDisplay.settleSeconds}); do
        sleep 1
        if kscreen-doctor --json | jq -e --arg output "$output" '.outputs[] | select(.name == $output and .connected)' > /dev/null; then
          break
        fi
      done

      if ! kscreen-doctor --json | jq -e --arg output "$output" '.outputs[] | select(.name == $output and .connected)' > /dev/null; then
        kill "$(cat "$pid_file")" 2>/dev/null || true
        rm -f "$pid_file"
        echo "Virtual display $output did not appear" >&2
        exit 1
      fi

      kscreen-doctor "output.$output.addCustomMode.$width.$height.$refresh_mhz.full" || true

      args=(
        "output.$output.enable"
        "output.$output.scale.${virtualDisplay.scale}"
        "output.$output.position.${virtualDisplay.position}"
        "output.$output.mode.''${width}x''${height}@''${fps}"
        "output.$output.priority.1"
      )

      if [ "$hdr_enabled" = true ]; then
        args+=("output.$output.hdr.enable")
        args+=("output.$output.wcg.enable")
      else
        args+=("output.$output.hdr.disable")
        args+=("output.$output.wcg.disable")
      fi

      if ${lib.boolToString virtualDisplay.disablePrimaryOnStream}; then
        args+=("output.$primary.disable")
      else
        args+=("output.$primary.priority.2")
      fi

      kscreen-doctor "''${args[@]}"
    '';
  };
  stopVirtualDisplay = pkgs.writeShellApplication {
    name = "sunshine-stop-virtual-display";
    runtimeInputs = with pkgs; [
      coreutils
      jq
      kdePackages.libkscreen
    ];
    text = ''
      set -euo pipefail

      export QT_QPA_PLATFORM=wayland

      state_dir="''${XDG_RUNTIME_DIR:-/tmp}/sunshine-virtual-display"
      state_file="$state_dir/state.json"
      pid_file="$state_dir/krfb-virtualmonitor.pid"

      if [ -s "$state_file" ]; then
        args=()

        while IFS=$'\t' read -r name mode x y scale hdr wcg priority; do
          [ -n "$name" ] || continue

          args+=("output.$name.enable")

          if [ -n "$mode" ] && [ "$mode" != "null" ]; then
            args+=("output.$name.mode.$mode")
          fi

          if [ -n "$x" ] && [ "$x" != "null" ] && [ -n "$y" ] && [ "$y" != "null" ]; then
            args+=("output.$name.position.$x,$y")
          fi

          if [ -n "$scale" ] && [ "$scale" != "null" ]; then
            args+=("output.$name.scale.$scale")
          fi

          if [ "$hdr" = "true" ]; then
            args+=("output.$name.hdr.enable")
          elif [ "$hdr" = "false" ]; then
            args+=("output.$name.hdr.disable")
          fi

          if [ "$wcg" = "true" ]; then
            args+=("output.$name.wcg.enable")
          elif [ "$wcg" = "false" ]; then
            args+=("output.$name.wcg.disable")
          fi

          if [ -n "$priority" ] && [ "$priority" != "null" ]; then
            args+=("output.$name.priority.$priority")
          fi
        done < <(jq -r '.outputs[] | [.name, .currentModeId, .pos.x, .pos.y, .scale, .hdr, .wcg, .priority] | @tsv' "$state_file")

        if [ "''${#args[@]}" -gt 0 ]; then
          kscreen-doctor "''${args[@]}" || true
        fi
      else
        kscreen-doctor "output.${primaryOutput}.enable" "output.${primaryOutput}.priority.1" || true
      fi

      if [ -s "$pid_file" ]; then
        kill "$(cat "$pid_file")" 2>/dev/null || true
        rm -f "$pid_file"
      fi

      rm -f "$state_file"
    '';
  };
  stopSteamVirtualDisplay = pkgs.writeShellApplication {
    name = "sunshine-stop-steam-virtual-display";
    runtimeInputs = with pkgs; [
      steam
      util-linux
    ];
    text = ''
      set +e
      ${stopVirtualDisplay}/bin/sunshine-stop-virtual-display
      setsid steam steam://close/bigpicture
      exit 0
    '';
  };
in lib.mkIf enabled {
  services.sunshine = {
    enable = true;
    openFirewall = false;
    autoStart = true;
    capSysAdmin = false;
    settings = {
      capture = "kwin";
      output_name = virtualOutput;
    };
    applications = {
      env.PATH = lib.makeBinPath (with pkgs; [
        coreutils
        steam
        util-linux
      ]);
      apps = [
        {
          name = "Desktop";
          image-path = "desktop.png";
          prep-cmd = [{
            do = "${startVirtualDisplay}/bin/sunshine-start-virtual-display";
            undo = "${stopVirtualDisplay}/bin/sunshine-stop-virtual-display";
          }];
        }
        {
          name = "Steam Big Picture";
          detached = [ "${pkgs.util-linux}/bin/setsid ${pkgs.steam}/bin/steam steam://open/bigpicture" ];
          image-path = "steam.png";
          prep-cmd = [{
            do = "${startVirtualDisplay}/bin/sunshine-start-virtual-display";
            undo = "${stopSteamVirtualDisplay}/bin/sunshine-stop-steam-virtual-display";
          }];
        }
      ];
    };
  };
  settings.networking.ports.tcp = [ 47984 47989 47990 48010 ];
  settings.networking.ports.udp = (lib.range 47998 48000) ++ (lib.range 8000 8010);

  environment.systemPackages = [
    pkgs.kdePackages.krfb
    pkgs.kdePackages.libkscreen
    startVirtualDisplay
    stopVirtualDisplay
  ];
}
