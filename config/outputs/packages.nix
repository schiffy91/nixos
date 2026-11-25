{ inputs, systems, installerPackages }:
let
  x86 = "x86_64-linux";
  arm = "aarch64-linux";
  pkgs = import inputs.nixpkgs { system = x86; config.allowUnfree = true; };
  vmDeveloperImage = (systems.nixosConfigurations."VM-DEVELOPER-Standard-Boot".extendModules {
    modules = [ ../system/nixos/images/vm-disk.nix ];
  }).config.system.build.image;
in installerPackages // {
  ${x86} = installerPackages.${x86} // {
    slopsmith = import ../pkgs/gaming/slopsmith/package.nix { inherit pkgs inputs; };
  };
  ${arm} = installerPackages.${arm} // {
    vm-developer-image = vmDeveloperImage;
  };
}
