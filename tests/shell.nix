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
  macHvfQemuPkgs = import
    (builtins.fetchTree {
      type = "github";
      owner = "NixOS";
      repo = "nixpkgs";
      rev = "64c08a7ca051951c8eae34e3e3cb1e202fe36786";
      narHash = "sha256-tpyBcxPpcQb8ukyNF7DoCwfSY3VPsxHoYwj00Cayv5o=";
    })
    { config.allowUnfree = true; };
  qemuPackage =
    if stdenv.hostPlatform.system == "aarch64-darwin"
    then macHvfQemuPkgs.qemu
    else pkgs.qemu;
  # aarch64 Secure Boot firmware: Debian's *enforcing* AAVMF_CODE.secboot.fd plus
  # the setup-mode AAVMF_VARS.fd, fetched + extracted reproducibly. nixpkgs'
  # OVMF.override{secureBoot=true} builds an AAVMF that does NOT enforce (boots
  # but reports Secure Boot unsupported); Debian's `.secboot` build does. Secure
  # Boot enforcement is a firmware signature check — it needs this firmware but
  # NOT QEMU's secure world (no -machine virt,secure=on / TrustZone).
  aavmfSecboot = stdenv.mkDerivation {
    name = "aavmf-secboot-debian-2025.11-5";
    src = pkgs.fetchurl {
      url = "https://deb.debian.org/debian/pool/main/e/edk2/qemu-efi-aarch64_2025.11-5_all.deb";
      hash = "sha256-lTiLdgboId2K8d2FJ2cJTVaf94yy6PHcIYtglZpS7oE=";
    };
    nativeBuildInputs = [ pkgs.dpkg ];
    dontUnpack = true;
    installPhase = ''
      mkdir -p $out/FV
      dpkg-deb -x $src x
      cp x/usr/share/AAVMF/AAVMF_CODE.secboot.fd $out/FV/
      cp x/usr/share/AAVMF/AAVMF_VARS.fd $out/FV/
    '';
  };
in
pkgs.mkShell {
  packages = (with pkgs; [
    bash
    coreutils
    curl
    findutils
    gawk
    git
    gnugrep
    gnused
    jq
    libarchive
    openssh
    python3
    qemuPackage
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
