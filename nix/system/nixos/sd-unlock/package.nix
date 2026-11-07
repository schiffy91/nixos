{ pkgs, systemd }:
pkgs.runCommandCC "nixos-sd-unlock" {
  nativeBuildInputs = [ pkgs.pkg-config ];
  buildInputs = [ pkgs.cryptsetup systemd ];
} ''
  mkdir -p $out/bin
  $CC -O2 -Wall -Wextra -Werror -I${./.} ${./unlock.c} \
    $(pkg-config --cflags --libs libcryptsetup libsystemd) -o $out/bin/nixos-sd-unlock
''
