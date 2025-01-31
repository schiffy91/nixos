{ settings, ... }: {
  xdg.configFile."konsolerc".text = ''
    [Desktop Entry]
    DefaultProfile=${settings.user.admin.username}.profile
    [General]
    ConfigVersion=1
  '';
  xdg.dataFile."konsole/${settings.user.admin.username}.profile".text = ''
    [Cursor Options]
    CursorShape=2

    [General]
    Name=${settings.user.admin.username}

    [Keyboard]
    KeyBindings=macos

    [Scrolling]
    HistoryMode=2

    [Terminal Features]
    BlinkingCursorEnabled=true

    [MainWindow]
    MenuBar=Disabled
  '';
}