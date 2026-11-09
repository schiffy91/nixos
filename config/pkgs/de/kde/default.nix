{ inputs, config, pkgs, lib, ... }:
let
  primary = lib.findFirst (o: o.primary) null config.settings.desktop.outputs;
  primaryScale = if primary == null then 1.0 else primary.scaleFactor;
  user = config.settings.users.admin.username;
  cursorPath = config.settings.desktop.cursor.path;
in {
  imports = [ ./persistence.nix ];
  config = lib.mkMerge [
    {
      home-manager = {
        sharedModules = [ inputs.plasma-manager.homeModules.plasma-manager ];
        users = lib.optionalAttrs config.settings.users.admin.homeManager.enable {
          ${user}.imports = [ ./home.nix ];
        };
      };
    }
    (lib.mkIf config.settings.desktop.enable {
      hardware.graphics.enable = true;
      programs.dconf.enable = true;
      programs.kde-pim.enable = false;
      services = {
        xserver.dpi = builtins.floor (96 * primaryScale);
        displayManager = {
          enable = true;
          sddm = {
            enable = true;
            wayland.enable = true;
            theme = "breeze";  # Plasma 6 Breeze SDDM theme; autofocuses password on single-user systems
            settings.Theme.CursorTheme = config.settings.desktop.cursor.theme;
          };
          defaultSession = config.settings.desktop.defaultSession;
        };
        desktopManager.plasma6 = {
          enable = true;
          enableQt5Integration = false;
        };
        accounts-daemon.enable = true;
      };
      environment = {
        systemPackages = (with pkgs; [
          wl-clipboard
          config.settings.desktop.cursor.package
          config.settings.desktop.cursor.defaultPackage
        ]) ++ (with pkgs.kdePackages; [
          plasma-thunderbolt
          kaccounts-providers
          kaccounts-integration
          kio-gdrive
        ]);
        sessionVariables = {
          XCURSOR_THEME = config.settings.desktop.cursor.theme;
          NIXOS_OZONE_WL = "1";
        };
        plasma6.excludePackages = with pkgs.kdePackages; [
          kate
          khelpcenter
          elisa
          okular
          print-manager
        ];
      };
      system.activationScripts.cursorIcons = lib.stringAfter [ "users" ] ''
        ${pkgs.coreutils}/bin/install -d -o ${user} /home/${user}/.icons /home/${user}/.local/share/icons
        ${pkgs.coreutils}/bin/ln -sfn ${cursorPath} /home/${user}/.icons/default
        ${pkgs.coreutils}/bin/ln -sfn ${cursorPath} /home/${user}/.local/share/icons/default
        ${pkgs.coreutils}/bin/chown -h ${user} /home/${user}/.icons/default /home/${user}/.local/share/icons/default
      '';
      security.pam.services.sddm.enableKwallet = config.settings.users.admin.autoUnlockWallet.enabled;
      security.pam.services.passwd.enableKwallet = config.settings.users.admin.autoUnlockWallet.enabled;  # pam_kwallet hooks chauthtok → wallet re-encrypts on passwd
    })
  ];
}
