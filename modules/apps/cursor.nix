{ settings, lib, ... }:
let src = { source = settings.desktop.cursor.path; }; in
lib.mkIf (settings.desktop.environment != "none") {
  home.file.".icons/default" = src;
  home.file.".local/share/icons/default" = src;
}
