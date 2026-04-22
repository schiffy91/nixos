{ config, pkgs, lib, ... }: let
  cfg = config.settings.vfio;
  adminUser = config.settings.user.admin.username;
  gpuPci = "0000:01:00.0";
  audioPci = "0000:01:00.1";
  gpuVendorDevice = "10de:2684";
  audioVendorDevice = "10de:22ba";
  hookScript = pkgs.writeShellScript "qemu-hook" ''
    GUEST="$1"
    OPERATION="$2"
    [ "$GUEST" != "${cfg.vmName}" ] && exit 0
    unbind() { echo "$1" > "/sys/bus/pci/devices/$1/driver/unbind" 2>/dev/null || true; }
    bind_vfio() {
      unbind "$1"
      sleep 0.5
      echo "$2" > /sys/bus/pci/drivers/vfio-pci/new_id 2>/dev/null || true
      echo "$1" > /sys/bus/pci/drivers/vfio-pci/bind 2>/dev/null || true
    }
    bind_host() {
      unbind "$1"
      sleep 0.5
      echo "$1" > "/sys/bus/pci/drivers/$2/bind" 2>/dev/null || true
    }
    case "$OPERATION" in
      prepare)
        bind_vfio "${gpuPci}" "${gpuVendorDevice}"
        bind_vfio "${audioPci}" "${audioVendorDevice}"
        ;;
      release)
        bind_host "${gpuPci}" "nvidia"
        bind_host "${audioPci}" "snd_hda_intel"
        ;;
    esac
  '';
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
    ##### Libvirt hook for automatic GPU detach/attach #####
    systemd.tmpfiles.rules = [
      "L /var/lib/libvirt/hooks/qemu - - - - ${hookScript}"
    ];
    ##### ACPI tables available at /run/acpi-spoofed-tables/ #####
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
