# Apps shared by the desktop and handheld; hardware-specific packages stay in each host.
{ pkgs, lib, ... }: {
  environment.systemPackages = with pkgs; [
    (mpv.override { youtubeSupport = false; })  # avoids V8 source build chain
    nixd
    claude-code
    codex
    solaar
    discord
    (pkgs.cider-2.overrideAttrs (old: {
      postFixup = (old.postFixup or "") + ''
        substituteInPlace $out/share/applications/cider-2.desktop \
          --replace-fail "Exec=cider-2" "Exec=cider-2 --force-device-scale-factor=1 --ozone-platform=wayland --enable-features=WaylandWindowDecorations"
      '';
    }))
  ];
  settings.apps.zsh.enable = true;
  programs.openvpn3.enable = true;
  hardware.logitech.wireless.enable = true;
  services.resolved.enable = true;
  services.mullvad-vpn = {
    enable = true;
    gui.enable = true;
  };
  systemd.services.mullvad-daemon = {
    wants = lib.mkForce [ "network.target" ];
    after = lib.mkForce [ "NetworkManager.service" "systemd-resolved.service" ];
  };
}
