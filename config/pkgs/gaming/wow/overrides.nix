{
  foreverFadeChat.patches = [ ./forever-fade-chat.patch ];
  consolePort.patches = [ ./consoleport-keyboard.patch ];
  flareUI = {
    patches = [ ./flareui-layout.patch ];
    files = {
      "FlareUI/ControllerTargeting.lua" = ./controller-targeting.lua;
      "FlareUI/DisplayLayout.lua" = ./display-layout.lua;
    };
    tocAppend."FlareUI/FlareUI.toc" = [ "ControllerTargeting.lua" "DisplayLayout.lua" ];  # the client loads what the .toc lists
  };
}
