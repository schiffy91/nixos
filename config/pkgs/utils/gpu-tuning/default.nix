{ config, lib, pkgs, ... }:
let
  settings = config.settings.apps.utils.gpuTuning;
  json = builtins.toJSON;
  # LACT's daemon owns the NVIDIA V/F curve (undocumented driver calls). The curve is
  # built from the card's live stock curve, so the setting stays mV + MHz. Its socket
  # is open to wheel, so the tray runs this as the admin.
  gpuTuning = pkgs.writeShellScriptBin "gpu-tuning" ''
    set -euo pipefail
    export PATH=${lib.makeBinPath [ pkgs.socat pkgs.jq pkgs.coreutils ]}
    lact() { printf '%s\n' "$1" | socat -t 30 - UNIX-CONNECT:/run/lactd.sock; }
    request() { jq -nc --arg command "$1" --argjson args "$2" '{command: $command, args: $args}'; }
    for _ in $(seq 60); do lact '{"command":"ping"}' > /dev/null 2>&1 && break; sleep 1; done
    id=$(lact '{"command":"list_devices"}' | jq -er '[.data[] | select(.id | startswith("10DE"))][0].id')
    device=$(jq -nc --arg id "$id" '{id: $id}')
    commit() {
      lact "$(request set_gpu_config "$(jq -nc --arg id "$id" --argjson config "$1" '{id: $id, config: $config}')")" | jq -e '.status == "ok"' > /dev/null
      sleep 1  # LACT reverts unconfirmed settings after a few seconds
      lact '{"command":"confirm_pending_config","args":{"command":"confirm"}}' | jq -e '.status == "ok"' > /dev/null
    }
    case "''${1:-}" in
      status)
        lact "$(request get_gpu_config "$device")" | jq -r 'if (.data // {} | .nvidia_gpu_vf_curve // .mem_clock_offsets // .power_cap) == null then "stock" else "tuned" end' ;;
      stock)
        commit '{}' && echo "stock" ;;
      tuned)
        curve=$(lact "$(request device_clocks_info "$device")" | jq -c '.data.table.value.gpu_vf_curve')
        power=$(lact "$(request device_stats "$device")" | jq -c '.data.power')
        tuning=$(jq -nc --argjson curve "$curve" --argjson power "$power" \
          --argjson voltage ${json settings.voltage} --argjson clock ${json settings.clock} \
          --argjson memory ${json (settings.memoryOffset * 2)} --argjson percent ${json settings.powerLimit} '
          (if $voltage == null then {} else
            ($curve | map(select(.base_voltage >= $voltage)) | min_by(.base_voltage).index) as $anchor
            | {nvidia_gpu_vf_curve: ($curve | map(select(.index >= $anchor))
                | map({key: (.index | tostring), value: {clockspeed_offset: ($clock - .base_freq), voltage: .base_voltage}}) | from_entries)}
          end)
          + (if $memory == 0 then {} else {mem_clock_offsets: {"0": $memory}} end)
          + (if $percent == null then {} else {power_cap: ($power.cap_default * $percent / 100)} end)')
        commit "$tuning"
        echo "tuned: $(jq -c '{power_cap, mem_clock_offsets, curve_points: (.nvidia_gpu_vf_curve // {} | length)}' <<< "$tuning")" ;;
      *) echo "usage: gpu-tuning tuned|stock|status" >&2; exit 2 ;;
    esac
  '';
  radio = mode: { kind = "radio"; run = "gpu-tuning ${mode}"; status = "test \"$(gpu-tuning status)\" = ${mode}"; };
in lib.mkIf settings.enable {
  assertions = [{
    assertion = (settings.voltage == null) == (settings.clock == null);
    message = "settings.apps.utils.gpuTuning.voltage and .clock are set together.";
  }];
  services.lact.enable = true;
  environment.systemPackages = [ gpuTuning ];
  settings.apps.utils.nixosctl.tray.menu.Tuning.GPU = { Stock = radio "stock"; Tuned = radio "tuned"; };
  systemd.services.gpu-tuning = {
    description = "Apply the boot GPU tuning through LACT";
    wantedBy = [ "graphical.target" ];  # LACT's daemon starts after multi-user.target, so wanting this there was an ordering cycle; systemd broke it by dropping one of the two at boot
    requires = [ "lactd.service" ];
    after = [ "lactd.service" ];
    unitConfig.ConditionVirtualization = "no";  # a VM of this host has no such GPU
    serviceConfig = {
      Type = "oneshot";
      RemainAfterExit = true;
      ExecStart = "${gpuTuning}/bin/gpu-tuning ${settings.mode}";
    };
  };
}
