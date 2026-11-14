{ pkgs, inputs }:
let
  build = import ../../../lib/btrc.nix { inherit pkgs inputs; };
  src = pkgs.lib.fileset.toSource {
    root = ./.;
    fileset = pkgs.lib.fileset.fileFilter (file: file.hasExt "btrc" || file.hasExt "h" || file.name == "btrc.toml" || file.name == "btrc.lock") ./.;
  };
  ask = build {  # the window that asks whether a program may download
    name = "drive-ask";
    inherit src;
    programs.drive-ask = "ask/Ask.btrc";
    linkPlan = true;
    buildInputs = [ pkgs.sdl3 pkgs.fontconfig pkgs.freetype inputs.btrc.packages.${pkgs.stdenv.hostPlatform.system}.wgpu-native ];
    wrapperArgs = [ "--prefix" "LD_LIBRARY_PATH" ":" "${pkgs.vulkan-loader}/lib" ];  # the window draws through Vulkan, whose loader is opened by name; not every system keeps it with its drivers
  };
in {
  inherit ask;
  # The question goes to the user's desktop session as the client's own window. With no
  # session to show it in (Game Mode, a console, nobody logged in) this fails, and the
  # engine stops the program's reads at once; `drive readers` lists what was stopped.
  askSession = pkgs.writeShellScriptBin "drive-ask-session" ''
    export XDG_RUNTIME_DIR="/run/user/$(${pkgs.coreutils}/bin/id -u)"
    session="$(${pkgs.systemd}/bin/systemctl --user show-environment 2>/dev/null)" || exit 1
    case "$session" in *XDG_CURRENT_DESKTOP=gamescope*) exit 1 ;; esac
    case "$session" in *WAYLAND_DISPLAY=*|*DISPLAY=*) ;; *) exit 1 ;; esac
    exec ${pkgs.systemd}/bin/systemd-run --user --collect --quiet --setenv=DRIVE_RUNTIME="$DRIVE_RUNTIME" -- ${ask}/bin/drive-ask "$1" "$2"
  '';
  drive = build {
    name = "drive";
    inherit src;
    programs.drive = "Main.btrc";
    runtimeInputs = [ pkgs.curl pkgs.coreutils ];  # every request is one curl process; adoption sums files with sha256sum
    postInstall = "install -Dm644 ${./readers.deny} $out/share/drive/readers.deny";  # the default list of programs that never download
  };
  fake = build {  # the Drive API from memory, for tests
    name = "drive-fake";
    inherit src;
    programs.drive-fake = "testing/FakeDrive.btrc";
  };
}
