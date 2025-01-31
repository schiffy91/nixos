{ settings, ... }: {
  xdg.configFile."konsolerc".text = ''
    [Desktop Entry]
    DefaultProfile=${settings.user.admin.username}.profile
    [General]
    ConfigVersion=1
    [KonsoleWindow]
    RememberWindowSize=false
    ShowMenuBarByDefault=false
    [MainWindow]
    MenuBar=Disabled
    StatusBar=Disabled
    ToolBarsMovable=Disabled
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

    [KonsoleWindow]
    RememberWindowSize=false
    ShowMenuBarByDefault=false
    [MainWindow]
    MenuBar=Disabled
    StatusBar=Disabled
    ToolBarsMovable=Disabled
  '';
}