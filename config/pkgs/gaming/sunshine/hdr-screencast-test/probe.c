#define _GNU_SOURCE
// Verifies KWin's screencast HDR path: starts a screencast of an output over
// zkde_screencast_unstable_v1, consumes it from PipeWire with a chosen format
// and colorimetry, and reports the negotiated format plus a sampled pixel.
//
// Modes:
//   hdr10    offer xBGR_210LE / BT2020 / SMPTE2084 over dmabuf
//   shm-pq   offer BGRx / BT2020 / SMPTE2084 over memfd, sample a pixel
//   shm-sdr  offer BGRx / BT709  / GAMMA22   over memfd, sample a pixel (control)
//
// The white reference window runs in a forked child with its own Wayland
// connection, because KWin hides the casting client's own windows from the
// capture.

#include <errno.h>
#include <signal.h>
#include <time.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <unistd.h>

#include <wayland-client.h>
#include <drm_fourcc.h>

#include <spa/param/video/format-utils.h>
#include <spa/pod/builder.h>
#include <spa/utils/result.h>
#include <pipewire/pipewire.h>

#include "zkde-screencast-unstable-v1-client-protocol.h"
#include "xdg-shell-client-protocol.h"

enum mode { MODE_HDR10, MODE_SHM_PQ, MODE_SHM_SDR, MODE_DUMP };

// The runner exports these unconditionally, so an unset switch arrives as an
// empty string rather than as NULL.
static bool envSet(const char *name)  // unset switches arrive empty, not NULL
{
    const char *value = getenv(name);
    return value && *value;
}

static enum mode g_mode;
static const char *g_mode_name;

struct wl_ctx {
    struct wl_display *display;
    struct wl_registry *registry;
    struct wl_compositor *compositor;
    struct wl_shm *shm;
    struct xdg_wm_base *wm_base;
    struct zkde_screencast_unstable_v1 *screencast;
    struct wl_output *output;
    uint32_t node_id;
    uint64_t serial;
    bool has_serial;
    bool node_ready;
    bool failed;
    char fail_msg[256];
};

static struct wl_ctx wl;


static void stream_closed(void *data, struct zkde_screencast_stream_unstable_v1 *s)
{
    struct wl_ctx *c = data;
    c->failed = true;
    snprintf(c->fail_msg, sizeof(c->fail_msg), "stream closed by compositor");
}

static void stream_created(void *data, struct zkde_screencast_stream_unstable_v1 *s, uint32_t node)
{
    struct wl_ctx *c = data;
    c->node_id = node;
    c->node_ready = true;
}

static void stream_failed(void *data, struct zkde_screencast_stream_unstable_v1 *s, const char *error)
{
    struct wl_ctx *c = data;
    c->failed = true;
    snprintf(c->fail_msg, sizeof(c->fail_msg), "%s", error);
}

static void stream_serial(void *data, struct zkde_screencast_stream_unstable_v1 *s,
                          uint32_t serial_hi, uint32_t serial_low)
{
    struct wl_ctx *c = data;
    c->serial = ((uint64_t)serial_hi << 32) | serial_low;
    c->has_serial = true;
}

static const struct zkde_screencast_stream_unstable_v1_listener stream_listener = {
    .closed = stream_closed,
    .created = stream_created,
    .failed = stream_failed,
    .serial = stream_serial,
};

static void wm_base_ping(void *data, struct xdg_wm_base *b, uint32_t serial)
{
    xdg_wm_base_pong(b, serial);
}
static const struct xdg_wm_base_listener wm_base_listener = {.ping = wm_base_ping};

