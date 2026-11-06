{ config, pkgs, lib, ... }:
let
  returnToGaming = pkgs.makeDesktopItem {
    name = "return-to-gaming-mode";
    desktopName = "Return to Gaming Mode";
    comment = "Leave Plasma and return to Steam Gaming Mode";
    exec = "${pkgs.jovianPackages.steamos-manager}/bin/steamosctl switch-to-game-mode";
    icon = "steam";
    categories = [ "Game" ];
    terminal = false;
  };
  keyboardFit = pkgs.runCommand "steam-keyboard-fit" { } ''
    install -Dm644 ${./steam-keyboard-fit.js} $out/contents/code/main.js
    cat > $out/metadata.json <<'EOF'
    {
      "KPackageStructure": "KWin/Script",
      "KPlugin": {
        "Id": "steam-keyboard-fit",
        "Name": "Fit Steam Keyboard",
        "Description": "Keep Steam's desktop keyboard inside the screen",
        "Version": "1.0",
        "License": "MIT"
      },
      "X-Plasma-API": "javascript",
      "X-Plasma-MainScript": "code/main.js"
    }
    EOF
  '';
in
{
  # The Steam client still invokes this legacy command; Jovian now ships steamosctl.
  environment.systemPackages = [
    returnToGaming
    (pkgs.writeShellApplication {
      name = "steamos-session-select";
      text = ''
        case "''${1-}" in
          plasma|desktop)
            exec ${pkgs.jovianPackages.steamos-manager}/bin/steamosctl switch-to-desktop-mode
            ;;
          gamescope|gaming)
            exec ${pkgs.jovianPackages.steamos-manager}/bin/steamosctl switch-to-game-mode
            ;;
          *)
            echo "Usage: steamos-session-select {plasma|gamescope}" >&2
            exit 2
            ;;
        esac
      '';
    })
  ];
  home-manager.users.${config.settings.users.admin.username} = {
    home.file = {
      "Desktop/Return to Gaming Mode.desktop" = {
        source = "${returnToGaming}/share/applications/return-to-gaming-mode.desktop";
        executable = true;
      };
      ".local/share/kwin/scripts/steam-keyboard-fit".source = keyboardFit;
    };
    programs.plasma.configFile.kwinrc.Plugins."steam-keyboard-fitEnabled" = true;
    # Steam+X owns automatic/controller keyboard entry in desktop mode.
    # Keep Plasma's taskbar button available without stacking two keyboards.
    programs.plasma.configFile.kwinrc.Wayland.VirtualKeyboardMode = lib.mkForce 0;
  };
}
