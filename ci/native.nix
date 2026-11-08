{ system ? builtins.currentSystem }:
let
  locked = (builtins.fromJSON (builtins.readFile ../flake.lock)).nodes.nixpkgs.locked;
  pkgs = import (builtins.fetchTree { inherit (locked) type owner repo rev narHash; }) { inherit system; };
  compiler = (builtins.fromJSON (builtins.readFile ../flake.lock)).nodes.btrc.locked;
  inputs.btrc = builtins.getFlake "github:${compiler.owner}/${compiler.repo}/${compiler.rev}";
  sleep = import ../config/system/nixos/sleep/packages.nix { inherit pkgs inputs; };
  # Keep this inventory explicit: repo-tools checks it against repository sources.
  coveredSources = [
    "ci/native.h"
    "config/system/nixos/sleep/helpers/posix.h"
    "config/system/nixos/sleep/helpers/pam.h"
    "config/system/nixos/sleep/helpers/drm.h"
    "config/system/nixos/opportunistic-unlock/handoff.h"
    "config/system/nixos/opportunistic-unlock/native.h"
    "config/system/nixos/networking/native.h"
    "config/pkgs/utils/chrome-launchers/native.h"
    "config/pkgs/gaming/wow/files.h"
    "config/pkgs/utils/nixosctl/secure-boot-keys/crypto.h"
    "config/pkgs/utils/nixosctl/secure-boot-keys/files.h"
    "config/pkgs/utils/nixosctl/lib/accounts/native.h"
    "config/pkgs/utils/nixosctl/lib/credentials.h"
    "config/pkgs/gaming/sunshine/ssh-login/native.h"
    "config/pkgs/gaming/assettocorsa/src/compression.h"
    "config/pkgs/gaming/battlenet/tests/battlenet-capture-window.c"
    "config/pkgs/gaming/sunshine/hdr-screencast-test/probe.c"
    "tests/plymouth-keyboard/input.c"
    "tests/ci/native-fixture/native.h"
    "tests/ci/process-probe.h"
    "tests/gaming/affinity/cpu-set.h"
    "tests/gaming/runtime/tls.h"
    "tests/gaming/runtime/signals.h"
    "tests/harness/timeout/process-tree.h"
    "tests/secure-boot/alarm-state.h"
    "tests/vm/vnc.h"
    "tests/gaming/proton/pdb/Folded.c"
    "tests/gaming/proton/pdb/Public.c"
    "tests/gaming/proton/pdb/PdbSymbols.c"
  ];
  helpers = pkgs.runCommandCC "native-source-helpers" {
    nativeBuildInputs = [ pkgs.pkg-config ];
    buildInputs = [ pkgs.glib pkgs.cryptsetup pkgs.liblzf pkgs.zlib pkgs.openssl pkgs.curl pkgs.pam pkgs.libdrm pkgs.libvncserver ];
  } ''
    mkdir -p "$out/bin"
    $CC -O2 -Wall -Werror ${../config/pkgs/gaming/battlenet/tests/battlenet-capture-window.c} \
      $(pkg-config --cflags --libs gio-2.0 gio-unix-2.0 glib-2.0) -o "$out/bin/battlenet-capture-window"
    $CC -Wall -Wextra -Werror ${../tests/plymouth-keyboard/input.c} -o "$out/bin/plymouth-test-input"
    for source in ${./native.h} \
      ${../config/system/nixos/opportunistic-unlock/native.h} \
      ${../config/system/nixos/networking/native.h} \
      ${../config/pkgs/utils/chrome-launchers/native.h} \
      ${../config/pkgs/gaming/wow/files.h} \
      ${../config/pkgs/utils/nixosctl/secure-boot-keys/crypto.h} \
      ${../config/pkgs/utils/nixosctl/secure-boot-keys/files.h} \
      ${../config/pkgs/utils/nixosctl/lib/accounts/native.h} \
      ${../config/pkgs/utils/nixosctl/lib/credentials.h} \
      ${../config/system/nixos/sleep/helpers/posix.h} \
      ${../config/system/nixos/sleep/helpers/pam.h} \
      ${../config/system/nixos/sleep/helpers/drm.h} \
      ${../config/pkgs/gaming/sunshine/ssh-login/native.h} \
      ${../config/pkgs/gaming/assettocorsa/src/compression.h} \
      ${../tests/ci/process-probe.h} \
      ${../tests/gaming/affinity/cpu-set.h} \
      ${../tests/gaming/runtime/tls.h} \
      ${../tests/gaming/runtime/signals.h} \
      ${../tests/harness/timeout/process-tree.h} \
      ${../tests/secure-boot/alarm-state.h} \
      ${../tests/vm/vnc.h} \
      ${../tests/ci/native-fixture/native.h}; do
      $CC -D_GNU_SOURCE=1 -Wall -Wextra -Werror -x c -fsyntax-only -I${../config/system/nixos/opportunistic-unlock} \
        $(pkg-config --cflags libcryptsetup openssl libcurl libdrm glib-2.0) -include "$source" /dev/null
    done
  '';
  packages = [
    sleep.blank sleep.native helpers
    (import ../config/system/nixos/opportunistic-unlock/package.nix { inherit pkgs inputs; })
    (import ../config/pkgs/gaming/sunshine/hdr-screencast-test/package.nix { inherit pkgs; })
    (import ../tests/gaming/proton/pdb { inherit pkgs; })
  ];
in pkgs.runCommand "native-source-check" {
  passthru = { inherit coveredSources; };
} ''
  mkdir -p "$out"
  ${pkgs.lib.concatMapStringsSep "\n" (package: ''test -e ${package}'') packages}
  printf '%s\n' ${pkgs.lib.escapeShellArgs coveredSources} > "$out/sources"
''