static void registry_global(void *data, struct wl_registry *reg, uint32_t name,
                            const char *iface, uint32_t version)
{
    struct wl_ctx *c = data;
    if (!strcmp(iface, wl_compositor_interface.name)) {
        c->compositor = wl_registry_bind(reg, name, &wl_compositor_interface, 4);
    } else if (!strcmp(iface, wl_shm_interface.name)) {
        c->shm = wl_registry_bind(reg, name, &wl_shm_interface, 1);
    } else if (!strcmp(iface, xdg_wm_base_interface.name)) {
        c->wm_base = wl_registry_bind(reg, name, &xdg_wm_base_interface, 1);
        xdg_wm_base_add_listener(c->wm_base, &wm_base_listener, c);
    } else if (!strcmp(iface, zkde_screencast_unstable_v1_interface.name)) {
        uint32_t v = version < 6 ? version : 6;
        c->screencast = wl_registry_bind(reg, name, &zkde_screencast_unstable_v1_interface, v);
    } else if (!strcmp(iface, wl_output_interface.name)) {
        if (!c->output) {
            c->output = wl_registry_bind(reg, name, &wl_output_interface, 2);
        }
    }
}
static void registry_global_remove(void *data, struct wl_registry *r, uint32_t name) {}
static const struct wl_registry_listener registry_listener = {
    .global = registry_global,
    .global_remove = registry_global_remove,
};

static void run_white_window(void)  // own pid, else KWin hides it from the capture
{
    struct wl_ctx c = {0};
    c.display = wl_display_connect(NULL);
    if (!c.display) {
        _exit(1);
    }
    c.registry = wl_display_get_registry(c.display);
    wl_registry_add_listener(c.registry, &registry_listener, &c);
    wl_display_roundtrip(c.display);
    if (!c.compositor || !c.shm || !c.wm_base || !c.output) {
        _exit(1);
    }

    const int w = 1024, h = 768, stride = w * 4;
    const int size = stride * h;
    int fd = memfd_create("white", MFD_CLOEXEC);
    if (fd < 0 || ftruncate(fd, size) < 0) {
        _exit(1);
    }
    uint32_t *px = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (px == MAP_FAILED) {
        _exit(1);
    }
    for (int i = 0; i < w * h; i++) {
        px[i] = 0xffffffffu;  // opaque white
    }

    struct wl_shm_pool *pool = wl_shm_create_pool(c.shm, fd, size);
    struct wl_buffer *buf = wl_shm_pool_create_buffer(pool, 0, w, h, stride, WL_SHM_FORMAT_XRGB8888);
    struct wl_surface *surface = wl_compositor_create_surface(c.compositor);
    struct xdg_surface *xdg = xdg_wm_base_get_xdg_surface(c.wm_base, surface);
    struct xdg_toplevel *top = xdg_surface_get_toplevel(xdg);
    xdg_toplevel_set_title(top, "hdr-probe-white");
    xdg_toplevel_set_fullscreen(top, c.output);

    // opaque, so KWin has no reason to blend it
    struct wl_region *opaque = wl_compositor_create_region(c.compositor);
    wl_region_add(opaque, 0, 0, w, h);
    wl_surface_set_opaque_region(surface, opaque);

    wl_surface_commit(surface);
    wl_display_roundtrip(c.display);

    // KWin only records on damage, so keep committing
    struct redraw {
        struct wl_surface *surface;
        struct wl_buffer *buffer;
        int w, h;
    } r = {surface, buf, w, h};

    static const struct wl_callback_listener frame_listener;
    void (*redraw_fn)(void *, struct wl_callback *, uint32_t) = NULL;
    (void)redraw_fn;
    (void)frame_listener;

    for (;;) {
        wl_surface_attach(r.surface, r.buffer, 0, 0);
        wl_surface_damage_buffer(r.surface, 0, 0, r.w, r.h);
        wl_surface_commit(r.surface);
        if (wl_display_flush(c.display) < 0 && errno != EAGAIN) {
            break;
        }
        if (wl_display_dispatch_pending(c.display) < 0) {
            break;
        }
        usleep(33000);  // ~30 fps is plenty to keep the output damaged
    }
    _exit(0);
}


struct pw_ctx {
    struct pw_main_loop *loop;
    struct pw_context *context;
    struct pw_core *core;
    struct pw_stream *stream;
    struct spa_hook stream_listener;

    bool got_format;
    struct spa_video_info_raw fmt;

    int frames;
    bool sampled;
    unsigned sample_r, sample_g, sample_b;
    unsigned sample_bits;
    bool dmabuf_frame;
    int64_t dmabuf_modifier;
    const char *error;
};

static struct pw_ctx pw;

static void on_stream_state(void *data, enum pw_stream_state old,
                            enum pw_stream_state state, const char *error)
{
    printf("RESULT: stream_state=%s\n", pw_stream_state_as_string(state));
    fflush(stdout);
    if (state == PW_STREAM_STATE_ERROR) {
        pw.error = error ? error : "stream error";
        pw_main_loop_quit(pw.loop);
    }
}

