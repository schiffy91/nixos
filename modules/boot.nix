{ pkgs, lib, inputs, config, ... }:
{
  imports = [ inputs.lanzaboote.nixosModules.lanzaboote ];
  config = {
    boot = {
      lanzaboote = lib.mkDefault {
        enable = lib.mkDefault false; # overriden by secure-boot target in ../flake.nix, which can be chosen by running bin/nixos-secure-boot --enable
        pkiBundle = lib.mkDefault config.variables.disk.pkiBundle;
      };
      loader = lib.mkDefault {
        timeout = lib.mkDefault 5;
        systemd-boot = lib.mkDefault {
          enable = lib.mkIf (!config.boot.lanzaboote.enable) true;
          configurationLimit = lib.mkDefault 3;
          consoleMode = lib.mkDefault "max";
          editor = lib.mkDefault false;
        };
        efi = lib.mkDefault {
          canTouchEfiVariables = lib.mkDefault true;
          efiSysMountPoint = lib.mkDefault "/boot";
        };
      };
      initrd = lib.mkDefault {
        systemd.enable = lib.mkDefault true;
        verbose = lib.mkDefault false;
      };
      plymouth = lib.mkDefault {
        enable =lib.mkDefault  true;
        theme = lib.mkDefault "breeze";
      };
      kernelParams = lib.mkDefault [ # Hide most messages during boot
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