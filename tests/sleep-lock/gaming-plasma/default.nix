{ system ? builtins.currentSystem }:
(import ../authentication { inherit system; lockSession = false; }).extend {
  modules = [ ({ lib, ... }: {
    name = lib.mkForce "sleep-gaming-plasma";
    nodes.machine = { pkgs, ... }: {
      environment.systemPackages = [ pkgs.pamtester ];
      settings.sleep.lockGamingSession = true;
      environment.etc."xdg/kscreenlockerrc".text = lib.mkForce ''
        [Daemon]
        Autolock=true
        LockOnResume=true
        Lock=true
        LockGrace=0
        RequirePassword=true
        Timeout=10
      '';
      users.users.observer.password = lib.mkForce "aB3! z";
    };
    testScript = lib.mkForce (builtins.readFile ./test.py);
  }) ];
}
