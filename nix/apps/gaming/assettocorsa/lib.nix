{ pkgs }:
let
  # Archives are unpacked in the sandbox, never over the live Steam library.
  # subdirectory selects the game-root tree inside an archive; target places it.
  archive = { name, src, subdirectory ? ".", target ? ".", format ? "zip" }:
    pkgs.runCommand name { nativeBuildInputs = [ pkgs.unzip pkgs.p7zip ]; } ''
      mkdir unpacked "$out"
      ${if format == "zip" then
        "unzip -q ${pkgs.lib.escapeShellArg (toString src)} -d unpacked"
      else "7z x ${pkgs.lib.escapeShellArg (toString src)} -ounpacked -y >/dev/null"}
      mkdir -p "$out"/${pkgs.lib.escapeShellArg target}
      cp -r unpacked/${pkgs.lib.escapeShellArg subdirectory}/. "$out"/${pkgs.lib.escapeShellArg target}/
    '';
in {
  inherit archive;
  contentManager = archive {
    name = "content-manager-0.8.2895.40668";
    src = pkgs.fetchurl {
      name = "content-manager-0.8.2895.40668.zip";
      url = "https://github.com/gro-ove/actools/releases/download/v0.8.2895.40668/Content.Manager.zip";
      hash = "sha256-2BEaCZfmwLAGC8tDAW9KQ6CPOYLjwU8bH84Rf1G6Vcg=";
    };
  };
  csp = archive {
    name = "custom-shaders-patch-0.2.11";
    src = pkgs.fetchurl {
      name = "lights-patch-v0.2.11.zip";
      url = "https://acstuff.club/patch/?get=0.2.11";
      hash = "sha256-BynMrrtAOnptHEBRaZDagRbLX1bupMH+C90phjpab/s=";
    };
  };
  fonts = archive {
    name = "content-manager-fonts";
    src = pkgs.fetchurl {
      url = "https://files.acstuff.ru/shared/T0Zj/fonts.zip";
      hash = "sha256-wY798M5KNtnOy9T5e39LYIvStuypmbRuwrTZP7LeSbI=";
    };
    subdirectory = "system";
    target = "content/fonts/system";
  };
}
