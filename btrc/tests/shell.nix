# Reproducible test shell: nixpkgs is pinned to the repo flake.lock rather than
# the ambient <nixpkgs> channel, so VM tests resolve the same nixpkgs the flake
# builds against regardless of the host's registry.
{ pkgs ? import
    (let node = (builtins.fromJSON (builtins.readFile ../../flake.lock)).nodes.nixpkgs.locked;
     in builtins.fetchTree {
       inherit (node) type owner repo rev narHash;
     })
    { config.allowUnfree = true; }
}:

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
