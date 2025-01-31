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
  # This is apparently the way to hide menu bars on konsole...SMH
  xdg.dataFile."konsolestaterc".text = ''
    [MainWindow]
    State=AAAA/wAAAAD9AAAAAQAAAAAAAAAAAAAAAPwCAAAAAvsAAAAiAFEAdQBpAGMAawBDAG8AbQBtAGEAbgBkAHMARABvAGMAawAAAAAA/////wAAAXIA////+wAAABwAUwBTAEgATQBhAG4AYQBnAGUAcgBEAG8AYwBrAAAAAAD/////AAABEQD///8AAAOPAAAB1AAAAAQAAAAEAAAACAAAAAj8AAAAAQAAAAIAAAACAAAAFgBtAGEAaQBuAFQAbwBvAGwAQgBhAHIAAAAAAP////8AAAAAAAAAAAAAABwAcwBlAHMAcwBpAG8AbgBUAG8AbwBsAGIAYQByAAAAAAD/////AAAAAAAAAAA=
  '';
}