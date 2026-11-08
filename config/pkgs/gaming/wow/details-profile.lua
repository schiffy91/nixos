-- Initial account settings only; Details! saves subsequent edits normally.
local function window(mode, y)
    return {
        __was_opened = true,
        __locked = false,
        __pos = {
            normal = {x = 0, y = 0, w = 280, h = 120},
            solo = {x = 0, y = 0, w = 280, h = 120},
        },
        libwindow = {point = "BOTTOMRIGHT", x = -24, y = y, scale = 1},
        modo = mode,
        atributo = 1,
        sub_atributo = 2, -- DPS
        last_raid_plugin = mode == 4 and "Details_TinyThreat" or nil,
        skin = "Minimalistic",
        bg_alpha = 0.85,
        bg_r = 0.06,
        bg_g = 0.05,
        bg_b = 0.04,
        show_sidebars = false,
        show_statusbar = false,
        hide_icon = true,
        titlebar_shown = true,
        titlebar_height = 18,
        titlebar_texture_color = {0.16, 0.13, 0.09, 1},
        fullborder_shown = true,
        fullborder_size = 1,
        fullborder_color = {0.35, 0.29, 0.18, 1},
        auto_hide_menu = {left = true, right = true},
        desaturated_menu = true,
        attribute_text = {
            enabled = true,
            text_face = "Friz Quadrata TT",
            text_size = 11,
            text_color = {0.9, 0.8, 0.6, 1},
            show_timer = false,
        },
        row_info = {
            height = 18,
            font_size = 11,
            font_face = "Friz Quadrata TT",
            font_face_file = "Fonts\\FRIZQT__.TTF",
            texture = "Blizzard",
            texture_file = "Interface\\TargetingFrame\\UI-StatusBar",
            texture_class_colors = false,
            fixed_texture_color = {0.35, 0.29, 0.18},
            texture_background_class_color = false,
            fixed_texture_background_color = {0.08, 0.07, 0.05, 0.5},
        },
        wallpaper = {enabled = false},
    }
end

_detalhes_global = {
    got_first_run = true,
    always_use_profile = true,
    always_use_profile_name = "Classic",
    __profiles = {
        Classic = {
            profile_save_pos = true,
            instances = {window(2, 180), window(4, 24)},
        },
    },
}
