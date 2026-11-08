{ pkgs }:
let
  metadata = pkgs.writeText "assetto-follow-cm.json" (builtins.toJSON {
    KPlugin = {
      Id = "assetto-follow-cm";
      Name = "Assetto Corsa follows Content Manager";
      Description = "Start AC on the display containing Content Manager";
      Version = "1.0";
    };
    "X-Plasma-API" = "javascript";
    "X-Plasma-MainScript" = "code/main.js";
    "KPackageStructure" = "KWin/Script";
  });
in pkgs.runCommand "assetto-follow-cm" { nativeBuildInputs = [ pkgs.nodejs ]; } ''
  node ${../../../../../tests/gaming/assettocorsa/Placement.js} ${./main.js}
  mkdir -p "$out/contents/code"
  cp ${metadata} "$out/metadata.json"
  cp ${./main.js} "$out/contents/code/main.js"
''
