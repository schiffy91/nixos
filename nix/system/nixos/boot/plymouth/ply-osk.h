/* SPDX-License-Identifier: GPL-2.0-or-later */
#pragma once
#include "ply-keyboard.h"
#include "ply-pixel-display.h"

typedef struct ply_osk ply_osk_t;
typedef void (*ply_osk_changed_t)(void *data);
ply_osk_t *ply_osk_new(ply_osk_changed_t changed, void *data);
void ply_osk_free(ply_osk_t *osk);
void ply_osk_set_keyboard(ply_osk_t *osk, ply_keyboard_t *keyboard);
void ply_osk_set_prompt(ply_osk_t *osk, bool prompt);
void ply_osk_set_active(ply_osk_t *osk, bool active);
bool ply_osk_visible(ply_osk_t *osk);
void ply_osk_add_display(ply_osk_t *osk, ply_pixel_display_t *display);
void ply_osk_remove_display(ply_osk_t *osk, ply_pixel_display_t *display);
void ply_osk_draw(ply_osk_t *osk, ply_pixel_display_t *display, ply_pixel_buffer_t *buffer);
