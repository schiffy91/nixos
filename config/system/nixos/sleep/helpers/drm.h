#ifndef NIXOS_SLEEP_DRM_H
#define NIXOS_SLEEP_DRM_H
#include <fcntl.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

typedef struct DisplayCardStorage {
    int descriptor;
    drmModeRes *resources;
    drmModePlaneRes *planes;
    bool master;
} *DisplayCardHandle;

static inline bool displayCrtcIsBlank(const drmModeCrtc *state) {
    return state != NULL && state->mode_valid == 0 && state->buffer_id == 0;
}

static inline bool displayPlaneIsBlank(const drmModePlane *state) {
    return state != NULL && state->fb_id == 0 && state->crtc_id == 0;
}

static inline long long displayNeutralPropertyValue(const char *name) {
    if (!name) return -1;
    if (strcmp(name, "rotation") == 0) return (long long)DRM_MODE_ROTATE_0;
    if (strcmp(name, "AMD_PLANE_HDR_MULT") == 0) return 4294967296LL;
    static const char *const zeroed[] = {
        "DEGAMMA_LUT", "CTM", "GAMMA_LUT", "HDR_OUTPUT_METADATA", "Colorspace",
        "AMD_CRTC_REGAMMA_TF", "AMD_PLANE_DEGAMMA_LUT", "AMD_PLANE_DEGAMMA_TF",
        "AMD_PLANE_CTM", "AMD_PLANE_SHAPER_LUT", "AMD_PLANE_SHAPER_TF",
        "AMD_PLANE_LUT3D", "AMD_PLANE_BLEND_LUT", "AMD_PLANE_BLEND_TF"
    };
    for (size_t index = 0; index < sizeof(zeroed) / sizeof(zeroed[0]); index++) {
        if (strcmp(name, zeroed[index]) == 0) return 0;
    }
    return -1;
}

static inline bool displayCardClose(DisplayCardHandle card) {
    if (!card) return true;
    if (card->planes) { drmModeFreePlaneResources(card->planes); card->planes = NULL; }
    if (card->resources) { drmModeFreeResources(card->resources); card->resources = NULL; }
    bool dropped = !card->master || drmDropMaster(card->descriptor) == 0;
    card->master = false;
    if (card->descriptor >= 0) { close(card->descriptor); card->descriptor = -1; }
    return dropped;
}

static inline void displayCardRelease(DisplayCardHandle card) {
    if (!card) return;
    displayCardClose(card);
    free(card);
}

static inline DisplayCardHandle displayCardOpen(const char *path) {
    if (!path) return NULL;
    DisplayCardHandle card = calloc(1, sizeof(*card));
    if (!card) return NULL;
    card->descriptor = open(path, O_RDWR | O_CLOEXEC | O_NOFOLLOW);
    return card;
}

static inline bool displayClearCrtc(int descriptor, uint32_t crtc) {
    bool cleared = drmModeSetCrtc(descriptor, crtc, 0, 0, 0, NULL, 0, NULL) == 0;
    drmModeCrtc *state = drmModeGetCrtc(descriptor, crtc);
    bool blank = displayCrtcIsBlank(state);
    if (state) drmModeFreeCrtc(state);
    return cleared && blank;
}

static inline bool displayClearPlane(int descriptor, uint32_t id) {
    drmModePlane *plane = drmModeGetPlane(descriptor, id);
    if (!plane) return false;
    bool cleared = displayPlaneIsBlank(plane) ||
        drmModeSetPlane(descriptor, id, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0) == 0;
    drmModeFreePlane(plane);
    plane = drmModeGetPlane(descriptor, id);
    bool blank = displayPlaneIsBlank(plane);
    if (plane) drmModeFreePlane(plane);
    return cleared && blank;
}

static inline bool displayNeutralProperties(int descriptor, uint32_t object, uint32_t type, drmModeAtomicReq *request) {
    drmModeObjectProperties *properties = drmModeObjectGetProperties(descriptor, object, type);
    if (!properties) return false;
    bool safe = true;
    for (uint32_t index = 0; index < properties->count_props; index++) {
        drmModePropertyRes *property = drmModeGetProperty(descriptor, properties->props[index]);
        if (!property) { safe = false; continue; }
        long long value = displayNeutralPropertyValue(property->name);
        if (value >= 0 && (property->flags & DRM_MODE_PROP_IMMUTABLE) == 0 &&
            properties->prop_values[index] != (uint64_t)value) {
            if (!request || drmModeAtomicAddProperty(request, object, property->prop_id, (uint64_t)value) < 0) {
                safe = false;
            }
        }
        drmModeFreeProperty(property);
    }
    drmModeFreeObjectProperties(properties);
    return safe;
}

static inline bool displayNeutralTransforms(DisplayCardHandle card, drmModeAtomicReq *request) {
    bool safe = true;
    for (int index = 0; index < card->resources->count_crtcs; index++) {
        if (!displayNeutralProperties(card->descriptor, card->resources->crtcs[index], DRM_MODE_OBJECT_CRTC, request)) safe = false;
    }
    for (uint32_t index = 0; index < card->planes->count_planes; index++) {
        if (!displayNeutralProperties(card->descriptor, card->planes->planes[index], DRM_MODE_OBJECT_PLANE, request)) safe = false;
    }
    for (int index = 0; index < card->resources->count_connectors; index++) {
        if (!displayNeutralProperties(card->descriptor, card->resources->connectors[index], DRM_MODE_OBJECT_CONNECTOR, request)) safe = false;
    }
    return safe;
}

static inline bool displayResetTransforms(DisplayCardHandle card) {
    if (drmSetClientCap(card->descriptor, DRM_CLIENT_CAP_ATOMIC, 1) != 0) return false;
    drmModeAtomicReq *request = drmModeAtomicAlloc();
    if (!request) return false;
    bool safe = displayNeutralTransforms(card, request);
    if (safe && drmModeAtomicGetCursor(request) > 0) {
        safe = drmModeAtomicCommit(card->descriptor, request, DRM_MODE_ATOMIC_ALLOW_MODESET, NULL) == 0;
    }
    drmModeAtomicFree(request);
    return safe && displayNeutralTransforms(card, NULL);
}

static inline bool displayCardBlank(DisplayCardHandle card) {
    if (!card || card->descriptor < 0) return false;
    card->resources = drmModeGetResources(card->descriptor);
    if (!card->resources || drmSetMaster(card->descriptor) != 0) return false;
    card->master = true;
    if (drmIsMaster(card->descriptor) != 1) return false;
    bool safe = true;
    for (int index = 0; index < card->resources->count_crtcs; index++) {
        if (!displayClearCrtc(card->descriptor, card->resources->crtcs[index])) safe = false;
    }
    if (drmSetClientCap(card->descriptor, DRM_CLIENT_CAP_UNIVERSAL_PLANES, 1) == 0) {
        card->planes = drmModeGetPlaneResources(card->descriptor);
    }
    if (!card->planes) return false;
    for (uint32_t index = 0; index < card->planes->count_planes; index++) {
        if (!displayClearPlane(card->descriptor, card->planes->planes[index])) safe = false;
    }
    return safe && displayResetTransforms(card);
}
#endif
