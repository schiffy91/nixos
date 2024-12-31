{ pkgs, lib, inputs, config, ... }:
{
  imports = [ inputs.lanzaboote.nixosModules.lanzaboote ];
  config = {
    boot = {
      lanzaboote = {
        enable = lib.mkDefault false; # run /etc/nixos/scripts/secure-boot enable after initial install
        pkiBundle = "/var/lib/sbctl";
      };
      loader = {
        timeout = 5;
        systemd-boot = {
          enable = lib.mkIf (!config.boot.lanzaboote.enable) true;
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
      kernelParams = [ # Hide most messages during boot
        "quiet"
        "splash"
        "loglevel=3"                  # Reduce kernel log verbosity
        "vt.global_cursor_default=0"  # Hide the console cursor during boot
        "rd.udev.log_priority=3"      # Minimize udev messages
        "systemd.show_status=auto"    # Hide systemd boot status unless there's an error
        "rd.systemd.show_status=auto" # Same as above, but for the initrd phase
      ];
      kernelPackages = lib.mkDefault pkgs.linuxPackages_latest;
    };
  };
}