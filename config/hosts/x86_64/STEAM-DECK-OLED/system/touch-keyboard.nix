{ ... }:
let
  # These are built-in firmware/controller interfaces, not attached typing keyboards.
  # Real USB and Bluetooth keyboards still suppress the automatic touch keyboard.
  rules = ''
    SUBSYSTEM=="input", KERNEL=="event*", ATTRS{name}=="AT Translated Set 2 keyboard", ENV{PLYMOUTH_IGNORE_KEYBOARD}="1"
    SUBSYSTEM=="input", KERNEL=="event*", ATTRS{name}=="Valve Software Steam Controller", ENV{PLYMOUTH_IGNORE_KEYBOARD}="1"
  '';
in {
  services.udev.extraRules = rules;
  boot.initrd.services.udev.rules = rules;
}
