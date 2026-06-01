{ pkgs ? import (builtins.getFlake "git+file:///etc/nixos").inputs.nixpkgs {
    system = "x86_64-linux";
    config.allowUnfree = true;
  }
, target ? "64"
, protonRoot ? "/etc/nixos/nix/apps/pkg-overrides/proton-custom"
}:

let
  shellHook = ''
    export PROTON_CUSTOM_IN_DEV_SHELL=1
    export PROTON_CUSTOM_DEV_ROOT="${protonRoot}/src"
    export PROTON_CUSTOM_WINE_SRC="$PROTON_CUSTOM_DEV_ROOT/wine"
    export PROTON_CUSTOM_WINE64_BUILD="$PROTON_CUSTOM_DEV_ROOT/wine64"
    export PROTON_CUSTOM_WINE32_BUILD="$PROTON_CUSTOM_DEV_ROOT/wine32"
    export PROTON_CUSTOM_DXVK_SRC="$PROTON_CUSTOM_DEV_ROOT/dxvk"
    export PROTON_CUSTOM_JOBS="''${PROTON_CUSTOM_JOBS:-$(nproc)}"
    export PATH="${protonRoot}/bin:$PATH"
    mkdir -p "$PROTON_CUSTOM_DEV_ROOT"

    alias proton-custom-dev=${protonRoot}/bin/proton-custom-dev
    alias proton-custom-make='make -C ${protonRoot}'

    if [ "''${PROTON_CUSTOM_QUIET_SHELL:-}" = 1 ]; then
      return
    fi

    cat <<EOF
proton-custom dev shell
  sources: $PROTON_CUSTOM_DEV_ROOT
  loop:    make setup | make dcomp | make dxvk
  helper:  proton-custom-make <target> works from any directory
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
    umu-launcher
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
