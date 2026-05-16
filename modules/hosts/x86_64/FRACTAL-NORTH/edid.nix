{ config, lib, pkgs, ... }:
let
  edid = pkgs.callPackage ../../../apps/sunshine/edid { };
  connector = edid.passthru.connector;
  mode = "1280x800@90";
  primary = lib.findFirst (o: o.primary) null config.settings.desktop.outputs;

  kd = "${pkgs.kdePackages.libkscreen}/bin/kscreen-doctor";
  enable = pkgs.writeShellScriptBin "sunshine-display-enable" ''
    ${kd} output.${connector}.enable output.${connector}.scale.1 output.${connector}.position.0,0 output.${connector}.primary
    ${kd} output.${connector}.mode.${mode} || true
    ${kd} output.${connector}.hdr.enable || true
  '';
  disable = pkgs.writeShellScriptBin "sunshine-display-disable" ''
    ${lib.optionalString (primary != null) "${kd} output.${primary.name}.primary"}
    ${kd} output.${connector}.disable || true
  '';
in {
  boot.kernelParams = [ "drm.edid_firmware=${connector}:${edid.passthru.firmwarePath}" ];
  hardware.firmware = [ edid ];
  environment.systemPackages = [ enable disable ];
}
