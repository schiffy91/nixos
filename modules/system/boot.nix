{ pkgs, inputs, config, lib, ... }: { 
  imports = [ inputs.lanzaboote.nixosModules.lanzaboote ]; } // lib.mkMerge [{
  boot = {
    kernelPackages = lib.mkDefault pkgs.linuxPackages_latest;
    loader = {
      timeout = 5;
      efi = {
        canTouchEfisettings = true;
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
      "ip=dhcp"                         # SSH during initrd to decrypt filesystem
      "quiet"                           # Plymouth
      "splash"                          # Plymouth
      "loglevel=3"                      # Reduce kernel log verbosity
      "vt.global_cursor_default=0"      # Hide the console cursor during boot
      "rd.udev.log_priority=3"          # Minimize udev messages
      "systemd.show_status=auto"        # Hide systemd boot status unless there's an error
      "rd.systemd.show_status=auto"     # Same as above, but for the initrd phase
      "plymouth.ignore-serial-consoles" # Force display rendering
    ];
  };
  environment.systemPackages = with pkgs; [ efibootmgr ];
}
(lib.mkIf (config.settings.boot.method == "Standard") {
  boot.loader.systemd-boot = {
    enable = true;
    configurationLimit = 3;
    consoleMode = "max";
    editor = false;
  };
})
(lib.mkIf (config.settings.boot.method == "Secure-Boot") {
  boot = {
    loader.systemd-boot.enable = lib.mkForce false; # Forcibly disable the systemd boot loader
    lanzaboote = {
      enable = true;
      pkiBundle = config.settings.boot.pkiBundle;
    };
  };
})]