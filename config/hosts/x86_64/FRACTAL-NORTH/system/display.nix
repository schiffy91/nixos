{ host, ... }: {
  settings.desktop.outputs = [{
    name = host.display.primary.name;
    scaleFactor = host.display.primary.scaleFactor;
    primary = true;
  }];

  # The XDR's internal control device exposes a serial interface; keep ModemManager from probing it.
  services.udev.extraRules = ''
    SUBSYSTEM=="tty", ATTRS{idVendor}=="05ac", ATTRS{idProduct}=="9243", ENV{ID_MM_DEVICE_IGNORE}="1"
  '';
}
