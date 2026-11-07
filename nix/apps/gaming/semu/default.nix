{ config, pkgs, lib, inputs, ... }:
let
  enabled = config.settings.apps.enable
    && config.settings.apps.gaming.enable
    && config.settings.apps.semu.enable
    && pkgs.stdenv.hostPlatform.isx86_64;
  user = config.settings.users.admin.username;
  group = config.users.users.${user}.group;
  home = "/home/${user}";
  target = config.settings.apps.semu.target;
  # semu deep-merges $SEMU_HOME/overrides over the target, so empty paths change nothing
  pathsOverride = pkgs.writeText "semu-nixos.json" (builtins.toJSON { inherit (config.settings.apps.semu) paths; });
  semu = import ./package.nix { inherit inputs pkgs; };
  icons = import ../icons/package.nix { inherit pkgs inputs; };
  # the bundle is SEMU_ASSET_ROOT, so its bin/ and share/ are every emulator's own
  # binaries and desktop entries; a profile should publish only semu's own two
  semuProfile = pkgs.runCommand "semu-profile" { meta.mainProgram = "semu"; } ''
    mkdir -p "$out/bin" "$out/share/applications"
    ln -s ${semu}/bin/semu "$out/bin/semu"
    ln -s ${semu}/bin/semu-es-de "$out/bin/semu-es-de"
    ln -s ${semu}/share/applications/semu.desktop "$out/share/applications/semu.desktop"
    install -Dm644 ${icons.semu} "$out/share/icons/hicolor/256x256/apps/es-de.png"
  '';
in lib.mkIf enabled {
  environment.systemPackages = [ semuProfile ];
  environment.sessionVariables.SEMU_TARGET = target;

  systemd.user.services.semu-sync = {  # Syncthing for saves, only while sync.enabled is set in semu.json
    description = "Semu save sync (Syncthing)";
    after = [ "network-online.target" ];
    wantedBy = [ "default.target" ];
    environment.SEMU_TARGET = target;
    serviceConfig = {
      ExecCondition = "${semu}/bin/semu sync enabled";
      ExecStart = "${semu}/bin/semu sync run";
      Restart = "on-failure";
      RestartSec = "5s";
    };
  };

  system.activationScripts.semu = lib.stringAfter [ "users" ] ''
    install -d -o ${user} -g ${group} "${home}/ES-DE" "${home}/.local/share/semu" "${home}/.config/semu/overrides"
    install -o ${user} -g ${group} -m 0644 ${pathsOverride} "${home}/.config/semu/overrides/nixos.json"
    ${pkgs.util-linux}/bin/runuser -u ${user} -- env HOME="${home}" SEMU_TARGET="${target}" \
      ${semu}/bin/semu prepare --target "${target}" || true
  '';
}
