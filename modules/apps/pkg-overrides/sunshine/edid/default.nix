{ pkgs, ... }: pkgs.runCommandLocal "llv-streaming-edid" {
  nativeBuildInputs = [ pkgs.python3 ];
  passthru.connector = "DP-3";
  passthru.firmwarePath = "edid/llv-streaming.bin";
} ''
  mkdir -p $out/lib/firmware/edid
  python3 ${./generate.py} > $out/lib/firmware/edid/llv-streaming.bin
  ${pkgs.edid-decode}/bin/edid-decode --check $out/lib/firmware/edid/llv-streaming.bin > /dev/null
''
