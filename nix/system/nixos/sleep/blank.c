#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <glob.h>
#include <linux/vt.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

/* The lock VT must already own input. Never save or restore private scanout:
 * failure leaves the console locked and the display disabled for a retry.
 * Keep this separate from PAM so the password helper has no DRM dependencies.
 */
static bool blank_card(const char *path) {
    bool safe = true;
    int fd = open(path, O_RDWR | O_CLOEXEC | O_NOFOLLOW);
    if (fd < 0) {
        perror(path);
        return false;
    }
    drmModeRes *resources = drmModeGetResources(fd);
    if (!resources || drmSetMaster(fd) != 0 || drmIsMaster(fd) != 1) {
        fprintf(stderr, "Cannot acquire DRM card %s: %m\n", path);
        if (resources) drmModeFreeResources(resources);
        close(fd);
        return false;
    }
    for (int i = 0; i < resources->count_crtcs; i++) {
        uint32_t crtc = resources->crtcs[i];
        if (drmModeSetCrtc(fd, crtc, 0, 0, 0, NULL, 0, NULL) != 0) {
            fprintf(stderr, "Cannot blank %s CRTC %u: %m\n", path, crtc);
            safe = false;
        }
        drmModeCrtc *state = drmModeGetCrtc(fd, crtc);
        if (!state || state->mode_valid || state->buffer_id != 0) {
            fprintf(stderr, "Blanking readback failed for %s CRTC %u\n", path, crtc);
            safe = false;
        }
        if (state) drmModeFreeCrtc(state);
    }
    /* CRTC disable alone may retain overlay/cursor bindings in atomic state.
     * Clear every plane before another renderer is allowed to enable a CRTC.
     */
    drmModePlaneRes *planes = NULL;
    if (drmSetClientCap(fd, DRM_CLIENT_CAP_UNIVERSAL_PLANES, 1) == 0)
        planes = drmModeGetPlaneResources(fd);
    if (!planes) {
        fprintf(stderr, "Cannot enumerate %s planes: %m\n", path);
        safe = false;
    } else {
        for (uint32_t i = 0; i < planes->count_planes; i++) {
            drmModePlane *plane = drmModeGetPlane(fd, planes->planes[i]);
            if (!plane) {
                safe = false;
                continue;
            }
            if ((plane->fb_id || plane->crtc_id) &&
                drmModeSetPlane(fd, plane->plane_id, 0, 0, 0,
                                0, 0, 0, 0, 0, 0, 0, 0) != 0) {
                fprintf(stderr, "Cannot clear %s plane %u: %m\n", path, plane->plane_id);
                safe = false;
            }
            drmModeFreePlane(plane);
            plane = drmModeGetPlane(fd, planes->planes[i]);
            if (!plane || plane->fb_id || plane->crtc_id) {
                fprintf(stderr, "Plane readback failed for %s plane %u\n", path, planes->planes[i]);
                safe = false;
            }
            if (plane) drmModeFreePlane(plane);
        }
        drmModeFreePlaneResources(planes);
    }
    drmModeFreeResources(resources);
    if (drmDropMaster(fd) != 0) safe = false;
    close(fd);
    return safe;
}

int main(void) {
    if (getuid() != 0 || geteuid() != 0) return 1;
    int tty = open("/dev/tty63", O_RDWR | O_CLOEXEC | O_NOCTTY);
    struct vt_stat state;
    if (tty < 0 || ioctl(tty, VT_GETSTATE, &state) < 0 || state.v_active != 63 ||
        ioctl(tty, VT_LOCKSWITCH) < 0) {
        fprintf(stderr, "Blanking requires the locked console\n");
        return 1;
    }
    close(tty);
    glob_t cards = {0};
    if (glob("/dev/dri/card[0-9]*", 0, NULL, &cards) != 0) {
        fprintf(stderr, "Cannot enumerate DRM cards\n");
        return 1;
    }
    bool safe = true;
    for (size_t i = 0; i < cards.gl_pathc; i++)
        if (!blank_card(cards.gl_pathv[i])) safe = false;
    globfree(&cards);
    return safe ? 0 : 1;
}
