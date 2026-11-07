#define _GNU_SOURCE
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <wayland-client.h>
#include <pipewire/pipewire.h>
#include <spa/param/video/format-utils.h>
#include <spa/pod/builder.h>
#include "zkde-screencast-unstable-v1-client-protocol.h"

// Keep a virtual-output creator alive, then capture its advertised wl_output
// using the same stream_output request as Sunshine. Save atomic PPM snapshots
// with a sequence and monotonic reception timestamp for actual pixel assertions.
static struct wl_display *display;
static struct zkde_screencast_unstable_v1 *manager;
static struct pw_main_loop *loop;
static struct pw_stream *consumer;
static struct spa_video_info_raw format;
static uint32_t node;
static unsigned long sequence;
static bool failed;
static const char *destination;
static FILE *history;
static struct wl_output *capture_output;

static void output_geometry(void *data, struct wl_output *output, int32_t x, int32_t y,
                            int32_t width, int32_t height, int32_t subpixel,
                            const char *make, const char *model, int32_t transform) {
    (void)data; (void)output; (void)x; (void)y; (void)width; (void)height;
    (void)subpixel; (void)make; (void)model; (void)transform;
}
static void output_mode(void *data, struct wl_output *output, uint32_t flags,
                        int32_t width, int32_t height, int32_t refresh) {
    (void)data; (void)output; (void)flags; (void)width; (void)height; (void)refresh;
}
static void output_done(void *data, struct wl_output *output) { (void)data; (void)output; }
static void output_scale(void *data, struct wl_output *output, int32_t scale) {
    (void)data; (void)output; (void)scale;
}
static void output_name(void *data, struct wl_output *output, const char *name) {
    (void)data;
    if (!strcmp(name, "Virtual-Sleep capture test")) {
        if (capture_output && capture_output != output) {
            fprintf(stderr, "Duplicate capture output name\n");
            failed = true;
        }
        capture_output = output;
    }
}
static void output_description(void *data, struct wl_output *output, const char *description) {
    (void)data; (void)output; (void)description;
}
static const struct wl_output_listener output_listener = {
    .geometry = output_geometry, .mode = output_mode, .done = output_done,
    .scale = output_scale, .name = output_name, .description = output_description,
};

static void closed(void *data, struct zkde_screencast_stream_unstable_v1 *stream) {
    (void)data; (void)stream;
    failed = true;
    if (loop) pw_main_loop_quit(loop);
}
static void created(void *data, struct zkde_screencast_stream_unstable_v1 *stream, uint32_t id) {
    (void)data; (void)stream;
    node = id;
}
static void error(void *data, struct zkde_screencast_stream_unstable_v1 *stream, const char *message) {
    fprintf(stderr, "KWin stream failed: %s\n", message);
    closed(data, stream);
}
static const struct zkde_screencast_stream_unstable_v1_listener listener = {
    .closed = closed, .created = created, .failed = error,
};
static void global(void *data, struct wl_registry *registry, uint32_t id, const char *interface, uint32_t version) {
    (void)data;
    if (!strcmp(interface, zkde_screencast_unstable_v1_interface.name) && version >= 2)
        manager = wl_registry_bind(registry, id, &zkde_screencast_unstable_v1_interface, version < 5 ? version : 5);
    if (!strcmp(interface, wl_output_interface.name) && version >= 4) {
        struct wl_output *output = wl_registry_bind(registry, id, &wl_output_interface, 4);
        wl_output_add_listener(output, &output_listener, NULL);
    }
}
static void removed(void *data, struct wl_registry *registry, uint32_t id) {
    (void)data; (void)registry; (void)id;
}
static const struct wl_registry_listener registry_listener = {.global = global, .global_remove = removed};

