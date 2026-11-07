#include <fcntl.h>
#include <stdbool.h>
#include <unistd.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

/* Read-only evidence when QEMU reports that a disabled output has no scanout. */
int main(int argc, char **argv) {
    if (argc != 2) return 1;
    int fd = open(argv[1], O_RDWR);
    if (fd < 0) return 1;
    drmModeRes *resources = drmModeGetResources(fd);
    if (!resources) return 1;
    bool blank = true;
    for (int i = 0; i < resources->count_crtcs; i++) {
        drmModeCrtc *crtc = drmModeGetCrtc(fd, resources->crtcs[i]);
        if (!crtc || crtc->mode_valid || crtc->buffer_id) blank = false;
        if (crtc) drmModeFreeCrtc(crtc);
    }
    drmModeFreeResources(resources);
    if (drmSetClientCap(fd, DRM_CLIENT_CAP_UNIVERSAL_PLANES, 1) != 0) return 1;
    drmModePlaneRes *planes = drmModeGetPlaneResources(fd);
    if (!planes) return 1;
    for (uint32_t i = 0; i < planes->count_planes; i++) {
        drmModePlane *plane = drmModeGetPlane(fd, planes->planes[i]);
        if (!plane || plane->fb_id || plane->crtc_id) blank = false;
        if (plane) drmModeFreePlane(plane);
    }
    drmModeFreePlaneResources(planes);
    close(fd);
    return blank ? 0 : 1;
}
