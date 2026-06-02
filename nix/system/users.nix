{ inputs, config, lib, ... }:
let
  cfg = config.settings.users;
  admin = cfg.admin;
  agent = cfg.agent;
  nixFiles = dir:
    let e = builtins.readDir dir; in
    lib.attrValues (lib.mapAttrs (n: _: dir + "/${n}")
      (lib.filterAttrs (n: t: t == "regular" && lib.hasSuffix ".nix" n) e));
in {
  imports = [
    inputs.home-manager.nixosModules.home-manager
  ] ++ nixFiles ../apps;

  users.mutableUsers = false;
  users.groups = {
    ${admin.username} = {};
  } // lib.optionalAttrs agent.enable {
    ${agent.username} = {};
  };
  users.users = {
    ${admin.username} = {
      isNormalUser = true;
      group = admin.username;
      extraGroups = admin.extraGroups;
      # Read the hash at NIX EVAL TIME (not at activation time). This bakes the
      # hash into the generation's /etc/static/users-groups.json, so the
      # `update-users-groups.pl` activation script never has to open the secrets
      # file on disk — sidestepping the boot-time bind-mount race where
      # initrd-nixos-activation runs before /etc/nixos's @persist bind-mount
      # lands and falls back to writing ":!:" into /etc/shadow.
      # Use a relative path (resolved by Nix from users.nix's directory) so the
      # file is read via the Nix store import mechanism and works under pure
      # evaluation. The file must exist at flake-eval time (which it does — the
      # installer and `nixosctl change-password` always write it before any
      # nixos-rebuild that uses this option).
      hashedPassword = lib.fileContents (../../secrets + "/${config.settings.secrets.hashedPasswordFile}");
      openssh.authorizedKeys.keys = admin.authorizedKeys;
    };
  } // lib.optionalAttrs agent.enable {
    ${agent.username} = {
      isNormalUser = true;
      createHome = true;
      group = agent.username;
      description = agent.publicName;
      extraGroups = agent.extraGroups;
      hashedPassword = "!";
      openssh.authorizedKeys.keys = agent.authorizedKeys;
    };
  };

  services.displayManager.autoLogin = {
    enable = admin.autoLogin.enable;
    user = admin.username;
  };

  home-manager = {
    useGlobalPkgs = true;
    useUserPackages = true;
    backupFileExtension = "hm-backup";
    sharedModules = [ inputs.plasma-manager.homeModules.plasma-manager ];
    extraSpecialArgs = { inherit (config) settings; };
    users = lib.optionalAttrs admin.homeManager.enable {
      ${admin.username} = { settings, ... }: {
        home = {
          username = settings.users.admin.username;
          homeDirectory = "/home/${settings.users.admin.username}";
          stateVersion = "24.11";
        };
        programs.home-manager.enable = true;
        programs.git = {
          enable = config.settings.apps.git.enable;
          settings = {
            user = {
              name = admin.publicName;
              email = admin.publicEmail;
            };
            gpg.format = "openpgp";
          };
        };
        imports = nixFiles ../apps/home-manager;
      };
    } // lib.optionalAttrs (agent.enable && agent.homeManager.enable) {
      ${agent.username} = {
        home = {
          username = agent.username;
          homeDirectory = "/home/${agent.username}";
          stateVersion = "24.11";
          packages = agent.packages;
          file."work/.keep".text = "";
        };
        programs = {
          home-manager.enable = true;
          bash = {
            enable = true;
            shellAliases = {
              ll = "ls -alF";
            };
          };
          direnv = {
            enable = true;
            nix-direnv.enable = true;
          };
          git = {
            enable = true;
            settings.user = {
              name = agent.publicName;
              email = agent.publicEmail;
            };
          };
        };
      };
    };
  };
}
