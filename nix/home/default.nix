{ settings, lib, ... }:
{
  imports = [ ./agents ];
  programs.home-manager.enable = true;
  programs.git = lib.mkIf (settings.apps.enable && settings.apps.dev.enable && settings.apps.git.enable) {
    enable = true;
    package = null;
    includes = map (path: { inherit path; }) settings.apps.git.includes;
    settings = {
      user = {
        name = settings.users.admin.publicName;
        email = settings.users.admin.publicEmail;
      };
      url."git@github.com:".insteadOf = "https://github.com/";
      gpg.format = "openpgp";
    };
  };
  home.file = lib.mkIf (settings.apps.enable && settings.apps.dev.enable && settings.apps.zsh.enable) {
    ".zprofile".text = settings.apps.zsh.profileExtra;
    ".zshrc".text = settings.apps.zsh.initContent + lib.concatStringsSep "\n" (
      lib.mapAttrsToList (name: value: "alias ${lib.escapeShellArg name}=${lib.escapeShellArg value}") settings.apps.zsh.shellAliases
    ) + "\n";
  };
}
