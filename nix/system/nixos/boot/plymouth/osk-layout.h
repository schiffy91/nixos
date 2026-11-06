/* SPDX-License-Identifier: GPL-2.0-or-later */
#pragma once
#include <stdbool.h>
#include <stddef.h>

typedef enum { OSK_CHARACTER, OSK_SHIFT, OSK_CAPS, OSK_BACKSPACE, OSK_ENTER } osk_action;
typedef struct {
        const char *normal, *shifted;
        osk_action action;
        unsigned row;
        double column, units;
} osk_key;
typedef struct { double x, y, width, height; } osk_rect;
typedef struct {
        unsigned touchscreens, keyboards;
        bool prompt, requested, shift, caps;
} osk_state;

extern const osk_key osk_keys[];
extern const size_t osk_key_count;
bool osk_visible(const osk_state *state);
void osk_prompt(osk_state *state, bool active);
osk_rect osk_key_rect(size_t key, double width, double height);
int osk_hit(double x, double y, double width, double height);
const char *osk_key_text(const osk_state *state, size_t key);
/* Rotation is clockwise degrees from logical screen to physical panel. */
void osk_transform(unsigned rotation, double *x, double *y);
