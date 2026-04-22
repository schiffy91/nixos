{ config, pkgs, lib, ... }: let
  cfg = config.settings.vfio;
  adminUser = config.settings.user.admin.username;
  ##### Hypervisor-Phantom QEMU anti-detection patch (AMD, QEMU 10.2.0) #####
  antiDetectPatch = pkgs.fetchurl {
    url = "https://raw.githubusercontent.com/Scrut1ny/Hypervisor-Phantom/main/patches/QEMU/AMD-v10.2.0.patch";
    sha256 = "sha256-kwrfMqBZkCBmclxJJxJ/RTwLBwCKH18Lx6Zr2VFlYBY=";
  };
  patchedQemu = pkgs.qemu.overrideAttrs (old: {
    patches = (old.patches or []) ++ [ antiDetectPatch ];
  });
  ##### Compile ACPI SSDT tables (fake battery + spoofed devices) #####
  acpiTables = pkgs.stdenvNoCC.mkDerivation {
    name = "acpi-spoofed-tables";
    src = ./acpi;
    nativeBuildInputs = [ pkgs.acpica-tools ];
    buildPhase = ''
      iasl -p fake_battery fake_battery.dsl
      iasl -p spoofed_devices spoofed_devices.dsl
    '';
    installPhase = ''
      mkdir -p $out
      cp fake_battery.aml spoofed_devices.aml $out/
    '';
  };
in lib.mkMerge [
  (lib.mkIf cfg.enable {
    ##### QEMU with Hypervisor-Phantom anti-detection patch #####
    virtualisation.libvirtd.qemu = {
      package = patchedQemu;
      runAsRoot = true;
    };
    ##### User groups for VFIO + input access #####
    users.users.${adminUser}.extraGroups = [ "input" "kvm" ];
    ##### Libvirt hooks (SharkWipf dispatcher + per-VM start/revert) #####
    ##### Pattern: PassthroughPOST/VFIO-Tools + joeknock90/QaidVoid       #####
    environment.etc."libvirt/hooks/qemu" = {
      source = ./vfio_hooks/qemu;
      mode = "0755";
    };
    environment.etc."libvirt/hooks/qemu.d/${cfg.vmName}/prepare/begin/start.sh" = {
      source = ./vfio_hooks/start.sh;
      mode = "0755";
    };
    environment.etc."libvirt/hooks/qemu.d/${cfg.vmName}/release/end/revert.sh" = {
      source = ./vfio_hooks/revert.sh;
      mode = "0755";
    };
    ##### Log dir for hook output #####
    systemd.tmpfiles.rules = [
      "d /var/log/libvirt 0755 root root -"
    ];
    ##### ACPI tables available at /etc/acpi-spoofed-tables/ #####
    environment.etc."acpi-spoofed-tables/fake_battery.aml".source = "${acpiTables}/fake_battery.aml";
    environment.etc."acpi-spoofed-tables/spoofed_devices.aml".source = "${acpiTables}/spoofed_devices.aml";
  })
  ##### Looking Glass (KVMFR) #####
  (lib.mkIf (cfg.enable && cfg.lookingGlass.enable) {
    boot.extraModulePackages = [ config.boot.kernelPackages.kvmfr ];
    boot.kernelModules = [ "kvmfr" ];
    boot.extraModprobeConfig = "options kvmfr static_size_mb=${toString cfg.lookingGlass.sharedMemoryMB}";
    services.udev.extraRules = ''
      SUBSYSTEM=="kvmfr", OWNER="${adminUser}", GROUP="kvm", MODE="0660"
    '';
  })
  ##### evdev input sharing #####
  (lib.mkIf (cfg.enable && cfg.evdev.enable) {
    services.udev.extraRules = ''
      KERNEL=="event*", SUBSYSTEM=="input", GROUP="kvm", MODE="0660"
    '';
  })
]
