{ config, pkgs, lib, ... }:
let
  worker = import ./sd-unlock/package.nix { inherit pkgs; systemd = config.systemd.package; };
in {
  config = lib.mkIf config.settings.disk.encryption.unlockSD {
    assertions = [{
      assertion = config.settings.disk.encryption.enable;
      message = "Opportunistic SD unlock requires an encrypted root.";
    }];
    boot.initrd.systemd.package = config.systemd.package.overrideAttrs (old: {
      patches = (old.patches or []) ++ [ ./sd-unlock/systemd-successful-passphrase.patch ];
      postPatch = (old.postPatch or "") + ''
        cp ${./sd-unlock/handoff.h} src/cryptsetup/nixos-sd-handoff.h
      '';
    });
    boot.initrd.luks.devices.${config.settings.disk.label.root}.crypttabExtraOpts = [ "x-nixos.sd-passphrase" ];
    services.udisks2.enable = true;
    systemd.services.nixos-sd-unlock = {
      description = "Try the authenticated boot password once on an inserted SD card";
      wantedBy = [ "multi-user.target" ];
      after = [ "local-fs.target" ];
      # Type=exec acknowledges execution, not completion. No boot target waits
      # for card I/O, password work, udev, or the mount operation.
      serviceConfig = {
        Type = "exec";
        ExecStart = "${worker}/bin/nixos-sd-unlock ${lib.escapeShellArg config.settings.disk.label.root} ${lib.escapeShellArg config.settings.users.admin.username}";
        Restart = "no";
        RuntimeMaxSec = 25;
        TimeoutStopSec = 1;
        LimitCORE = 0;
        LimitMEMLOCK = "infinity";
        MemoryMax = "2G";
        MemorySwapMax = 0;
        OOMPolicy = "kill";
        UMask = "0077";
      };
    };
  };
}
