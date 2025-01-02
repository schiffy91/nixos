{ pkgs, inputs, config, lib, ... }:
{
  imports = [ inputs.lanzaboote.nixosModules.lanzaboote ];
  boot = {
    lanzaboote = {
      enable = false; # Overridden by secure-boot target in ../flake.nix
      pkiBundle = config.variables.disk.pkiBundle;
    };
    loader = {
      timeout = 5;
      systemd-boot = {
        enable = if !config.boot.lanzaboote.enable then true else false;
        configurationLimit = 3;
        consoleMode = "max";
        editor = false;
      };
      efi = {
        canTouchEfiVariables = true;
        efiSysMountPoint = "/boot";
      };
    };
    initrd = {
      systemd.enable = true;
      verbose = false;
    };
    plymouth = {
      enable = true;
      theme = "breeze";
    };
    kernelParams = [
      "quiet"
      "splash"
      "loglevel=3"                      # Reduce kernel log verbosity
      "vt.global_cursor_default=0"      # Hide the console cursor during boot
      "rd.udev.log_priority=3"          # Minimize udev messages
      "systemd.show_status=auto"        # Hide systemd boot status unless there's an error
      "rd.systemd.show_status=auto"     # Same as above, but for the initrd phase
      "plymouth.ignore-serial-consoles" # Force display rendering
    ];
    kernelPackages = lib.mkDefault pkgs.linuxPackages_latest;
  };
}