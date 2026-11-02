{ config, lib, ... }:
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
in {
  settings.apps.assettocorsa = {
    presets = [
      { name = "Video Settings/FRACTAL-NORTH.cmpreset"; source = "~/Games/Assetto Corsa/FRACTAL-NORTH.cmpreset"; }
      { name = "Audio Settings/FRACTAL-NORTH.cmpreset"; source = "~/Games/Assetto Corsa/Audio/FRACTAL-NORTH.cmpreset"; }
      { name = "Controls/FRACTAL-NORTH.ini"; source = "~/Games/Assetto Corsa/Controls/FRACTAL-NORTH.ini"; }
    ];
    brakeGamma = 1.0; # Linear response for the pressure-sensing PRO brake.
    # Pure LCS + Pure's own filter (PHOTOreal darkens cockpits by design). CSP keys are the
    # graphics sections of PHOTOreal's Medium preset (acstuff.club/s/Qxm3);
    # video keys follow content-manager.org. 4K120 on the HDMI TV.
    gameSettings = {
      "video.ini" = {
        VIDEO = { ANISOTROPIC = "16"; AASAMPLES = "4"; SHADOW_MAP_SIZE = "4096"; VSYNC = "1"; FPS_CAP_MS = "0"; };
        EFFECTS = { MOTION_BLUR = "12"; FXAA = "0"; };
        POST_PROCESS = { ENABLED = "1"; FILTER = "pure"; FXAA = "1"; DOF = "0"; GLARE = "5"; HEAT_SHIMMER = "1"; }; # CSP intercepts FXAA for its AA mode.
        CUBEMAP = { FACES_PER_FRAME = "1"; };
        ASSETTOCORSA = { WORLD_DETAIL = "5"; };
      };
      "extension/weather_fx.ini" = {
        BASIC = { ENABLED = "1"; IMPLEMENTATION = "Pure LCS"; CONTROLLER = "pureCtrl static"; };
        MISCELLANEOUS = { SWITCH_HEADLIGHTS_WITH_AI = "1"; };
        NOISE_VOLUME = { DEPTH = "128"; RESOLUTION = "512"; };
      };
      "extension/graphics_adjustments.ini" = {
        ANTIALIASING = { MODE = "FXAA"; FXAA3_QUALITY_SUBPIX = "1"; FXAA3_GREEN_AS_LUMA = "0"; };
        COLOR_BUFFER = { EXTRAFX_INTEGRATION = "1"; };
        HUMAN_SHADER = { ENABLED = "1"; };
        SCRIPTABLE_FILTER = { IMPLEMENTATION = "pure"; };
        SHADER_REPLACEMENTS = { ALLOW_TAG_NEW_CARPAINT = "1"; ALLOW_TAG_NEW_WATER = "1"; ALLOW_TAG_PBR = "1"; };
        VISIBLE_CARS = {
          MAIN = "2,4,10,200"; GBUFFER = "2,4,8,12"; SHADOWS = "2,4,6,24"; MIRROR = "1,3,7,12";
          OUTSIDE_MAIN = "2,4,20,200"; OUTSIDE_GBUFFER = "2,4,8,12"; OUTSIDE_SHADOWS = "2,4,8,24";
          SPECTATING_MAIN = "6,12,40,200"; SPECTATING_GBUFFER = "3,6,12,24"; SPECTATING_SHADOWS = "3,6,6,24";
        };
      };
      "extension/extra_fx.ini" = {
        BASIC = { ENABLED = "1"; };
        ASSAO = { ENABLED = "1"; QUALITY = "1"; };
        SSGI = { ENABLED = "1"; };
        SSLR = { ENABLED = "0"; };
        VOLUMETRIC_LIGHTS = { INTENSITY_MULT = "1.25"; SCALE = "0.22"; };
        TAA = { EXTRA_SHARPEN_PASS = "0.25"; BLUR_NEW = "0.75"; HISTORY_SHARPEN = "0.25"; };
      };
      "extension/lighting_fx.ini" = {
        BASIC = {
          HEADLIGHTS_RANGE_MULT = "1.1"; HEADLIGHTS_SPOT_MULT = "1.05"; HEADLIGHTS_SATURATION_MULT = "0.75";
          SPECULAR_BYFLAMES_MULT = "0.6"; EMISSIVE_CAMERA_GAIN = "1"; CARS_LIT_MULT = "0.45";
          SPECULAR_MULT = "1.05"; SPECULAR_MULT_CAR = "1.2"; LIGHT_FROM_BEHIND_MULT = "1.25";
          DYNAMIC_AMBIENT_MULT = "1.2"; DYNAMIC_AMBIENT_GAMMA = "1.2"; DYNAMIC_AMBIENT_BRIGHTNESS = "1.5"; DYNAMIC_AMBIENT_SATURATION = "0.9";
        };
        SHADOWS = { ENABLED = "1"; CARS_SHADOWS = "1"; };
      };
      "extension/reflections_fx.ini" = {
        MAIN_CUBEMAP = { USE_64BPP_2 = "0"; USE_PROPER_IBL = "1"; };
        LOCAL_CUBEMAPS_TRACKS2 = { ENABLED = "1"; ALLOW_DYNAMIC = "1"; };
        LOCAL_CUBEMAPS_CARS2 = { ENABLED = "1"; ALLOW_DYNAMIC = "1"; };
      };
      "extension/smart_shadows.ini" = {
        BASIC = { BIASES = "0.04,0.25,0.8,4"; };
        CUSTOM_SHADOW_MATRICES = { FOURTH_CASCADE_DISTANCE = "1800"; };
        DOUBLE_FACE_BIASED = { DEPTH_BIAS = "0"; DEPTH_BIAS_CLAMP = "0"; SLOPE_SCALED_DEPTH_BIAS = "2"; };
        LAZIER_UPDATE = { LAZIER_UPDATE_EXTERIOR = "1"; LAZIER_UPDATE_INTERIOR = "1"; };
      };
      "extension/particles_fx.ini" = {
        AC_PARTICLES = { SHADOWED_PARTICLES = "1"; LIT_PARTICLES = "0"; };
        SKIDWET = { ENABLED = "1"; LIMIT = "10000"; };
        CARDROPS = { ENABLED = "1"; LIMIT = "8000"; };
        WINDSCREEN = { ENABLED = "1"; LIMIT = "100000"; };
        MIRAGEDISRUPTION = { ENABLED = "1"; LIMIT = "4000"; };
        OILSPILL = { ENABLED = "1"; LIMIT = "100"; };
        FLAMES = { LIMIT = "2000"; FLAMING_WHEELS = "0"; };
        PIECES = { COLLIDE_WITH_CARS = "1"; USE_COLLIDER_MESH = "1"; COLLISION_QUANTITY = "3"; USE_SHARDS_COLOR = "1"; };
        SMOKE = { HEATING_EXTRA = "0.05"; };
        FIREWORKS = { ENABLED = "0"; BEHAVIOUR = "6"; };
        MARBLES_2 = { ENABLED = "0"; };
      };
      "extension/rain_fx.ini".SCREEN_DROPS = { STYLE_CHASE_CAMERA = "DISTANT"; STYLE_CAR_CAMERAS = "DISTANT"; STYLE_FREE_CAMERA = "DISTANT"; INTENSITY_MULT = "0.75"; };
      "extension/grass_fx.ini" = {
        BASIC = { QUALITY = "3"; };
        RENDERING = { CAST_SHADOWS = "1"; SMOOTHER_BLENDING = "1"; };
      };
      "extension/surfaces_fx.ini".BASIC.ENABLED = "1";
      "extension/smart_mirror.ini".REAL_MIRRORS.ENABLED = "1";
    };
    wheelMenu.enable = true;
    # Clear the earlier modifier layout: Options is always pause and Share
    # captures a replay. Session exit/reset belongs in the visible pause menu.
    controlBindings = map (action: { inherit action; button = null; }) [
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
      { action = "TCUP"; button = if config.settings.apps.assettocorsa.wheelMenu.enable then null else wheel.rightClockwise; }
      { action = "TCDN"; button = if config.settings.apps.assettocorsa.wheelMenu.enable then null else wheel.rightCounterclockwise; }
      { action = "BALANCEDN"; button = if config.settings.apps.assettocorsa.wheelMenu.enable then null else wheel.leftClockwise; }
      { action = "BALANCEUP"; button = if config.settings.apps.assettocorsa.wheelMenu.enable then null else wheel.leftCounterclockwise; }
    ] ++ lib.optionals config.settings.apps.assettocorsa.wheelMenu.enable [
      { action = "__NIX_MFD_PREVIOUS"; button = wheel.leftCounterclockwise; }
      { action = "__NIX_MFD_NEXT"; button = wheel.leftClockwise; }
      { action = "__NIX_MFD_DECREASE"; button = wheel.rightCounterclockwise; }
      { action = "__NIX_MFD_INCREASE"; button = wheel.rightClockwise; }
      { action = "__NIX_MFD_TOGGLE"; button = wheel.rightPress; }
    ];
  };
}
