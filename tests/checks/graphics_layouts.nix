{ lib }:
{ pkgs, system, configurations }:
let
  fractal = lib.findFirst (entry: entry.name == "FRACTAL-NORTH-Standard-Boot") null configurations;
  check = condition: message: lib.optional (!condition) message;
  failures = lib.optionals (fractal != null) (let
    declare = display: render: (fractal.configuration.extendModules {
      modules = [{ settings.graphics = { gpus.integrated.enable = lib.mkForce true; display = lib.mkForce display; render = lib.mkForce render; }; }];
    }).config;
    single = fractal.configuration.config;
    tunneled = declare "integrated" "nvidia";
    direct = declare "nvidia" "nvidia";
    missing = declare "absent" "nvidia";
    primary = config: (lib.findFirst (output: output.primary) null config.settings.desktop.outputs).name;
    television = config: lib.findFirst (name: config.settings.apps.utils.nixosctl.settings.displays.${name} == "S89C") null
      (builtins.attrNames config.settings.apps.utils.nixosctl.settings.displays);
  in
    check (single.settings.graphics.connectors == { nvidia = { DP = [ "DP-1" "DP-2" "DP-3" ]; "HDMI-A" = [ "HDMI-A-1" ]; }; }
      && primary single == "DP-1" && television single == "HDMI-A-1")
      "One GPU must own the first connector of every type."
    ++ check (!single.hardware.nvidia.prime.offload.enable && single.services.xserver.videoDrivers == [ "nvidia" ]
      && !single.hardware.amdgpu.initrd.enable && single.settings.boot.plymouth.drmDevice == ""
      && !(single.environment.sessionVariables ? KWIN_DRM_DEVICES)
      && single.settings.graphics.environment == { LIBVA_DRIVER_NAME = "nvidia"; VDPAU_DRIVER = "nvidia"; })
      "One GPU must not carry offload, a compositor device list, a pinned splash or another card's driver."
    ++ check (tunneled.settings.graphics.connectors.integrated.DP == [ "DP-1" "DP-2" ]
      && tunneled.settings.graphics.connectors.nvidia == { DP = [ "DP-3" "DP-4" "DP-5" ]; "HDMI-A" = [ "HDMI-A-2" ]; }
      && primary tunneled == "DP-1" && television tunneled == "HDMI-A-2")
      "A card that binds later must continue the first card's connector numbers."
    ++ check (tunneled.hardware.nvidia.prime.offload.enable
      && tunneled.hardware.nvidia.prime.amdgpuBusId == "PCI:107:0:0" && tunneled.hardware.nvidia.prime.nvidiaBusId == "PCI:1:0:0"
      && tunneled.settings.graphics.environment.__NV_PRIME_RENDER_OFFLOAD == "1"
      && tunneled.settings.apps.gaming.steam.extraEnv == tunneled.settings.graphics.environment
      && tunneled.services.xserver.videoDrivers == [ "amdgpu" "nvidia" ] && tunneled.hardware.amdgpu.initrd.enable)
      "Rendering on NVIDIA for an AMD display must enable offload with bus IDs taken from the PCI addresses."
    ++ check (tunneled.environment.sessionVariables.KWIN_DRM_DEVICES == "/dev/dri/nvidia-card:/dev/dri/integrated-card"
      && tunneled.settings.boot.plymouth.drmDevice == "/dev/dri/by-path/pci-0000:6b:00.0-card")
      "Two GPUs must composite on the render card and show the splash on the display card."
    ++ check (!direct.hardware.nvidia.prime.offload.enable && primary direct == "DP-3"
      && direct.environment.sessionVariables.KWIN_DRM_DEVICES == "/dev/dri/nvidia-card:/dev/dri/integrated-card"
      && direct.settings.boot.plymouth.drmDevice == "/dev/dri/by-path/pci-0000:01:00.0-card"
      && direct.services.xserver.videoDrivers == [ "nvidia" "amdgpu" ])
      "Displaying on the render card must drop offload while keeping the second card usable."
    ++ check (lib.any (entry: !entry.assertion) missing.assertions)
      "Naming a GPU that is not declared must fail an assertion."
  );
in {
  name = "graphics-layouts";
  value = pkgs.runCommand "graphics-layouts-${system}" { } (
    if failures == [ ] then "touch $out"
    else "echo ${lib.escapeShellArg (lib.concatStringsSep "; " failures)} >&2; exit 1");
}
