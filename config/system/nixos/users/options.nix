{ lib, config, pkgs, ... }:
let
  mkSetting = type: defaultValue: lib.mkOption { inherit type; default = defaultValue; };
  adminHome = "/home/${config.settings.users.admin.username}";
  agentHome = "/home/${config.settings.users.agent.username}";
in {
  options = with lib.types; {
    settings.users.admin.homeDirectory = mkSetting str (if pkgs.stdenv.hostPlatform.isDarwin then "/Users/${config.settings.users.admin.username}" else adminHome);
    settings.users.admin.username = mkSetting str "alexanderschiffhauer";
    settings.users.admin.hashedPasswordFile = mkSetting str "/var/lib/nixos/credentials/password-hash";
    settings.users.admin.publicName = mkSetting str "Alexander Schiffhauer";
    settings.users.admin.publicEmail = mkSetting str "Alexander.Schiffhauer@gmail.com";
    settings.users.admin.autoLogin.enable = mkSetting bool false;
    settings.users.admin.autoLock.enable = mkSetting bool true;
    settings.users.admin.authorizedKeys = mkSetting (listOf str) [
      "ssh-ed25519 AAAAC3NzaC1lZDI1NTE5AAAAINJsoluI1m5T4iwuCbpSdHvLVdemN3v7wMrqk4e+XJA0"
      "ssh-ed25519 AAAAC3NzaC1lZDI1NTE5AAAAIAOOxJXmhrSalqwuZKRncqzBgSuWDXiOhvSlS8pLDeFI"  # the 1Password agent's key
    ];
    settings.users.admin.extraGroups = mkSetting (listOf str) [ "wheel" "networkmanager" ];
    settings.users.admin.homeManager.enable = mkSetting bool true;
    settings.users.mutable = mkSetting bool false;
    settings.users.agent.enable = mkSetting bool true;
    settings.users.agent.username = mkSetting str "agent";
    settings.users.agent.publicName = mkSetting str "Agent";
    settings.users.agent.publicEmail = mkSetting str "agent@localhost";
    settings.users.agent.authorizedKeys = mkSetting (listOf str) config.settings.users.admin.authorizedKeys;
    settings.users.agent.extraGroups = mkSetting (listOf str) [];
    settings.users.agent.homeManager.enable = mkSetting bool true;
    settings.users.agent.packages = mkSetting (listOf package) (with pkgs; [
      bashInteractive
      coreutils
      git
      ripgrep
      fd
      jq
      curl
      wget
      gnumake
      gcc
      python313
      nodejs
      (import ../../../pkgs/dev/nixd/package.nix { inherit pkgs; })
      shellcheck
    ]);
    settings.users.agent.persist.paths = mkSetting (listOf str) [
      "${agentHome}/work"
      "${agentHome}/Downloads"
      "${agentHome}/.bash_history"
      "${agentHome}/.ssh/known_hosts"
      "${agentHome}/.cache"
      "${agentHome}/.config"
      "${agentHome}/.local/bin"
      "${agentHome}/.local/share"
      "${agentHome}/.local/state"
      "${agentHome}/.npm"
      "${agentHome}/.cargo"
      "${agentHome}/.codex"
      "${agentHome}/.claude"
      "${agentHome}/.claude.json"
      "${agentHome}/.gemini"
    ];
  };
}
