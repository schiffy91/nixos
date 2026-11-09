{ inputs, config, lib, pkgs, ... }:
let
  cfg = config.settings.users;
  admin = cfg.admin;
  agent = cfg.agent;
  shell = import ../../pkgs/dev/shell.nix { inherit pkgs inputs; };
in {
  imports = [
    inputs.home-manager.nixosModules.home-manager
    ./users/password.nix
    ./users/ssh-agent-auth.nix
  ];

  settings.disk.immutability.persist.paths = [
    "${admin.homeDirectory}/Downloads"
    "${admin.homeDirectory}/.bash_history"
    "${admin.homeDirectory}/.cache"
    "${admin.homeDirectory}/.pki"
  ] ++ lib.optionals agent.enable agent.persist.paths;

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
      home = admin.homeDirectory;
      group = admin.username;
      extraGroups = admin.extraGroups;
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

  systemd.tmpfiles.rules =
    lib.optional admin.homeManager.enable
      "d /nix/var/nix/profiles/per-user/${admin.username} 0755 ${admin.username} ${admin.username} - "
    ++ lib.optional (agent.enable && agent.homeManager.enable)
      "d /nix/var/nix/profiles/per-user/${agent.username} 0755 ${agent.username} ${agent.username} - ";

  home-manager = {
    useGlobalPkgs = true;
    useUserPackages = true;
    backupFileExtension = "hm-backup";
    extraSpecialArgs = { inherit inputs; inherit (config) settings; };
    users = lib.optionalAttrs admin.homeManager.enable {
      ${admin.username} = { settings, ... }: {
        home = {
          username = settings.users.admin.username;
          homeDirectory = settings.users.admin.homeDirectory;
          stateVersion = "24.11";
        };
        imports = [ ../../home.nix ];
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
        imports = [ ../../pkgs/dev/agents ];
        programs = {
          home-manager.enable = true;
          bash = {
            enable = true;
            shellAliases = shell.aliases // {
              ll = "ls -alF";
            };
            initExtra = shell.init;
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
