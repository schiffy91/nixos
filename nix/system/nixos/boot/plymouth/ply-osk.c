/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "ply-osk.h"
#include "osk-layout.h"
#include "ply-label.h"
#include "ply-logger.h"
#include <errno.h>
#include <fcntl.h>
#include <libinput.h>
#include <libudev.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct display {
        ply_pixel_display_t *pixels;
        struct display *next;
} display_t;

struct ply_osk {
        osk_state state;
        struct libinput *input;
        struct udev *udev;
        ply_fd_watch_t *watch;
        ply_keyboard_t *keyboard;
        ply_label_t *label;
        display_t *displays;
        ply_osk_changed_t changed;
        void *data;
        bool active;
        int slot, pressed;
};

static int open_device(const char *path, int flags, void *data)
{
        int fd = open(path, flags | O_CLOEXEC);
        return fd < 0 ? -errno : fd;
}

static void close_device(int fd, void *data) { close(fd); }
static const struct libinput_interface input_interface = {
        .open_restricted = open_device, .close_restricted = close_device,
};

static bool is_keyboard(struct libinput_device *device)
{
        struct udev_device *udev = libinput_device_get_udev_device(device);
        if (!udev) return false;
        const char *typing = udev_device_get_property_value(udev, "ID_INPUT_KEYBOARD");
        const char *ignore = udev_device_get_property_value(udev, "PLYMOUTH_IGNORE_KEYBOARD");
        bool result = typing && !strcmp(typing, "1") && !(ignore && !strcmp(ignore, "1"));
        udev_device_unref(udev);
        return result;
}

static ply_pixel_buffer_t *display_buffer(ply_pixel_display_t *display)
{
        return ply_renderer_get_buffer_for_head(ply_pixel_display_get_renderer(display),
                                                ply_pixel_display_get_renderer_head(display));
}

static ply_pixel_display_t *touch_display(ply_osk_t *osk)
{
        /* Prefer the built-in panel when a dock adds another display. */
        for (display_t *d = osk->displays; d; d = d->next) {
                int width, height, scale;
                ply_pixel_buffer_rotation_t rotation;
                if (ply_renderer_get_panel_properties(ply_pixel_display_get_renderer(d->pixels),
                                                      &width, &height, &rotation, &scale)) {
                        if (rotation == PLY_PIXEL_BUFFER_ROTATE_CLOCKWISE ||
                            rotation == PLY_PIXEL_BUFFER_ROTATE_COUNTER_CLOCKWISE) {
                                int swap = width; width = height; height = swap;
                        }
                        if (width / scale == (int)ply_pixel_display_get_width(d->pixels) &&
                            height / scale == (int)ply_pixel_display_get_height(d->pixels))
                                return d->pixels;
                }
        }
        return osk->displays ? osk->displays->pixels : NULL;
}

static int touched_key(ply_osk_t *osk, struct libinput_event_touch *event)
{
        ply_pixel_display_t *display = touch_display(osk);
        if (!display) return -1;
        double x = libinput_event_touch_get_x_transformed(event, 1);
        double y = libinput_event_touch_get_y_transformed(event, 1);
        unsigned rotation = 0;
        switch (ply_pixel_buffer_get_device_rotation(display_buffer(display))) {
        case PLY_PIXEL_BUFFER_ROTATE_CLOCKWISE: rotation = 90; break;
        case PLY_PIXEL_BUFFER_ROTATE_UPSIDE_DOWN: rotation = 180; break;
        case PLY_PIXEL_BUFFER_ROTATE_COUNTER_CLOCKWISE: rotation = 270; break;
        default: break;
        }
        osk_transform(rotation, &x, &y);
        double width = ply_pixel_display_get_width(display);
        double height = ply_pixel_display_get_height(display);
        return osk_hit(x * width, y * height, width, height);
}

static void activate_key(ply_osk_t *osk, int key)
{
        if (key < 0 || !osk->keyboard) return;
        switch (osk_keys[key].action) {
        case OSK_SHIFT: osk->state.shift = !osk->state.shift; break;
        case OSK_CAPS: osk->state.caps = !osk->state.caps; break;
        case OSK_BACKSPACE: ply_keyboard_input(osk->keyboard, "\177"); break;
        case OSK_ENTER: ply_keyboard_input(osk->keyboard, "\n"); break;
        case OSK_CHARACTER:
                ply_keyboard_input(osk->keyboard, osk_key_text(&osk->state, key));
                osk->state.shift = false;
                break;
        }
}

