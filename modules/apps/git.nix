{ settings, pkgs, ... }: {
    home.packages = with pkgs; [
        git
    ];
    programs.git = {
        enable = true;
        signing.format = "openpgp";
        settings.user = {
            email = settings.user.admin.publicEmail;
            name =  settings.user.admin.publicName;
        };
    };
}