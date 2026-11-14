{ pkgs, inputs }:
let
  build = import ../../../lib/btrc.nix { inherit pkgs inputs; };
  src = pkgs.lib.fileset.toSource {
    root = ./.;
    fileset = pkgs.lib.fileset.fileFilter (file: file.hasExt "btrc" || file.hasExt "h" || file.name == "btrc.toml" || file.name == "btrc.lock") ./.;
  };
  window = {  # what a program with a window of its own links and runs with
    linkPlan = true;
    buildInputs = [ pkgs.sdl3 pkgs.fontconfig pkgs.freetype inputs.btrc.packages.${pkgs.stdenv.hostPlatform.system}.wgpu-native ];
    wrapperArgs = [ "--prefix" "LD_LIBRARY_PATH" ":" "${pkgs.vulkan-loader}/lib" ];  # the window draws through Vulkan, whose loader is opened by name; not every system keeps it with its drivers
  };
  ask = build ({  # the window that asks whether a program may download
    name = "drive-ask";
    inherit src;
    programs.drive-ask = "ask/Ask.btrc";
  } // window);
  # The tray's pictures: the Drive's mark from an icon theme, and the same with how far along it is.
  icons = pkgs.runCommand "drive-tray-icons" { nativeBuildInputs = [ pkgs.python3 ]; } ''
    python3 ${./status/icons.py} ${pkgs.papirus-icon-theme}/share/icons/Papirus/64x64/apps/google-drive.svg $out
  '';
  drive = build {
    name = "drive";
    inherit src;
    programs.drive = "Main.btrc";
    runtimeInputs = [ pkgs.curl pkgs.coreutils ];  # every request is one curl process; adoption sums files with sha256sum
    postInstall = "install -Dm644 ${./readers.deny} $out/share/drive/readers.deny";  # the default list of programs that never download
  };
  tray = build {  # the icon in the system tray
    name = "drive-tray";
    inherit src;
    programs.drive-tray = "status/Tray.btrc";
    buildInputs = [ pkgs.dbus ];  # Library.Tray binds libdbus-1
    env.NIX_CFLAGS_COMPILE = "-I${pkgs.lib.getDev pkgs.dbus}/include/dbus-1.0 -I${pkgs.lib.getLib pkgs.dbus}/lib/dbus-1.0/include";
    linkFlags = [ "-ldbus-1" ];
  };
  status = build ({  # the window a click on that icon opens
    name = "drive-status";
    inherit src;
    programs.drive-status = "status/Window.btrc";
  } // window // {
    wrapperArgs = window.wrapperArgs ++ [
      "--prefix" "PATH" ":" (pkgs.lib.makeBinPath [ drive pkgs.util-linux pkgs.gnugrep pkgs.xdg-utils ])  # its buttons run `drive`, and open the folder
      "--set" "SDL_APP_ID" "drive-status"  # the name the desktop looks the window's icon up by
    ];
  });
  statusEntry = pkgs.makeDesktopItem {  # not a launcher: it gives the window its name and the Drive's mark
    name = "drive-status";
    desktopName = "Google Drive";
    exec = "${status}/bin/drive-status";
    icon = "${icons}/synced.svg";
    noDisplay = true;
  };
  showStatus = pkgs.writeShellScript "drive-show-status" ''
    exec ${pkgs.systemd}/bin/systemd-run --user --collect --quiet --unit=drive-status ${status}/bin/drive-status  # one window: a click while it is open does nothing
  '';
in {
  inherit ask drive icons status statusEntry;
  # The tray's program with its pictures, the window a click opens, and the commands its menu runs.
  tray = pkgs.runCommand "drive-tray" { nativeBuildInputs = [ pkgs.makeWrapper ]; } ''
    makeWrapper ${tray}/bin/drive-tray $out/bin/drive-tray \
      --set DRIVE_TRAY_ICONS ${icons} --set-default DRIVE_TRAY_WINDOW ${showStatus} \
      --prefix PATH : ${pkgs.lib.makeBinPath [ drive pkgs.systemd pkgs.util-linux pkgs.gnugrep pkgs.xdg-utils ]}
  '';
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
  fake = build {  # the Drive API from memory, for tests
    name = "drive-fake";
    inherit src;
    programs.drive-fake = "testing/FakeDrive.btrc";
  };
}