static void handle_events(void *data, int fd)
{
        ply_osk_t *osk = data;
        struct libinput_event *event;
        bool changed = false;
        if (libinput_dispatch(osk->input) < 0) return;
        while ((event = libinput_get_event(osk->input))) {
                enum libinput_event_type type = libinput_event_get_type(event);
                struct libinput_device *device = libinput_event_get_device(event);
                if (type == LIBINPUT_EVENT_DEVICE_ADDED || type == LIBINPUT_EVENT_DEVICE_REMOVED) {
                        bool added = type == LIBINPUT_EVENT_DEVICE_ADDED;
                        if (libinput_device_has_capability(device, LIBINPUT_DEVICE_CAP_TOUCH)) {
                                if (added) osk->state.touchscreens++;
                                else if (osk->state.touchscreens) osk->state.touchscreens--;
                        }
                        if (is_keyboard(device)) {
                                if (added) osk->state.keyboards++;
                                else if (osk->state.keyboards) osk->state.keyboards--;
                        }
                        ply_trace("touch keyboard: %s %s; %u touchscreens, %u typing keyboards",
                                  added ? "added" : "removed", libinput_device_get_name(device),
                                  osk->state.touchscreens, osk->state.keyboards);
                        osk->slot = osk->pressed = -1;
                        changed = true;
                } else if (osk->active && osk->state.prompt &&
                           type >= LIBINPUT_EVENT_TOUCH_DOWN && type <= LIBINPUT_EVENT_TOUCH_CANCEL) {
                        struct libinput_event_touch *touch = libinput_event_get_touch_event(event);
                        int slot = libinput_event_touch_get_seat_slot(touch);
                        if (type == LIBINPUT_EVENT_TOUCH_DOWN && osk->slot == -1) {
                                bool visible = osk_visible(&osk->state);
                                osk->state.requested = true;
                                osk->slot = slot;
                                /* A reveal tap must never type an unseen key. */
                                osk->pressed = visible ? touched_key(osk, touch) : -1;
                                changed = true;
                        } else if (slot == osk->slot && type == LIBINPUT_EVENT_TOUCH_MOTION) {
                                if (touched_key(osk, touch) != osk->pressed) osk->pressed = -1;
                                changed = true;
                        } else if (slot == osk->slot && type == LIBINPUT_EVENT_TOUCH_UP) {
                                int key = osk->pressed;
                                osk->slot = osk->pressed = -1;
                                activate_key(osk, key);
                                changed = true;
                        } else if (type == LIBINPUT_EVENT_TOUCH_CANCEL) {
                                osk->slot = osk->pressed = -1;
                                changed = true;
                        }
                }
                libinput_event_destroy(event);
        }
        if (changed) osk->changed(osk->data);
}

ply_osk_t *ply_osk_new(ply_osk_changed_t changed, void *data)
{
        ply_osk_t *osk = calloc(1, sizeof(*osk));
        osk->slot = osk->pressed = -1;
        osk->changed = changed;
        osk->data = data;
        osk->label = ply_label_new();
        osk->udev = udev_new();
        if (osk->udev) osk->input = libinput_udev_create_context(&input_interface, NULL, osk->udev);
        if (osk->input && libinput_udev_assign_seat(osk->input, "seat0") == 0) {
                osk->watch = ply_event_loop_watch_fd(ply_event_loop_get_default(),
                        libinput_get_fd(osk->input), PLY_EVENT_LOOP_FD_STATUS_HAS_DATA,
                        handle_events, NULL, osk);
        } else {
                ply_error("Unable to initialize Plymouth touchscreen input");
        }
        return osk;
}

void ply_osk_free(ply_osk_t *osk)
{
        if (!osk) return;
        if (osk->watch) ply_event_loop_stop_watching_fd(ply_event_loop_get_default(), osk->watch);
        if (osk->input) libinput_unref(osk->input);
        if (osk->udev) udev_unref(osk->udev);
        while (osk->displays) {
                display_t *next = osk->displays->next;
                free(osk->displays);
                osk->displays = next;
        }
        ply_label_free(osk->label);
        free(osk);
}

void ply_osk_set_keyboard(ply_osk_t *osk, ply_keyboard_t *keyboard) { osk->keyboard = keyboard; }
bool ply_osk_visible(ply_osk_t *osk) { return osk && osk->active && osk_visible(&osk->state); }
void ply_osk_set_prompt(ply_osk_t *osk, bool prompt)
{
        if (osk->state.prompt != prompt) osk->slot = osk->pressed = -1;
        osk_prompt(&osk->state, prompt);
}
void ply_osk_set_active(ply_osk_t *osk, bool active)
{
        osk->active = active;
        if (!active) osk->slot = osk->pressed = -1;
        /* assign_seat queues existing devices without making the fd readable. */
        if (active && osk->watch) handle_events(osk, libinput_get_fd(osk->input));
}

