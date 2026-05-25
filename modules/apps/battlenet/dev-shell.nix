{ pkgs ? import (builtins.getFlake "git+file:///etc/nixos").inputs.nixpkgs {
    system = "x86_64-linux";
    config.allowUnfree = true;
  }
, battlenetRoot ? "/etc/nixos/modules/apps/battlenet"
, protonRoot ? "/etc/nixos/modules/apps/pkg-overrides/proton-custom"
}:

pkgs.mkShell {
  packages = with pkgs; [
    coreutils
    dotool
    findutils
    gawk
    git
    glib
    gnumake
    imagemagick
    kdotool
    python3
    umu-launcher
    kdePackages.qttools
    kdePackages.spectacle
  ];

  shellHook = ''
    export BATTLENET_IN_DEV_SHELL=1
    export PATH="${battlenetRoot}/bin:${protonRoot}/bin:$PATH"

    alias battlenet-make='make -C ${battlenetRoot}'
    alias proton-custom-make='make -C ${protonRoot}'

    if [ "''${BATTLENET_QUIET_SHELL:-}" = 1 ]; then
      return
    fi

    cat <<EOF
Battle.net dev shell
  app:     ${battlenetRoot}
  proton:  ${protonRoot}
  loop:    battlenet-make cycle BUILD_MODE=changed
EOF
  '';
}
