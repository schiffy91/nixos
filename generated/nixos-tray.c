#include "btrc_stdlib.h"
#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <ctype.h>
#include <math.h>
#include <assert.h>
#include <limits.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <pwd.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <fnmatch.h>
#include <time.h>
#include <btrc_tray.h>

#define _DARWIN_C_SOURCE

typedef struct TraySignal TraySignal;
typedef struct SystemTray SystemTray;
void* btrc_tray_create(char* title);
void btrc_tray_set_icon(void* tray, char* icon_path);
void btrc_tray_set_tooltip(void* tray, char* tooltip);
int btrc_tray_add_item(void* tray, char* label, char* command, bool enabled);
void btrc_tray_add_separator(void* tray);
void btrc_tray_set_menu(void* tray);
bool btrc_tray_show(void* tray);
bool btrc_tray_run_iteration(void* tray, int timeout_ms);
char* btrc_tray_take_command(void* tray);
bool btrc_tray_should_quit(void* tray);
void btrc_tray_request_quit(void* tray);
void btrc_tray_destroy(void* tray);
char* TraySignal_quit(void);
void SystemTray_init(SystemTray* self, char* title);
SystemTray* SystemTray_new(char* title);
SystemTray* SystemTray_tip(SystemTray* self, char* text);
SystemTray* SystemTray_item(SystemTray* self, char* label, char* command);
bool SystemTray_available(SystemTray* self);
bool SystemTray_show(SystemTray* self);
bool SystemTray_pump(SystemTray* self, int timeoutMs);
void SystemTray_run(SystemTray* self);

struct TraySignal {
    int __rc;
};

struct SystemTray {
    int __rc;
    Tray* model;
    void* handle;
    bool realized;
    UnixShell* shell;
};

char* TraySignal_quit(void) {
    return "__quit__";
}

void SystemTray_init(SystemTray* self, char* title) {
    self->__rc = 1;
    if (self->model != NULL) {
        if ((--self->model->__rc) <= 0) {
            Tray_destroy(self->model);
        }
    }
    (self->model = Tray_new(title));
    (Tray_new(title)->__rc++);
    (self->handle = NULL);
    (self->realized = false);
    if (self->shell != NULL) {
        if ((--self->shell->__rc) <= 0) {
            UnixShell_destroy(self->shell);
        }
    }
    (self->shell = UnixShell_new());
    (UnixShell_new()->__rc++);
}

SystemTray* SystemTray_new(char* title) {
    SystemTray* self = ((SystemTray*)malloc(sizeof(SystemTray)));
    memset(self, 0, sizeof(SystemTray));
    SystemTray_init(self, title);
    return self;
}

SystemTray* SystemTray_tip(SystemTray* self, char* text) {
    Tray_tip(self->model, text);
    return self;
}

SystemTray* SystemTray_item(SystemTray* self, char* label, char* command) {
    Tray_item(self->model, label, command);
    return self;
}

bool SystemTray_available(SystemTray* self) {
    return (self->handle != NULL);
}

bool SystemTray_show(SystemTray* self) {
    if (self->realized) {
        return SystemTray_available(self);
    }
    (self->handle = btrc_tray_create(self->model->title));
    if (self->handle == NULL) {
        return false;
    }
    if (!__btrc_isEmpty(self->model->iconPath)) {
        btrc_tray_set_icon(self->handle, self->model->iconPath);
    }
    btrc_tray_set_tooltip(self->handle, self->model->tooltip);
    int __n_65 = btrc_Vector_TrayItem_iterLen(self->model->items);
    for (int __i_64 = 0; (__i_64 < __n_65); (__i_64++)) {
        TrayItem* entry = btrc_Vector_TrayItem_iterGet(self->model->items, __i_64);
        btrc_tray_add_item(self->handle, entry->label, entry->command, entry->enabled);
    }
    btrc_tray_set_menu(self->handle);
    bool ok = btrc_tray_show(self->handle);
    (self->realized = ok);
    return ok;
}

bool SystemTray_pump(SystemTray* self, int timeoutMs) {
    if (self->handle == NULL) {
        return false;
    }
    bool alive = btrc_tray_run_iteration(self->handle, timeoutMs);
    char* command = btrc_tray_take_command(self->handle);
    if (command != NULL) {
        if (strcmp(command, TraySignal_quit()) == 0) {
            btrc_tray_request_quit(self->handle);
            return false;
        }
        UnixShell_runRaw(self->shell, command, false, false, "");
    }
    return (alive && (!btrc_tray_should_quit(self->handle)));
}

void SystemTray_run(SystemTray* self) {
    if (!SystemTray_show(self)) {
        return;
    }
    bool alive = true;
    while (alive) {
        (alive = SystemTray_pump(self, (-1)));
    }
}

int main(void) {
    SystemTray* tray = SystemTray_item(SystemTray_item(SystemTray_item(SystemTray_item(SystemTray_item(SystemTray_item(SystemTray_tip(SystemTray_new("NixOS"), "NixOS management"), "Rebuild (switch)", "konsole -e sudo nixosctl update"), "Upgrade & rebuild", "konsole -e sudo nixosctl update --upgrade"), "Snapshot now", "konsole -e sudo nixosctl snapshot"), "Show changed files", "konsole -e nixosctl diff"), "Open /etc/nixos", "xdg-open /etc/nixos"), "Quit tray", TraySignal_quit());
    if (!SystemTray_show(tray)) {
        printf("%s\n", "nixos-tray: no native tray available (headless / no GUI session).");
        return 0;
    }
    SystemTray_run(tray);
    return 0;
}

