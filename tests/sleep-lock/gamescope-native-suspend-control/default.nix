{ system ? builtins.currentSystem, renderNode ? "/dev/dri/renderD128" }:
(import ../gamescope-native { inherit system renderNode; }).extend {
  modules = [ ({ lib, ... }: {
    name = lib.mkForce "sleep-gamescope-native-suspend-control";
    nodes.machine.services.logind.settings.Login.HandlePowerKey = "ignore";
    testScript = lib.mkForce (builtins.readFile ./test.py);
  }) ];
}
