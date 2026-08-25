{ inputs, config, lib, pkgs, ... }:
let
  cfg = config.settings.users;
  admin = cfg.admin;
  agent = cfg.agent;
  nixModuleFiles = skippedDirs: dir:
    let
      ignoredNames = [ "dev-shell.nix" "flake.nix" "package.nix" ];
      ignoredDir = path: dirName: lib.hasInfix "/${dirName}/" (toString path);
      importable = path:
        lib.hasSuffix ".nix" (baseNameOf path)
        && !(lib.elem (baseNameOf path) ignoredNames)
        && !(lib.any (ignoredDir path) skippedDirs);
    in lib.filter importable (lib.filesystem.listFilesRecursive dir);
  appModules = nixModuleFiles [ "assets" "custom" "home-manager" "tests" ] ../apps;
  homeModules = nixModuleFiles [] ../apps/home-manager;
in {
  imports = [
    inputs.home-manager.nixosModules.home-manager
  ] ++ appModules;

  users.mutableUsers = cfg.mutable;
  /* /nix/store is nosuid, and stock NixOS only auto-wraps unix_chkpwd/unix_group_chkpwd
     (pam_unix's own helpers) — passwd itself is never wrapped unless a config opts in.
     Without this, `passwd` invoked unprivileged can authenticate (delegated to the
     already-wrapped unix_chkpwd) but fails to actually write /etc/shadow: pam_unix's
     chauthtok logic calls setuid(0) expecting the setuid-root bit to have granted it,
     and that fails with EPERM. Root-invoked changes never hit this since root doesn't
     need to escalate at all — it only surfaces for a real, unprivileged self password
     change (needed for KDE Wallet's pam_kwallet5 hook to see the real old password). */
  security.wrappers.passwd = {
    source = "${pkgs.shadow}/bin/passwd";
    owner = "root";
    group = "root";
    setuid = true;
  };
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
      hashedPasswordFile = "${config.settings.secrets.path}/${config.settings.secrets.hashedPasswordFile}";
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
            url."git@github.com:".insteadOf = "https://github.com/";
            gpg.format = "openpgp";
          };
        };
        imports = homeModules;
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
