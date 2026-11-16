{ pkgs, inputs }:
(import ../lib/build-tools.nix { inherit pkgs inputs; }) {
  name = "assetto-tools";
  sources = [
    ./src
    ../lib/ModTree.btrc
    ../steam/src/SteamLibrary.btrc
    ../steam/src/SteamVdf.btrc
    ../wine/src/WinePrefix.btrc
  ];
  programs.assetto = "assettocorsa/src/Main.btrc";
  buildInputs = [ pkgs.liblzf pkgs.zlib ];
  runtimeInputs = [ pkgs.coreutils pkgs.procps pkgs.libarchive ];
}