static void on_param_changed(void *data, uint32_t id, const struct spa_pod *param)
{
    if (id != SPA_PARAM_Format || !param) {
        return;
    }
    uint32_t media_type, media_subtype;
    if (spa_format_parse(param, &media_type, &media_subtype) < 0) {
        return;
    }
    if (media_type != SPA_MEDIA_TYPE_video || media_subtype != SPA_MEDIA_SUBTYPE_raw) {
        return;
    }
    spa_zero(pw.fmt);
    if (spa_format_video_raw_parse(param, &pw.fmt) < 0) {
        return;
    }
    pw.got_format = true;

    printf("RESULT: negotiated_format=%u\n", pw.fmt.format);
    printf("RESULT: negotiated_primaries=%u\n", pw.fmt.color_primaries);
    printf("RESULT: negotiated_transfer=%u\n", pw.fmt.transfer_function);
    printf("RESULT: negotiated_size=%ux%u\n", pw.fmt.size.width, pw.fmt.size.height);
    fflush(stdout);

    // KWin allocates the buffers, so do not override its params
    if (!envSet("PROBE_SEND_BUFFERS")) {
        return;
    }
    uint8_t buffer[1024];
    struct spa_pod_builder b = SPA_POD_BUILDER_INIT(buffer, sizeof(buffer));
    const int types = (g_mode == MODE_HDR10)
        ? ((1 << SPA_DATA_DmaBuf) | (1 << SPA_DATA_MemFd) | (1 << SPA_DATA_MemPtr))
        : ((1 << SPA_DATA_MemFd) | (1 << SPA_DATA_MemPtr));
    const struct spa_pod *params[1];
    params[0] = spa_pod_builder_add_object(&b,
        SPA_TYPE_OBJECT_ParamBuffers, SPA_PARAM_Buffers,
        SPA_PARAM_BUFFERS_buffers, SPA_POD_CHOICE_RANGE_Int(3, 2, 8),
        SPA_PARAM_BUFFERS_dataType, SPA_POD_CHOICE_FLAGS_Int(types));
    pw_stream_update_params(pw.stream, params, 1);
}

static void on_process(void *data)
{
    struct pw_buffer *b = pw_stream_dequeue_buffer(pw.stream);
    if (!b) {
        return;
    }
    struct spa_buffer *buf = b->buffer;
    if (buf->n_datas > 0) {
        struct spa_data *d = &buf->datas[0];
        if (d->type == SPA_DATA_DmaBuf) {
            pw.dmabuf_frame = true;
            pw.frames++;
        } else if (d->data && d->chunk && d->chunk->size > 0) {
            const uint32_t stride = d->chunk->stride ? d->chunk->stride : pw.fmt.size.width * 4;
            const uint32_t x = pw.fmt.size.width / 2;
            const uint32_t y = pw.fmt.size.height / 2;
            const uint8_t *row = (const uint8_t *)d->data + d->chunk->offset + (size_t)y * stride;
            const uint8_t *p = row + (size_t)x * 4;
            if ((const uint8_t *)d->data + d->maxsize >= p + 4) {
                // little endian memory order is B,G,R,x
                pw.sample_b = p[0];
                pw.sample_g = p[1];
                pw.sample_r = p[2];
                pw.sample_bits = 8;
                pw.sampled = true;
            }
            pw.frames++;
        }
    }
    pw_stream_queue_buffer(pw.stream, b);

    if (pw.frames >= 3) {
        pw_main_loop_quit(pw.loop);
    }
}

static const struct pw_stream_events stream_events = {
    PW_VERSION_STREAM_EVENTS,
    .state_changed = on_stream_state,
    .param_changed = on_param_changed,
    .process = on_process,
};

