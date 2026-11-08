{ pkgs }:
pkgs.runCommandCC "hdr-screencast-probe" {
  nativeBuildInputs = [ pkgs.python3 pkgs.pkg-config pkgs.wayland-scanner ];
  buildInputs = [ pkgs.wayland pkgs.wayland-protocols pkgs.pipewire pkgs.libdrm ];
  XDG_DATA_DIRS = "${pkgs.kdePackages.plasma-wayland-protocols}/share";
} ''
  python3 - <<'PY'
  import runpy
  from pathlib import Path
  probe = runpy.run_path("${./hdr-screencast-test}")["Probe"]
  probe.build(Path("${./probe.c}"), Path.cwd())
  PY
  install -Dm755 probe "$out/bin/hdr-screencast-probe"
''
