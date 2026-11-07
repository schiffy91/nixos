{ inputs, config, pkgs, lib, ... }: {
  imports = [ inputs.lanzaboote.nixosModules.lanzaboote ./boot/safety.nix ./boot/touch-keyboard.nix ./boot/kernel-trust.nix ];
  config = lib.mkMerge [
{
  ##### Shared Boot Settings #####
  boot = {
    kernelPackages = config.settings.boot.kernelPackages;
    loader = {
      timeout = config.settings.boot.timeout;
      systemd-boot.editor = false;  # lanzaboote's loader.conf reads this too
      efi = {
        canTouchEfiVariables = true;
        efiSysMountPoint = config.settings.disk.boot.efiSysMountPoint;
      };
    };
    plymouth = {
      enable = config.settings.boot.plymouth.enable;
      theme = "bgrt";
    };
    kernelParams = [
      "quiet"                           # Plymouth
      "loglevel=3"                      # Reduce kernel log verbosity
      "vt.global_cursor_default=0"      # Hide the console cursor during boot
      "rd.udev.log_priority=3"          # Minimize udev messages
      "systemd.show_status=auto"        # Hide systemd boot status unless there's an error
      "rd.systemd.show_status=auto"     # Same as above, but for the initrd phase
      "plymouth.ignore-serial-consoles" # Force display rendering
    ] ++ lib.optional (config.settings.boot.plymouth.enable && config.settings.boot.plymouth.drmDevice != "") (
      "plymouth.drm-device=${config.settings.boot.plymouth.drmDevice}"
    ) ++ [
      "mem_sleep_default=s2idle"        # Use s2idle instead of S3 suspend
    ];
  };
  systemd.sleep.settings.Sleep.AllowHibernation = if config.settings.sleep.allowHibernation || config.settings.sleep.hibernateOnSuspend then "yes" else "no";
  # Steam and Plasma only ever ask logind to suspend, so the suspend unit itself hibernates.
  systemd.services.systemd-suspend = lib.mkIf config.settings.sleep.hibernateOnSuspend {
    serviceConfig.ExecStart = [ "" "${config.systemd.package}/lib/systemd/systemd-sleep hibernate" ];
  };
  systemd.tmpfiles.rules = [ "d /tmp 1777 root root -" ];
  environment.systemPackages = with pkgs; [ efibootmgr ];
}
##### Standard Boot Settings #####
(lib.mkIf (config.settings.boot.method == "Standard-Boot") {
  boot.loader.systemd-boot = {
    enable = true;
    configurationLimit = config.settings.boot.previousGenerationLimit;
    consoleMode = "max";
  };
})
##### Secure Boot Settings #####
(lib.mkIf (config.settings.boot.method == "Secure-Boot") {
  boot = {
    loader.systemd-boot.enable = lib.mkForce false; # Forcibly disable the systemd boot loader
    lanzaboote = {
      enable = true;
      settings.default = "nixos-generation-*.efi";
      pkiBundle = config.settings.boot.pkiBundle;
      configurationLimit = config.settings.boot.previousGenerationLimit;
      autoEnrollKeys = {
        enable = config.settings.boot.autoEnrollKeys;
        includeMicrosoftKeys = false;
        allowBrickingMyMachine = true;
      };
    };
  };
})];
}
