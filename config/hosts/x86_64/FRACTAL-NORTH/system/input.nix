{ host, ... }:
let mouse = host.input.mouse; in {
  boot.blacklistedKernelModules = [ "hid_sensor_hub" ];
  hardware = {
    xone.enable = true;              # Xbox controllers
  };
  settings.input.libinputMice = [{
    inherit (mouse) vendorId productId name;
  }];
}
