{ pkgs, inputs }:
let
  icons = import ./icons.nix { inherit pkgs inputs; };
  sources = builtins.fromJSON (builtins.readFile ./sources.json);
  legacy = builtins.fromJSON (builtins.readFile ./legacy.json);
  filenames = { portrait = "p"; landscape = "landscape"; hero = "_hero"; logo = "_logo"; };
in pkgs.lib.mapAttrs (name: assets: {
  icon = "${icons.${name}}";
  artwork = pkgs.runCommand "${name}-steam-artwork" {} ''
    mkdir -p "$out"
    ${pkgs.lib.concatStringsSep "\n" (pkgs.lib.mapAttrsToList (role: asset: ''
      cp ${pkgs.fetchurl { inherit (asset) url hash; }} "$out/${filenames.${role}}.png"
    '') assets)}
    cp ${pkgs.writeText "${name}-legacy-artwork.json" (builtins.toJSON legacy.${name})} "$out/legacy.json"
  '';
}) sources // {
  # Deck Pad brings its own pictures: nobody has drawn the Deck as a Steam Controller, so that repository made a set.
  deckpad = let pictures = inputs.deckpad.packages.${pkgs.stdenv.hostPlatform.system}.artwork; in { icon = "${pictures}/icon.png"; artwork = pictures; };
}
