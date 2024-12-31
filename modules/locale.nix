{ config, lib, ... }: {
  config = {
    time.timeZone = lib.mkDefault "America/Los_Angeles";
    i18n.defaultLocale = lib.mkDefault "en_US.UTF-8";
    console.keyMap = lib.mkDefault "us";
  };
}
