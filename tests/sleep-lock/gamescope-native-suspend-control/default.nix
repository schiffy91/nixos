{ system ? builtins.currentSystem, renderNode ? "/dev/dri/renderD128" }:
(import ../gamescope-native {
  inherit system renderNode;
  settings = { mode = "suspend"; realSuspend = true; withSunshine = false; };
  extraModules = [ ({ lib, ... }: {
    name = lib.mkForce "sleep-gamescope-native-suspend-control";
    nodes.machine.services.logind.settings.Login.HandlePowerKey = "ignore";
  }) ];
})
