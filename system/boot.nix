{ pkgs, inputs, config, lib, ... }: 
lib.mkMerge [
  # Base configuration with imports
  {
    imports = [ inputs.lanzaboote.nixosModules.lanzaboote ];
    boot = {
      kernelPackages = lib.mkDefault pkgs.linuxPackages_latest;
      loader = {
        timeout = 5;
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
        "loglevel=3"
        "vt.global_cursor_default=0"
        "rd.udev.log_priority=3"
        "systemd.show_status=auto"
        "rd.systemd.show_status=auto"
        "plymouth.ignore-serial-consoles"
      ];
    };
    environment.systemPackages = with pkgs; [ efibootmgr ];
  }

  # Standard boot configuration
  (lib.mkIf (config.variables.boot.method == "Standard") {
    boot.loader.systemd-boot = {
      enable = true;
      configurationLimit = 3;
      consoleMode = "max";
      editor = false;
    };
  })

  # Secure boot configuration
  (lib.mkIf (config.variables.boot.method == "Secure-Boot") {
    boot = {
      systemd-boot.enable = lib.mkForce false;
      lanzaboote = {
        enable = true;
        pkiBundle = config.variables.disk.pkiBundle;
      };
    };
  })
]