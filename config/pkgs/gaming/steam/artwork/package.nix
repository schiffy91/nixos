{ pkgs, name, icon }:
let
  images = {
    p = { width = 600; height = 900; };
    landscape = { width = 920; height = 430; };
    _hero = { width = 1920; height = 620; };
  };
  template = dimensions: pkgs.writeText "steam-artwork.svg" (import ./svg.nix ({ inherit name; inherit (pkgs) lib; } // dimensions));
in pkgs.runCommand "steam-shortcut-artwork" {
  nativeBuildInputs = [ pkgs.librsvg ];
  FONTCONFIG_FILE = pkgs.makeFontsConf { fontDirectories = [ pkgs.dejavu_fonts ]; };
} ''
  export XDG_CACHE_HOME="$TMPDIR/cache"
  mkdir -p "$out"
  icon=$(base64 -w0 ${pkgs.lib.escapeShellArg (toString icon)})
  ${pkgs.lib.concatStringsSep "\n" (pkgs.lib.mapAttrsToList (filename: dimensions: ''
    substitute ${template dimensions} "$out/${filename}.svg" --replace-fail '@ICON@' "$icon"
    rsvg-convert "$out/${filename}.svg" -o "$out/${filename}.png"
  '') images)}
  cp ${pkgs.lib.escapeShellArg (toString icon)} "$out/_logo.png"
''
