{ config, pkgs, lib, ... }:
let
  repoPath = "~/Drive/dev/nixos";
  hostName = "mbp21";
  flake = "${repoPath}#${hostName}";
in {
  nixpkgs.hostPlatform = "aarch64-darwin";
  home-manager.users.${config.settings.users.admin.username} = {
    imports = [ ../../../home.nix ];
    home = {
      username = config.settings.users.admin.username;
      homeDirectory = config.settings.users.admin.homeDirectory;
      stateVersion = "24.11";
    };
    programs.zsh = {
      initContent = lib.mkOrder 550 ''
        . "$HOME/.local/bin/env"
      '';
      profileExtra = "setopt no_nomatch"; # let patterns with no matches pass through
    };
  };
  settings.apps = {
    gaming.enable = false;
    dev.packages = with pkgs; [ direnv gh glfw nodejs podman python313 qemu wgpu-native ];
    utils.packages = with pkgs; [ _1password-cli coreutils imagemagick qpdf ];
    dev.zsh.enable = true;
    dev.shell.aliases = {
      nix-update = "sudo darwin-rebuild switch --flake ${flake}";
      nix-upgrade = "nix flake update nixpkgs-darwin home-manager-darwin nix-darwin --flake ${repoPath} && sudo darwin-rebuild switch --flake ${flake}";
    };
    dev.git.includes = [ "${config.settings.users.admin.homeDirectory}/.config/git/local-before-nix.gitconfig" ];
    dev.gemini.enable = true;
    dev.lmstudio.enable = true;
    dev.agents.rules = true;
  };
  # Permission intent is reviewed/applied through System Settings.
  settings.macos.permissions = {
    schemaVersion = 1;
    enforcement = "system-settings";
    apps = {
      codex = {
        name = "Codex (displayed as ChatGPT)";
        path = "/Applications/ChatGPT.app";
        bundleId = "com.openai.codex";
        teamId = "2DC432GLL2";
        reason = "User explicitly requires full access for computer administration.";
        allow = [
          "fullDiskAccess"
          "deviceControlAndDataAccess"
          "screenAndSystemAudioRecording"
          "localNetwork"
          "microphone"
        ];
      };
      claudeDesktop = {
        name = "Claude Desktop";
        path = "/Applications/Claude.app";
        bundleId = "com.anthropic.claudefordesktop";
        teamId = "Q6L2SF6YDW";
        reason = "User explicitly requires full access for computer administration.";
        allow = [
          "fullDiskAccess"
          "deviceControlAndDataAccess"
          "screenAndSystemAudioRecording"
          "localNetwork"
          "microphone"
        ];
      };
      codexComputerUse = {
        name = "Codex Computer Use";
        path = "/Applications/ChatGPT.app/Contents/Resources/cua_node/lib/node_modules/@oai/sky/Codex Computer Use.app";
        bundleId = "com.openai.sky.CUAService";
        teamId = "2DC432GLL2";
        reason = "Retain the existing Codex computer-control workflow.";
        allow = [
          "deviceControlAndDataAccess"
          "screenAndSystemAudioRecording"
        ];
      };
      claudeCode = {
        name = "Claude Code";
        path = "~/.local/bin/claude";
        bundleId = "com.anthropic.claude-code";
        teamId = "Q6L2SF6YDW";
        reason = "User approved standalone Claude Code Full Disk Access for computer administration.";
        allow = [
          "fullDiskAccess"
          "deviceControlAndDataAccess"
          "screenAndSystemAudioRecording"
          "localNetwork"
          "microphone"
        ];
      };
      chromeRemoteDesktop = {
        name = "Chrome Remote Desktop";
        path = "/Library/PrivilegedHelperTools/ChromeRemoteDesktopHost.app";
        bundleId = "com.google.chromeremotedesktop.me2me-host";
        teamId = "EQHXZ8M8AV";
        reason = "User confirmed continued remote access.";
        allow = [
          "deviceControlAndDataAccess"
          "screenAndSystemAudioRecording"
          "remoteDesktop"
          "localNetwork"
        ];
      };
    };
    review = [
      "Preserve Terminal FDA for CLI administration; audit it independently."
      "Review obsolete test-binary microphone grants and DaisyDisk Calendar access."
      "Resolve duplicate Local Network and generic background-task identities."
    ];
  };
}
