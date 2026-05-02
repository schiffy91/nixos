{ settings, lib, ... }: lib.mkIf (settings.desktop.environment != "none") {
  home.file = {
    ".icons/default".source = settings.desktop.cursor.path;
    ".local/share/icons/default".source = settings.desktop.cursor.path;
  };
}
