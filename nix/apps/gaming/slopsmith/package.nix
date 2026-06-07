{ pkgs
, rocksmithDlc ? "$HOME/.local/share/Steam/steamapps/common/Rocksmith2014/dlc"
, configDir ? "$HOME/Games/Rocksmith/slopsmith/config"
, port ? 8000
}:

let
  desktopIconRevision = "f4c71354585f506a3df936f952620190b3005c63";
  iconSvg = pkgs.fetchurl {
    url = "https://raw.githubusercontent.com/byrongamatos/slopsmith-desktop/${desktopIconRevision}/resources/icons/icon.svg";
    hash = "sha256-bBcd0Gxqbmk2UKD5k2giuiqVLXqFNdvNgWPrizYRz68=";
  };
  iconPng = pkgs.fetchurl {
    url = "https://raw.githubusercontent.com/byrongamatos/slopsmith-desktop/${desktopIconRevision}/resources/icons/512x512.png";
    hash = "sha256-GQsl7MMrFoL3o/t3vv9AKPi3tyyLWfrnGJD7bIiBwvI=";
  };
  icons = pkgs.runCommand "slopsmith-icons" { } ''
    install -Dm644 ${iconSvg} $out/share/icons/hicolor/scalable/apps/slopsmith.svg
    install -Dm644 ${iconPng} $out/share/icons/hicolor/512x512/apps/slopsmith.png
  '';
  src = pkgs.fetchFromGitHub {
    owner = "byrongamatos";
    repo = "slopsmith";
    rev = "84acc2f345dbf8117f4c06fc36a4b69d8b888bb6";
    hash = "sha256-Rf2aFCxcfqV97JFLQ1A+LNe/rHw+Ypl2c8iV31o2Okc=";
  };
  launcher = pkgs.writeShellApplication {
    name = "slopsmith";
    runtimeInputs = with pkgs; [
      coreutils
      podman
      podman-compose
    ];
    text = ''
      set -euo pipefail

      dlc_dir="''${DLC_PATH:-${rocksmithDlc}}"
      config_dir="''${CONFIG_DIR:-${configDir}}"
      port="''${SLOPSMITH_PORT:-${toString port}}"
      source_dir="''${SLOPSMITH_SOURCE:-${src}}"

      if [ ! -d "$dlc_dir" ]; then
        echo "Rocksmith DLC directory does not exist: $dlc_dir" >&2
        exit 1
      fi

      if ! podman info >/dev/null 2>&1; then
        echo "Podman is not ready. Run nixos update so the Rocksmith module can enable it." >&2
        exit 1
      fi

      mkdir -p "$config_dir"
      runtime_dir="''${XDG_RUNTIME_DIR:-/tmp}/slopsmith"
      mkdir -p "$runtime_dir"
      compose_file="$runtime_dir/docker-compose.yml"

      cat > "$compose_file" <<EOF
services:
  web:
    build:
      context: "$source_dir"
    ports:
      - "127.0.0.1:$port:8000"
    volumes:
      - "$dlc_dir:/dlc:ro"
      - "$config_dir:/config"
    environment:
      - PYTHONPATH=/app/lib:/app
      - DLC_DIR=/dlc
      - CONFIG_DIR=/config
      - RSCLI_PATH=/opt/rscli/RsCli
      - DOTNET_SYSTEM_GLOBALIZATION_INVARIANT=1
      - LOG_FILE=/config/slopsmith.log
EOF

      exec podman-compose -p slopsmith -f "$compose_file" up --build
    '';
  };
  session = pkgs.writeShellApplication {
    name = "slopsmith-session";
    runtimeInputs = with pkgs; [
      coreutils
      curl
      podman
      podman-compose
      util-linux
      xdg-utils
    ];
    text = ''
      set -euo pipefail

      port="''${SLOPSMITH_PORT:-${toString port}}"
      url="http://127.0.0.1:$port"
      runtime_dir="''${XDG_RUNTIME_DIR:-/tmp}/slopsmith"
      state_dir="''${XDG_STATE_HOME:-$HOME/.local/state}/slopsmith"
      pid_file="$runtime_dir/slopsmith.pid"
      log_file="$state_dir/slopsmith.log"
      compose_file="$runtime_dir/docker-compose.yml"
      lock_file="$runtime_dir/session.lock"
      service_pid=""
      opener_pid=""

      mkdir -p "$runtime_dir" "$state_dir"

      cleanup() {
        status=$?
        trap - EXIT INT TERM HUP
        echo
        echo "Stopping Slopsmith..."
        [ -n "$opener_pid" ] && kill "$opener_pid" >/dev/null 2>&1 || true
        if [ -f "$compose_file" ]; then
          podman-compose -p slopsmith -f "$compose_file" down --remove-orphans || true
        fi
        podman rm -f slopsmith_web_1 >/dev/null 2>&1 || true
        podman network rm slopsmith_default >/dev/null 2>&1 || true
        rm -f "$pid_file"
        echo "Slopsmith stopped."
        exit "$status"
      }

      server_ready() {
        curl --fail --silent --show-error "$url" >/dev/null 2>&1
      }

      open_when_ready() {
        for _ in $(seq 1 120); do
          if server_ready; then
            xdg-open "$url" >/dev/null 2>&1 || true
            return
          fi
          sleep 0.5
        done
        echo "Slopsmith did not become ready at $url. Logs are in $log_file."
      }

      exec 9>"$lock_file"
      if ! flock -n 9; then
        echo "Slopsmith is already running; opening the existing session."
        open_when_ready
        exit 0
      fi

      trap cleanup EXIT
      trap 'exit 130' INT
      trap 'exit 143' TERM HUP

      echo "Starting Slopsmith. Close this window to stop the Podman service."
      echo "Logs are also written to $log_file"
      echo

      open_when_ready &
      opener_pid=$!

      ${launcher}/bin/slopsmith 2>&1 | tee "$log_file" &
      service_pid=$!
      echo "$service_pid" > "$pid_file"
      wait "$service_pid"
    '';
  };
  stop = pkgs.writeShellApplication {
    name = "slopsmith-stop";
    runtimeInputs = with pkgs; [
      coreutils
      podman
      podman-compose
    ];
    text = ''
      set -euo pipefail

      runtime_dir="''${XDG_RUNTIME_DIR:-/tmp}/slopsmith"
      compose_file="$runtime_dir/docker-compose.yml"

      if [ -f "$compose_file" ]; then
        podman-compose -p slopsmith -f "$compose_file" down --remove-orphans || true
      fi
      podman rm -f slopsmith_web_1 >/dev/null 2>&1 || true
      podman network rm slopsmith_default >/dev/null 2>&1 || true
      rm -f "$runtime_dir/slopsmith.pid"
    '';
  };
  desktop = pkgs.makeDesktopItem {
    name = "slopsmith";
    desktopName = "Slopsmith";
    genericName = "Rocksmith DLC Manager";
    exec = "${session}/bin/slopsmith-session";
    icon = "slopsmith";
    categories = [ "Game" "AudioVideo" "Audio" ];
    comment = "Manage Rocksmith 2014 custom DLC";
    terminal = true;
    startupNotify = true;
    keywords = [ "Rocksmith" "CDLC" "Guitar" "Bass" ];
  };
in pkgs.symlinkJoin {
  name = "slopsmith";
  paths = [ launcher session stop desktop icons ];
}
