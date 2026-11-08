{ host, ... }: {
  imports = [ ./audio.nix ./cpu.nix ./gpu.nix ./input.nix ./networking.nix ];
  settings = {
    boot.timeout = 1;
    disk = {
      inherit (host.disk) device;
      encryption.enable = true;
      encryption.opportunisticUnlock.enable = true;
      recovery.enable = true;
      immutability.enable = true;
      swap.size = host.disk.swapSize;
    };
    tpm.device = host.tpm.device;
    desktop.plasma.taskbarLaunchers = [
      "applications:org.kde.dolphin.desktop"
      "applications:google-chrome.desktop"
      "applications:chrome-fmgjjmmmlfnkbppncabfkddbjimcfncm-Default.desktop" # Gmail
      "applications:chrome-kjbdgfilnfhdoflbpgamdcdgpehopbep-Default.desktop" # Calendar
      "applications:chrome-hnpfjngllnobngcgfapefoaidbinmjnm-Default.desktop" # WhatsApp
      "applications:cider-2.desktop"
      "applications:code.desktop"
      "applications:org.kde.konsole.desktop"
      "applications:battlenet.desktop"
      "applications:steam.desktop"
    ];
    desktop.outputs = [{
      name        = host.display.primary.name;
      scaleFactor = host.display.primary.scaleFactor;
      primary     = true;
    }];
  };
}
