{ config, ... }: {
  users.extraUsers.alexanderschiffhauer = {
    isNormalUser = true;
    extraGroups = [ "wheel" "libvirtd" ];
    hashedPasswordFile = config.shared.password.permanentHashedFile;
  };
  services.displayManager.autoLogin = {
    enable = true;
    user = "alexanderschiffhauer";
  };
}
