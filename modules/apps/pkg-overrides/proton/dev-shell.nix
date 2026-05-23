{ pkgs ? import (builtins.getFlake "git+file:///etc/nixos").inputs.nixpkgs {
    system = "x86_64-linux";
    config.allowUnfree = true;
  }
, target ? "64"
, protonRoot ? "/etc/nixos/modules/apps/pkg-overrides/proton"
}:

let
  shellHook = ''
    export SCWHINE_IN_DEV_SHELL=1
    export SCWHINE_DEV_ROOT="${protonRoot}/src"
    export SCWHINE_WINE_SRC="$SCWHINE_DEV_ROOT/wine"
    export SCWHINE_WINE64_BUILD="$SCWHINE_DEV_ROOT/wine64"
    export SCWHINE_WINE32_BUILD="$SCWHINE_DEV_ROOT/wine32"
    export SCWHINE_DXVK_SRC="$SCWHINE_DEV_ROOT/dxvk"
    export SCWHINE_JOBS="''${SCWHINE_JOBS:-$(nproc)}"
    mkdir -p "$SCWHINE_DEV_ROOT"

    alias bnet-dev=${protonRoot}/bin/bnet-dev
    alias bnet-make='make -C ${protonRoot}'

    if [ "''${SCWHINE_QUIET_SHELL:-}" = 1 ]; then
      return
    fi

    cat <<EOF
scwhine Proton dev shell
  sources: $SCWHINE_DEV_ROOT
  loop:    make setup | make dcomp | make shot
  helper:  bnet-make <target> works from any directory
EOF
  '';

  commonPackages = with pkgs; [
    autoconf
    automake
    bison
    dotool
    flex
    glslang
    git
    gnumake
    imagemagick
    kdotool
    meson
    ninja
    perl
    pkg-config
    python3
    wayland-scanner
    pkgsCross.mingwW64.buildPackages.gcc
    pkgsCross.mingw32.buildPackages.gcc
  ];

  buildInputs64 = with pkgs; [
      dbus
      fontconfig
      freetype
      libGL
      libxkbcommon
      mesa
      vulkan-headers
      vulkan-loader
      wayland
      libx11
  ];

  buildInputs32 = with pkgs.pkgsi686Linux; [
      dbus
      fontconfig
      freetype
      libGL
      libxkbcommon
      mesa
      vulkan-loader
      wayland
      libx11
  ];
in
if target == "32" then
  pkgs.pkgsi686Linux.mkShell {
    packages = commonPackages;
    buildInputs = buildInputs32;
    inherit shellHook;
  }
else
  pkgs.mkShell {
    packages = commonPackages;
    buildInputs = buildInputs64;
    inherit shellHook;
  }
