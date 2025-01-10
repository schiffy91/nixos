{ unstable-pkgs, ... }: {
  home.packages = [
    unstable-pkgs.vscode.fhs # VHS variants allows using extensions with precompiled binaries https://nixos.wiki/wiki/Visual_Studio_Code
  ];
}