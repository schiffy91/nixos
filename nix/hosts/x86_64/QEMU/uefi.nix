{ lib, ... }:
{
  boot = {
    loader = {
      efi.canTouchEfiVariables = lib.mkDefault true;
      timeout = lib.mkDefault 0;
    };
    kernelParams = [
      "console=ttyS0,115200n8"
      "console=tty0"
    ];
  };
}
