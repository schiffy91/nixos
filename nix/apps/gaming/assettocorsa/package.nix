{ pkgs, inputs, mods ? [], csp ? null, wheelLauncher ? "" }:
let
  tools = import ../package.nix { inherit pkgs inputs; };
  assets = import ./lib.nix { inherit pkgs; };
  payload = pkgs.runCommand "assettocorsa-mods" {} ''
    ${tools}/bin/assetto merge "$out" ${pkgs.lib.escapeShellArgs (map toString (
      [ assets.contentManager (if csp == null then assets.csp else csp) assets.fonts ] ++ mods
    ))}
  '';
in pkgs.runCommand "assetto" { nativeBuildInputs = [ pkgs.makeWrapper ]; } ''
  mkdir -p "$out/bin"
  makeWrapper ${tools}/bin/assetto "$out/bin/assetto" \
    --set ASSETTO_MODS ${payload} \
    --set ASSETTO_WHEEL_LAUNCHER ${pkgs.lib.escapeShellArg wheelLauncher}
  ln -s ${payload} "$out/mods"
''
