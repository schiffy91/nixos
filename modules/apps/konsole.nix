{ ... }: {
  xdg.configFile."konsolerc".text = ''
    [Desktop Entry]
    DefaultProfile=alexanderschiffhauer.profile
    [General]
    ConfigVersion=1
  '';
  xdg.dataFile."konsole/alexanderschiffhauer.profile".text = ''
    [Cursor Options]
    CursorShape=2

    [General]
    Name=alexanderschiffhauer

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