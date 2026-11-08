{ flake ? builtins.getFlake "git+file://${toString ../../..}"
, inputs ? flake.inputs
, system ? builtins.currentSystem
}:
let
  pkgs = import inputs.nixpkgs { inherit system; config.allowUnfree = true; };
in (import ../../vm/test.nix { inherit pkgs inputs; }) {
  name = "onepassword-integration";
  nodes.machine = { ... }: {
    imports = [ ../../../config/settings.nix ../../../config/pkgs/utils/1password.nix ];
    security.polkit.enable = true;
    security.pam.sshAgentAuth.enable = true;
    programs.zsh.enable = true;
    settings.users.admin = {
      username = "operator";
      homeDirectory = "/srv/operator";
    };
    users.users.operator = {
      isNormalUser = true;
      home = "/srv/operator";
      createHome = true;
      extraGroups = [ "wheel" ];
      hashedPassword = "!";
    };
    virtualisation.memorySize = 1024;
  };
  scenario = ./Main.btrc;
}
