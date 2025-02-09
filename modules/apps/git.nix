{ settings, pkgs, ... }: {
    home.packages = with pkgs; [
        git
    ];
    programs.git = {
        enable = true;
        userEmail = settings.user.admin.publicEmail;
        userName = settings.user.admin.publicName;
    };
}