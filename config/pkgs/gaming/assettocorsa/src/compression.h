#ifndef NIXOS_CONTENT_COMPRESSION_H
#define NIXOS_CONTENT_COMPRESSION_H
#include <lzf.h>
#include <zlib.h>

enum { cmZlibOk = Z_OK };
// Avoid liblzf's top-level const pointer qualifiers in the imported signature.
static inline unsigned int cmLzfCompress(const char *input, unsigned int length, char *output, unsigned int capacity) {
    return lzf_compress(input, length, output, capacity);
}

#include <stdlib.h>
#include <string.h>

typedef struct ContentSettingsStorage { char *bytes; int length; } *ContentSettingsHandle;
static inline void contentSettingsRelease(ContentSettingsHandle settings) {
    if (!settings) return;
    free(settings->bytes); free(settings);
}
static inline ContentSettingsHandle contentSettingsDecode(const char *input, int length) {
    if (!input || length < 1) return NULL;
    const int maximum = 16 * 1024 * 1024;
    ContentSettingsHandle settings = calloc(1, sizeof(*settings));
    if (!settings) return NULL;
    settings->bytes = malloc((size_t)maximum + 1);
    if (!settings->bytes) { contentSettingsRelease(settings); return NULL; }
    if ((unsigned char)input[0] == 11) {
        settings->length = (int)lzf_decompress(input + 1, (unsigned int)(length - 1), settings->bytes, (unsigned int)maximum);
    } else if (input[0] == 0) {
        z_stream stream = {0};
        stream.next_in = (unsigned char *)(input + 1);
        stream.avail_in = (unsigned int)(length - 1);
        stream.next_out = (unsigned char *)settings->bytes;
        stream.avail_out = (unsigned int)maximum;
        if (inflateInit2(&stream, -15) == Z_OK) {
            if (inflate(&stream, Z_FINISH) == Z_STREAM_END) settings->length = (int)stream.total_out;
            inflateEnd(&stream);
        }
    }
    if (!settings->length) { contentSettingsRelease(settings); return NULL; }
    settings->bytes[settings->length] = '\0';
    return settings;
}
static inline const char *contentSettingsText(ContentSettingsHandle settings) { return settings->bytes; }
#endif
