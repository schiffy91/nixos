{ config, ... }: {
  programs.git = {
    enable = true;
    config = {
      user = {
        name = config.settings.user.admin.publicName;
        email = config.settings.user.admin.publicEmail;
      };
      gpg.format = "openpgp";
    };
  };
}
