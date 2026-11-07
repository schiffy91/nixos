{ config, lib, pkgs, inputs, ... }:
let
  cfg = config.boot.lanzaboote;
  enabled = config.settings.tpm.managedPolicy.enable;
  state = "/var/lib/nixos/tpm-policy";
  lanzaboote = inputs.lanzaboote.packages.${pkgs.stdenv.hostPlatform.system};
  strictStub = lanzaboote.stub.overrideAttrs (old: {
    patches = (old.patches or [ ]) ++ [ ./lanzaboote-restrict-initrd.patch ];
  });
  policyConfig = pkgs.writeText "nixos-tpm-policy.json" (builtins.toJSON {
    inherit state;
    policy = cfg.measuredBoot.pcrlockPolicy;
    components = cfg.measuredBoot.pcrlockDirectory;
    staticComponents = config.environment.etc.pcrlock.source;
    esps = [ config.boot.loader.efi.efiSysMountPoint ] ++ cfg.extraEfiSysMountPoints;
    rootDevice = config.settings.disk.by.partlabel.root;
    device = config.settings.tpm.device;
    pcrlock = "${config.systemd.package}/lib/systemd/systemd-pcrlock";
    installCommand = cfg.installCommand;
    publicKey = toString cfg.publicKeyFile;
    privateKey = toString cfg.privateKeyFile;
    protectedSystem = cfg.protectedSystem;
  });
  helper = pkgs.writeShellApplication {
    name = "nixos-tpm-policy";
    runtimeInputs = [ pkgs.python3 pkgs.cryptsetup config.systemd.package ];
    text = ''
      export PATH=${config.systemd.package}/lib/systemd:$PATH
      export LD_LIBRARY_PATH=${config.systemd.package}/lib/cryptsetup
      exec python3 ${./tpm-policy.py} ${policyConfig} "$@"
    '';
  };
in {
  config = lib.mkIf enabled {
    assertions = [
      { # The pinned NixOS has only systemd initrds; the former
        # boot.initrd.systemd.enable option has been removed.
        assertion = cfg.enable && config.boot.initrd.enable && config.boot.initrd.systemd.tpm2.enable;
        message = "Managed TPM unlock requires Lanzaboote and a systemd initrd."; }
      { assertion = config.settings.disk.encryption.enable;
        message = "Managed TPM unlock requires an encrypted root."; }
      { assertion = !config.settings.users.admin.autoLogin.enable
          && !config.services.displayManager.autoLogin.enable
          && config.services.getty.autologinUser == null;
        message = "TPM automatic disk unlock does not authenticate a user; disable automatic login first."; }
    ];
    # The pinned stub otherwise appends sysexts even after failed measurements.
    # This opt-in boot policy refuses executable companions before Linux starts;
    # credential transport remains available for the authenticated PCR policy.
    boot.lanzaboote.package = lib.mkForce (lanzaboote.lzbt.override { stub = strictStub; });
    boot.initrd.luks.devices.${config.settings.disk.label.root}.crypttabExtraOpts =
      lib.mkAfter [ "tpm2-device=${config.settings.tpm.device}" ];
    boot.lanzaboote.measuredBoot = {
      enable = true;
      # This upstream enum governs its component producers. Our replacement
      # writer additionally requires empty PCR13 in the NV policy itself.
      pcrs = lib.mkForce [ 4 7 ];
      autoCryptenroll.enable = lib.mkForce false;
    };
    # Presence in a signed generation is necessary for admission, not a numeric
    # generation threshold: an old fallback may have been given a new number.
    system.systemBuilderCommands = ''echo 1 > "$out/tpm-policy-version"'';
    environment.systemPackages = [ helper ];
    environment.etc."nixosctl/tpm-policy".source = "${helper}/bin/nixos-tpm-policy";
    systemd.tmpfiles.rules = [ "d ${state} 0700 root root -" ];
    # Replace BOTH upstream writers. Their default permits missing PCRs to be
    # dropped, which can otherwise turn a requested PCR4 policy into PCR7 only.
    boot.loader.external.installHook = lib.mkForce "${helper}/bin/nixos-tpm-policy install";
    systemd.services.systemd-pcrlock-make-policy.serviceConfig.ExecStart = lib.mkForce [
      "" "${helper}/bin/nixos-tpm-policy update"
    ];
    # Custom reset layouts may mount persistent component directories separately.
    # Do not let the upstream producers write beneath those pending mounts.
    systemd.services.systemd-pcrlock-secureboot-policy.after = [ "local-fs.target" ];
    systemd.services.systemd-pcrlock-secureboot-authority.after = [ "local-fs.target" ];
    # Switching configuration may prepare an eligible next boot while the
    # current boot is still old. Enrollment remains explicitly guarded.
    systemd.services.systemd-pcrlock-make-policy.unitConfig.ConditionPathExists =
      "/run/booted-system/tpm-policy-version";
    # Companion extensions are not part of the signed thin image. Empty PCR13
    # protects both the LUKS token and the policy's sealed NV-update secret.
    boot.kernelParams = [ "systemd.credentials_boot_policy=strict" ];
  };
}
