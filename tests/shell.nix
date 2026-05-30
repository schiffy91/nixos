# Reproducible test shell: nixpkgs is pinned to the repo flake.lock rather than
# the ambient <nixpkgs> channel, so VM tests resolve the same nixpkgs the flake
# builds against regardless of the host's registry.
{ pkgs ? import
    (let node = (builtins.fromJSON (builtins.readFile ../flake.lock)).nodes.nixpkgs.locked;
     in builtins.fetchTree {
       inherit (node) type owner repo rev narHash;
     })
    { config.allowUnfree = true; }
}:

let
  inherit (pkgs) lib stdenv;
  # aarch64 Secure-Boot firmware (AAVMF_CODE.fd / AAVMF_VARS.fd), substituted from
  # the binary cache so the harness can boot aarch64 Secure-Boot VMs even on a
  # non-Linux host (QEMU on macOS can't locally build the firmware).
  aarch64Pkgs = import pkgs.path { system = "aarch64-linux"; config.allowUnfree = true; };
  aavmfSecboot = (aarch64Pkgs.OVMF.override { secureBoot = true; }).fd;
in
pkgs.mkShell {
  packages = (with pkgs; [
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
  ]) ++ [ aavmfSecboot ] ++ lib.optionals stdenv.isLinux [
    pkgs.OVMF
  ];

  shellHook = ''
    export NIXOS_BTRC_TEST_SHELL=1
  '';
}
