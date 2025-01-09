{ lib, pkgs, ... }: {
  home.packages = with pkgs; [ blackbox-terminal ];
  dconf.settings = {
    "com/raggesilver/BlackBox" = {
      cursor-blink-mode = lib.hm.gvariant.mkUint32 1;
      cursor-shape = lib.hm.gvariant.mkUint32 2;
      easy-copy-paste = true;
      floating-controls = true;
      floating-controls-hover-area = lib.hm.gvariant.mkUint32 20;
      font = "Hack 11";
      pretty = true;
      remember-window-size = true;
      scrollback-lines = lib.hm.gvariant.mkUint32 10240;
      theme-dark = "Catppuccin-Frappe";
      window-height = lib.hm.gvariant.mkUint32 1150;
      window-width = lib.hm.gvariant.mkUint32 1450;
      opacity = lib.hm.gvariant.mkUint32 95;
    };
  };
  home.file = {
    ".local/share/blackbox/schemes/Catppuccin-Mocha.json".text = ''
      {
        "name": "Catppuccin-Mocha",
        "comment": "Soothing pastel theme for the high-spirited!",
        "background-color": "#1E1E2E",
        "foreground-color": "#CDD6F4",
        "badge-color": "#585B70",
        "bold-color": "#585B70",
        "cursor-background-color": "#F5E0DC",
        "cursor-foreground-color": "#1E1E2E",
        "highlight-background-color": "#F5E0DC",
        "highlight-foreground-color": "#1E1E2E",
        "palette": [
          "#45475A",
          "#F38BA8",
          "#A6E3A1",
          "#F9E2AF",
          "#89B4FA",
          "#F5C2E7",
          "#94E2D5",
          "#BAC2DE",
          "#585B70",
          "#F38BA8",
          "#A6E3A1",
          "#F9E2AF",
          "#89B4FA",
          "#F5C2E7",
          "#94E2D5",
          "#A6ADC8"
        ],
        "use-badge-color": false,
        "use-bold-color": false,
        "use-cursor-color": true,
        "use-highlight-color": true,
        "use-theme-colors": false
      }
    '';
    ".local/share/blackbox/schemes/Catppuccin-Frappe.json".text = ''
      {
        "name": "Catppuccin-Frappe",
        "comment": "Soothing pastel theme for the high-spirited!",
        "background-color": "#303446",
        "foreground-color": "#c6d0f5",
        "badge-color": "#626880",
        "bold-color": "#626880",
        "cursor-background-color": "#f2d5cf",
        "cursor-foreground-color": "#303446",
        "highlight-background-color": "#f2d5cf",
        "highlight-foreground-color": "#303446",
        "palette": [
          "#b5bfe2",
          "#e78284",
          "#a6d189",
          "#e5c890",
          "#8caaee",
          "#f4b8e4",
          "#81c8be",
          "#626880",
          "#a5adce",
          "#e78284",
          "#a6d189",
          "#e5c890",
          "#8caaee",
          "#f4b8e4",
          "#81c8be",
          "#51576d"
        ],
        "use-badge-color": false,
        "use-bold-color": false,
        "use-cursor-color": true,
        "use-highlight-color": true,
        "use-theme-colors": false
      }
    '';
  };
}