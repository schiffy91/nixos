{ config, pkgs, lib, inputs, ... }:
let
  worker = import ./opportunistic-unlock/package.nix { inherit pkgs inputs; };
in {
  config = lib.mkIf config.settings.disk.encryption.opportunisticUnlock.enable {
    assertions = [{
      assertion = config.settings.disk.encryption.enable;
      message = "Opportunistic unlock requires an encrypted root.";
    }];
    boot.initrd.systemd.package = config.systemd.package.overrideAttrs (old: {
      patches = (old.patches or []) ++ [ ./opportunistic-unlock/systemd-successful-passphrase.patch ];
      postPatch = (old.postPatch or "") + ''
        cp ${./opportunistic-unlock/handoff.h} src/cryptsetup/nixos-unlock-handoff.h
      '';
    });
    boot.initrd.luks.devices.${config.settings.disk.label.root}.crypttabExtraOpts = [ "x-nixos.opportunistic-passphrase" ];
    systemd.services.nixos-opportunistic-unlock = {
      description = "Unlock attached LUKS volumes with the successful boot password";
      wantedBy = [ "multi-user.target" ];
      after = [ "local-fs.target" ];
      # Type=exec acknowledges execution, not completion. No boot target waits
      # for device I/O or password work. This service never mounts filesystems.
      serviceConfig = {
        Type = "exec";
        ExecStart = "${worker}/bin/nixos-opportunistic-unlock ${lib.escapeShellArg config.settings.disk.label.root}";
        Restart = "no";
        RuntimeMaxSec = 25;
        TimeoutStopSec = 1;
        LimitCORE = 0;
        LimitMEMLOCK = "infinity";
        MemoryMax = "3G";
        MemorySwapMax = 0;
        OOMPolicy = "kill";
        UMask = "0077";
        NoNewPrivileges = true;
        ProtectHome = true;
        ProtectSystem = "full";
        PrivateTmp = true;
      };
    };
  };
}
