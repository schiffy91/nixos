{ pkgs, inputs }:
let
  png = name: source: pkgs.runCommand "${name}-icon.png" {
    nativeBuildInputs = [ pkgs.librsvg ];
  } ''
    rsvg-convert --width 256 --height 256 ${source} > "$out"
  '';
in {
  battlenet = ./battlenet.png;
  semu = png "es-de" "${inputs.semu.packages.${pkgs.stdenv.hostPlatform.system}.es-de}/share/icons/hicolor/scalable/apps/org.es_de.frontend.svg";
  moonlight = png "moonlight" "${pkgs.moonlight-qt}/share/icons/hicolor/scalable/apps/moonlight.svg";
  chiaki = "${pkgs.chiaki-ng}/share/icons/hicolor/512x512/apps/chiaking.png";
  chrome = "${pkgs.google-chrome}/share/icons/hicolor/256x256/apps/google-chrome.png";
}
