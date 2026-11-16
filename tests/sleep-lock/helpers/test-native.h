#ifndef NIXOS_SLEEP_HELPER_TESTS_H
#define NIXOS_SLEEP_HELPER_TESTS_H
#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif
#include "../../../config/system/nixos/sleep/helpers/pam.h"
#include "../../../config/system/nixos/sleep/helpers/drm.h"

static inline volatile int *sleepTestWipeCountSlot(void) {
    static volatile int wipeCount = 0;
    return &wipeCount;
}

void testWipe(void *buffer, size_t length);
void testCheckedWipe(void *buffer, size_t length, size_t bound);

void testWipe(void *buffer, size_t length) {
    volatile unsigned char *wiping = (volatile unsigned char *)buffer;
    for (size_t index = 0; index < length; index++) wiping[index] = 0;
    const unsigned char *bytes = (const unsigned char *)buffer;
    for (size_t index = 0; index < length; index++) {
        if (bytes[index] != 0) _exit(99);
    }
    (*sleepTestWipeCountSlot())++;
}

void testCheckedWipe(void *buffer, size_t length, size_t bound) {
    if (length > bound) _exit(98);
    testWipe(buffer, length);
}

static inline void testResetWipeCount(void) {
    testWipe(NULL, 0);
    testCheckedWipe(NULL, 0, 0);
    *sleepTestWipeCountSlot() = 0;
}

static inline int testReadWipeCount(void) {
    return *sleepTestWipeCountSlot();
}

static inline bool testPamConversationContracts(const char *plymouth) {
    if (!plymouth) return false;
    struct SleepPamPromptData request = { "success", plymouth, PR_SET_PDEATHSIG };
    struct pam_message secret = { PAM_PROMPT_ECHO_OFF, "Password" };
    struct pam_message info = { PAM_TEXT_INFO, "Info" };
    struct pam_message rejected = { PAM_PROMPT_ECHO_ON, "Username" };
    const struct pam_message *messages[2] = { &secret, &info };
    struct pam_response *responses = NULL;
    if (sleepPamConverse(0, messages, &responses, &request) != PAM_CONV_ERR) return false;
    if (sleepPamConverse(PAM_MAX_NUM_MSG + 1, messages, &responses, &request) != PAM_CONV_ERR) return false;
    if (sleepPamConverse(2, messages, &responses, &request) != PAM_SUCCESS) return false;
    bool matched = responses != NULL && responses[0].resp != NULL &&
        strcmp(responses[0].resp, "fixture-password") == 0 && responses[1].resp == NULL;
    sleepFreePamResponses(responses, 2);
    responses = NULL;
    if (!matched) return false;

    const struct pam_message *rejectedMessages[2] = { &secret, &rejected };
    int before = testReadWipeCount();
    if (sleepPamConverse(2, rejectedMessages, &responses, &request) != PAM_CONV_ERR) return false;
    return responses == NULL && testReadWipeCount() == before + 2;
}

static inline bool testDrmBlankPredicates(void) {
    drmModeCrtc crtc = {0};
    drmModePlane plane = {0};
    if (displayCrtcIsBlank(NULL) || displayPlaneIsBlank(NULL)) return false;
    if (!displayCrtcIsBlank(&crtc) || !displayPlaneIsBlank(&plane)) return false;
    crtc.mode_valid = 1;
    if (displayCrtcIsBlank(&crtc)) return false;
    crtc.mode_valid = 0;
    crtc.buffer_id = 42;
    if (displayCrtcIsBlank(&crtc)) return false;
    plane.fb_id = 42;
    if (displayPlaneIsBlank(&plane)) return false;
    plane.fb_id = 0;
    plane.crtc_id = 42;
    if (displayPlaneIsBlank(&plane)) return false;
    return true;
}
#endif
