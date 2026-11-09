{ config, lib, ... }:
let homeDirectory = config.settings.users.admin.homeDirectory;
in lib.mkIf config.settings.desktop.enable {
  settings.disk.immutability.persist.paths = [
    ##### Shared Desktop State #####
    "${homeDirectory}/.config/dconf/user"
    "${homeDirectory}/.config/xsettingsd/xsettingsd.conf"
    "${homeDirectory}/.local/share/applications"
    "${homeDirectory}/.local/share/icons"
    "${homeDirectory}/.local/share/desktop-directories"
    "${homeDirectory}/.local/share/flatpak/db"
    ##### Plasma #####
    "${homeDirectory}/.config/gtk-3.0"
    "${homeDirectory}/.config/gtk-4.0"
    "${homeDirectory}/.config/gtkrc-2.0"
    "${homeDirectory}/.config/gtkrc"
    "${homeDirectory}/.icons"
    "${homeDirectory}/.config/kcmfonts"
    "${homeDirectory}/.config/kcminputrc"
    "${homeDirectory}/.config/kdedefaults"
    "${homeDirectory}/.config/kdeglobals"
    "${homeDirectory}/.config/kglobalshortcutsrc"  # apps re-prompt to register shortcuts without it
    "${homeDirectory}/.config/konsolesshconfig"
    "${homeDirectory}/.config/kwalletrc"
    "${homeDirectory}/.config/kwinoutputconfig.json"
    "${homeDirectory}/.config/kwinrc"
    "${homeDirectory}/.config/menus"
    "${homeDirectory}/.config/plasma-org.kde.plasma.desktop-appletsrc"
    "${homeDirectory}/.config/plasmashellrc"
    "${homeDirectory}/.config/QtProject.conf"
    "${homeDirectory}/.config/systemsettingsrc"
    "${homeDirectory}/.config/Trolltech.conf"
    "${homeDirectory}/.gtkrc-2.0"
    "${homeDirectory}/.local/share/kactivitymanagerd"
    # Keep the applied-script hashes with the layout so login does not rebuild
    # unchanged panels and restart plasmashell after every home reset.
    "${homeDirectory}/.local/share/plasma-manager/last_run_desktop_script_panels"
    "${homeDirectory}/.local/share/plasma-manager/last_run_desktop_script_wallpaper_picture"
    "${homeDirectory}/.local/share/recently-used.xbel"
    "${homeDirectory}/.local/state/konsolestaterc"
    "${homeDirectory}/.local/state/systemsettingsstaterc"
    ##### Kwallet #####
    "${homeDirectory}/.local/share/kwalletd"
    ##### Klipper #####
    "${homeDirectory}/.local/share/klipper/history2.lst"
  ];
}
