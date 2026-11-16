#ifndef NIXOS_CONTENT_COMPRESSION_H
#define NIXOS_CONTENT_COMPRESSION_H
#include <lzf.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

typedef struct ContentSettingsStorage { char *bytes; int length; } *ContentSettingsHandle;

static inline void contentSettingsRelease(ContentSettingsHandle settings) {
    if (!settings) return;
    free(settings->bytes);
    free(settings);
}

static inline int contentSettingsNibble(char digit) {
    if (digit >= '0' && digit <= '9') return digit - '0';
    if (digit >= 'a' && digit <= 'f') return digit - 'a' + 10;
    if (digit >= 'A' && digit <= 'F') return digit - 'A' + 10;
    return -1;
}

static inline ContentSettingsHandle contentSettingsDecodeHex(const char *hex) {
    if (!hex) return NULL;
    size_t hexLength = strlen(hex);
    if (hexLength < 2 || (hexLength % 2) != 0) return NULL;
    int length = (int)(hexLength / 2);
    unsigned char *input = malloc((size_t)length);
    if (!input) return NULL;
    for (int index = 0; index < length; index++) {
        int high = contentSettingsNibble(hex[index * 2]);
        int low = contentSettingsNibble(hex[index * 2 + 1]);
        if (high < 0 || low < 0) { free(input); return NULL; }
        input[index] = (unsigned char)((high << 4) | low);
    }
    const int maximum = 16 * 1024 * 1024;
    ContentSettingsHandle settings = calloc(1, sizeof(*settings));
    if (!settings) { free(input); return NULL; }
    settings->bytes = malloc((size_t)maximum + 1);
    if (!settings->bytes) { free(input); contentSettingsRelease(settings); return NULL; }
    if (input[0] == 11) {
        settings->length = (int)lzf_decompress(input + 1, (unsigned int)(length - 1), settings->bytes, (unsigned int)maximum);
    } else if (input[0] == 0) {
        z_stream stream = {0};
        stream.next_in = input + 1;
        stream.avail_in = (unsigned int)(length - 1);
        stream.next_out = (unsigned char *)settings->bytes;
        stream.avail_out = (unsigned int)maximum;
        if (inflateInit2(&stream, -15) == Z_OK) {
            if (inflate(&stream, Z_FINISH) == Z_STREAM_END) settings->length = (int)stream.total_out;
            inflateEnd(&stream);
        }
    }
    free(input);
    if (!settings->length) { contentSettingsRelease(settings); return NULL; }
    settings->bytes[settings->length] = '\0';
    return settings;
}

static inline ContentSettingsHandle contentSettingsPackLzf(const char *text) {
    if (!text) return NULL;
    size_t inputLength = strlen(text);
    size_t capacity = inputLength * 2 + 64;
    ContentSettingsHandle settings = calloc(1, sizeof(*settings));
    if (!settings) return NULL;
    settings->bytes = malloc(capacity + 1);
    if (!settings->bytes) { contentSettingsRelease(settings); return NULL; }
    settings->bytes[0] = 11;
    unsigned int compressed = lzf_compress(text, (unsigned int)inputLength, settings->bytes + 1, (unsigned int)capacity);
    if (!compressed) { contentSettingsRelease(settings); return NULL; }
    settings->length = 1 + (int)compressed;
    return settings;
}

static inline ContentSettingsHandle contentSettingsPackDeflate(const char *text) {
    if (!text) return NULL;
    unsigned long inputLength = (unsigned long)strlen(text);
    unsigned long compressedLength = compressBound(inputLength);
    unsigned char *zlibBuffer = malloc(compressedLength);
    if (!zlibBuffer) return NULL;
    if (compress2(zlibBuffer, &compressedLength, (const unsigned char *)text, inputLength, 6) != Z_OK || compressedLength < 6) {
        free(zlibBuffer);
        return NULL;
    }
    ContentSettingsHandle settings = calloc(1, sizeof(*settings));
    if (!settings) { free(zlibBuffer); return NULL; }
    int rawLength = (int)(compressedLength - 6);
    settings->bytes = malloc((size_t)rawLength + 1);
    if (!settings->bytes) { free(zlibBuffer); contentSettingsRelease(settings); return NULL; }
    settings->bytes[0] = 0;
    memcpy(settings->bytes + 1, zlibBuffer + 2, (size_t)rawLength);
    settings->length = 1 + rawLength;
    free(zlibBuffer);
    return settings;
}

static inline const char *contentSettingsText(ContentSettingsHandle settings) { return settings->bytes; }
static inline const unsigned char *contentSettingsBytes(ContentSettingsHandle settings) { return (const unsigned char *)settings->bytes; }
static inline int contentSettingsLength(ContentSettingsHandle settings) { return settings->length > 0 ? settings->length : 0; }
#endif
