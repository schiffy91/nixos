/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "osk-layout.h"
#include <math.h>
#include <string.h>

#define KEY(row, col, lo, hi) { lo, hi, OSK_CHARACTER, row, col, 1 }
#define ACTION(row, col, width, name, action) { name, name, action, row, col, width }
const osk_key osk_keys[] = {
        KEY(0, 0, "`", "~"), KEY(0, 1, "1", "!"), KEY(0, 2, "2", "@"),
        KEY(0, 3, "3", "#"), KEY(0, 4, "4", "$"), KEY(0, 5, "5", "%"),
        KEY(0, 6, "6", "^"), KEY(0, 7, "7", "&"), KEY(0, 8, "8", "*"),
        KEY(0, 9, "9", "("), KEY(0, 10, "0", ")"), KEY(0, 11, "-", "_"),
        KEY(0, 12, "=", "+"), ACTION(0, 13, 2, "Backspace", OSK_BACKSPACE),
        KEY(1, 1, "q", "Q"), KEY(1, 2, "w", "W"), KEY(1, 3, "e", "E"),
        KEY(1, 4, "r", "R"), KEY(1, 5, "t", "T"), KEY(1, 6, "y", "Y"),
        KEY(1, 7, "u", "U"), KEY(1, 8, "i", "I"), KEY(1, 9, "o", "O"),
        KEY(1, 10, "p", "P"), KEY(1, 11, "[", "{"), KEY(1, 12, "]", "}"),
        KEY(1, 13, "\\", "|"),
        KEY(2, 2, "a", "A"), KEY(2, 3, "s", "S"), KEY(2, 4, "d", "D"),
        KEY(2, 5, "f", "F"), KEY(2, 6, "g", "G"), KEY(2, 7, "h", "H"),
        KEY(2, 8, "j", "J"), KEY(2, 9, "k", "K"), KEY(2, 10, "l", "L"),
        KEY(2, 11, ";", ":"), KEY(2, 12, "'", "\""),
        ACTION(3, 0, 2.5, "Shift", OSK_SHIFT),
        KEY(3, 2.5, "z", "Z"), KEY(3, 3.5, "x", "X"), KEY(3, 4.5, "c", "C"),
        KEY(3, 5.5, "v", "V"), KEY(3, 6.5, "b", "B"), KEY(3, 7.5, "n", "N"),
        KEY(3, 8.5, "m", "M"), KEY(3, 9.5, ",", "<"), KEY(3, 10.5, ".", ">"),
        KEY(3, 11.5, "/", "?"), ACTION(3, 12.5, 2.5, "Shift", OSK_SHIFT),
        ACTION(4, 0, 2.5, "Caps", OSK_CAPS),
        { " ", " ", OSK_CHARACTER, 4, 2.5, 8 },
        ACTION(4, 10.5, 4.5, "Unlock", OSK_ENTER),
};
const size_t osk_key_count = sizeof(osk_keys) / sizeof(osk_keys[0]);

bool osk_visible(const osk_state *state)
{
        return state->prompt && state->touchscreens > 0 &&
               (state->requested || state->keyboards == 0);
}

void osk_prompt(osk_state *state, bool active)
{
        state->prompt = active;
        if (!active) state->shift = false;
        /* Keep a user's touch preference across cryptsetup retries. */
}

osk_rect osk_key_rect(size_t index, double width, double height)
{
        const osk_key *key = &osk_keys[index];
        double keyboard_width = fmin(width * 0.96, 1280.0);
        double left = (width - keyboard_width) / 2;
        double top = height * 0.48;
        double unit = keyboard_width / 15;
        double row = (height * 0.50) / 5;
        double gap = fmax(3, fmin(6, unit * 0.08));
        return (osk_rect) { left + key->column * unit + gap / 2,
                            top + key->row * row + gap / 2,
                            key->units * unit - gap, row - gap };
}

int osk_hit(double x, double y, double width, double height)
{
        for (size_t i = 0; i < osk_key_count; i++) {
                osk_rect r = osk_key_rect(i, width, height);
                if (x >= r.x && x < r.x + r.width && y >= r.y && y < r.y + r.height)
                        return (int)i;
        }
        return -1;
}

const char *osk_key_text(const osk_state *state, size_t index)
{
        const osk_key *key = &osk_keys[index];
        bool shifted = state->shift;
        if (key->normal[0] >= 'a' && key->normal[0] <= 'z' && strlen(key->normal) == 1)
                shifted ^= state->caps;
        return shifted ? key->shifted : key->normal;
}

void osk_transform(unsigned rotation, double *x, double *y)
{
        double a = *x, b = *y;
        switch (rotation) {
        case 90: *x = b; *y = 1 - a; break;
        case 180: *x = 1 - a; *y = 1 - b; break;
        case 270: *x = 1 - b; *y = a; break;
        default: break;
        }
}
