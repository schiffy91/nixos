{ host, ... }:
let mouse = host.input.mouse; in {
  boot.blacklistedKernelModules = [ "hid_sensor_hub" ];
  hardware = {
    xone.enable = true;              # Xbox controllers
    logitech.wireless.enable = true; # Logitech Bolt receiver
  };
  settings.input.libinputMice = [{
    vendorId  = mouse.vendorId;
    productId = mouse.productId;
    name      = mouse.name;
  }];
}
