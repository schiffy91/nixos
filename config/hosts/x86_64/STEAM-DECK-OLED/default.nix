_:
let
  disk = {
    device   = "/dev/nvme0n1";
    swapSize = "20G";  # hibernation image for 16 GB of RAM
  };
  sdCard = {
    label = "SD";  # Jovian automounts it at /run/media/<user>/<label>
  };
in {
  imports = [ ./system ./pkgs ./card.nix ];
  _module.args.host = { inherit disk sdCard; };
  networking.hostName = "STEAM-DECK-OLED";
}