static void state_changed(void *data, enum pw_stream_state old, enum pw_stream_state state, const char *message) {
    (void)data; (void)old;
    fprintf(stderr, "PipeWire state %s: %s\n", pw_stream_state_as_string(state), message ? message : "");
    if (state == PW_STREAM_STATE_ERROR) { failed = true; pw_main_loop_quit(loop); }
}
static void param_changed(void *data, uint32_t id, const struct spa_pod *param) {
    (void)data;
    if (id != SPA_PARAM_Format || !param) return;
    if (spa_format_video_raw_parse(param, &format) < 0 || format.format != SPA_VIDEO_FORMAT_BGRx) {
        failed = true; pw_main_loop_quit(loop); return;
    }
    fprintf(stderr, "PipeWire format BGRx %ux%u\n", format.size.width, format.size.height);
}
static void process(void *data) {
    (void)data;
    struct pw_buffer *buffer = pw_stream_dequeue_buffer(consumer);
    if (!buffer) return;
    struct spa_buffer *frame = buffer->buffer;
    if (frame->n_datas && format.size.width && format.size.height) {
        struct spa_data *pixels = &frame->datas[0];
        if (pixels->data && pixels->chunk && pixels->chunk->size && pixels->type != SPA_DATA_DmaBuf) {
            int32_t stride = pixels->chunk->stride;
            size_t width = format.size.width, height = format.size.height, offset = pixels->chunk->offset;
            if (stride >= (int32_t)(width * 4) && offset + (height - 1) * (size_t)stride + width * 4 <= pixels->maxsize) {
                char *temporary = NULL;
                if (asprintf(&temporary, "%s.next", destination) < 0) abort();
                FILE *file = fopen(temporary, "wb");
                if (!file) { perror("snapshot"); exit(1); }
                struct timespec now;
                clock_gettime(CLOCK_MONOTONIC, &now);
                fprintf(file, "P6\n# %lu %ld.%09ld\n%zu %zu\n255\n", ++sequence, now.tv_sec, now.tv_nsec, width, height);
                unsigned long private_pixels = 0;
                uint8_t *row = malloc(width * 3);
                if (!row) abort();
                for (size_t y = 0; y < height; y++) {
                    const uint8_t *source = (uint8_t *)pixels->data + offset + y * (size_t)stride;
                    for (size_t x = 0; x < width; x++) {
                        row[x * 3] = source[x * 4 + 2]; row[x * 3 + 1] = source[x * 4 + 1]; row[x * 3 + 2] = source[x * 4];
                        if ((row[x * 3] == 0x90 && row[x * 3 + 1] == 0x20 && row[x * 3 + 2] == 0x20) ||
                            (row[x * 3] == 0x20 && row[x * 3 + 1] == 0x60 && row[x * 3 + 2] == 0x90)) private_pixels++;
                    }
                    if (fwrite(row, 3, width, file) != width) abort();
                }
                free(row);
                if (fclose(file) || rename(temporary, destination)) { perror("snapshot publish"); exit(1); }
                free(temporary);
                fprintf(history, "%lu %ld.%09ld %lu\n", sequence, now.tv_sec, now.tv_nsec, private_pixels);
                fflush(history);
            }
        }
    }
    pw_stream_queue_buffer(consumer, buffer);
}
static const struct pw_stream_events events = {
    PW_VERSION_STREAM_EVENTS, .state_changed = state_changed, .param_changed = param_changed, .process = process,
};
static void wayland_event(void *data, int fd, uint32_t mask) {
    (void)data; (void)fd;
    if (mask & (SPA_IO_ERR | SPA_IO_HUP) || wl_display_dispatch(display) < 0) {
        failed = true; pw_main_loop_quit(loop);
    }
}
int main(int argc, char **argv) {
    if (argc != 2) { fprintf(stderr, "usage: sleep-capture-probe OUTPUT.ppm\n"); return 2; }
    destination = argv[1];
    char *history_path = NULL;
    if (asprintf(&history_path, "%s.history", destination) < 0) abort();
    history = fopen(history_path, "w");
    free(history_path);
    if (!history) { perror("history"); return 1; }
    display = wl_display_connect(NULL);
    if (!display) { perror("Wayland connect"); return 1; }
    struct wl_registry *registry = wl_display_get_registry(display);
    wl_registry_add_listener(registry, &registry_listener, NULL);
    if (wl_display_roundtrip(display) < 0 || !manager) { fprintf(stderr, "KWin screencast protocol unavailable\n"); return 1; }
    struct zkde_screencast_stream_unstable_v1 *stream = zkde_screencast_unstable_v1_stream_virtual_output(
        manager, "Sleep capture test", 1024, 768, wl_fixed_from_int(1), ZKDE_SCREENCAST_UNSTABLE_V1_POINTER_HIDDEN);
    zkde_screencast_stream_unstable_v1_add_listener(stream, &listener, NULL);
    for (unsigned tries = 0; tries < 100 && !node && !failed; tries++) {
        if (wl_display_roundtrip(display) < 0) return 1;
        usleep(50000);
    }
    if (!node || failed) { fprintf(stderr, "No PipeWire node\n"); return 1; }
    fprintf(stderr, "KWin virtual-output creator PipeWire node %u\n", node);
    // The creator stream remains alive until this Wayland connection closes.
    // Two round trips discover its global and receive wl_output.name events.
    if (wl_display_roundtrip(display) < 0 || wl_display_roundtrip(display) < 0 || !capture_output || failed) {
        fprintf(stderr, "Named capture output unavailable\n"); return 1;
    }
    node = 0;
    struct zkde_screencast_stream_unstable_v1 *capture = zkde_screencast_unstable_v1_stream_output(
        manager, capture_output, ZKDE_SCREENCAST_UNSTABLE_V1_POINTER_EMBEDDED);
    zkde_screencast_stream_unstable_v1_add_listener(capture, &listener, NULL);
    for (unsigned tries = 0; tries < 100 && !node && !failed; tries++) {
        if (wl_display_roundtrip(display) < 0) return 1;
        usleep(50000);
    }
    if (!node || failed) { fprintf(stderr, "No output-capture PipeWire node\n"); return 1; }
    fprintf(stderr, "KWin stream_output PipeWire node %u\n", node);
    pw_init(NULL, NULL);
    loop = pw_main_loop_new(NULL);
    if (!loop) return 1;
    struct pw_loop *pwloop = pw_main_loop_get_loop(loop);
    consumer = pw_stream_new_simple(pwloop, "sleep-capture-probe", pw_properties_new(
        PW_KEY_MEDIA_TYPE, "Video", PW_KEY_MEDIA_CATEGORY, "Capture", PW_KEY_MEDIA_ROLE, "Screen", NULL), &events, NULL);
    if (!consumer) return 1;
    uint8_t storage[1024];
    struct spa_pod_builder builder = SPA_POD_BUILDER_INIT(storage, sizeof(storage));
    const struct spa_pod *params[] = {spa_pod_builder_add_object(&builder, SPA_TYPE_OBJECT_Format, SPA_PARAM_EnumFormat,
        SPA_FORMAT_mediaType, SPA_POD_Id(SPA_MEDIA_TYPE_video), SPA_FORMAT_mediaSubtype, SPA_POD_Id(SPA_MEDIA_SUBTYPE_raw),
        SPA_FORMAT_VIDEO_format, SPA_POD_Id(SPA_VIDEO_FORMAT_BGRx),
        SPA_FORMAT_VIDEO_size, SPA_POD_Rectangle(&SPA_RECTANGLE(1024, 768)),
        SPA_FORMAT_VIDEO_framerate, SPA_POD_Fraction(&SPA_FRACTION(0, 1)))};
    if (pw_stream_connect(consumer, PW_DIRECTION_INPUT, node, PW_STREAM_FLAG_AUTOCONNECT | PW_STREAM_FLAG_MAP_BUFFERS, params, 1) < 0) return 1;
    pw_loop_add_io(pwloop, wl_display_get_fd(display), SPA_IO_IN | SPA_IO_ERR | SPA_IO_HUP, false, wayland_event, NULL);
    pw_main_loop_run(loop);
    pw_stream_destroy(consumer); pw_main_loop_destroy(loop); pw_deinit();
    return failed ? 1 : 0;
}