static const struct spa_pod *build_enum_format(struct spa_pod_builder *b)
{
    struct spa_pod_frame f;
    spa_pod_builder_push_object(b, &f, SPA_TYPE_OBJECT_Format, SPA_PARAM_EnumFormat);
    spa_pod_builder_add(b,
        SPA_FORMAT_mediaType, SPA_POD_Id(SPA_MEDIA_TYPE_video),
        SPA_FORMAT_mediaSubtype, SPA_POD_Id(SPA_MEDIA_SUBTYPE_raw), 0);

    uint32_t video_format = (g_mode == MODE_HDR10)
        ? SPA_VIDEO_FORMAT_xBGR_210LE
        : SPA_VIDEO_FORMAT_BGRx;
    if (envSet("PROBE_FORMAT")) {
        video_format = (uint32_t)atoi(getenv("PROBE_FORMAT"));
    }
    spa_pod_builder_add(b, SPA_FORMAT_VIDEO_format, SPA_POD_Id(video_format), 0);

    if (g_mode == MODE_HDR10) {
        // KWin marks the modifier property mandatory for dmabuf
        static const int64_t mods[] = {DRM_FORMAT_MOD_INVALID, DRM_FORMAT_MOD_LINEAR};
        struct spa_pod_frame mf;
        spa_pod_builder_prop(b, SPA_FORMAT_VIDEO_modifier,
                             SPA_POD_PROP_FLAG_MANDATORY | SPA_POD_PROP_FLAG_DONT_FIXATE);
        spa_pod_builder_push_choice(b, &mf, SPA_CHOICE_Enum, 0);
        spa_pod_builder_long(b, mods[0]);
        for (size_t i = 0; i < SPA_N_ELEMENTS(mods); i++) {
            spa_pod_builder_long(b, mods[i]);
        }
        spa_pod_builder_pop(b, &mf);
    }

    const uint32_t primaries = (g_mode == MODE_SHM_SDR)
        ? SPA_VIDEO_COLOR_PRIMARIES_BT709
        : SPA_VIDEO_COLOR_PRIMARIES_BT2020;
    const uint32_t transfer = (g_mode == MODE_SHM_SDR)
        ? SPA_VIDEO_TRANSFER_GAMMA22
        : SPA_VIDEO_TRANSFER_SMPTE2084;
    if (!envSet("PROBE_NO_COLOR")) {
        spa_pod_builder_add(b,
            SPA_FORMAT_VIDEO_colorPrimaries, SPA_POD_Id(primaries),
            SPA_FORMAT_VIDEO_transferFunction, SPA_POD_Id(transfer), 0);
    }

    if (!envSet("PROBE_NO_SIZE")) {
        spa_pod_builder_add(b,
            SPA_FORMAT_VIDEO_size,
                SPA_POD_CHOICE_RANGE_Rectangle(&SPA_RECTANGLE(1024, 768),
                                               &SPA_RECTANGLE(1, 1),
                                               &SPA_RECTANGLE(16384, 16384)),
            0);
    }
    if (!envSet("PROBE_NO_RATE")) {
        spa_pod_builder_add(b,
            SPA_FORMAT_VIDEO_framerate, SPA_POD_Fraction(&SPA_FRACTION(0, 1)), 0);
    }
    return spa_pod_builder_pop(b, &f);
}

static int run_consumer(uint32_t node_id)
{
    pw_init(NULL, NULL);
    pw.loop = pw_main_loop_new(NULL);
    if (!pw.loop) {
        fprintf(stderr, "probe: pw_main_loop_new failed\n");
        return 1;
    }

    pw.stream = pw_stream_new_simple(
        pw_main_loop_get_loop(pw.loop), "hdr-probe",
        pw_properties_new(PW_KEY_MEDIA_TYPE, "Video",
                          PW_KEY_MEDIA_CATEGORY, "Capture",
                          PW_KEY_MEDIA_ROLE, "Screen",
                          NULL),
        &stream_events, &pw);
    if (!pw.stream) {
        fprintf(stderr, "probe: pw_stream_new_simple failed\n");
        return 1;
    }

    uint8_t buffer[2048];
    struct spa_pod_builder b = SPA_POD_BUILDER_INIT(buffer, sizeof(buffer));
    const struct spa_pod *params[1] = {build_enum_format(&b)};

    int res = pw_stream_connect(pw.stream, PW_DIRECTION_INPUT, node_id,
                               PW_STREAM_FLAG_AUTOCONNECT | PW_STREAM_FLAG_MAP_BUFFERS,
                               params, 1);
    if (res < 0) {
        fprintf(stderr, "probe: pw_stream_connect failed: %s\n", spa_strerror(res));
        return 1;
    }

    // never hang if negotiation stalls
    struct spa_source *timer = pw_loop_add_timer(pw_main_loop_get_loop(pw.loop),
                                                 (void (*)(void *, uint64_t))pw_main_loop_quit, pw.loop);
    struct timespec value = {.tv_sec = 15};
    pw_loop_update_timer(pw_main_loop_get_loop(pw.loop), timer, &value, NULL, false);

    pw_main_loop_run(pw.loop);

    pw_stream_destroy(pw.stream);
    pw_main_loop_destroy(pw.loop);
    pw_deinit();
    return 0;
}


