{ config, lib, ... }:
let
  contentDirectory = "~/Drive/media/Games/Assetto Corsa";
  presetName = "FRACTAL-NORTH";

  # Pure LCS + Pure's own filter (PHOTOreal darkens cockpits by design). CSP keys are the
  # graphics sections of PHOTOreal's Medium preset (acstuff.club/s/Qxm3);
  # video keys follow content-manager.org. 4K120 on the HDMI TV.
  # Game/INI values stay strings so decimal precision and comma-separated lists survive unchanged.
  options = {
    controls = {
      brakeGamma = 1.0; # Linear response for the pressure-sensing PRO brake.
      wheelMenu = true;
    };
    video = {
      anisotropicFiltering = "16";
      msaaSamples = "4";
      shadowMapSize = "4096";
      vsync = "1";
      frameTimeLimitMs = "0";
      motionBlur = "12";
      cubemapFacesPerFrame = "1";
      worldDetail = "5";
    };
    postProcessing = {
      filter = "pure";
      scriptableFilter = "pure";
      depthOfField = "0";
      glare = "5";
      heatShimmer = "1";
    };
    antialiasing = {
      mode = "FXAA";
      subpixelQuality = "1";
      greenAsLuma = "0";
    };
    weather = {
      implementation = "Pure LCS";
      controller = "pureCtrl static";
      noiseDepth = "128";
      noiseResolution = "512";
    };
    visibleCars = {
      main = "2,4,10,200";
      gbuffer = "2,4,8,12";
      shadows = "2,4,6,24";
      mirror = "1,3,7,12";
      outsideMain = "2,4,20,200";
      outsideGbuffer = "2,4,8,12";
      outsideShadows = "2,4,8,24";
      spectatingMain = "6,12,40,200";
      spectatingGbuffer = "3,6,12,24";
      spectatingShadows = "3,6,6,24";
    };
    extraEffects = {
      ambientOcclusionQuality = "1";
      screenSpaceReflections = "0";
      volumetricLightIntensity = "1.25";
      volumetricLightScale = "0.22";
      taaExtraSharpen = "0.25";
      taaNewFrameBlur = "0.75";
      taaHistorySharpen = "0.25";
    };
    lighting = {
      headlightRange = "1.1";
      headlightSpot = "1.05";
      headlightSaturation = "0.75";
      flameSpecular = "0.6";
      emissiveCameraGain = "1";
      carLighting = "0.45";
      specular = "1.05";
      carSpecular = "1.2";
      lightFromBehind = "1.25";
      ambientIntensity = "1.2";
      ambientGamma = "1.2";
      ambientBrightness = "1.5";
      ambientSaturation = "0.9";
    };
    shadows = {
      biases = "0.04,0.25,0.8,4";
      fourthCascadeDistance = "1800";
      depthBias = "0";
      depthBiasClamp = "0";
      slopeScaledDepthBias = "2";
    };
    particles = {
      wetSkidLimit = "10000";
      carDropLimit = "8000";
      windscreenLimit = "100000";
      mirageDisruptionLimit = "4000";
      oilSpillLimit = "100";
      flameLimit = "2000";
      debrisCollisionQuantity = "3";
      smokeHeating = "0.05";
    };
    rain = {
      dropStyle = "DISTANT";
      intensity = "0.75";
    };
    grassQuality = "3";
  };

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
    inherit contentDirectory;
    presets = [
      { name = "Video Settings/${presetName}.cmpreset"; source = "${contentDirectory}/${presetName}.cmpreset"; }
      { name = "Audio Settings/${presetName}.cmpreset"; source = "${contentDirectory}/Audio/${presetName}.cmpreset"; }
      { name = "Controls/${presetName}.ini"; source = "${contentDirectory}/Controls/${presetName}.ini"; }
    ];
    brakeGamma = options.controls.brakeGamma;
    gameSettings = {
      "video.ini" = {
        VIDEO = {
          ANISOTROPIC = options.video.anisotropicFiltering;
          AASAMPLES = options.video.msaaSamples;
          SHADOW_MAP_SIZE = options.video.shadowMapSize;
          VSYNC = options.video.vsync;
          FPS_CAP_MS = options.video.frameTimeLimitMs;
        };
        EFFECTS = { MOTION_BLUR = options.video.motionBlur; FXAA = "0"; };
        POST_PROCESS = {
          ENABLED = "1";
          FILTER = options.postProcessing.filter;
          FXAA = "1";
          DOF = options.postProcessing.depthOfField;
          GLARE = options.postProcessing.glare;
          HEAT_SHIMMER = options.postProcessing.heatShimmer;
        }; # CSP intercepts FXAA for its AA mode.
        CUBEMAP = { FACES_PER_FRAME = options.video.cubemapFacesPerFrame; };
        ASSETTOCORSA = { WORLD_DETAIL = options.video.worldDetail; };
      };
      "extension/weather_fx.ini" = {
        BASIC = {
          ENABLED = "1";
          IMPLEMENTATION = options.weather.implementation;
          CONTROLLER = options.weather.controller;
        };
        MISCELLANEOUS = { SWITCH_HEADLIGHTS_WITH_AI = "1"; };
        NOISE_VOLUME = { DEPTH = options.weather.noiseDepth; RESOLUTION = options.weather.noiseResolution; };
      };
      "extension/graphics_adjustments.ini" = {
        ANTIALIASING = {
          MODE = options.antialiasing.mode;
          FXAA3_QUALITY_SUBPIX = options.antialiasing.subpixelQuality;
          FXAA3_GREEN_AS_LUMA = options.antialiasing.greenAsLuma;
        };
        COLOR_BUFFER = { EXTRAFX_INTEGRATION = "1"; };
        HUMAN_SHADER = { ENABLED = "1"; };
        SCRIPTABLE_FILTER = { IMPLEMENTATION = options.postProcessing.scriptableFilter; };
        SHADER_REPLACEMENTS = { ALLOW_TAG_NEW_CARPAINT = "1"; ALLOW_TAG_NEW_WATER = "1"; ALLOW_TAG_PBR = "1"; };
        VISIBLE_CARS = {
          MAIN = options.visibleCars.main;
          GBUFFER = options.visibleCars.gbuffer;
          SHADOWS = options.visibleCars.shadows;
          MIRROR = options.visibleCars.mirror;
          OUTSIDE_MAIN = options.visibleCars.outsideMain;
          OUTSIDE_GBUFFER = options.visibleCars.outsideGbuffer;
          OUTSIDE_SHADOWS = options.visibleCars.outsideShadows;
          SPECTATING_MAIN = options.visibleCars.spectatingMain;
          SPECTATING_GBUFFER = options.visibleCars.spectatingGbuffer;
          SPECTATING_SHADOWS = options.visibleCars.spectatingShadows;
        };
      };
      "extension/extra_fx.ini" = {
        BASIC = { ENABLED = "1"; };
        ASSAO = { ENABLED = "1"; QUALITY = options.extraEffects.ambientOcclusionQuality; };
        SSGI = { ENABLED = "1"; };
        SSLR = { ENABLED = options.extraEffects.screenSpaceReflections; };
        VOLUMETRIC_LIGHTS = {
          INTENSITY_MULT = options.extraEffects.volumetricLightIntensity;
          SCALE = options.extraEffects.volumetricLightScale;
        };
        TAA = {
          EXTRA_SHARPEN_PASS = options.extraEffects.taaExtraSharpen;
          BLUR_NEW = options.extraEffects.taaNewFrameBlur;
          HISTORY_SHARPEN = options.extraEffects.taaHistorySharpen;
        };
      };
      "extension/lighting_fx.ini" = {
        BASIC = {
          HEADLIGHTS_RANGE_MULT = options.lighting.headlightRange;
          HEADLIGHTS_SPOT_MULT = options.lighting.headlightSpot;
          HEADLIGHTS_SATURATION_MULT = options.lighting.headlightSaturation;
          SPECULAR_BYFLAMES_MULT = options.lighting.flameSpecular;
          EMISSIVE_CAMERA_GAIN = options.lighting.emissiveCameraGain;
          CARS_LIT_MULT = options.lighting.carLighting;
          SPECULAR_MULT = options.lighting.specular;
          SPECULAR_MULT_CAR = options.lighting.carSpecular;
          LIGHT_FROM_BEHIND_MULT = options.lighting.lightFromBehind;
          DYNAMIC_AMBIENT_MULT = options.lighting.ambientIntensity;
          DYNAMIC_AMBIENT_GAMMA = options.lighting.ambientGamma;
          DYNAMIC_AMBIENT_BRIGHTNESS = options.lighting.ambientBrightness;
          DYNAMIC_AMBIENT_SATURATION = options.lighting.ambientSaturation;
        };
        SHADOWS = { ENABLED = "1"; CARS_SHADOWS = "1"; };
      };
      "extension/reflections_fx.ini" = {
        MAIN_CUBEMAP = { USE_64BPP_2 = "0"; USE_PROPER_IBL = "1"; };
        LOCAL_CUBEMAPS_TRACKS2 = { ENABLED = "1"; ALLOW_DYNAMIC = "1"; };
        LOCAL_CUBEMAPS_CARS2 = { ENABLED = "1"; ALLOW_DYNAMIC = "1"; };
      };
      "extension/smart_shadows.ini" = {
        BASIC = { BIASES = options.shadows.biases; };
        CUSTOM_SHADOW_MATRICES = { FOURTH_CASCADE_DISTANCE = options.shadows.fourthCascadeDistance; };
        DOUBLE_FACE_BIASED = {
          DEPTH_BIAS = options.shadows.depthBias;
          DEPTH_BIAS_CLAMP = options.shadows.depthBiasClamp;
          SLOPE_SCALED_DEPTH_BIAS = options.shadows.slopeScaledDepthBias;
        };
        LAZIER_UPDATE = { LAZIER_UPDATE_EXTERIOR = "1"; LAZIER_UPDATE_INTERIOR = "1"; };
      };
      "extension/particles_fx.ini" = {
        AC_PARTICLES = { SHADOWED_PARTICLES = "1"; LIT_PARTICLES = "0"; };
        SKIDWET = { ENABLED = "1"; LIMIT = options.particles.wetSkidLimit; };
        CARDROPS = { ENABLED = "1"; LIMIT = options.particles.carDropLimit; };
        WINDSCREEN = { ENABLED = "1"; LIMIT = options.particles.windscreenLimit; };
        MIRAGEDISRUPTION = { ENABLED = "1"; LIMIT = options.particles.mirageDisruptionLimit; };
        OILSPILL = { ENABLED = "1"; LIMIT = options.particles.oilSpillLimit; };
        FLAMES = { LIMIT = options.particles.flameLimit; FLAMING_WHEELS = "0"; };
        PIECES = {
          COLLIDE_WITH_CARS = "1";
          USE_COLLIDER_MESH = "1";
          COLLISION_QUANTITY = options.particles.debrisCollisionQuantity;
          USE_SHARDS_COLOR = "1";
        };
        SMOKE = { HEATING_EXTRA = options.particles.smokeHeating; };
        FIREWORKS = { ENABLED = "0"; BEHAVIOUR = "6"; };
        MARBLES_2 = { ENABLED = "0"; };
      };
      "extension/rain_fx.ini".SCREEN_DROPS = {
        STYLE_CHASE_CAMERA = options.rain.dropStyle;
        STYLE_CAR_CAMERAS = options.rain.dropStyle;
        STYLE_FREE_CAMERA = options.rain.dropStyle;
        INTENSITY_MULT = options.rain.intensity;
      };
      "extension/grass_fx.ini" = {
        BASIC = { QUALITY = options.grassQuality; };
        RENDERING = { CAST_SHADOWS = "1"; SMOOTHER_BLENDING = "1"; };
      };
      "extension/surfaces_fx.ini".BASIC.ENABLED = "1";
      "extension/smart_mirror.ini".REAL_MIRRORS.ENABLED = "1";
    };
    wheelMenu.enable = options.controls.wheelMenu;
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
