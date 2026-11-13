{ config, lib, pkgs, ... }:
let
  settings = config.settings.apps.utils.cpuTuning;
  cores = builtins.length settings.offsets;
  # ryzen_smu's RSMU mailbox: six little-endian u32 args, then the command; status 1 = ok.
  # Zen 4 Raphael: 0x06 sets a core's CO, 0xD5 reads it; arg = CCD << 28 | slot << 20 | count.
  cpuTuning = pkgs.writeShellScriptBin "cpu-tuning" ''
    set -euo pipefail
    export PATH=${lib.makeBinPath [ pkgs.coreutils pkgs.gawk pkgs.kmod ]}
    state=/run/cpu-tuning
    mode="''${1:-}"
    case "$mode" in
      status) cat $state/mode 2> /dev/null || echo unknown; exit 0 ;;
      stock|bios|software) ;;
      *) echo "usage: cpu-tuning stock|bios|software|status" >&2; exit 2 ;;
    esac
    family=$(awk -F': ' '/^cpu family/ {print $2; exit}' /proc/cpuinfo); model=$(awk -F': ' '/^model\t/ {print $2; exit}' /proc/cpuinfo)
    [ "$family" = 25 ] && (( model >= 0x60 && model <= 0x6F )) || { echo "not a Zen 4 Raphael CPU (family $family model $model)" >&2; exit 1; }
    smu=/sys/kernel/ryzen_smu_drv
    [ -e $smu/smu_args ] || modprobe ryzen_smu 2> /dev/null || modprobe -d /run/current-system/kernel-modules ryzen_smu  # added since boot
    u32() { local v=$(( $1 & 0xFFFFFFFF )); printf '\\x%02x\\x%02x\\x%02x\\x%02x' $(( v & 255 )) $(( v >> 8 & 255 )) $(( v >> 16 & 255 )) $(( v >> 24 & 255 )); }
    send() {
      printf "$(u32 "$2")$(u32 0)$(u32 0)$(u32 0)$(u32 0)$(u32 0)" | dd of=$smu/smu_args bs=24 count=1 iflag=fullblock status=none
      printf "$(u32 "$1")" | dd of=$smu/rsmu_cmd bs=4 count=1 iflag=fullblock status=none
      [ "$(od -An -tu4 -N4 $smu/rsmu_cmd | tr -d ' ')" = 1 ] || { echo "SMU rejected command $1" >&2; return 1; }
      od -An -tu4 -N4 $smu/smu_args | tr -d ' '
    }
    address() { echo $(( ($1 / 8) << 28 | ($1 % 8) << 20 )); }
    get() { local count=$(( $(send 0xD5 "$(address "$1")") & 0xFFFF )); (( count < 0x8000 )) || count=$(( count - 0x10000 )); echo $count; }
    mkdir -p $state
    if [ ! -e $state/bios ]; then  # first run this boot: nothing has overwritten the firmware's counts yet
      snapshot=""; for core in $(seq 0 ${toString (cores - 1)}); do snapshot="$snapshot$(get $core) "; done
      echo "$snapshot" > $state/bios.tmp && mv $state/bios.tmp $state/bios
    fi
    case "$mode" in
      stock) counts=$(printf '0 %.0s' $(seq ${toString cores})) ;;
      bios) counts=$(cat $state/bios) ;;
      software) counts="${lib.concatMapStringsSep " " toString settings.offsets}" ;;
    esac
    core=0
    for count in $counts; do
      send 0x06 $(( $(address $core) | (count & 0xFFFF) )) > /dev/null
      [ "$(get $core)" = "$count" ] || { echo "core $core: wrote $count, read back $(get $core)" >&2; exit 1; }
      core=$(( core + 1 ))
    done
    echo "$mode" > $state/mode
    echo "$mode: $counts"
  '';
  admin = config.settings.users.admin.username;
  radio = mode: { kind = "radio"; run = "systemctl start cpu-tuning@${mode}.service"; status = "test \"$(cpu-tuning status)\" = ${mode}"; };
in lib.mkIf settings.enable {
  assertions = [{ assertion = cores > 0; message = "settings.apps.utils.cpuTuning.offsets lists one count per core."; }];
  boot.extraModulePackages = [ config.boot.kernelPackages.ryzen-smu ];
  boot.kernelModules = [ "ryzen_smu" ];
  environment.systemPackages = [ cpuTuning ];
  settings.apps.utils.nixosctl.tray.menu.Tuning.CPU = {
    Stock = radio "stock";
    "Tuned (BIOS)" = radio "bios";
    "Tuned (Software)" = radio "software";
  };
  systemd.services."cpu-tuning@" = {
    description = "Set the CPU Curve Optimizer to %i";
    after = [ "systemd-modules-load.service" ];
    serviceConfig = {
      Type = "oneshot";
      ExecStart = "${cpuTuning}/bin/cpu-tuning %i";
    };
  };
  systemd.services.cpu-tuning = {
    description = "Apply the boot CPU Curve Optimizer mode";
    wantedBy = [ "multi-user.target" ];
    after = [ "systemd-modules-load.service" ];
    serviceConfig = {
      Type = "oneshot";
      RemainAfterExit = true;
      ExecStart = "${cpuTuning}/bin/cpu-tuning ${settings.mode}";
    };
  };
  # The tray switches modes as the admin without a password prompt, and only through these units.
  security.polkit.extraConfig = ''
    polkit.addRule(function(action, subject) {
      if (action.id == "org.freedesktop.systemd1.manage-units" && subject.user == "${admin}"
          && /^cpu-tuning@(stock|bios|software)\.service$/.test(action.lookup("unit") || "")
          && action.lookup("verb") == "start") {
        return polkit.Result.YES;
      }
    });
  '';
}