int main(int argc, char **argv)
{
    const char *mode = argc > 1 ? argv[1] : "shm-pq";
    if (!strcmp(mode, "hdr10")) {
        g_mode = MODE_HDR10;
    } else if (!strcmp(mode, "shm-pq")) {
        g_mode = MODE_SHM_PQ;
    } else if (!strcmp(mode, "shm-sdr")) {
        g_mode = MODE_SHM_SDR;
    } else if (!strcmp(mode, "dump")) {
        g_mode = MODE_DUMP;
    } else {
        fprintf(stderr, "usage: probe <hdr10|shm-pq|shm-sdr>\n");
        return 2;
    }
    g_mode_name = mode;
    printf("RESULT: mode=%s\n", g_mode_name);

    wl.display = wl_display_connect(NULL);
    if (!wl.display) {
        fprintf(stderr, "probe: cannot connect to WAYLAND_DISPLAY\n");
        return 1;
    }
    wl.registry = wl_display_get_registry(wl.display);
    wl_registry_add_listener(wl.registry, &registry_listener, &wl);
    wl_display_roundtrip(wl.display);
    wl_display_roundtrip(wl.display);

    if (!wl.screencast) {
        fprintf(stderr, "probe: compositor does not offer zkde_screencast_unstable_v1\n");
        return 1;
    }
    if (!wl.output) {
        fprintf(stderr, "probe: no wl_output\n");
        return 1;
    }

    pid_t child = fork();
    if (child == 0) {
        run_white_window();
        _exit(0);
    }
    // let the white window map and composite
    sleep(2);

    struct zkde_screencast_stream_unstable_v1 *stream =
        zkde_screencast_unstable_v1_stream_output(wl.screencast, wl.output,
                                                  ZKDE_SCREENCAST_UNSTABLE_V1_POINTER_HIDDEN);
    zkde_screencast_stream_unstable_v1_add_listener(stream, &stream_listener, &wl);
    wl_display_roundtrip(wl.display);

    for (int i = 0; i < 100 && !wl.node_ready && !wl.failed; i++) {
        wl_display_roundtrip(wl.display);
        usleep(50000);
    }
    if (wl.failed) {
        fprintf(stderr, "probe: screencast failed: %s\n", wl.fail_msg);
        kill(child, SIGTERM);
        return 1;
    }
    if (!wl.node_ready) {
        fprintf(stderr, "probe: compositor never reported a pipewire node\n");
        kill(child, SIGTERM);
        return 1;
    }
    printf("RESULT: node=%u\n", wl.node_id);
    if (wl.has_serial) {
        printf("RESULT: serial=%llu\n", (unsigned long long)wl.serial);
    }
    fflush(stdout);

    if (g_mode == MODE_DUMP) {
        // hold the stream open for an external pw-dump
        sleep(12);
        kill(child, SIGTERM);
        waitpid(child, NULL, 0);
        return 0;
    }

    run_consumer(wl.node_id);

    kill(child, SIGTERM);
    waitpid(child, NULL, 0);

    printf("RESULT: frames=%d\n", pw.frames);
    printf("RESULT: dmabuf=%d\n", pw.dmabuf_frame ? 1 : 0);
    if (pw.sampled) {
        printf("RESULT: center_rgb=%u,%u,%u\n", pw.sample_r, pw.sample_g, pw.sample_b);
    }
    if (pw.error) {
        printf("RESULT: error=%s\n", pw.error);
    }
    if (!pw.got_format) {
        fprintf(stderr, "probe: format was never negotiated\n");
        return 1;
    }
    if (pw.frames == 0) {
        fprintf(stderr, "probe: no frames received\n");
        return 1;
    }
    return 0;
}
