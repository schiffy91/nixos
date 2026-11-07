{ system ? builtins.currentSystem, patched ? true, video ? false }:
(import ../capture { inherit system; lockSession = patched; }).extend {
  modules = [ ({ lib, ... }: {
    name = lib.mkOverride 40 (if !patched then "sleep-lock-restart-upstream" else if video then "sleep-lock-restart-video" else "sleep-lock-restart-control");
    nodes.machine = { config, pkgs, ... }: {
      users.users.observer.extraGroups = lib.optional video "video";
      environment.etc."restart-control-packages.json".text = builtins.toJSON {
        inherit video;
        inherit (config.settings.sleep) lockSession;
        kwin = toString pkgs.kdePackages.kwin;
        patches = map toString pkgs.kdePackages.kwin.patches;
      };
    };
    testScript = lib.mkOverride 40 (builtins.readFile ./test.py);
  }) ];
}
