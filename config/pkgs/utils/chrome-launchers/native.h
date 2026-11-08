#ifndef NIXOS_DESKTOP_ENTRY_H
#define NIXOS_DESKTOP_ENTRY_H
#include <glib.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>

typedef struct DesktopEntryStorage {
    GKeyFile *file;
    char *text;
    char **arguments;
    int count;
} *DesktopEntryHandle;

static inline void desktopEntryRelease(DesktopEntryHandle entry) {
    if (!entry) return;
    if (entry->file) g_key_file_unref(entry->file);
    g_free(entry->text);
    g_strfreev(entry->arguments);
    free(entry);
}
static inline DesktopEntryHandle desktopEntryOpen(const char *path) {
    DesktopEntryHandle entry = calloc(1, sizeof(*entry));
    if (!entry) return NULL;
    entry->file = g_key_file_new();
    if (!g_key_file_load_from_file(entry->file, path,
            G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS, NULL)) {
        desktopEntryRelease(entry); return NULL;
    }
    return entry;
}
static inline const char *desktopEntryValue(DesktopEntryHandle entry, const char *key) {
    g_free(entry->text);
    entry->text = g_key_file_get_string(entry->file, "Desktop Entry", key, NULL);
    return entry->text;
}
static inline bool desktopEntryHidden(DesktopEntryHandle entry) {
    return g_key_file_get_boolean(entry->file, "Desktop Entry", "Hidden", NULL);
}
static inline int desktopEntryParseArguments(DesktopEntryHandle entry, const char *command) {
    g_strfreev(entry->arguments); entry->arguments = NULL; entry->count = 0;
    return g_shell_parse_argv(command, &entry->count, &entry->arguments, NULL) ? entry->count : -1;
}
static inline const char *desktopEntryArgument(DesktopEntryHandle entry, int index) {
    return index >= 0 && index < entry->count ? entry->arguments[index] : NULL;
}
static inline const char *desktopEntryAlias(DesktopEntryHandle entry) {
    g_key_file_set_boolean(entry->file, "Desktop Entry", "NoDisplay", true);
    g_free(entry->text);
    entry->text = g_key_file_to_data(entry->file, NULL, NULL);
    return entry->text;
}
static inline bool desktopEntryPublish(const char *path, const char *text) {
    return g_file_set_contents_full(path, text, -1, G_FILE_SET_CONTENTS_CONSISTENT, 0644, NULL);
}
static inline bool desktopEntryRemove(const char *path) { return unlink(path) == 0; }
#endif
