{ pkgs ? import <nixpkgs> {} }:

let
  inherit (pkgs) lib stdenv;
in
pkgs.mkShell {
  packages = with pkgs; [
    bash
    coreutils
    curl
    findutils
    gawk
    gnugrep
    gnused
    jq
    libarchive
    openssh
    qemu
    socat
    swtpm
    util-linux
  ] ++ lib.optionals stdenv.isLinux [
    OVMF
  ];

  shellHook = ''
    export NIXOS_BTRC_TEST_SHELL=1
  '';
}
