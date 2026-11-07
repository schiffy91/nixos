{ system ? builtins.currentSystem }:
let
  locked = (builtins.fromJSON (builtins.readFile ../../../flake.lock)).nodes.nixpkgs.locked;
  nixpkgs = builtins.fetchTree { inherit (locked) type owner repo rev narHash; };
in
(import ../capture { inherit system; }).extend {
  modules = [ ({ lib, ... }: {
    name = lib.mkOverride 40 "sleep-lock-sunshine";
    nodes.machine = { pkgs, ... }: let
      client = pkgs.stdenv.mkDerivation {
        name = "sunshine-private-client";
        dontUnpack = true;
        nativeBuildInputs = [ pkgs.pkg-config pkgs.qt6.wrapQtAppsHook ];
        buildInputs = [ pkgs.qt6.qtbase pkgs.qt6.qtwayland ];
        buildPhase = ''$CXX -Wall -Wextra -Werror -fPIC ${./client.cpp} -o client $(pkg-config --cflags --libs Qt6Widgets)'';
        installPhase = ''mkdir -p $out/bin; cp client $out/bin/sunshine-private-client'';
      };
    in {
      users.users.observer.extraGroups = [ "video" "input" "audio" ];
      services.pipewire.pulse.enable = true;
      services.sunshine = {
        enable = true;
        autoStart = false;
        openFirewall = true;
        capSysAdmin = false;
        package = pkgs.sunshine.overrideAttrs (old: {
          patches = (old.patches or []) ++ [ ../../../nix/apps/gaming/sunshine/require-credentials.patch ];
        });
        applications.apps = [{ name = "Desktop"; }];
        settings = {
          capture = "kwin";
          output_name = "Virtual-Sleep capture test";
          encoder = "software";
          system_tray = "disabled";
          audio_sink = "fixture";
          credentials_file = "/home/observer/.config/sunshine/credentials.json";
        };
      };
      environment.systemPackages = [ client pkgs.pulseaudio ];
      environment.etc."sunshine-api.py".source = ./api.py;
    };
    nodes.client = { pkgs, ... }: {
      imports = [ (nixpkgs + "/nixos/tests/common/x11.nix") ];
      users.users.viewer = { isNormalUser = true; uid = 1000; };
      test-support.displayManager.auto.user = "viewer";
      environment.systemPackages = [ pkgs.moonlight-qt pkgs.xdotool ];
      virtualisation.memorySize = 4096;
      virtualisation.cores = 2;
    };
    testScript = lib.mkOverride 40 (builtins.readFile ./test.py);
  }) ];
}
