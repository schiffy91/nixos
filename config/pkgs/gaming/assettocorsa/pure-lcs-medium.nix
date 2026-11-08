# Pure LCS with Pure's filter; CSP graphics follow PHOTOreal's Medium preset
# (acstuff.club/s/Qxm3). INI values stay strings to preserve precision and lists.
{
  "video.ini" = {
    VIDEO = {
      ANISOTROPIC = "16";
      AASAMPLES = "4";
      SHADOW_MAP_SIZE = "4096";
      VSYNC = "1";
      FPS_CAP_MS = "0";
    };
    EFFECTS = { MOTION_BLUR = "12"; FXAA = "0"; };
    POST_PROCESS = {
      ENABLED = "1";
      FILTER = "pure";
      FXAA = "1";
      DOF = "0";
      GLARE = "5";
      HEAT_SHIMMER = "1";
    }; # CSP intercepts FXAA for its AA mode.
    CUBEMAP = { FACES_PER_FRAME = "1"; };
    ASSETTOCORSA = { WORLD_DETAIL = "5"; };
  };
  "extension/weather_fx.ini" = {
    BASIC = {
      ENABLED = "1";
      IMPLEMENTATION = "Pure LCS";
      CONTROLLER = "pureCtrl static";
    };
    MISCELLANEOUS = { SWITCH_HEADLIGHTS_WITH_AI = "1"; };
    NOISE_VOLUME = { DEPTH = "128"; RESOLUTION = "512"; };
  };
  "extension/graphics_adjustments.ini" = {
    ANTIALIASING = {
      MODE = "FXAA";
      FXAA3_QUALITY_SUBPIX = "1";
      FXAA3_GREEN_AS_LUMA = "0";
    };
    COLOR_BUFFER = { EXTRAFX_INTEGRATION = "1"; };
    HUMAN_SHADER = { ENABLED = "1"; };
    SCRIPTABLE_FILTER = { IMPLEMENTATION = "pure"; };
    SHADER_REPLACEMENTS = { ALLOW_TAG_NEW_CARPAINT = "1"; ALLOW_TAG_NEW_WATER = "1"; ALLOW_TAG_PBR = "1"; };
    VISIBLE_CARS = {
      MAIN = "2,4,10,200";
      GBUFFER = "2,4,8,12";
      SHADOWS = "2,4,6,24";
      MIRROR = "1,3,7,12";
      OUTSIDE_MAIN = "2,4,20,200";
      OUTSIDE_GBUFFER = "2,4,8,12";
      OUTSIDE_SHADOWS = "2,4,8,24";
      SPECTATING_MAIN = "6,12,40,200";
      SPECTATING_GBUFFER = "3,6,12,24";
      SPECTATING_SHADOWS = "3,6,6,24";
    };
  };
  "extension/extra_fx.ini" = {
    BASIC = { ENABLED = "1"; };
    ASSAO = { ENABLED = "1"; QUALITY = "1"; };
    SSGI = { ENABLED = "1"; };
    SSLR = { ENABLED = "0"; };
    VOLUMETRIC_LIGHTS = {
      INTENSITY_MULT = "1.25";
      SCALE = "0.22";
    };
    TAA = {
      EXTRA_SHARPEN_PASS = "0.25";
      BLUR_NEW = "0.75";
      HISTORY_SHARPEN = "0.25";
    };
  };
  "extension/lighting_fx.ini" = {
    BASIC = {
      HEADLIGHTS_RANGE_MULT = "1.1";
      HEADLIGHTS_SPOT_MULT = "1.05";
      HEADLIGHTS_SATURATION_MULT = "0.75";
      SPECULAR_BYFLAMES_MULT = "0.6";
      EMISSIVE_CAMERA_GAIN = "1";
      CARS_LIT_MULT = "0.45";
      SPECULAR_MULT = "1.05";
      SPECULAR_MULT_CAR = "1.2";
      LIGHT_FROM_BEHIND_MULT = "1.25";
      DYNAMIC_AMBIENT_MULT = "1.2";
      DYNAMIC_AMBIENT_GAMMA = "1.2";
      DYNAMIC_AMBIENT_BRIGHTNESS = "1.5";
      DYNAMIC_AMBIENT_SATURATION = "0.9";
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
    DOUBLE_FACE_BIASED = {
      DEPTH_BIAS = "0";
      DEPTH_BIAS_CLAMP = "0";
      SLOPE_SCALED_DEPTH_BIAS = "2";
    };
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
    PIECES = {
      COLLIDE_WITH_CARS = "1";
      USE_COLLIDER_MESH = "1";
      COLLISION_QUANTITY = "3";
      USE_SHARDS_COLOR = "1";
    };
    SMOKE = { HEATING_EXTRA = "0.05"; };
    FIREWORKS = { ENABLED = "0"; BEHAVIOUR = "6"; };
    MARBLES_2 = { ENABLED = "0"; };
  };
  "extension/rain_fx.ini".SCREEN_DROPS = {
    STYLE_CHASE_CAMERA = "DISTANT";
    STYLE_CAR_CAMERAS = "DISTANT";
    STYLE_FREE_CAMERA = "DISTANT";
    INTENSITY_MULT = "0.75";
  };
  "extension/grass_fx.ini" = {
    BASIC = { QUALITY = "3"; };
    RENDERING = { CAST_SHADOWS = "1"; SMOOTHER_BLENDING = "1"; };
  };
  "extension/surfaces_fx.ini".BASIC.ENABLED = "1";
  "extension/smart_mirror.ini".REAL_MIRRORS.ENABLED = "1";
}
