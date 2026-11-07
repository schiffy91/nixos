{ flake ? builtins.getFlake "git+file://${toString ../..}" }:
let
  host = flake.nixosConfigurations.FRACTAL-NORTH-Secure-Boot.extendModules {
    modules = [ { settings.boot.verifyModules = true; } ];
  };
in assert host.config.boot.uki.settings.UKI.Linux ==
  "${host.config.system.build.kernel}/${host.config.system.boot.loader.kernelFile}";
  host.config.system.build.kernel
