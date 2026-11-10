{ config, lib, ... }:
let
  graphics = config.settings.graphics;
  gpus = lib.filterAttrs (_: gpu: gpu.enable) graphics.gpus;
  names = builtins.attrNames gpus;
  valid = gpus ? ${graphics.display} && gpus ? ${graphics.render};
  display = gpus.${graphics.display};
  render = gpus.${graphics.render};
  offload = graphics.render != graphics.display;
  several = builtins.length names > 1;
  uses = driver: lib.any (gpu: gpu.driver == driver) (builtins.attrValues gpus);
  field = index: text: builtins.elemAt (lib.splitString ":" text) index;
  busId = pci: let slot = lib.splitString "." (field 2 pci);  # 0000:6b:00.0 is PCI:107:0:0
    in "PCI:${toString (lib.fromHexString (field 1 pci))}:${toString (lib.fromHexString (builtins.head slot))}:${lib.last slot}";
  bound = lib.sortOn (name: (if gpus.${name}.driver == "nvidia" then "1" else "0") + gpus.${name}.pci) names;  # in-tree drivers bind in the initrd, NVIDIA's later
  connectors = (lib.foldl' (seen: name: {  # the kernel numbers each type in binding order
    count = seen.count // lib.mapAttrs (type: ports: (seen.count.${type} or 0) + ports) gpus.${name}.connectors;
    names = seen.names // {
      ${name} = lib.mapAttrs (type: ports:
        lib.genList (port: "${type}-${toString ((seen.count.${type} or 0) + port + 1)}") ports) gpus.${name}.connectors;
    };
  }) { count = {}; names = {}; } bound).names;
in {
  config = lib.mkMerge [
    {
      settings.graphics = {
        inherit connectors;
        environment = lib.optionalAttrs (valid && render.driver == "nvidia") ({
          LIBVA_DRIVER_NAME = "nvidia";
          VDPAU_DRIVER = "nvidia";
        } // lib.optionalAttrs offload {
          __NV_PRIME_RENDER_OFFLOAD = "1";
          __NV_PRIME_RENDER_OFFLOAD_PROVIDER = "NVIDIA-G0";
          __VK_LAYER_NV_optimus = "NVIDIA_only";
          __GLX_VENDOR_LIBRARY_NAME = "nvidia";
        });
      };
      assertions = lib.optionals (graphics.gpus != {}) [
        {
          assertion = valid;
          message = "settings.graphics.display and settings.graphics.render must each name an enabled GPU.";
        }
        {
          assertion = !valid || !offload || (render.driver == "nvidia" && display.driver == "amdgpu");
          message = "Rendering on one GPU and displaying on another is only wired for NVIDIA rendering to an AMD display.";
        }
      ];
    }
    (lib.mkIf valid {
      services.xserver.videoDrivers = lib.unique (map (name: gpus.${name}.driver) ([ graphics.display ] ++ names));
      boot.kernelParams = lib.optional (uses "amdgpu") "amdgpu.dc=1";
      hardware.amdgpu.initrd.enable = uses "amdgpu";
      hardware.nvidia.prime = lib.mkIf offload {
        offload.enable = true;
        offload.enableOffloadCmd = true;
        amdgpuBusId = busId display.pci;
        nvidiaBusId = busId render.pci;
      };
      environment.sessionVariables = graphics.environment // lib.optionalAttrs several {
        KWIN_DRM_DEVICES = lib.concatMapStringsSep ":" (name: "/dev/dri/${name}-card")
          ([ graphics.render ] ++ lib.remove graphics.render names);  # KWin composites on the first
      };
      services.udev.extraRules = lib.mkIf several (lib.concatMapStrings (name: ''
        SUBSYSTEM=="drm", KERNEL=="card[0-9]*", ENV{ID_PATH}=="pci-${gpus.${name}.pci}", SYMLINK+="dri/${name}-card"
      '') names);  # KWin splits on ':', which by-path names contain
      settings.boot.plymouth.drmDevice = lib.mkIf several (lib.mkDefault "/dev/dri/by-path/pci-${display.pci}-card");  # one GPU follows the firmware framebuffer
    })
  ];
}
