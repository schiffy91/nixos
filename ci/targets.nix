# What CI and `make cache` build, and where the cache lives. Adding a system or an
# attribute here is the whole change: the Makefile targets and both workflows read it.
{
  cache = {
    name = "nixos-cache-1";  # the signing key's name; its secret never leaves the machine that signs
    publicKey = "nixos-cache-1:UlVz7H6ROdjAipqWb58854E7jNS8j7KpiBLydRjaZmA=";  # what the other hosts trust; the secret half stays on the publisher
    cachix = "";  # a Cachix cache name; CI pushes there only when this and CACHIX_AUTH_TOKEN are set
  };

  # One CI job per attribute, so a runner's disk holds one closure. The installer ISOs
  # are about 1.9 GB each, which fits a hosted runner but not a GitHub Pages site.
  systems = [
    {
      system = "x86_64-linux";
      runner = "ubuntu-24.04";
      attrs = [ "installer" "installer-FRACTAL-NORTH" "installer-STEAM-DECK-OLED" ];
    }
    {
      system = "aarch64-linux";
      runner = "ubuntu-24.04-arm";  # free for public repositories
      attrs = [ "installer" ];
    }
  ];

  # Attached to a GitHub Release on a v* tag. Only the generic installer: it is 1.9 GB
  # against a 2 GiB asset limit, while a host installer embeds that host's whole system
  # closure for an offline install and measured 13 GB (FRACTAL-NORTH) and 11 GB
  # (STEAM-DECK-OLED) here — past the limit and past a hosted runner's disk. Those belong
  # in the cache below, where size does not matter.
  release = {
    system = "x86_64-linux";
    attrs = [ "installer" ];
  };

  # What this machine should build and serve to the others. The Deck's own system is the
  # payload that matters: with it in the cache a handheld activates instead of compiling.
  local = [
    "nixosConfigurations.STEAM-DECK-OLED-Secure-Boot.config.system.build.toplevel"
    "nixosConfigurations.FRACTAL-NORTH-Secure-Boot.config.system.build.toplevel"
    "installer-FRACTAL-NORTH"
    "installer-STEAM-DECK-OLED"
  ];
}
