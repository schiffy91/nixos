{ pkgs, ... }: {
  environment.systemPackages = [
    (pkgs.writeShellScriptBin "nixos" ''
      exec /etc/nixos/scripts/bin/nixos "$@"
    '')
  ];
}
