{ config, pkgs, lib, inputs, ... }:
let
  settings = config.settings.apps.gaming.semu;
  home = config.settings.users.admin.homeDirectory;
  client = config.settings.apps.utils.drive.enable;  # the Drive client keeps the library's folder downloaded in place
  library = if settings.root != null then settings.root else "${home}/Games/Emulation";  # where a machine without the client keeps its copy
  packages = inputs.semu.packages.${pkgs.stdenv.hostPlatform.system};
  semu = packages.semu;
  # the bundle is SEMU_ASSET_ROOT, so its bin/ and share/ are every emulator's own
  # binaries and desktop entries; a profile should publish only semu's own two
  environment = ''
    export SEMU_TARGET="''${SEMU_TARGET:-${settings.target}}"
    ${lib.optionalString (settings.root != null) ''export SEMU_ROOT="''${SEMU_ROOT:-${settings.root}}"''}
  '';
  # Without the client the library is a separate copy that a timer syncs. That is too slow for
  # "saved on the Deck, continue on the desktop": the frontend also asks for a sync as it opens
  # (other machines' saves come in) and as it closes (these go out).
  syncNow = lib.optionalString (settings.drive != null && !client && config.settings.apps.utils.rclone.enable)
    "${pkgs.systemd}/bin/systemctl --user start --no-block rclone-offline-emulation.service >/dev/null 2>&1 || true";
  launcher = name: pkgs.writeShellScript "semu-${name}" (environment + (if name == "semu-es-de" then ''
    ${syncNow}
    ${semu}/bin/${name} "$@"
    status=$?
    ${syncNow}
    exit $status
  '' else ''
    exec ${semu}/bin/${name} "$@"
  ''));
  semuProfile = pkgs.runCommand "semu-profile" { meta.mainProgram = "semu"; } ''
    mkdir -p "$out/bin" "$out/share/applications"
    ln -s ${launcher "semu"} "$out/bin/semu"
    ln -s ${launcher "semu-es-de"} "$out/bin/semu-es-de"
    ln -s ${semu}/share/applications/semu.desktop "$out/share/applications/semu.desktop"
    install -Dm644 ${packages.es-de}/share/icons/hicolor/scalable/apps/org.es_de.frontend.svg \
      "$out/share/icons/hicolor/scalable/apps/es-de.svg"
  '';
in lib.mkIf settings.enable {
  # Semu keeps every asset, saves and settings included, under the library root; what it
  # writes elsewhere is disposable state in ~/.cache/semu, so nothing of its own persists.
  environment.systemPackages = [ semuProfile ];
  environment.sessionVariables = { SEMU_TARGET = settings.target; }
    // lib.optionalAttrs (settings.root != null) { SEMU_ROOT = settings.root; };
  # With the client, every target's library is its Drive folder, in place under ~/Drive.
  settings.apps.utils.drive = lib.mkIf (settings.drive != null) {
    pinned = [ settings.drive ];
    ignore = [ "*.semu-partial" "*.semu-tmp" "*.semu-link" ];  # unfinished downloads and links that only mean something here
  };
  settings.apps.utils.rclone.offline = lib.mkIf (settings.drive != null && !client) {
    emulation = {
      remote = settings.drive;
      local = library;
      marker = ".semu-layout";  # written by `semu migrate`; absent while the card is not mounted
      exclude = [ "/_legacy/**" "*.semu-partial" "*.semu-partial/**" "*.semu-tmp" "*.semu-link" ];
    };
  };
}