void ply_osk_add_display(ply_osk_t *osk, ply_pixel_display_t *display)
{
        display_t *d = calloc(1, sizeof(*d));
        d->pixels = display;
        d->next = osk->displays;
        osk->displays = d;
}

void ply_osk_remove_display(ply_osk_t *osk, ply_pixel_display_t *display)
{
        for (display_t **d = &osk->displays; *d; d = &(*d)->next) {
                if ((*d)->pixels == display) {
                        display_t *old = *d;
                        *d = old->next;
                        free(old);
                        break;
                }
        }
        osk->slot = osk->pressed = -1;
}

/* Size labels from their key, not the monitor width: a capped keyboard on an
 * ultrawide display must keep its text inside the same touch targets. */
static void draw_label(ply_osk_t *osk, ply_pixel_buffer_t *buffer, osk_rect area,
                       const char *text, double size, double brightness)
{
        char font[40];
        ply_label_set_text(osk->label, text);
        ply_label_set_color(osk->label, brightness, brightness, brightness, 1);
        for (;;) {
                snprintf(font, sizeof(font), "Sans %.1f", size);
                ply_label_set_font(osk->label, font);
                /* Paint into the current frame without recursive redraws. */
                ply_label_show(osk->label, NULL, 0, 0);
                long w = ply_label_get_width(osk->label), h = ply_label_get_height(osk->label);
                double fit = fmin((area.width * 0.90) / fmax(w, 1),
                                  (area.height * 0.90) / fmax(h, 1));
                if (fit >= 1 || size <= 4) {
                        ply_label_show(osk->label, NULL, area.x + (area.width - w) / 2,
                                       area.y + (area.height - h) / 2);
                        ply_label_draw_area(osk->label, buffer, area.x, area.y, area.width, area.height);
                        return;
                }
                size = fmax(4, size * fmin(fit, 0.9));
        }
}

void ply_osk_draw(ply_osk_t *osk, ply_pixel_display_t *display, ply_pixel_buffer_t *buffer)
{
        if (!ply_osk_visible(osk)) return;
        double width = ply_pixel_display_get_width(display), height = ply_pixel_display_get_height(display);
        ply_rectangle_t background = { .x = 0, .y = height * 0.46, .width = width, .height = height * 0.54 + 1 };
        /* Steam's default keyboard palette; no Steam assets are needed at boot. */
        ply_pixel_buffer_fill_with_hex_color(buffer, &background, 0x23262e);
        double unit = fmin(width * 0.96, 1280.0) / 15;
        double font_size = fmin(20, unit * 0.24);
        for (size_t i = 0; i < osk_key_count; i++) {
                osk_rect r = osk_key_rect(i, width, height);
                bool selected = (int)i == osk->pressed ||
                        (osk_keys[i].action == OSK_SHIFT && osk->state.shift) ||
                        (osk_keys[i].action == OSK_CAPS && osk->state.caps);
                uint32_t color = selected ? 0x1a9fff :
                        osk_keys[i].action == OSK_CHARACTER ? 0x0e141b : 0x000000;
                ply_rectangle_t rect = { .x = r.x, .y = r.y, .width = r.width, .height = r.height };
                ply_pixel_buffer_fill_with_hex_color(buffer, &rect, color);
                const char *text = osk_key_text(&osk->state, i);
                if (!strcmp(text, " ")) {
                        /* Steam leaves Space unlabelled. A small bar remains legible
                         * without a language-specific word or an icon font. */
                        ply_rectangle_t bar = { .x = r.x + r.width * 0.4, .y = r.y + r.height * 0.65,
                                                .width = r.width * 0.2, .height = fmax(1, unit * 0.025) };
                        ply_pixel_buffer_fill_with_hex_color(buffer, &bar, 0xb8bcbf);
                        continue;
                }
                bool symbol = osk_keys[i].action == OSK_CHARACTER &&
                        !(osk_keys[i].normal[0] >= 'a' && osk_keys[i].normal[0] <= 'z');
                if (symbol && !osk->state.shift) {
                        osk_rect hint = { r.x, r.y + r.height * 0.08, r.width, r.height * 0.33 };
                        draw_label(osk, buffer, hint, osk_keys[i].shifted, font_size * 0.7, 0.45);
                        r.y += r.height * 0.38;
                        r.height *= 0.54;
                }
                draw_label(osk, buffer, r, text, font_size, 1.0);
        }
}
