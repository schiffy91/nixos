{ config, ... }: {
  boot = {
    initrd.systemd.enable = true;
    loader = {
      efi = {
        canTouchEfiVariables = true;
        efiSysMountPoint = config.shared.driveConfig.efiSysMountPoint;
      };
    };
    plymouth = {
      enable = true;
      theme = "breeze";
    };
    kernelParams = ["quiet"];
  };
}