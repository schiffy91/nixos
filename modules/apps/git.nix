{ settings, pkgs, ... }: {
    home.packages = with pkgs; [
        git
    ];
    programs.git = {
        enable = true;
        settings.user = {
            email = settings.user.admin.publicEmail;
            name =  settings.user.admin.publicName;
        };
    };
}