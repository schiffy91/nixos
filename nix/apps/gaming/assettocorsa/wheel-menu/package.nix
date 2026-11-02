{ pkgs }:
pkgs.runCommand "assetto-wheel-menu" { nativeBuildInputs = [ pkgs.lua ]; } ''
  lua ${../../../../../tests/gaming/assettocorsa/Menu.lua} ${./Menu.lua}
  lua ${../../../../../tests/gaming/assettocorsa/WheelMenu.lua} ${./.}
  luac -p ${./WheelMenu.lua}
  luac -p ${./CarMenu.lua}
  mkdir -p "$out/apps/lua/WheelMenu"
  cp ${./Menu.lua} "$out/apps/lua/WheelMenu/Menu.lua"
  cp ${./CarMenu.lua} "$out/apps/lua/WheelMenu/CarMenu.lua"
  cp ${./WheelMenu.lua} "$out/apps/lua/WheelMenu/WheelMenu.lua"
  cp ${./manifest.ini} "$out/apps/lua/WheelMenu/manifest.ini"
''
