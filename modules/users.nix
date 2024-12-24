{ ... }:
let username = "alexanderschiffhauer";
in
{
  users.extraUsers.${username} = {
    isNormalUser = true;
    extraGroups = [ "wheel" "libvirtd" ];
    hashedPasswordFile = "/etc/nixos/secrets/hashed_password.txt";
  };

  services.displayManager.autoLogin = {
    enable = true;
    user = username;
  };
}