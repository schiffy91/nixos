{ config, host, ... }: {
  boot.blacklistedKernelModules = [ "hid_sensor_hub" ];
  ##### XBox Controller #####
  hardware.xone.enable = true;
  ##### Logitech Bolt #####
  hardware.logitech.wireless.enable = true;
  home-manager.users.${config.settings.user.admin.username}.programs.plasma.configFile.kcminputrc."Libinput][${host.input.logitechBolt.vendorId}][${host.input.logitechBolt.productId}][${host.input.logitechBolt.mouseName}" = {
    PointerAccelerationProfile = 2;  # 2=flat, no accel
    PointerAcceleration = "0.000";
  };
}
