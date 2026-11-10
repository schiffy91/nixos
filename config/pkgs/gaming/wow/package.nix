{ pkgs, inputs, settings ? null }:
(import ../../../lib/btrc.nix { inherit pkgs inputs; }) {
  name = "wow-tools";
  src = pkgs.lib.fileset.toSource { root = ./.; fileset = pkgs.lib.fileset.unions [ ./Main.btrc ./Lock.btrc ./Install.btrc ./InstallMain.btrc ./Settings.btrc ./SettingsMain.btrc ./Files.btrc ./files.h ./btrc.toml ./btrc.lock ]; };
  programs = {
    wow-addons-lock = "Main.btrc";
    wow-addons-install = "InstallMain.btrc";
    wow-settings = "SettingsMain.btrc";
  };
  postInstall = ''
    wrapProgram "$out/bin/wow-addons-lock" --prefix PATH : ${pkgs.lib.makeBinPath [ pkgs.curl pkgs.nix pkgs.git pkgs.coreutils ]}
    ${pkgs.lib.optionalString (settings != null) ''
      wrapProgram "$out/bin/wow-addons-install" --add-flags ${pkgs.lib.escapeShellArg (toString settings)}
      wrapProgram "$out/bin/wow-settings" --add-flags ${pkgs.lib.escapeShellArg (toString settings)}
    ''}
  '';
}
