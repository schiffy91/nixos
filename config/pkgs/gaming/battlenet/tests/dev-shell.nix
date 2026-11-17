{ pkgs ? import (builtins.getFlake "git+file:///etc/nixos").inputs.nixpkgs {
    system = "x86_64-linux";
    config.allowUnfree = true;
  }
, battlenetRoot ? "/etc/nixos/config/pkgs/gaming/battlenet/tests"
, protonRoot ? "${builtins.getEnv "HOME"}/Projects/proton-sge"  # a checkout of schiffy91/proton-sge
}:

pkgs.mkShell {
  packages = with pkgs; [
    (builtins.getFlake "git+file:///etc/nixos").inputs.proton-sge.packages.x86_64-linux.tools
    coreutils
    dotool
    elfutils  # eu-stack for battlenet-debug stacks
    findutils
    gawk
    gcc
    git
    glib
    gnumake
    imagemagick
    kdotool
    kdePackages.kservice
    pkg-config
    (python3.withPackages (pythonPackages: [
      pythonPackages."dbus-python"
    ]))
    umu-launcher
    kdePackages.qttools
    kdePackages.spectacle
  ];

  shellHook = ''
    export BATTLENET_IN_DEV_SHELL=1
    export PROTON_SGE_ROOT="${protonRoot}"
    export PATH="${battlenetRoot}/bin:$PATH"

    alias battlenet-make='make -C ${battlenetRoot}'
    alias proton-sge-make='make -C ${protonRoot}'

    if [ "''${BATTLENET_QUIET_SHELL:-}" = 1 ]; then
      return
    fi

    cat <<EOF
Battle.net test shell
  tests:   ${battlenetRoot}
  proton:  ${protonRoot}
  loop:    battlenet-make cycle BUILD_MODE=changed
EOF
  '';
}
