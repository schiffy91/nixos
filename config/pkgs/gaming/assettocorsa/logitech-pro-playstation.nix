{ lib, wheelMenu }:
let
  # PRO PlayStation/PC DirectInput indices; D-pad remains a POV hat.
  wheel = {
    cross = 0;
    square = 1;
    circle = 2;
    triangle = 3;
    shiftUp = 4;
    shiftDown = 5;
    r2 = 6;
    l2 = 7;
    share = 8;
    options = 9;
    r3 = 10;
    l3 = 11;
    rightClockwise = 21;
    rightCounterclockwise = 22;
    rightPress = 23;
    leftClockwise = 24;
    leftCounterclockwise = 25;
    leftPress = 26;
  };
in
# Clear the earlier modifier layout: Options is always pause and Share
# captures a replay. Session exit/reset belongs in the visible pause menu.
map (action: { inherit action; button = null; }) [
  "__CM_START_SESSION" "ABSUP" "ABSDN" "ABS" "__CM_ABS_DECREASE"
  "__EXT_STARTER" "__EXT_LOW_BEAM" "__CM_TO_PITS" "__EXT_HAZARDS"
  "__EXT_GEAR_NEUTRAL" "__EXT_TURNSIGNAL_RIGHT" "__EXT_TURNSIGNAL_LEFT"
  "__EXT_WIPERS_MORE" "__EXT_WIPERS_LESS" "__CM_SETUP_CAR" "__CM_EXIT"
  "__EXT_ENGINEMAP_UP" "__EXT_SWITCH_HSHIFTER" "__EXT_TURNSIGNAL_CANCEL"
  "__EXT_WIPERS_OFF" "__EXT_MANUAL_CUTOFF" "__EXT_HIDE_UI"
  "__EXT_TC2_UP" "__EXT_TC2_DOWN" "__CM_ENGINE_BRAKE" "__CM_ENGINE_BRAKE_DECREASE"
  "__CM_MGU_1" "__CM_MGU_1_DECREASE" "__CM_MGU_3" "__CM_NEXT_APPS_DESKTOP"
] ++ [
  { action = "GEARUP"; button = wheel.shiftUp; }
  { action = "GEARDN"; button = wheel.shiftDown; }
  { action = "__EXT_FOCUSMOVEMENT_CLICK"; button = wheel.cross; }
  { action = "__CM_PAUSE"; button = wheel.options; }
  { action = "ACTION_CHANGE_CAMERA"; button = wheel.square; }
  { action = "HANDBRAKE"; button = wheel.circle; }
  { action = "ACTION_HEADLIGHTS"; button = wheel.triangle; }
  { action = "ACTION_HEADLIGHTS_FLASH"; button = wheel.l3; }
  { action = "ACTION_HORN"; button = wheel.leftPress; }
  { action = "DRS"; button = wheel.r2; }
  { action = "KERS"; button = wheel.l2; }
  { action = "__EXT_PIT_LIMITER"; button = wheel.r3; }
  { action = "__EXT_SAVE_CLIP"; button = wheel.share; }
  { action = "TCUP"; button = if wheelMenu then null else wheel.rightClockwise; }
  { action = "TCDN"; button = if wheelMenu then null else wheel.rightCounterclockwise; }
  { action = "BALANCEDN"; button = if wheelMenu then null else wheel.leftClockwise; }
  { action = "BALANCEUP"; button = if wheelMenu then null else wheel.leftCounterclockwise; }
] ++ lib.optionals wheelMenu [
  { action = "__NIX_MFD_PREVIOUS"; button = wheel.leftCounterclockwise; }
  { action = "__NIX_MFD_NEXT"; button = wheel.leftClockwise; }
  { action = "__NIX_MFD_DECREASE"; button = wheel.rightCounterclockwise; }
  { action = "__NIX_MFD_INCREASE"; button = wheel.rightClockwise; }
  { action = "__NIX_MFD_TOGGLE"; button = wheel.rightPress; }
]
