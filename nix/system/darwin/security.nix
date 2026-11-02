{ config, ... }:
{
  networking.applicationFirewall = config.settings.macos.firewall;
  system.defaults = {
    loginwindow.GuestEnabled = config.settings.macos.guest.enable;
    screensaver.askForPassword = config.settings.macos.screenLock.requirePassword;
    screensaver.askForPasswordDelay = config.settings.macos.screenLock.delay;
    SoftwareUpdate.AutomaticallyInstallMacOSUpdates = config.settings.macos.updates.automaticallyInstall;
  };
}
