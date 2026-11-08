{ pkgs, inputs
, rocksmithDlc ? "$HOME/.local/share/Steam/steamapps/common/Rocksmith2014/dlc"
, configDir ? "$HOME/Games/Rocksmith/slopsmith/config"
, port ? 8000
}:

let
  iconSvg = pkgs.writeText "slopsmith.svg" ''
    <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 128 128">
      <rect width="128" height="128" rx="24" fill="#202326"/>
      <path d="M30 70c18-33 48-33 68 0" fill="none" stroke="#f4c95d" stroke-width="12" stroke-linecap="round"/>
      <path d="M37 83h54" stroke="#6ed3cf" stroke-width="10" stroke-linecap="round"/>
      <circle cx="41" cy="63" r="7" fill="#f7f7f7"/>
      <circle cx="64" cy="53" r="7" fill="#f7f7f7"/>
      <circle cx="87" cy="63" r="7" fill="#f7f7f7"/>
    </svg>
  '';
  icons = pkgs.runCommand "slopsmith-icons" { } ''
    install -Dm644 ${iconSvg} $out/share/icons/hicolor/scalable/apps/slopsmith.svg
  '';
  src = pkgs.fetchFromGitHub {
    owner = "schiffy91";
    repo = "slopsmith";
    rev = "846d5e08c85b3ad53c1fa42a0ad53e89431cec8f";
    hash = "sha256-Rf2aFCxcfqV97JFLQ1A+LNe/rHw+Ypl2c8iV31o2Okc=";
  };
  configuration = pkgs.writeText "slopsmith.json" (builtins.toJSON {
    inherit rocksmithDlc configDir port;
    source = src;
  });
  controller = (import ../lib/build-tools.nix { inherit pkgs inputs; }) {
    name = "slopsmith-runtime";
    sources = [ ./src ../../../lib/BackgroundProcess.btrc ];
    programs.slopsmith-runtime = "slopsmith/src/Main.btrc";
    runtimeInputs = with pkgs; [ bash coreutils curl podman podman-compose xdg-utils ];
  };
  command = name: action: pkgs.writeShellApplication {
    inherit name;
    text = ''exec ${controller}/bin/slopsmith-runtime ${configuration} ${action}'';
  };
  launcher = command "slopsmith" "launch";
  session = command "slopsmith-session" "session";
  stop = command "slopsmith-stop" "stop";
  desktop = pkgs.makeDesktopItem {
    name = "slopsmith";
    desktopName = "Slopsmith";
    genericName = "Rocksmith DLC Manager";
    exec = "${session}/bin/slopsmith-session";
    icon = "slopsmith";
    categories = [ "Game" "AudioVideo" "Audio" ];
    comment = "Manage Rocksmith 2014 custom DLC";
    terminal = true;
    startupNotify = true;
    keywords = [ "Rocksmith" "CDLC" "Guitar" "Bass" ];
  };
in pkgs.symlinkJoin {
  name = "slopsmith";
  paths = [ launcher session stop desktop icons ];
}
