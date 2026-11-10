{ config, lib, pkgs, ... }:
let
  usesKeyboard = config.boot.initrd.unl0kr.enable or false;
  unlockAgent = if usesKeyboard then "unl0kr" else "plymouth";
  unlockUnit = if usesKeyboard then "unl0kr-agent.service" else "plymouth-start.service";
  healthReporter = pkgs.writeShellScript "e2e-health-report" ''
    set +e  # probes are best effort; the report must survive
    export PATH=${lib.makeBinPath [ pkgs.coreutils pkgs.gnugrep pkgs.systemd pkgs.btrfs-progs pkgs.util-linux pkgs.sbctl ]}:/run/current-system/sw/bin:$PATH
    report=/var/log/e2e-health.log
    exec > "$report" 2>&1
    ${pkgs.systemd}/bin/systemctl is-system-running --wait > /dev/null 2>&1
    printf 'E2E-HEALTH-BEGIN boot=%s\n' "$(${pkgs.systemd}/bin/journalctl --list-boots --no-pager 2>/dev/null | wc -l)"
    printf 'E2E-STATE %s\n' "$(${pkgs.systemd}/bin/systemctl is-system-running 2>/dev/null || true)"
    ${pkgs.systemd}/bin/systemctl --failed --no-legend --plain | while read -r unit _; do
      [ -n "$unit" ] && printf 'E2E-FAILED %s\n' "$unit"
    done
    ${pkgs.systemd}/bin/journalctl -b --no-pager -o cat 2>/dev/null | grep 'Service has no ExecStart' | while read -r line; do
      printf 'E2E-REFUSED %s\n' "$line"
    done
    printf 'E2E-HOMEMANAGER %s\n' "$(${pkgs.systemd}/bin/systemctl is-active home-manager-${config.settings.users.admin.username}.service 2>/dev/null || true)"
    printf 'E2E-UNLOCKAGENT %s %s\n' "${unlockAgent}" "$(${pkgs.systemd}/bin/journalctl -b --no-pager 2>/dev/null | grep -ci "${unlockAgent}" || true)"
    printf 'E2E-UNLOCKUNIT %s\n' "$(${pkgs.systemd}/bin/systemctl show -p Result --value '${unlockUnit}' 2>/dev/null || echo unknown)"
    signable="/boot/EFI/BOOT/|/boot/EFI/systemd/|/boot/EFI/Linux/"
    printf 'E2E-SBCTL %s\n' "$(sbctl verify 2>&1 | grep -E "$signable" | grep -c 'is not signed' || echo unavailable)"
    sbctl verify 2>&1 | grep -E "$signable" | grep 'is not signed' | while read -r unsigned; do
      printf 'E2E-UNSIGNED %s\n' "$unsigned"
    done
    printf 'E2E-SIGNED %s\n' "$(sbctl verify 2>&1 | grep -E "$signable" | grep -c 'is signed' || echo 0)"
    printf 'E2E-VENDORKEYS %s\n' "$(sbctl list-enrolled-keys 2>/dev/null | grep -ci 'microsoft' || echo 0)"
    printf 'E2E-CMDLINE-QUIET %s\n' "$(grep -c -E '(^| )quiet( |$)' /proc/cmdline || echo 0)"
    printf 'E2E-CMDLINE-LOGLEVEL %s\n' "$(tr ' ' '\n' < /proc/cmdline | grep '^loglevel=' | tail -n 1 | grep -c '^loglevel=[0-3]$' || echo 0)"
    printf 'E2E-SECUREBOOT %s\n' "$(${pkgs.systemd}/bin/bootctl status 2>/dev/null | grep -i 'secure boot' | head -1 | tr -s ' ' | tr -d '\n' || true)"
    printf 'E2E-SNAPSHOTS %s\n' "$(${pkgs.btrfs-progs}/bin/btrfs subvolume list / 2>/dev/null | grep -c CLEAN || true)"
    printf 'E2E-SWAP %s\n' "$(${pkgs.util-linux}/bin/swapon --noheadings --show=NAME 2>/dev/null | tr '\n' ' ' || true)"
    printf 'E2E-HEALTH-END\n'
    ${pkgs.coreutils}/bin/cat "$report" > /dev/ttyS0

    ${lib.optionalString (config.settings.boot.method == "Secure-Boot" && !config.settings.boot.autoEnrollKeys) ''
      enabled=/nix/var/e2e-secure-boot-enabled
      if [ ! -e "$enabled" ] && bootctl status 2>/dev/null | grep -qi setup; then
        printf 'E2E-ENABLING-SECURE-BOOT\n' > /dev/ttyS0
        log=/var/log/e2e-secure-boot.log
        systemctl start generate-sb-keys.service > "$log" 2>&1
        /run/current-system/bin/switch-to-configuration boot >> "$log" 2>&1
        signed=$?
        sbctl enroll-keys --yes-this-might-brick-my-machine >> "$log" 2>&1
        enrolled=$?
        printf 'E2E-SECURE-BOOT-ENABLE sign=%s enroll=%s\n' "$signed" "$enrolled" > /dev/ttyS0
        if [ "$signed" -eq 0 ] && [ "$enrolled" -eq 0 ]; then
          : > "$enabled"
        else
          tail -n 15 "$log" | while read -r detail; do printf 'E2E-SB-LOG %s\n' "$detail" > /dev/ttyS0; done
        fi
      fi
    ''}
    counter=/nix/var/e2e-hibernations
    cycles=$(cat "$counter" 2>/dev/null || echo 0)
    while [ "$cycles" -lt 2 ]; do
      cycles=$((cycles + 1))
      echo "$cycles" > "$counter"
      ${pkgs.coreutils}/bin/sync
      printf 'E2E-HIBERNATING cycle=%s\n' "$cycles" > /dev/ttyS0
      ${pkgs.systemd}/bin/systemctl hibernate
      printf 'E2E-RESUMED yes cycle=%s\n' "$cycles" > /dev/ttyS0
      left=$(ls /sys/firmware/efi/efivars 2>/dev/null | grep -c HibernateLocation || true)
      printf 'E2E-HIBERNATELOCATION %s swap=%s\n' "$left" "$(swapon --show=NAME --noheadings | tr '\n' ' ')" > /dev/ttyS0
    done
  '';
in
{
  boot.kernelParams = [ "console=tty0" "console=ttyS0,115200n8" ];
  boot.initrd.availableKernelModules = [
    "virtio_pci" "virtio_blk" "virtio_net" "nvme" "tpm_tis"
    "virtio_gpu" "bochs" "simpledrm"          # unl0kr needs a DRM device to draw on
    "xhci_pci" "usbhid" "hid_generic" "atkbd" "i8042"  # and a keyboard to read
  ];
  boot.initrd.kernelModules = [ "virtio_gpu" ];
  boot.lanzaboote = lib.mkIf (config.settings.boot.method == "Secure-Boot") {
    autoGenerateKeys.enable = true;
  };
  boot.loader.timeout = lib.mkForce 0;
  settings.boot.plymouth.drmDevice = lib.mkForce "";

  systemd.services.e2e-health-report = {
    description = "Launch the e2e health reporter outside the boot transaction";
    wantedBy = [ "multi-user.target" ];
    serviceConfig = {
      Type = "oneshot";
      RemainAfterExit = true;
      ExecStart = "${pkgs.systemd}/bin/systemd-run --no-block --unit=e2e-health-worker ${healthReporter}";
    };
  };
}