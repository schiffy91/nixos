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

static inline char* __btrc_strdup(const char* s) {
    if (!s) return NULL;
    size_t len = strlen(s) + 1;
    char* copy = (char*)malloc(len);
    if (!copy) { fprintf(stderr, "btrc: out of memory (strdup %zu bytes)\n", len); exit(1); }
    memcpy(copy, s, len);
    return copy;
}

static inline void* __btrc_safe_realloc(void* ptr, size_t size) {
    void* result = realloc(ptr, size);
    if (!result && size > 0) { fprintf(stderr, "btrc: out of memory (realloc %zu bytes)\n", size); exit(1); }
    return result;
}

static inline void* __btrc_safe_calloc(size_t count, size_t size) {
    void* result = calloc(count, size);
    if (!result && count > 0) { fprintf(stderr, "btrc: out of memory (calloc %zu bytes)\n", count * size); exit(1); }
    return result;
}

static inline int __btrc_div_int(int a, int b) {
    if (b == 0) { fprintf(stderr, "Division by zero\n"); exit(1); }
    return a / b;
}

static inline double __btrc_div_double(double a, double b) {
    if (b == 0.0) { fprintf(stderr, "Division by zero\n"); exit(1); }
    return a / b;
}

static inline int __btrc_mod_int(int a, int b) {
    if (b == 0) { fprintf(stderr, "Modulo by zero\n"); exit(1); }
    return a % b;
}

/* btrc string temp pool (dynamic) */
static int __btrc_str_pool_cap = 256;
static char** __btrc_str_pool = NULL;
static int __btrc_str_pool_top = 0;

static inline char* __btrc_str_track(char* s) {
    if (!__btrc_str_pool) {
        __btrc_str_pool = (char**)malloc(sizeof(char*) * __btrc_str_pool_cap);
    }
    if (__btrc_str_pool_top >= __btrc_str_pool_cap) {
        __btrc_str_pool_cap *= 2;
        __btrc_str_pool = (char**)realloc(__btrc_str_pool, sizeof(char*) * __btrc_str_pool_cap);
        if (!__btrc_str_pool) { fprintf(stderr, "btrc: string pool OOM\n"); exit(1); }
    }
    __btrc_str_pool[__btrc_str_pool_top++] = s;
    return s;
}

static inline char* __btrc_substring(const char* s, int start, int len) {
    if (!s) { char* r = (char*)malloc(1); r[0] = '\0'; return r; }
    int slen = (int)strlen(s);
    if (start < 0) start = 0;
    if (start > slen) start = slen;
    if (start + len > slen) len = slen - start;
    if (len < 0) len = 0;
    char* result = (char*)malloc(len + 1);
    strncpy(result, s + start, len);
    result[len] = '\0';
    return result;
}

static inline char* __btrc_trim(const char* s) {
    if (!s) { char* r = (char*)malloc(1); r[0] = '\0'; return r; }
    while (*s && isspace((unsigned char)*s)) s++;
    if (*s == '\0') { char* r = (char*)malloc(1); r[0]='\0'; return r; }
    const char* end = s + strlen(s) - 1;
    while (end > s && isspace((unsigned char)*end)) end--;
    int len = (int)(end - s + 1);
    char* result = (char*)malloc(len + 1);
    strncpy(result, s, len);
    result[len] = '\0';
    return result;
}

static inline char* __btrc_strcat(const char* a, const char* b) {
    if (!a && !b) { char* r = (char*)malloc(1); r[0] = '\0'; return r; }
    if (!a) return __btrc_strdup(b);
    if (!b) return __btrc_strdup(a);
    int la = (int)strlen(a), lb = (int)strlen(b);
    char* r = (char*)malloc(la + lb + 1);
    memcpy(r, a, la);
    memcpy(r + la, b, lb + 1);
    return r;
}

static inline bool __btrc_isEmpty(const char* s) {
    if (!s) return true;
    return s[0] == '\0';
}

static inline bool __btrc_startsWith(const char* s, const char* prefix) {
    if (!s || !prefix) return false;
    return strncmp(s, prefix, strlen(prefix)) == 0;
}

static inline bool __btrc_endsWith(const char* s, const char* suffix) {
    if (!s || !suffix) return false;
    int slen = (int)strlen(s);
    int suflen = (int)strlen(suffix);
    if (suflen > slen) return false;
    return strcmp(s + slen - suflen, suffix) == 0;
}

static inline bool __btrc_strContains(const char* s, const char* sub) {
    if (!s || !sub) return false;
    return strstr(s, sub) != NULL;
}

static inline unsigned int __btrc_hash_str(const char* s) {
    unsigned int h = 5381;
    while (*s) { h = ((h << 5) + h) + (unsigned char)*s++; }
    return h;
}

/* ARC cascade-destroy tracking: avoid reading freed memory */
static int __btrc_tracking = 0;
static void** __btrc_destroyed = NULL;
static int __btrc_destroyed_count = 0;
static int __btrc_destroyed_cap = 0;
static void __btrc_mark_destroyed(void* ptr) {
    if (__btrc_destroyed_count >= __btrc_destroyed_cap) {
        __btrc_destroyed_cap = __btrc_destroyed_cap ? __btrc_destroyed_cap * 2 : 256;
        __btrc_destroyed = (void**)__btrc_safe_realloc(__btrc_destroyed, sizeof(void*) * __btrc_destroyed_cap);
    }
    __btrc_destroyed[__btrc_destroyed_count++] = ptr;
}
static int __btrc_is_destroyed(void* ptr) {
    for (int i = 0; i < __btrc_destroyed_count; i++)
        if (__btrc_destroyed[i] == ptr) return 1;
    return 0;
}

#define _DARWIN_C_SOURCE

typedef struct Strings Strings;
void Strings_destroy(Strings* self);
typedef struct Console Console;
void Console_destroy(Console* self);
typedef struct File File;
void File_destroy(File* self);
typedef struct Path Path;
void Path_destroy(Path* self);
typedef struct UnixPlatform UnixPlatform;
void UnixPlatform_destroy(UnixPlatform* self);
typedef struct Platform Platform;
void Platform_destroy(Platform* self);
typedef struct Environment Environment;
void Environment_destroy(Environment* self);
typedef struct ProcessStatus ProcessStatus;
void ProcessStatus_destroy(ProcessStatus* self);
typedef struct UnixPipe UnixPipe;
void UnixPipe_destroy(UnixPipe* self);
typedef struct UnixProcess UnixProcess;
void UnixProcess_destroy(UnixProcess* self);
typedef struct ShellWords ShellWords;
void ShellWords_destroy(ShellWords* self);
typedef struct ExecResult ExecResult;
void ExecResult_destroy(ExecResult* self);
typedef struct Command Command;
void Command_destroy(Command* self);
typedef struct UnixShell UnixShell;
void UnixShell_destroy(UnixShell* self);
typedef struct PowerShell PowerShell;
void PowerShell_destroy(PowerShell* self);
typedef struct UnixPamPassword UnixPamPassword;
void UnixPamPassword_destroy(UnixPamPassword* self);
typedef struct FileStatus FileStatus;
void FileStatus_destroy(FileStatus* self);
typedef struct Directory Directory;
void Directory_destroy(Directory* self);
typedef struct UnixFileSystem UnixFileSystem;
void UnixFileSystem_destroy(UnixFileSystem* self);
typedef struct PathTools PathTools;
void PathTools_destroy(PathTools* self);
typedef struct FileSystem FileSystem;
void FileSystem_destroy(FileSystem* self);
typedef struct DaemonSpec DaemonSpec;
void DaemonSpec_destroy(DaemonSpec* self);
typedef struct DaemonController DaemonController;
void DaemonController_destroy(DaemonController* self);
typedef struct AppSpec AppSpec;
void AppSpec_destroy(AppSpec* self);
typedef struct DaemonApp DaemonApp;
void DaemonApp_destroy(DaemonApp* self);
typedef struct Html Html;
void Html_destroy(Html* self);
typedef struct UiNode UiNode;
void UiNode_destroy(UiNode* self);
typedef struct UiDocument UiDocument;
void UiDocument_destroy(UiDocument* self);
typedef struct HtmlView HtmlView;
void HtmlView_destroy(HtmlView* self);
typedef struct NativeView NativeView;
void NativeView_destroy(NativeView* self);
typedef struct Window Window;
void Window_destroy(Window* self);
typedef struct TrayItem TrayItem;
void TrayItem_destroy(TrayItem* self);
typedef struct Tray Tray;
void Tray_destroy(Tray* self);
typedef struct HtmlUiBackend HtmlUiBackend;
void HtmlUiBackend_destroy(HtmlUiBackend* self);
typedef struct NativeUiBackend NativeUiBackend;
void NativeUiBackend_destroy(NativeUiBackend* self);
typedef struct LinuxUiBuilder LinuxUiBuilder;
void LinuxUiBuilder_destroy(LinuxUiBuilder* self);
typedef struct MacUiBuilder MacUiBuilder;
void MacUiBuilder_destroy(MacUiBuilder* self);
typedef struct WindowsUiBuilder WindowsUiBuilder;
void WindowsUiBuilder_destroy(WindowsUiBuilder* self);
typedef struct Ui Ui;
void Ui_destroy(Ui* self);
typedef struct NativeUi NativeUi;
void NativeUi_destroy(NativeUi* self);
typedef struct UiRuntime UiRuntime;
void UiRuntime_destroy(UiRuntime* self);
typedef struct Signal Signal;
void Signal_destroy(Signal* self);
typedef struct JsonObject JsonObject;
void JsonObject_destroy(JsonObject* self);
typedef struct Toml Toml;
void Toml_destroy(Toml* self);
typedef struct UnixPattern UnixPattern;
void UnixPattern_destroy(UnixPattern* self);
typedef struct Pattern Pattern;
void Pattern_destroy(Pattern* self);
typedef struct Math Math;
void Math_destroy(Math* self);
typedef struct DateTime DateTime;
void DateTime_destroy(DateTime* self);
typedef struct Timer Timer;
void Timer_destroy(Timer* self);
typedef struct Random Random;
void Random_destroy(Random* self);
typedef struct Error Error;
void Error_destroy(Error* self);
typedef struct ValueError ValueError;
void ValueError_destroy(ValueError* self);
typedef struct IOError IOError;
void IOError_destroy(IOError* self);
typedef struct TypeError TypeError;
void TypeError_destroy(TypeError* self);
typedef struct IndexError IndexError;
void IndexError_destroy(IndexError* self);
typedef struct KeyError KeyError;
void KeyError_destroy(KeyError* self);
typedef struct CliArgs CliArgs;
void CliArgs_destroy(CliArgs* self);
typedef struct CliCommand CliCommand;
void CliCommand_destroy(CliCommand* self);
typedef struct TraySignal TraySignal;
void TraySignal_destroy(TraySignal* self);
typedef struct SystemTray SystemTray;
void SystemTray_destroy(SystemTray* self);
typedef struct btrc_Vector_string btrc_Vector_string;
typedef struct btrc_Vector_UiNode btrc_Vector_UiNode;
typedef struct btrc_Vector_TrayItem btrc_Vector_TrayItem;
typedef struct btrc_Vector_bool btrc_Vector_bool;
typedef struct btrc_Vector_int btrc_Vector_int;
typedef struct btrc_Vector_float btrc_Vector_float;
typedef struct btrc_Map_string_string btrc_Map_string_string;
typedef struct btrc_Map_string_bool btrc_Map_string_bool;
void Strings_init(Strings* self);
char* Strings_copy(char* s);
char* Strings_replace(char* s, char* old, char* replacement);
bool Strings_isDigit(char c);
bool Strings_isAlpha(char c);
int Strings_toInt(char* s);
int Strings_find(char* s, char* sub, int start);
int Strings_compare(char* left, char* right);
char* Strings_fromInt(int n);
void Console_init(Console* self);
void File_init(File* self, char* path, char* mode);
File* File_new(char* path, char* mode);
bool File_ok(File* self);
char* File_read(File* self);
void File_write(File* self, char* text);
void File_close(File* self);
void Path_init(Path* self);
char* Path_readAll(char* path);
void Path_writeAll(char* path, char* content);
void UnixPlatform_init(UnixPlatform* self);
int UnixPlatform_pid(void);
int UnixPlatform_euid(void);
void Platform_init(Platform* self);
int Platform_pid(void);
int Platform_euid(void);
void Environment_init(Environment* self);
char* Environment_get(char* name, char* fallback);
FILE* popen(const char* command, const char* mode);
int pclose(FILE* stream);
void ProcessStatus_init(ProcessStatus* self, int raw);
ProcessStatus* ProcessStatus_new(int raw);
int ProcessStatus_code(ProcessStatus* self);
void UnixPipe_init(UnixPipe* self, char* command);
UnixPipe* UnixPipe_new(char* command);
bool UnixPipe_ok(UnixPipe* self);
char* UnixPipe_readAll(UnixPipe* self);
ProcessStatus* UnixPipe_close(UnixPipe* self);
void UnixProcess_init(UnixProcess* self);
ProcessStatus* UnixProcess_system(char* command);
UnixPipe* UnixProcess_pipe(char* command);
void ShellWords_init(ShellWords* self);
bool ShellWords_isSafeArgChar(char c);
bool ShellWords_isSafeArg(char* raw);
char* ShellWords_quote(char* raw);
char* ShellWords_redact(char* text, char* sensitive);
void ExecResult_init(ExecResult* self, int code, char* out, char* err, char* command);
ExecResult* ExecResult_new(int code, char* out, char* err, char* command);
char* ExecResult_stdout(ExecResult* self);
void Command_init(Command* self, char* executable);
Command* Command_new(char* executable);
Command* Command_arg(Command* self, char* value);
Command* Command_capture(Command* self, bool enabled);
Command* Command_check(Command* self, bool enabled);
char* Command_renderEnv(Command* self, char* item);
char* Command_render(Command* self);
void UnixShell_init(UnixShell* self);
UnixShell* UnixShell_new(void);
char* UnixShell_quote(char* raw);
char* UnixShell_redactText(char* text, char* sensitive);
ExecResult* UnixShell_run(UnixShell* self, char* command);
ExecResult* UnixShell_runUnchecked(UnixShell* self, char* command);
ExecResult* UnixShell_runCommand(UnixShell* self, Command* command);
ExecResult* UnixShell_runRaw(UnixShell* self, char* command, bool captureOutput, bool checkStatus, char* sensitive);
void PowerShell_init(PowerShell* self);
int forkpty(int* amaster, char* name, void* termp, void* winp);
void UnixPamPassword_init(UnixPamPassword* self);
char* mkdtemp(char* templatePath);
void FileStatus_init(FileStatus* self, char* path);
FileStatus* FileStatus_new(char* path);
bool FileStatus_exists(FileStatus* self);
bool FileStatus_isDir(FileStatus* self);
bool FileStatus_isFile(FileStatus* self);
bool FileStatus_isSymlink(FileStatus* self);
void Directory_init(Directory* self, char* path);
Directory* Directory_new(char* path);
btrc_Vector_string* Directory_entries(Directory* self);
void UnixFileSystem_init(UnixFileSystem* self);
int UnixFileSystem_statusCode(int raw);
int UnixFileSystem_chmodPath(char* path, int mode);
int UnixFileSystem_mkdirPath(char* path, int mode);
int UnixFileSystem_runShell(char* command);
int UnixFileSystem_mkdirp(char* path);
int UnixFileSystem_removeRecursive(char* path);
int UnixFileSystem_symlinkPath(char* target, char* linkPath);
char* UnixFileSystem_readLink(char* path);
char* UnixFileSystem_tempDir(char* prefix);
void PathTools_init(PathTools* self);
char* PathTools_shellQuote(char* raw);
char* PathTools_join(char* left, char* right);
void FileSystem_init(FileSystem* self);
int FileSystem_chmod(char* path, int mode);
int FileSystem_mkdir(char* path, int mode);
int FileSystem_mkdirp(char* path);
int FileSystem_removeRecursive(char* path);
int FileSystem_symlink(char* target, char* linkPath);
char* FileSystem_readLink(char* path);
char* FileSystem_tempDir(char* prefix);
void FileSystem_writeText(char* path, char* content);
void DaemonSpec_init(DaemonSpec* self, char* name, Command* command);
char* DaemonSpec_renderStartCommand(DaemonSpec* self);
void DaemonController_init(DaemonController* self);
void AppSpec_init(AppSpec* self, char* name);
AppSpec* AppSpec_withVersion(AppSpec* self, char* version);
void DaemonApp_init(DaemonApp* self, char* name, DaemonSpec* daemon);
void Html_init(Html* self);
char* Html_escape(char* raw);
void UiNode_init(UiNode* self, char* tag);
UiNode* UiNode_new(char* tag);
UiNode* UiNode_text(UiNode* self, char* value);
UiNode* UiNode_raw(UiNode* self, char* value);
UiNode* UiNode_attr(UiNode* self, char* name, char* value);
char* UiNode_renderAttributes(UiNode* self);
bool UiNode_isVoidElement(UiNode* self);
char* UiNode_renderHtml(UiNode* self);
void UiDocument_init(UiDocument* self, char* title, UiNode* body);
UiDocument* UiDocument_new(char* title, UiNode* body);
char* UiDocument_renderHtml(UiDocument* self);
void UiDocument_writeHtml(UiDocument* self, char* path);
void HtmlView_init(HtmlView* self, UiDocument* document);
void HtmlView_write(HtmlView* self, char* path);
void NativeView_init(NativeView* self, UiNode* root);
void Window_init(Window* self, char* title, int width, int height, HtmlView* html);
void TrayItem_init(TrayItem* self, char* label, char* command);
TrayItem* TrayItem_new(char* label, char* command);
void Tray_init(Tray* self, char* title);
Tray* Tray_new(char* title);
Tray* Tray_icon(Tray* self, char* path);
Tray* Tray_tip(Tray* self, char* text);
Tray* Tray_item(Tray* self, char* label, char* command);
Tray* Tray_add(Tray* self, TrayItem* item);
void HtmlUiBackend_init(HtmlUiBackend* self, char* opener);
HtmlUiBackend* HtmlUiBackend_new(char* opener);
Command* HtmlUiBackend_openCommand(HtmlUiBackend* self, char* path);
ExecResult* HtmlUiBackend_openFile(HtmlUiBackend* self, char* path);
ExecResult* HtmlUiBackend_openWindow(HtmlUiBackend* self, Window* window, char* path);
void NativeUiBackend_init(NativeUiBackend* self, char* name, HtmlUiBackend* htmlBackend);
NativeUiBackend* NativeUiBackend_new(char* name, HtmlUiBackend* htmlBackend);
bool NativeUiBackend_isMac(NativeUiBackend* self);
bool NativeUiBackend_isLinux(NativeUiBackend* self);
Command* NativeUiBackend_notifyCommand(NativeUiBackend* self, char* title, char* body);
Command* NativeUiBackend_alertCommand(NativeUiBackend* self, char* title, char* body);
ExecResult* NativeUiBackend_notify(NativeUiBackend* self, char* title, char* body);
ExecResult* NativeUiBackend_alert(NativeUiBackend* self, char* title, char* body);
void LinuxUiBuilder_init(LinuxUiBuilder* self);
HtmlUiBackend* LinuxUiBuilder_html(void);
NativeUiBackend* LinuxUiBuilder_native(void);
void MacUiBuilder_init(MacUiBuilder* self);
HtmlUiBackend* MacUiBuilder_html(void);
NativeUiBackend* MacUiBuilder_native(void);
void WindowsUiBuilder_init(WindowsUiBuilder* self);
HtmlUiBackend* WindowsUiBuilder_html(void);
NativeUiBackend* WindowsUiBuilder_native(void);
void Ui_init(Ui* self);
void NativeUi_init(NativeUi* self);
char* NativeUi_applescriptString(char* raw);
void UiRuntime_init(UiRuntime* self);
typedef struct __btrc_spawn_env_1 __btrc_spawn_env_1;
typedef struct __btrc_spawn_env_2 __btrc_spawn_env_2;
void Signal_init(Signal* self);
void JsonObject_init(JsonObject* self);
JsonObject* JsonObject_new(void);
char* JsonObject_escape(char* text);
char* JsonObject_unescape(char* text);
void JsonObject_setString(JsonObject* self, char* key, char* value);
void JsonObject_setRaw(JsonObject* self, char* key, char* value);
char* JsonObject_stringify(JsonObject* self);
int JsonObject_skipSpaces(char* text, int i);
char* JsonObject_slice(char* text, int start, int end);
int JsonObject_stringEnd(char* text, int start);
JsonObject* JsonObject_parse(char* text);
void Toml_init(Toml* self);
char* Toml_stripInlineComment(char* raw);
char* Toml_unquote(char* raw);
void UnixPattern_init(UnixPattern* self);
bool UnixPattern_matches(char* pattern, char* text);
void Pattern_init(Pattern* self);
bool Pattern_matches(char* pattern, char* text);
void Math_init(Math* self);
int Math_abs(int x);
int Math_factorial(int n);
int Math_gcd(int a, int b);
void DateTime_init(DateTime* self, int year, int month, int day, int hour, int minute, int second);
DateTime* DateTime_new(int year, int month, int day, int hour, int minute, int second);
void Timer_init(Timer* self);
void Random_init(Random* self);
void Random_seed(Random* self, int s);
void Random_seedTime(Random* self);
int Random_randint(Random* self, int lo, int hi);
float Random_random(Random* self);
void Error_init(Error* self, char* message, int code);
char* Error_toString(Error* self);
void ValueError_init(ValueError* self, char* message);
void IOError_init(IOError* self, char* message);
void TypeError_init(TypeError* self, char* message);
void IndexError_init(IndexError* self, char* message);
void KeyError_init(KeyError* self, char* message);
void CliArgs_init(CliArgs* self, int argc, char** argv);
char* CliArgs_command(CliArgs* self);
void CliCommand_init(CliCommand* self, char* name);
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
void TraySignal_init(TraySignal* self);
char* TraySignal_quit(void);
void SystemTray_init(SystemTray* self, char* title);
SystemTray* SystemTray_new(char* title);
SystemTray* SystemTray_tip(SystemTray* self, char* text);
SystemTray* SystemTray_item(SystemTray* self, char* label, char* command);
bool SystemTray_available(SystemTray* self);
bool SystemTray_show(SystemTray* self);
bool SystemTray_pump(SystemTray* self, int timeoutMs);
void SystemTray_run(SystemTray* self);
typedef bool (*__btrc_fn_bool_string)(char*);
typedef void (*__btrc_fn_void_string)(char*);
typedef char* (*__btrc_fn_string_string)(char*);
typedef char* (*__btrc_fn_string_string_string)(char*, char*);
typedef bool (*__btrc_fn_bool_UiNode)(UiNode*);
typedef void (*__btrc_fn_void_UiNode)(UiNode*);
typedef UiNode* (*__btrc_fn_UiNode_UiNode)(UiNode*);
typedef UiNode* (*__btrc_fn_UiNode_UiNode_UiNode)(UiNode*, UiNode*);
typedef bool (*__btrc_fn_bool_TrayItem)(TrayItem*);
typedef void (*__btrc_fn_void_TrayItem)(TrayItem*);
typedef TrayItem* (*__btrc_fn_TrayItem_TrayItem)(TrayItem*);
typedef TrayItem* (*__btrc_fn_TrayItem_TrayItem_TrayItem)(TrayItem*, TrayItem*);
typedef bool (*__btrc_fn_bool_bool)(bool);
typedef void (*__btrc_fn_void_bool)(bool);
typedef bool (*__btrc_fn_bool_bool_bool)(bool, bool);
typedef bool (*__btrc_fn_bool_int)(int);
typedef void (*__btrc_fn_void_int)(int);
typedef int (*__btrc_fn_int_int)(int);
typedef int (*__btrc_fn_int_int_int)(int, int);
typedef bool (*__btrc_fn_bool_float)(float);
typedef void (*__btrc_fn_void_float)(float);
typedef float (*__btrc_fn_float_float)(float);
typedef float (*__btrc_fn_float_float_float)(float, float);

struct btrc_Vector_string {
    int __rc;
    char** data;
    int len;
    int cap;
};

struct btrc_Vector_UiNode {
    int __rc;
    UiNode** data;
    int len;
    int cap;
};

struct btrc_Vector_TrayItem {
    int __rc;
    TrayItem** data;
    int len;
    int cap;
};

struct btrc_Vector_bool {
    int __rc;
    bool* data;
    int len;
    int cap;
};

struct btrc_Vector_int {
    int __rc;
    int* data;
    int len;
    int cap;
};

struct btrc_Vector_float {
    int __rc;
    float* data;
    int len;
    int cap;
};

struct btrc_Map_string_string {
    int __rc;
    char** keys;
    char** values;
    bool* occupied;
    int len;
    int cap;
};

struct btrc_Map_string_bool {
    int __rc;
    char** keys;
    bool* values;
    bool* occupied;
    int len;
    int cap;
};

struct Strings {
    int __rc;
};

struct Console {
    int __rc;
};

struct File {
    int __rc;
    FILE* handle;
    char* path;
    char* mode;
    bool is_open;
};

struct Path {
    int __rc;
};

struct UnixPlatform {
    int __rc;
};

struct Platform {
    int __rc;
};

struct Environment {
    int __rc;
};

struct ProcessStatus {
    int __rc;
    int raw;
};

struct UnixPipe {
    int __rc;
    FILE* handle;
    char* command;
};

struct UnixProcess {
    int __rc;
};

struct ShellWords {
    int __rc;
};

struct ExecResult {
    int __rc;
    int code;
    char* out;
    char* err;
    char* command;
};

struct Command {
    int __rc;
    char* executable;
    btrc_Vector_string* args;
    btrc_Vector_string* env;
    bool useSudo;
    bool captureOutput;
    bool checkStatus;
    bool mergeStderr;
    char* sensitive;
};

struct UnixShell {
    int __rc;
    bool logCommands;
    char* chrootPath;
};

struct PowerShell {
    int __rc;
};

struct UnixPamPassword {
    int __rc;
};

struct FileStatus {
    int __rc;
    char* path;
    int mode;
    int linkMode;
    bool found;
    bool linkFound;
};

struct Directory {
    int __rc;
    char* path;
};

struct UnixFileSystem {
    int __rc;
};

struct PathTools {
    int __rc;
};

struct FileSystem {
    int __rc;
};

struct DaemonSpec {
    int __rc;
    char* name;
    Command* command;
    char* pidFile;
    char* logFile;
    char* workingDirectory;
    bool autoRestart;
};

struct DaemonController {
    int __rc;
    UnixShell* shell;
};

struct AppSpec {
    int __rc;
    char* name;
    char* version;
};

struct DaemonApp {
    int __rc;
    char* name;
    char* version;
    DaemonSpec* daemon;
};

struct Html {
    int __rc;
};

struct UiNode {
    int __rc;
    char* tag;
    char* textContent;
    bool rawText;
    btrc_Vector_string* attributes;
    btrc_Vector_UiNode* children;
};

struct UiDocument {
    int __rc;
    char* title;
    char* css;
    UiNode* body;
};

struct HtmlView {
    int __rc;
    UiDocument* document;
};

struct NativeView {
    int __rc;
    UiNode* root;
};

struct Window {
    int __rc;
    char* title;
    int width;
    int height;
    HtmlView* html;
};

struct TrayItem {
    int __rc;
    char* label;
    char* command;
    bool enabled;
};

struct Tray {
    int __rc;
    char* title;
    char* tooltip;
    char* iconPath;
    btrc_Vector_TrayItem* items;
};

struct HtmlUiBackend {
    int __rc;
    char* opener;
};

struct NativeUiBackend {
    int __rc;
    char* name;
    HtmlUiBackend* htmlBackend;
};

struct LinuxUiBuilder {
    int __rc;
};

struct MacUiBuilder {
    int __rc;
};

struct WindowsUiBuilder {
    int __rc;
};

struct Ui {
    int __rc;
};

struct NativeUi {
    int __rc;
};

struct UiRuntime {
    int __rc;
};

struct __btrc_spawn_env_1 {
    Command* command;
};

struct __btrc_spawn_env_2 {
    NativeUiBackend* backend;
    char* body;
    char* title;
};

struct Signal {
    int __rc;
    btrc_Vector_string* events;
};

struct JsonObject {
    int __rc;
    btrc_Map_string_string* values;
    btrc_Map_string_bool* quoted;
};

struct Toml {
    int __rc;
};

struct UnixPattern {
    int __rc;
};

struct Pattern {
    int __rc;
};

struct Math {
    int __rc;
};

struct DateTime {
    int __rc;
    int year;
    int month;
    int day;
    int hour;
    int minute;
    int second;
};

struct Timer {
    int __rc;
    clock_t start_time;
    clock_t end_time;
    bool running;
};

struct Random {
    int __rc;
    bool seeded;
};

struct Error {
    int __rc;
    char* message;
    int code;
};

struct ValueError {
    int __rc;
    char* message;
    int code;
};

struct IOError {
    int __rc;
    char* message;
    int code;
};

struct TypeError {
    int __rc;
    char* message;
    int code;
};

struct IndexError {
    int __rc;
    char* message;
    int code;
};

struct KeyError {
    int __rc;
    char* message;
    int code;
};

struct CliArgs {
    int __rc;
    char* program;
    btrc_Vector_string* values;
};

struct CliCommand {
    int __rc;
    char* name;
    btrc_Vector_string* aliases;
};

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

/* Type-dependent comparison/hashing macros for generic collections.
 * Uses __builtin_choose_expr — unselected branch is NOT evaluated.
 * Cast chain (void*)(intptr_t) avoids float-to-pointer hard errors. */
#define __btrc_eq(a, b) __builtin_choose_expr( \
    __builtin_types_compatible_p(__typeof__(a), char*), \
    strcmp((const char*)(void*)(intptr_t)(a), (const char*)(void*)(intptr_t)(b)) == 0, \
    (a) == (b))
#define __btrc_lt(a, b) __builtin_choose_expr( \
    __builtin_types_compatible_p(__typeof__(a), char*), \
    strcmp((const char*)(void*)(intptr_t)(a), (const char*)(void*)(intptr_t)(b)) < 0, \
    (a) < (b))
#define __btrc_gt(a, b) __builtin_choose_expr( \
    __builtin_types_compatible_p(__typeof__(a), char*), \
    strcmp((const char*)(void*)(intptr_t)(a), (const char*)(void*)(intptr_t)(b)) > 0, \
    (a) > (b))
#define __btrc_hash(k) __builtin_choose_expr( \
    __builtin_types_compatible_p(__typeof__(k), char*), \
    __btrc_hash_str((const char*)(void*)(intptr_t)(k)), \
    (unsigned int)(intptr_t)(k))

static void btrc_Vector_string_init(btrc_Vector_string* self);
static btrc_Vector_string* btrc_Vector_string_new(void);
static void btrc_Vector_string_destroy(btrc_Vector_string* self);
static void btrc_Vector_string_push(btrc_Vector_string* self, char* val);
static char* btrc_Vector_string_pop(btrc_Vector_string* self);
static char* btrc_Vector_string_get(btrc_Vector_string* self, int i);
static void btrc_Vector_string_set(btrc_Vector_string* self, int i, char* val);
static void btrc_Vector_string_free(btrc_Vector_string* self);
static void btrc_Vector_string_remove(btrc_Vector_string* self, int idx);
static void btrc_Vector_string_reverse(btrc_Vector_string* self);
static btrc_Vector_string* btrc_Vector_string_reversed(btrc_Vector_string* self);
static void btrc_Vector_string_swap(btrc_Vector_string* self, int i, int j);
static void btrc_Vector_string_clear(btrc_Vector_string* self);
static void btrc_Vector_string_fill(btrc_Vector_string* self, char* val);
static int btrc_Vector_string_size(btrc_Vector_string* self);
static bool btrc_Vector_string_isEmpty(btrc_Vector_string* self);
static char* btrc_Vector_string_first(btrc_Vector_string* self);
static char* btrc_Vector_string_last(btrc_Vector_string* self);
static btrc_Vector_string* btrc_Vector_string_slice(btrc_Vector_string* self, int start, int end);
static btrc_Vector_string* btrc_Vector_string_take(btrc_Vector_string* self, int n);
static btrc_Vector_string* btrc_Vector_string_drop(btrc_Vector_string* self, int n);
static void btrc_Vector_string_extend(btrc_Vector_string* self, btrc_Vector_string* other);
static void btrc_Vector_string_insert(btrc_Vector_string* self, int idx, char* val);
static bool btrc_Vector_string_contains(btrc_Vector_string* self, char* val);
static int btrc_Vector_string_indexOf(btrc_Vector_string* self, char* val);
static int btrc_Vector_string_lastIndexOf(btrc_Vector_string* self, char* val);
static int btrc_Vector_string_count(btrc_Vector_string* self, char* val);
static void btrc_Vector_string_removeAll(btrc_Vector_string* self, char* val);
static btrc_Vector_string* btrc_Vector_string_distinct(btrc_Vector_string* self);
static void btrc_Vector_string_sort(btrc_Vector_string* self);
static btrc_Vector_string* btrc_Vector_string_sorted(btrc_Vector_string* self);
static char* btrc_Vector_string_min(btrc_Vector_string* self);
static char* btrc_Vector_string_max(btrc_Vector_string* self);
static char* btrc_Vector_string_sum(btrc_Vector_string* self);
static char* btrc_Vector_string_join(btrc_Vector_string* self, char* sep);
static char* btrc_Vector_string_joinToString(btrc_Vector_string* self, char* sep);
static btrc_Vector_string* btrc_Vector_string_filter(btrc_Vector_string* self, __btrc_fn_bool_string pred);
static int btrc_Vector_string_findIndex(btrc_Vector_string* self, __btrc_fn_bool_string pred);
static void btrc_Vector_string_forEach(btrc_Vector_string* self, __btrc_fn_void_string fn);
static btrc_Vector_string* btrc_Vector_string_map(btrc_Vector_string* self, __btrc_fn_string_string fn);
static bool btrc_Vector_string_any(btrc_Vector_string* self, __btrc_fn_bool_string pred);
static bool btrc_Vector_string_all(btrc_Vector_string* self, __btrc_fn_bool_string pred);
static char* btrc_Vector_string_reduce(btrc_Vector_string* self, char* init, __btrc_fn_string_string_string fn);
static btrc_Vector_string* btrc_Vector_string_copy(btrc_Vector_string* self);
static void btrc_Vector_string_removeAt(btrc_Vector_string* self, int idx);
static int btrc_Vector_string_iterLen(btrc_Vector_string* self);
static char* btrc_Vector_string_iterGet(btrc_Vector_string* self, int i);

static void btrc_Vector_UiNode_init(btrc_Vector_UiNode* self);
static btrc_Vector_UiNode* btrc_Vector_UiNode_new(void);
static void btrc_Vector_UiNode_destroy(btrc_Vector_UiNode* self);
static void btrc_Vector_UiNode_push(btrc_Vector_UiNode* self, UiNode* val);
static UiNode* btrc_Vector_UiNode_pop(btrc_Vector_UiNode* self);
static UiNode* btrc_Vector_UiNode_get(btrc_Vector_UiNode* self, int i);
static void btrc_Vector_UiNode_set(btrc_Vector_UiNode* self, int i, UiNode* val);
static void btrc_Vector_UiNode_free(btrc_Vector_UiNode* self);
static void btrc_Vector_UiNode_remove(btrc_Vector_UiNode* self, int idx);
static void btrc_Vector_UiNode_reverse(btrc_Vector_UiNode* self);
static btrc_Vector_UiNode* btrc_Vector_UiNode_reversed(btrc_Vector_UiNode* self);
static void btrc_Vector_UiNode_swap(btrc_Vector_UiNode* self, int i, int j);
static void btrc_Vector_UiNode_clear(btrc_Vector_UiNode* self);
static void btrc_Vector_UiNode_fill(btrc_Vector_UiNode* self, UiNode* val);
static int btrc_Vector_UiNode_size(btrc_Vector_UiNode* self);
static bool btrc_Vector_UiNode_isEmpty(btrc_Vector_UiNode* self);
static UiNode* btrc_Vector_UiNode_first(btrc_Vector_UiNode* self);
static UiNode* btrc_Vector_UiNode_last(btrc_Vector_UiNode* self);
static btrc_Vector_UiNode* btrc_Vector_UiNode_slice(btrc_Vector_UiNode* self, int start, int end);
static btrc_Vector_UiNode* btrc_Vector_UiNode_take(btrc_Vector_UiNode* self, int n);
static btrc_Vector_UiNode* btrc_Vector_UiNode_drop(btrc_Vector_UiNode* self, int n);
static void btrc_Vector_UiNode_extend(btrc_Vector_UiNode* self, btrc_Vector_UiNode* other);
static void btrc_Vector_UiNode_insert(btrc_Vector_UiNode* self, int idx, UiNode* val);
static bool btrc_Vector_UiNode_contains(btrc_Vector_UiNode* self, UiNode* val);
static int btrc_Vector_UiNode_indexOf(btrc_Vector_UiNode* self, UiNode* val);
static int btrc_Vector_UiNode_lastIndexOf(btrc_Vector_UiNode* self, UiNode* val);
static int btrc_Vector_UiNode_count(btrc_Vector_UiNode* self, UiNode* val);
static void btrc_Vector_UiNode_removeAll(btrc_Vector_UiNode* self, UiNode* val);
static btrc_Vector_UiNode* btrc_Vector_UiNode_distinct(btrc_Vector_UiNode* self);
static void btrc_Vector_UiNode_sort(btrc_Vector_UiNode* self);
static btrc_Vector_UiNode* btrc_Vector_UiNode_sorted(btrc_Vector_UiNode* self);
static UiNode* btrc_Vector_UiNode_min(btrc_Vector_UiNode* self);
static UiNode* btrc_Vector_UiNode_max(btrc_Vector_UiNode* self);
static UiNode* btrc_Vector_UiNode_sum(btrc_Vector_UiNode* self);
static char* btrc_Vector_UiNode_join(btrc_Vector_UiNode* self, char* sep);
static char* btrc_Vector_UiNode_joinToString(btrc_Vector_UiNode* self, char* sep);
static btrc_Vector_UiNode* btrc_Vector_UiNode_filter(btrc_Vector_UiNode* self, __btrc_fn_bool_UiNode pred);
static int btrc_Vector_UiNode_findIndex(btrc_Vector_UiNode* self, __btrc_fn_bool_UiNode pred);
static void btrc_Vector_UiNode_forEach(btrc_Vector_UiNode* self, __btrc_fn_void_UiNode fn);
static btrc_Vector_UiNode* btrc_Vector_UiNode_map(btrc_Vector_UiNode* self, __btrc_fn_UiNode_UiNode fn);
static bool btrc_Vector_UiNode_any(btrc_Vector_UiNode* self, __btrc_fn_bool_UiNode pred);
static bool btrc_Vector_UiNode_all(btrc_Vector_UiNode* self, __btrc_fn_bool_UiNode pred);
static UiNode* btrc_Vector_UiNode_reduce(btrc_Vector_UiNode* self, UiNode* init, __btrc_fn_UiNode_UiNode_UiNode fn);
static btrc_Vector_UiNode* btrc_Vector_UiNode_copy(btrc_Vector_UiNode* self);
static void btrc_Vector_UiNode_removeAt(btrc_Vector_UiNode* self, int idx);
static int btrc_Vector_UiNode_iterLen(btrc_Vector_UiNode* self);
static UiNode* btrc_Vector_UiNode_iterGet(btrc_Vector_UiNode* self, int i);

static void btrc_Vector_TrayItem_init(btrc_Vector_TrayItem* self);
static btrc_Vector_TrayItem* btrc_Vector_TrayItem_new(void);
static void btrc_Vector_TrayItem_destroy(btrc_Vector_TrayItem* self);
static void btrc_Vector_TrayItem_push(btrc_Vector_TrayItem* self, TrayItem* val);
static TrayItem* btrc_Vector_TrayItem_pop(btrc_Vector_TrayItem* self);
static TrayItem* btrc_Vector_TrayItem_get(btrc_Vector_TrayItem* self, int i);
static void btrc_Vector_TrayItem_set(btrc_Vector_TrayItem* self, int i, TrayItem* val);
static void btrc_Vector_TrayItem_free(btrc_Vector_TrayItem* self);
static void btrc_Vector_TrayItem_remove(btrc_Vector_TrayItem* self, int idx);
static void btrc_Vector_TrayItem_reverse(btrc_Vector_TrayItem* self);
static btrc_Vector_TrayItem* btrc_Vector_TrayItem_reversed(btrc_Vector_TrayItem* self);
static void btrc_Vector_TrayItem_swap(btrc_Vector_TrayItem* self, int i, int j);
static void btrc_Vector_TrayItem_clear(btrc_Vector_TrayItem* self);
static void btrc_Vector_TrayItem_fill(btrc_Vector_TrayItem* self, TrayItem* val);
static int btrc_Vector_TrayItem_size(btrc_Vector_TrayItem* self);
static bool btrc_Vector_TrayItem_isEmpty(btrc_Vector_TrayItem* self);
static TrayItem* btrc_Vector_TrayItem_first(btrc_Vector_TrayItem* self);
static TrayItem* btrc_Vector_TrayItem_last(btrc_Vector_TrayItem* self);
static btrc_Vector_TrayItem* btrc_Vector_TrayItem_slice(btrc_Vector_TrayItem* self, int start, int end);
static btrc_Vector_TrayItem* btrc_Vector_TrayItem_take(btrc_Vector_TrayItem* self, int n);
static btrc_Vector_TrayItem* btrc_Vector_TrayItem_drop(btrc_Vector_TrayItem* self, int n);
static void btrc_Vector_TrayItem_extend(btrc_Vector_TrayItem* self, btrc_Vector_TrayItem* other);
static void btrc_Vector_TrayItem_insert(btrc_Vector_TrayItem* self, int idx, TrayItem* val);
static bool btrc_Vector_TrayItem_contains(btrc_Vector_TrayItem* self, TrayItem* val);
static int btrc_Vector_TrayItem_indexOf(btrc_Vector_TrayItem* self, TrayItem* val);
static int btrc_Vector_TrayItem_lastIndexOf(btrc_Vector_TrayItem* self, TrayItem* val);
static int btrc_Vector_TrayItem_count(btrc_Vector_TrayItem* self, TrayItem* val);
static void btrc_Vector_TrayItem_removeAll(btrc_Vector_TrayItem* self, TrayItem* val);
static btrc_Vector_TrayItem* btrc_Vector_TrayItem_distinct(btrc_Vector_TrayItem* self);
static void btrc_Vector_TrayItem_sort(btrc_Vector_TrayItem* self);
static btrc_Vector_TrayItem* btrc_Vector_TrayItem_sorted(btrc_Vector_TrayItem* self);
static TrayItem* btrc_Vector_TrayItem_min(btrc_Vector_TrayItem* self);
static TrayItem* btrc_Vector_TrayItem_max(btrc_Vector_TrayItem* self);
static TrayItem* btrc_Vector_TrayItem_sum(btrc_Vector_TrayItem* self);
static char* btrc_Vector_TrayItem_join(btrc_Vector_TrayItem* self, char* sep);
static char* btrc_Vector_TrayItem_joinToString(btrc_Vector_TrayItem* self, char* sep);
static btrc_Vector_TrayItem* btrc_Vector_TrayItem_filter(btrc_Vector_TrayItem* self, __btrc_fn_bool_TrayItem pred);
static int btrc_Vector_TrayItem_findIndex(btrc_Vector_TrayItem* self, __btrc_fn_bool_TrayItem pred);
static void btrc_Vector_TrayItem_forEach(btrc_Vector_TrayItem* self, __btrc_fn_void_TrayItem fn);
static btrc_Vector_TrayItem* btrc_Vector_TrayItem_map(btrc_Vector_TrayItem* self, __btrc_fn_TrayItem_TrayItem fn);
static bool btrc_Vector_TrayItem_any(btrc_Vector_TrayItem* self, __btrc_fn_bool_TrayItem pred);
static bool btrc_Vector_TrayItem_all(btrc_Vector_TrayItem* self, __btrc_fn_bool_TrayItem pred);
static TrayItem* btrc_Vector_TrayItem_reduce(btrc_Vector_TrayItem* self, TrayItem* init, __btrc_fn_TrayItem_TrayItem_TrayItem fn);
static btrc_Vector_TrayItem* btrc_Vector_TrayItem_copy(btrc_Vector_TrayItem* self);
static void btrc_Vector_TrayItem_removeAt(btrc_Vector_TrayItem* self, int idx);
static int btrc_Vector_TrayItem_iterLen(btrc_Vector_TrayItem* self);
static TrayItem* btrc_Vector_TrayItem_iterGet(btrc_Vector_TrayItem* self, int i);

static void btrc_Vector_bool_init(btrc_Vector_bool* self);
static btrc_Vector_bool* btrc_Vector_bool_new(void);
static void btrc_Vector_bool_destroy(btrc_Vector_bool* self);
static void btrc_Vector_bool_push(btrc_Vector_bool* self, bool val);
static bool btrc_Vector_bool_pop(btrc_Vector_bool* self);
static bool btrc_Vector_bool_get(btrc_Vector_bool* self, int i);
static void btrc_Vector_bool_set(btrc_Vector_bool* self, int i, bool val);
static void btrc_Vector_bool_free(btrc_Vector_bool* self);
static void btrc_Vector_bool_remove(btrc_Vector_bool* self, int idx);
static void btrc_Vector_bool_reverse(btrc_Vector_bool* self);
static btrc_Vector_bool* btrc_Vector_bool_reversed(btrc_Vector_bool* self);
static void btrc_Vector_bool_swap(btrc_Vector_bool* self, int i, int j);
static void btrc_Vector_bool_clear(btrc_Vector_bool* self);
static void btrc_Vector_bool_fill(btrc_Vector_bool* self, bool val);
static int btrc_Vector_bool_size(btrc_Vector_bool* self);
static bool btrc_Vector_bool_isEmpty(btrc_Vector_bool* self);
static bool btrc_Vector_bool_first(btrc_Vector_bool* self);
static bool btrc_Vector_bool_last(btrc_Vector_bool* self);
static btrc_Vector_bool* btrc_Vector_bool_slice(btrc_Vector_bool* self, int start, int end);
static btrc_Vector_bool* btrc_Vector_bool_take(btrc_Vector_bool* self, int n);
static btrc_Vector_bool* btrc_Vector_bool_drop(btrc_Vector_bool* self, int n);
static void btrc_Vector_bool_extend(btrc_Vector_bool* self, btrc_Vector_bool* other);
static void btrc_Vector_bool_insert(btrc_Vector_bool* self, int idx, bool val);
static bool btrc_Vector_bool_contains(btrc_Vector_bool* self, bool val);
static int btrc_Vector_bool_indexOf(btrc_Vector_bool* self, bool val);
static int btrc_Vector_bool_lastIndexOf(btrc_Vector_bool* self, bool val);
static int btrc_Vector_bool_count(btrc_Vector_bool* self, bool val);
static void btrc_Vector_bool_removeAll(btrc_Vector_bool* self, bool val);
static btrc_Vector_bool* btrc_Vector_bool_distinct(btrc_Vector_bool* self);
static void btrc_Vector_bool_sort(btrc_Vector_bool* self);
static btrc_Vector_bool* btrc_Vector_bool_sorted(btrc_Vector_bool* self);
static bool btrc_Vector_bool_min(btrc_Vector_bool* self);
static bool btrc_Vector_bool_max(btrc_Vector_bool* self);
static bool btrc_Vector_bool_sum(btrc_Vector_bool* self);
static char* btrc_Vector_bool_join(btrc_Vector_bool* self, char* sep);
static char* btrc_Vector_bool_joinToString(btrc_Vector_bool* self, char* sep);
static btrc_Vector_bool* btrc_Vector_bool_filter(btrc_Vector_bool* self, __btrc_fn_bool_bool pred);
static int btrc_Vector_bool_findIndex(btrc_Vector_bool* self, __btrc_fn_bool_bool pred);
static void btrc_Vector_bool_forEach(btrc_Vector_bool* self, __btrc_fn_void_bool fn);
static btrc_Vector_bool* btrc_Vector_bool_map(btrc_Vector_bool* self, __btrc_fn_bool_bool fn);
static bool btrc_Vector_bool_any(btrc_Vector_bool* self, __btrc_fn_bool_bool pred);
static bool btrc_Vector_bool_all(btrc_Vector_bool* self, __btrc_fn_bool_bool pred);
static bool btrc_Vector_bool_reduce(btrc_Vector_bool* self, bool init, __btrc_fn_bool_bool_bool fn);
static btrc_Vector_bool* btrc_Vector_bool_copy(btrc_Vector_bool* self);
static void btrc_Vector_bool_removeAt(btrc_Vector_bool* self, int idx);
static int btrc_Vector_bool_iterLen(btrc_Vector_bool* self);
static bool btrc_Vector_bool_iterGet(btrc_Vector_bool* self, int i);

static void btrc_Vector_int_init(btrc_Vector_int* self);
static btrc_Vector_int* btrc_Vector_int_new(void);
static void btrc_Vector_int_destroy(btrc_Vector_int* self);
static void btrc_Vector_int_push(btrc_Vector_int* self, int val);
static int btrc_Vector_int_pop(btrc_Vector_int* self);
static int btrc_Vector_int_get(btrc_Vector_int* self, int i);
static void btrc_Vector_int_set(btrc_Vector_int* self, int i, int val);
static void btrc_Vector_int_free(btrc_Vector_int* self);
static void btrc_Vector_int_remove(btrc_Vector_int* self, int idx);
static void btrc_Vector_int_reverse(btrc_Vector_int* self);
static btrc_Vector_int* btrc_Vector_int_reversed(btrc_Vector_int* self);
static void btrc_Vector_int_swap(btrc_Vector_int* self, int i, int j);
static void btrc_Vector_int_clear(btrc_Vector_int* self);
static void btrc_Vector_int_fill(btrc_Vector_int* self, int val);
static int btrc_Vector_int_size(btrc_Vector_int* self);
static bool btrc_Vector_int_isEmpty(btrc_Vector_int* self);
static int btrc_Vector_int_first(btrc_Vector_int* self);
static int btrc_Vector_int_last(btrc_Vector_int* self);
static btrc_Vector_int* btrc_Vector_int_slice(btrc_Vector_int* self, int start, int end);
static btrc_Vector_int* btrc_Vector_int_take(btrc_Vector_int* self, int n);
static btrc_Vector_int* btrc_Vector_int_drop(btrc_Vector_int* self, int n);
static void btrc_Vector_int_extend(btrc_Vector_int* self, btrc_Vector_int* other);
static void btrc_Vector_int_insert(btrc_Vector_int* self, int idx, int val);
static bool btrc_Vector_int_contains(btrc_Vector_int* self, int val);
static int btrc_Vector_int_indexOf(btrc_Vector_int* self, int val);
static int btrc_Vector_int_lastIndexOf(btrc_Vector_int* self, int val);
static int btrc_Vector_int_count(btrc_Vector_int* self, int val);
static void btrc_Vector_int_removeAll(btrc_Vector_int* self, int val);
static btrc_Vector_int* btrc_Vector_int_distinct(btrc_Vector_int* self);
static void btrc_Vector_int_sort(btrc_Vector_int* self);
static btrc_Vector_int* btrc_Vector_int_sorted(btrc_Vector_int* self);
static int btrc_Vector_int_min(btrc_Vector_int* self);
static int btrc_Vector_int_max(btrc_Vector_int* self);
static int btrc_Vector_int_sum(btrc_Vector_int* self);
static char* btrc_Vector_int_join(btrc_Vector_int* self, char* sep);
static char* btrc_Vector_int_joinToString(btrc_Vector_int* self, char* sep);
static btrc_Vector_int* btrc_Vector_int_filter(btrc_Vector_int* self, __btrc_fn_bool_int pred);
static int btrc_Vector_int_findIndex(btrc_Vector_int* self, __btrc_fn_bool_int pred);
static void btrc_Vector_int_forEach(btrc_Vector_int* self, __btrc_fn_void_int fn);
static btrc_Vector_int* btrc_Vector_int_map(btrc_Vector_int* self, __btrc_fn_int_int fn);
static bool btrc_Vector_int_any(btrc_Vector_int* self, __btrc_fn_bool_int pred);
static bool btrc_Vector_int_all(btrc_Vector_int* self, __btrc_fn_bool_int pred);
static int btrc_Vector_int_reduce(btrc_Vector_int* self, int init, __btrc_fn_int_int_int fn);
static btrc_Vector_int* btrc_Vector_int_copy(btrc_Vector_int* self);
static void btrc_Vector_int_removeAt(btrc_Vector_int* self, int idx);
static int btrc_Vector_int_iterLen(btrc_Vector_int* self);
static int btrc_Vector_int_iterGet(btrc_Vector_int* self, int i);

static void btrc_Vector_float_init(btrc_Vector_float* self);
static btrc_Vector_float* btrc_Vector_float_new(void);
static void btrc_Vector_float_destroy(btrc_Vector_float* self);
static void btrc_Vector_float_push(btrc_Vector_float* self, float val);
static float btrc_Vector_float_pop(btrc_Vector_float* self);
static float btrc_Vector_float_get(btrc_Vector_float* self, int i);
static void btrc_Vector_float_set(btrc_Vector_float* self, int i, float val);
static void btrc_Vector_float_free(btrc_Vector_float* self);
static void btrc_Vector_float_remove(btrc_Vector_float* self, int idx);
static void btrc_Vector_float_reverse(btrc_Vector_float* self);
static btrc_Vector_float* btrc_Vector_float_reversed(btrc_Vector_float* self);
static void btrc_Vector_float_swap(btrc_Vector_float* self, int i, int j);
static void btrc_Vector_float_clear(btrc_Vector_float* self);
static void btrc_Vector_float_fill(btrc_Vector_float* self, float val);
static int btrc_Vector_float_size(btrc_Vector_float* self);
static bool btrc_Vector_float_isEmpty(btrc_Vector_float* self);
static float btrc_Vector_float_first(btrc_Vector_float* self);
static float btrc_Vector_float_last(btrc_Vector_float* self);
static btrc_Vector_float* btrc_Vector_float_slice(btrc_Vector_float* self, int start, int end);
static btrc_Vector_float* btrc_Vector_float_take(btrc_Vector_float* self, int n);
static btrc_Vector_float* btrc_Vector_float_drop(btrc_Vector_float* self, int n);
static void btrc_Vector_float_extend(btrc_Vector_float* self, btrc_Vector_float* other);
static void btrc_Vector_float_insert(btrc_Vector_float* self, int idx, float val);
static bool btrc_Vector_float_contains(btrc_Vector_float* self, float val);
static int btrc_Vector_float_indexOf(btrc_Vector_float* self, float val);
static int btrc_Vector_float_lastIndexOf(btrc_Vector_float* self, float val);
static int btrc_Vector_float_count(btrc_Vector_float* self, float val);
static void btrc_Vector_float_removeAll(btrc_Vector_float* self, float val);
static btrc_Vector_float* btrc_Vector_float_distinct(btrc_Vector_float* self);
static void btrc_Vector_float_sort(btrc_Vector_float* self);
static btrc_Vector_float* btrc_Vector_float_sorted(btrc_Vector_float* self);
static float btrc_Vector_float_min(btrc_Vector_float* self);
static float btrc_Vector_float_max(btrc_Vector_float* self);
static float btrc_Vector_float_sum(btrc_Vector_float* self);
static char* btrc_Vector_float_join(btrc_Vector_float* self, char* sep);
static char* btrc_Vector_float_joinToString(btrc_Vector_float* self, char* sep);
static btrc_Vector_float* btrc_Vector_float_filter(btrc_Vector_float* self, __btrc_fn_bool_float pred);
static int btrc_Vector_float_findIndex(btrc_Vector_float* self, __btrc_fn_bool_float pred);
static void btrc_Vector_float_forEach(btrc_Vector_float* self, __btrc_fn_void_float fn);
static btrc_Vector_float* btrc_Vector_float_map(btrc_Vector_float* self, __btrc_fn_float_float fn);
static bool btrc_Vector_float_any(btrc_Vector_float* self, __btrc_fn_bool_float pred);
static bool btrc_Vector_float_all(btrc_Vector_float* self, __btrc_fn_bool_float pred);
static float btrc_Vector_float_reduce(btrc_Vector_float* self, float init, __btrc_fn_float_float_float fn);
static btrc_Vector_float* btrc_Vector_float_copy(btrc_Vector_float* self);
static void btrc_Vector_float_removeAt(btrc_Vector_float* self, int idx);
static int btrc_Vector_float_iterLen(btrc_Vector_float* self);
static float btrc_Vector_float_iterGet(btrc_Vector_float* self, int i);

static void btrc_Map_string_string_init(btrc_Map_string_string* self);
static btrc_Map_string_string* btrc_Map_string_string_new(void);
static void btrc_Map_string_string_destroy(btrc_Map_string_string* self);
static void btrc_Map_string_string_resize(btrc_Map_string_string* self);
static void btrc_Map_string_string_put(btrc_Map_string_string* self, char* key, char* value);
static char* btrc_Map_string_string_get(btrc_Map_string_string* self, char* key);
static char* btrc_Map_string_string_getOrDefault(btrc_Map_string_string* self, char* key, char* fallback);
static bool btrc_Map_string_string_has(btrc_Map_string_string* self, char* key);
static bool btrc_Map_string_string_contains(btrc_Map_string_string* self, char* key);
static void btrc_Map_string_string_putIfAbsent(btrc_Map_string_string* self, char* key, char* value);
static void btrc_Map_string_string_free(btrc_Map_string_string* self);
static void btrc_Map_string_string_remove(btrc_Map_string_string* self, char* key);
static void btrc_Map_string_string_clear(btrc_Map_string_string* self);
static int btrc_Map_string_string_size(btrc_Map_string_string* self);
static bool btrc_Map_string_string_isEmpty(btrc_Map_string_string* self);
static btrc_Vector_string* btrc_Map_string_string_keys(btrc_Map_string_string* self);
static btrc_Vector_string* btrc_Map_string_string_values(btrc_Map_string_string* self);
static bool btrc_Map_string_string_containsValue(btrc_Map_string_string* self, char* value);
static void btrc_Map_string_string_set(btrc_Map_string_string* self, char* key, char* value);
static void btrc_Map_string_string_merge(btrc_Map_string_string* self, btrc_Map_string_string* other);
static int btrc_Map_string_string_iterLen(btrc_Map_string_string* self);
static char* btrc_Map_string_string_iterGet(btrc_Map_string_string* self, int n);
static char* btrc_Map_string_string_iterValueAt(btrc_Map_string_string* self, int n);

static void btrc_Map_string_bool_init(btrc_Map_string_bool* self);
static btrc_Map_string_bool* btrc_Map_string_bool_new(void);
static void btrc_Map_string_bool_destroy(btrc_Map_string_bool* self);
static void btrc_Map_string_bool_resize(btrc_Map_string_bool* self);
static void btrc_Map_string_bool_put(btrc_Map_string_bool* self, char* key, bool value);
static bool btrc_Map_string_bool_get(btrc_Map_string_bool* self, char* key);
static bool btrc_Map_string_bool_getOrDefault(btrc_Map_string_bool* self, char* key, bool fallback);
static bool btrc_Map_string_bool_has(btrc_Map_string_bool* self, char* key);
static bool btrc_Map_string_bool_contains(btrc_Map_string_bool* self, char* key);
static void btrc_Map_string_bool_putIfAbsent(btrc_Map_string_bool* self, char* key, bool value);
static void btrc_Map_string_bool_free(btrc_Map_string_bool* self);
static void btrc_Map_string_bool_remove(btrc_Map_string_bool* self, char* key);
static void btrc_Map_string_bool_clear(btrc_Map_string_bool* self);
static int btrc_Map_string_bool_size(btrc_Map_string_bool* self);
static bool btrc_Map_string_bool_isEmpty(btrc_Map_string_bool* self);
static btrc_Vector_string* btrc_Map_string_bool_keys(btrc_Map_string_bool* self);
static btrc_Vector_bool* btrc_Map_string_bool_values(btrc_Map_string_bool* self);
static bool btrc_Map_string_bool_containsValue(btrc_Map_string_bool* self, bool value);
static void btrc_Map_string_bool_set(btrc_Map_string_bool* self, char* key, bool value);
static void btrc_Map_string_bool_merge(btrc_Map_string_bool* self, btrc_Map_string_bool* other);
static int btrc_Map_string_bool_iterLen(btrc_Map_string_bool* self);
static char* btrc_Map_string_bool_iterGet(btrc_Map_string_bool* self, int n);
static bool btrc_Map_string_bool_iterValueAt(btrc_Map_string_bool* self, int n);

static void UiNode_visit(UiNode* self, void (*fn)(void**)) {
    (void)self;
    (void)fn;
}

static void btrc_Vector_string_init(btrc_Vector_string* self) {
    self->__rc = 1;
    (self->data = NULL);
    (self->len = 0);
    (self->cap = 0);
}

static btrc_Vector_string* btrc_Vector_string_new(void) {
    btrc_Vector_string* self = ((btrc_Vector_string*)malloc(sizeof(btrc_Vector_string)));
    memset(self, 0, sizeof(btrc_Vector_string));
    btrc_Vector_string_init(self);
    return self;
}

static void btrc_Vector_string_destroy(btrc_Vector_string* self) {
    free(self);
}

static void btrc_Vector_string_push(btrc_Vector_string* self, char* val) {
    if (self->len >= self->cap) {
        (self->cap = ((self->cap == 0) ? 4 : (self->cap * 2)));
        (self->data = ((char**)__btrc_safe_realloc(self->data, (sizeof(char*) * self->cap))));
    }
    (self->data[self->len] = val);
    (self->len++);
}

static char* btrc_Vector_string_pop(btrc_Vector_string* self) {
    if (self->len <= 0) {
        fprintf(stderr, "Vector pop from empty list\n");
        exit(1);
    }
    (self->len--);
    return self->data[self->len];
}

static char* btrc_Vector_string_get(btrc_Vector_string* self, int i) {
    if ((i < 0) || (i >= self->len)) {
        fprintf(stderr, "Vector index out of bounds: %d (len=%d)\n", i, self->len);
        exit(1);
    }
    return self->data[i];
}

static void btrc_Vector_string_set(btrc_Vector_string* self, int i, char* val) {
    if ((i < 0) || (i >= self->len)) {
        fprintf(stderr, "Vector index out of bounds: %d (len=%d)\n", i, self->len);
        exit(1);
    }
    (self->data[i] = val);
}

static void btrc_Vector_string_free(btrc_Vector_string* self) {
    for (int i = 0; (i < self->len); (i++)) {
    }
    free(self->data);
    (self->data = NULL);
    (self->len = 0);
    (self->cap = 0);
}

static void btrc_Vector_string_remove(btrc_Vector_string* self, int idx) {
    if ((idx < 0) || (idx >= self->len)) {
        fprintf(stderr, "Vector remove index out of bounds: %d (len=%d)\n", idx, self->len);
        exit(1);
    }
    for (int i = idx; (i < (self->len - 1)); (i++)) {
        (self->data[i] = self->data[(i + 1)]);
    }
    (self->len--);
}

static void btrc_Vector_string_reverse(btrc_Vector_string* self) {
    for (int i = 0; (i < (self->len / 2)); (i++)) {
        char* tmp = self->data[i];
        (self->data[i] = self->data[((self->len - 1) - i)]);
        (self->data[((self->len - 1) - i)] = tmp);
    }
}

static btrc_Vector_string* btrc_Vector_string_reversed(btrc_Vector_string* self) {
    btrc_Vector_string* result = btrc_Vector_string_new();
    for (int i = (self->len - 1); (i >= 0); (i--)) {
        btrc_Vector_string_push(result, self->data[i]);
    }
    return result;
}

static void btrc_Vector_string_swap(btrc_Vector_string* self, int i, int j) {
    if ((((i < 0) || (i >= self->len)) || (j < 0)) || (j >= self->len)) {
        fprintf(stderr, "Vector swap index out of bounds\n");
        exit(1);
    }
    char* tmp = self->data[i];
    (self->data[i] = self->data[j]);
    (self->data[j] = tmp);
}

static void btrc_Vector_string_clear(btrc_Vector_string* self) {
    for (int i = 0; (i < self->len); (i++)) {
    }
    (self->len = 0);
}

static void btrc_Vector_string_fill(btrc_Vector_string* self, char* val) {
    for (int i = 0; (i < self->len); (i++)) {
        (self->data[i] = val);
    }
}

static int btrc_Vector_string_size(btrc_Vector_string* self) {
    return self->len;
}

static bool btrc_Vector_string_isEmpty(btrc_Vector_string* self) {
    return (self->len == 0);
}

static char* btrc_Vector_string_first(btrc_Vector_string* self) {
    if (self->len == 0) {
        fprintf(stderr, "Vector.first() called on empty list\n");
        exit(1);
    }
    return self->data[0];
}

static char* btrc_Vector_string_last(btrc_Vector_string* self) {
    if (self->len == 0) {
        fprintf(stderr, "Vector.last() called on empty list\n");
        exit(1);
    }
    return self->data[(self->len - 1)];
}

static btrc_Vector_string* btrc_Vector_string_slice(btrc_Vector_string* self, int start, int end) {
    if (start < 0) {
        (start = (self->len + start));
    }
    if (end < 0) {
        (end = (self->len + end));
    }
    if (start < 0) {
        (start = 0);
    }
    if (end > self->len) {
        (end = self->len);
    }
    btrc_Vector_string* result = btrc_Vector_string_new();
    for (int i = start; (i < end); (i++)) {
        btrc_Vector_string_push(result, self->data[i]);
    }
    return result;
}

static btrc_Vector_string* btrc_Vector_string_take(btrc_Vector_string* self, int n) {
    if (n > self->len) {
        (n = self->len);
    }
    if (n < 0) {
        (n = 0);
    }
    return btrc_Vector_string_slice(self, 0, n);
}

static btrc_Vector_string* btrc_Vector_string_drop(btrc_Vector_string* self, int n) {
    if (n > self->len) {
        (n = self->len);
    }
    if (n < 0) {
        (n = 0);
    }
    return btrc_Vector_string_slice(self, n, self->len);
}

static void btrc_Vector_string_extend(btrc_Vector_string* self, btrc_Vector_string* other) {
    for (int i = 0; (i < other->len); (i++)) {
        btrc_Vector_string_push(self, other->data[i]);
    }
}

static void btrc_Vector_string_insert(btrc_Vector_string* self, int idx, char* val) {
    if ((idx < 0) || (idx > self->len)) {
        fprintf(stderr, "Vector insert index out of bounds: %d (size %d)\n", idx, self->len);
        exit(1);
    }
    if (self->len >= self->cap) {
        (self->cap = ((self->cap == 0) ? 4 : (self->cap * 2)));
        (self->data = ((char**)__btrc_safe_realloc(self->data, (sizeof(char*) * self->cap))));
    }
    for (int i = self->len; (i > idx); (i--)) {
        (self->data[i] = self->data[(i - 1)]);
    }
    (self->data[idx] = val);
    (self->len++);
}

static bool btrc_Vector_string_contains(btrc_Vector_string* self, char* val) {
    for (int i = 0; (i < self->len); (i++)) {
        if (__btrc_eq(self->data[i], val)) {
            return true;
        }
    }
    return false;
}

static int btrc_Vector_string_indexOf(btrc_Vector_string* self, char* val) {
    for (int i = 0; (i < self->len); (i++)) {
        if (__btrc_eq(self->data[i], val)) {
            return i;
        }
    }
    return (-1);
}

static int btrc_Vector_string_lastIndexOf(btrc_Vector_string* self, char* val) {
    for (int i = (self->len - 1); (i >= 0); (i--)) {
        if (__btrc_eq(self->data[i], val)) {
            return i;
        }
    }
    return (-1);
}

static int btrc_Vector_string_count(btrc_Vector_string* self, char* val) {
    int c = 0;
    for (int i = 0; (i < self->len); (i++)) {
        if (__btrc_eq(self->data[i], val)) {
            (c++);
        }
    }
    return c;
}

static void btrc_Vector_string_removeAll(btrc_Vector_string* self, char* val) {
    int j = 0;
    for (int i = 0; (i < self->len); (i++)) {
        if (!__btrc_eq(self->data[i], val)) {
            (self->data[j] = self->data[i]);
            (j++);
        }
    }
    (self->len = j);
}

static btrc_Vector_string* btrc_Vector_string_distinct(btrc_Vector_string* self) {
    btrc_Vector_string* result = btrc_Vector_string_new();
    for (int i = 0; (i < self->len); (i++)) {
        if (!btrc_Vector_string_contains(result, self->data[i])) {
            btrc_Vector_string_push(result, self->data[i]);
        }
    }
    return result;
}

static void btrc_Vector_string_sort(btrc_Vector_string* self) {
    for (int i = 1; (i < self->len); (i++)) {
        char* key = self->data[i];
        int j = (i - 1);
        while ((j >= 0) && __btrc_lt(key, self->data[j])) {
            (self->data[(j + 1)] = self->data[j]);
            (j = (j - 1));
        }
        (self->data[(j + 1)] = key);
    }
}

static btrc_Vector_string* btrc_Vector_string_sorted(btrc_Vector_string* self) {
    btrc_Vector_string* result = btrc_Vector_string_new();
    for (int i = 0; (i < self->len); (i++)) {
        btrc_Vector_string_push(result, self->data[i]);
    }
    btrc_Vector_string_sort(result);
    return result;
}

static char* btrc_Vector_string_min(btrc_Vector_string* self) {
    if (self->len <= 0) {
        fprintf(stderr, "Vector min on empty list\n");
        exit(1);
    }
    char* m = self->data[0];
    for (int i = 1; (i < self->len); (i++)) {
        if (__btrc_lt(self->data[i], m)) {
            (m = self->data[i]);
        }
    }
    return m;
}

static char* btrc_Vector_string_max(btrc_Vector_string* self) {
    if (self->len <= 0) {
        fprintf(stderr, "Vector max on empty list\n");
        exit(1);
    }
    char* m = self->data[0];
    for (int i = 1; (i < self->len); (i++)) {
        if (__btrc_gt(self->data[i], m)) {
            (m = self->data[i]);
        }
    }
    return m;
}

static char* btrc_Vector_string_join(btrc_Vector_string* self, char* sep) {
    int total = 0;
    int sep_len = ((int)strlen(sep));
    for (int i = 0; (i < self->len); (i++)) {
        (total = (total + ((int)strlen(self->data[i]))));
        if (i < (self->len - 1)) {
            (total = (total + sep_len));
        }
    }
    char* result = ((char*)malloc((total + 1)));
    int pos = 0;
    for (int i = 0; (i < self->len); (i++)) {
        int slen = ((int)strlen(self->data[i]));
        memcpy((result + pos), self->data[i], slen);
        (pos = (pos + slen));
        if (i < (self->len - 1)) {
            memcpy((result + pos), sep, sep_len);
            (pos = (pos + sep_len));
        }
    }
    (result[pos] = '\0');
    return result;
}

static char* btrc_Vector_string_joinToString(btrc_Vector_string* self, char* sep) {
    return btrc_Vector_string_join(self, sep);
}

static btrc_Vector_string* btrc_Vector_string_filter(btrc_Vector_string* self, __btrc_fn_bool_string pred) {
    btrc_Vector_string* result = btrc_Vector_string_new();
    for (int i = 0; (i < self->len); (i++)) {
        if (pred(self->data[i])) {
            btrc_Vector_string_push(result, self->data[i]);
        }
    }
    return result;
}

static int btrc_Vector_string_findIndex(btrc_Vector_string* self, __btrc_fn_bool_string pred) {
    for (int i = 0; (i < self->len); (i++)) {
        if (pred(self->data[i])) {
            return i;
        }
    }
    return (-1);
}

static void btrc_Vector_string_forEach(btrc_Vector_string* self, __btrc_fn_void_string fn) {
    for (int i = 0; (i < self->len); (i++)) {
        fn(self->data[i]);
    }
}

static btrc_Vector_string* btrc_Vector_string_map(btrc_Vector_string* self, __btrc_fn_string_string fn) {
    btrc_Vector_string* result = btrc_Vector_string_new();
    for (int i = 0; (i < self->len); (i++)) {
        btrc_Vector_string_push(result, fn(self->data[i]));
    }
    return result;
}

static bool btrc_Vector_string_any(btrc_Vector_string* self, __btrc_fn_bool_string pred) {
    for (int i = 0; (i < self->len); (i++)) {
        if (pred(self->data[i])) {
            return true;
        }
    }
    return false;
}

static bool btrc_Vector_string_all(btrc_Vector_string* self, __btrc_fn_bool_string pred) {
    for (int i = 0; (i < self->len); (i++)) {
        if (!pred(self->data[i])) {
            return false;
        }
    }
    return true;
}

static char* btrc_Vector_string_reduce(btrc_Vector_string* self, char* init, __btrc_fn_string_string_string fn) {
    char* acc = init;
    for (int i = 0; (i < self->len); (i++)) {
        (acc = fn(acc, self->data[i]));
    }
    return acc;
}

static btrc_Vector_string* btrc_Vector_string_copy(btrc_Vector_string* self) {
    btrc_Vector_string* result = btrc_Vector_string_new();
    for (int i = 0; (i < self->len); (i++)) {
        btrc_Vector_string_push(result, self->data[i]);
    }
    return result;
}

static void btrc_Vector_string_removeAt(btrc_Vector_string* self, int idx) {
    btrc_Vector_string_remove(self, idx);
}

static int btrc_Vector_string_iterLen(btrc_Vector_string* self) {
    return self->len;
}

static char* btrc_Vector_string_iterGet(btrc_Vector_string* self, int i) {
    return self->data[i];
}

static void btrc_Vector_UiNode_init(btrc_Vector_UiNode* self) {
    self->__rc = 1;
    (self->data = NULL);
    (self->len = 0);
    (self->cap = 0);
}

static btrc_Vector_UiNode* btrc_Vector_UiNode_new(void) {
    btrc_Vector_UiNode* self = ((btrc_Vector_UiNode*)malloc(sizeof(btrc_Vector_UiNode)));
    memset(self, 0, sizeof(btrc_Vector_UiNode));
    btrc_Vector_UiNode_init(self);
    return self;
}

static void btrc_Vector_UiNode_destroy(btrc_Vector_UiNode* self) {
    free(self);
}

static void btrc_Vector_UiNode_push(btrc_Vector_UiNode* self, UiNode* val) {
    (val->__rc++);
    if (self->len >= self->cap) {
        (self->cap = ((self->cap == 0) ? 4 : (self->cap * 2)));
        (self->data = ((UiNode**)__btrc_safe_realloc(self->data, (sizeof(UiNode*) * self->cap))));
    }
    (self->data[self->len] = val);
    (self->len++);
}

static UiNode* btrc_Vector_UiNode_pop(btrc_Vector_UiNode* self) {
    if (self->len <= 0) {
        fprintf(stderr, "Vector pop from empty list\n");
        exit(1);
    }
    (self->len--);
    return self->data[self->len];
}

static UiNode* btrc_Vector_UiNode_get(btrc_Vector_UiNode* self, int i) {
    if ((i < 0) || (i >= self->len)) {
        fprintf(stderr, "Vector index out of bounds: %d (len=%d)\n", i, self->len);
        exit(1);
    }
    return self->data[i];
}

static void btrc_Vector_UiNode_set(btrc_Vector_UiNode* self, int i, UiNode* val) {
    if ((i < 0) || (i >= self->len)) {
        fprintf(stderr, "Vector index out of bounds: %d (len=%d)\n", i, self->len);
        exit(1);
    }
    if (self->data[i]) {
        if ((--self->data[i]->__rc) <= 0) {
            UiNode_destroy(self->data[i]);
        }
    }
    (val->__rc++);
    (self->data[i] = val);
}

static void btrc_Vector_UiNode_free(btrc_Vector_UiNode* self) {
    for (int i = 0; (i < self->len); (i++)) {
        if (self->data[i]) {
            if ((--self->data[i]->__rc) <= 0) {
                UiNode_destroy(self->data[i]);
            }
        }
    }
    free(self->data);
    (self->data = NULL);
    (self->len = 0);
    (self->cap = 0);
}

static void btrc_Vector_UiNode_remove(btrc_Vector_UiNode* self, int idx) {
    if ((idx < 0) || (idx >= self->len)) {
        fprintf(stderr, "Vector remove index out of bounds: %d (len=%d)\n", idx, self->len);
        exit(1);
    }
    if (self->data[idx]) {
        if ((--self->data[idx]->__rc) <= 0) {
            UiNode_destroy(self->data[idx]);
        }
    }
    for (int i = idx; (i < (self->len - 1)); (i++)) {
        (self->data[i] = self->data[(i + 1)]);
    }
    (self->len--);
}

static void btrc_Vector_UiNode_reverse(btrc_Vector_UiNode* self) {
    for (int i = 0; (i < (self->len / 2)); (i++)) {
        UiNode* tmp = self->data[i];
        (self->data[i] = self->data[((self->len - 1) - i)]);
        (self->data[((self->len - 1) - i)] = tmp);
    }
}

static btrc_Vector_UiNode* btrc_Vector_UiNode_reversed(btrc_Vector_UiNode* self) {
    btrc_Vector_UiNode* result = btrc_Vector_UiNode_new();
    for (int i = (self->len - 1); (i >= 0); (i--)) {
        btrc_Vector_UiNode_push(result, self->data[i]);
    }
    return result;
}

static void btrc_Vector_UiNode_swap(btrc_Vector_UiNode* self, int i, int j) {
    if ((((i < 0) || (i >= self->len)) || (j < 0)) || (j >= self->len)) {
        fprintf(stderr, "Vector swap index out of bounds\n");
        exit(1);
    }
    UiNode* tmp = self->data[i];
    (self->data[i] = self->data[j]);
    (self->data[j] = tmp);
}

static void btrc_Vector_UiNode_clear(btrc_Vector_UiNode* self) {
    for (int i = 0; (i < self->len); (i++)) {
        if (self->data[i]) {
            if ((--self->data[i]->__rc) <= 0) {
                UiNode_destroy(self->data[i]);
            }
        }
    }
    (self->len = 0);
}

static void btrc_Vector_UiNode_fill(btrc_Vector_UiNode* self, UiNode* val) {
    for (int i = 0; (i < self->len); (i++)) {
        if (self->data[i]) {
            if ((--self->data[i]->__rc) <= 0) {
                UiNode_destroy(self->data[i]);
            }
        }
        (val->__rc++);
        (self->data[i] = val);
    }
}

static int btrc_Vector_UiNode_size(btrc_Vector_UiNode* self) {
    return self->len;
}

static bool btrc_Vector_UiNode_isEmpty(btrc_Vector_UiNode* self) {
    return (self->len == 0);
}

static UiNode* btrc_Vector_UiNode_first(btrc_Vector_UiNode* self) {
    if (self->len == 0) {
        fprintf(stderr, "Vector.first() called on empty list\n");
        exit(1);
    }
    return self->data[0];
}

static UiNode* btrc_Vector_UiNode_last(btrc_Vector_UiNode* self) {
    if (self->len == 0) {
        fprintf(stderr, "Vector.last() called on empty list\n");
        exit(1);
    }
    return self->data[(self->len - 1)];
}

static btrc_Vector_UiNode* btrc_Vector_UiNode_slice(btrc_Vector_UiNode* self, int start, int end) {
    if (start < 0) {
        (start = (self->len + start));
    }
    if (end < 0) {
        (end = (self->len + end));
    }
    if (start < 0) {
        (start = 0);
    }
    if (end > self->len) {
        (end = self->len);
    }
    btrc_Vector_UiNode* result = btrc_Vector_UiNode_new();
    for (int i = start; (i < end); (i++)) {
        btrc_Vector_UiNode_push(result, self->data[i]);
    }
    return result;
}

static btrc_Vector_UiNode* btrc_Vector_UiNode_take(btrc_Vector_UiNode* self, int n) {
    if (n > self->len) {
        (n = self->len);
    }
    if (n < 0) {
        (n = 0);
    }
    return btrc_Vector_UiNode_slice(self, 0, n);
}

static btrc_Vector_UiNode* btrc_Vector_UiNode_drop(btrc_Vector_UiNode* self, int n) {
    if (n > self->len) {
        (n = self->len);
    }
    if (n < 0) {
        (n = 0);
    }
    return btrc_Vector_UiNode_slice(self, n, self->len);
}

static void btrc_Vector_UiNode_extend(btrc_Vector_UiNode* self, btrc_Vector_UiNode* other) {
    for (int i = 0; (i < other->len); (i++)) {
        btrc_Vector_UiNode_push(self, other->data[i]);
    }
}

static void btrc_Vector_UiNode_insert(btrc_Vector_UiNode* self, int idx, UiNode* val) {
    if ((idx < 0) || (idx > self->len)) {
        fprintf(stderr, "Vector insert index out of bounds: %d (size %d)\n", idx, self->len);
        exit(1);
    }
    (val->__rc++);
    if (self->len >= self->cap) {
        (self->cap = ((self->cap == 0) ? 4 : (self->cap * 2)));
        (self->data = ((UiNode**)__btrc_safe_realloc(self->data, (sizeof(UiNode*) * self->cap))));
    }
    for (int i = self->len; (i > idx); (i--)) {
        (self->data[i] = self->data[(i - 1)]);
    }
    (self->data[idx] = val);
    (self->len++);
}

static bool btrc_Vector_UiNode_contains(btrc_Vector_UiNode* self, UiNode* val) {
    for (int i = 0; (i < self->len); (i++)) {
        if (__btrc_eq(self->data[i], val)) {
            return true;
        }
    }
    return false;
}

static int btrc_Vector_UiNode_indexOf(btrc_Vector_UiNode* self, UiNode* val) {
    for (int i = 0; (i < self->len); (i++)) {
        if (__btrc_eq(self->data[i], val)) {
            return i;
        }
    }
    return (-1);
}

static int btrc_Vector_UiNode_lastIndexOf(btrc_Vector_UiNode* self, UiNode* val) {
    for (int i = (self->len - 1); (i >= 0); (i--)) {
        if (__btrc_eq(self->data[i], val)) {
            return i;
        }
    }
    return (-1);
}

static int btrc_Vector_UiNode_count(btrc_Vector_UiNode* self, UiNode* val) {
    int c = 0;
    for (int i = 0; (i < self->len); (i++)) {
        if (__btrc_eq(self->data[i], val)) {
            (c++);
        }
    }
    return c;
}

static void btrc_Vector_UiNode_removeAll(btrc_Vector_UiNode* self, UiNode* val) {
    int j = 0;
    for (int i = 0; (i < self->len); (i++)) {
        if (!__btrc_eq(self->data[i], val)) {
            (self->data[j] = self->data[i]);
            (j++);
        } else if (self->data[i]) {
            if ((--self->data[i]->__rc) <= 0) {
                UiNode_destroy(self->data[i]);
            }
        }
    }
    (self->len = j);
}

static btrc_Vector_UiNode* btrc_Vector_UiNode_distinct(btrc_Vector_UiNode* self) {
    btrc_Vector_UiNode* result = btrc_Vector_UiNode_new();
    for (int i = 0; (i < self->len); (i++)) {
        if (!btrc_Vector_UiNode_contains(result, self->data[i])) {
            btrc_Vector_UiNode_push(result, self->data[i]);
        }
    }
    return result;
}

static void btrc_Vector_UiNode_sort(btrc_Vector_UiNode* self) {
    for (int i = 1; (i < self->len); (i++)) {
        UiNode* key = self->data[i];
        int j = (i - 1);
        while ((j >= 0) && __btrc_lt(key, self->data[j])) {
            (self->data[(j + 1)] = self->data[j]);
            (j = (j - 1));
        }
        (self->data[(j + 1)] = key);
    }
}

static btrc_Vector_UiNode* btrc_Vector_UiNode_sorted(btrc_Vector_UiNode* self) {
    btrc_Vector_UiNode* result = btrc_Vector_UiNode_new();
    for (int i = 0; (i < self->len); (i++)) {
        btrc_Vector_UiNode_push(result, self->data[i]);
    }
    btrc_Vector_UiNode_sort(result);
    return result;
}

static UiNode* btrc_Vector_UiNode_min(btrc_Vector_UiNode* self) {
    if (self->len <= 0) {
        fprintf(stderr, "Vector min on empty list\n");
        exit(1);
    }
    UiNode* m = self->data[0];
    for (int i = 1; (i < self->len); (i++)) {
        if (__btrc_lt(self->data[i], m)) {
            (m = self->data[i]);
        }
    }
    return m;
}

static UiNode* btrc_Vector_UiNode_max(btrc_Vector_UiNode* self) {
    if (self->len <= 0) {
        fprintf(stderr, "Vector max on empty list\n");
        exit(1);
    }
    UiNode* m = self->data[0];
    for (int i = 1; (i < self->len); (i++)) {
        if (__btrc_gt(self->data[i], m)) {
            (m = self->data[i]);
        }
    }
    return m;
}

static btrc_Vector_UiNode* btrc_Vector_UiNode_filter(btrc_Vector_UiNode* self, __btrc_fn_bool_UiNode pred) {
    btrc_Vector_UiNode* result = btrc_Vector_UiNode_new();
    for (int i = 0; (i < self->len); (i++)) {
        if (pred(self->data[i])) {
            btrc_Vector_UiNode_push(result, self->data[i]);
        }
    }
    return result;
}

static int btrc_Vector_UiNode_findIndex(btrc_Vector_UiNode* self, __btrc_fn_bool_UiNode pred) {
    for (int i = 0; (i < self->len); (i++)) {
        if (pred(self->data[i])) {
            return i;
        }
    }
    return (-1);
}

static void btrc_Vector_UiNode_forEach(btrc_Vector_UiNode* self, __btrc_fn_void_UiNode fn) {
    for (int i = 0; (i < self->len); (i++)) {
        fn(self->data[i]);
    }
}

static btrc_Vector_UiNode* btrc_Vector_UiNode_map(btrc_Vector_UiNode* self, __btrc_fn_UiNode_UiNode fn) {
    btrc_Vector_UiNode* result = btrc_Vector_UiNode_new();
    for (int i = 0; (i < self->len); (i++)) {
        btrc_Vector_UiNode_push(result, fn(self->data[i]));
    }
    return result;
}

static bool btrc_Vector_UiNode_any(btrc_Vector_UiNode* self, __btrc_fn_bool_UiNode pred) {
    for (int i = 0; (i < self->len); (i++)) {
        if (pred(self->data[i])) {
            return true;
        }
    }
    return false;
}

static bool btrc_Vector_UiNode_all(btrc_Vector_UiNode* self, __btrc_fn_bool_UiNode pred) {
    for (int i = 0; (i < self->len); (i++)) {
        if (!pred(self->data[i])) {
            return false;
        }
    }
    return true;
}

static UiNode* btrc_Vector_UiNode_reduce(btrc_Vector_UiNode* self, UiNode* init, __btrc_fn_UiNode_UiNode_UiNode fn) {
    UiNode* acc = init;
    for (int i = 0; (i < self->len); (i++)) {
        (acc = fn(acc, self->data[i]));
    }
    return acc;
}

static btrc_Vector_UiNode* btrc_Vector_UiNode_copy(btrc_Vector_UiNode* self) {
    btrc_Vector_UiNode* result = btrc_Vector_UiNode_new();
    for (int i = 0; (i < self->len); (i++)) {
        btrc_Vector_UiNode_push(result, self->data[i]);
    }
    return result;
}

static void btrc_Vector_UiNode_removeAt(btrc_Vector_UiNode* self, int idx) {
    btrc_Vector_UiNode_remove(self, idx);
}

static int btrc_Vector_UiNode_iterLen(btrc_Vector_UiNode* self) {
    return self->len;
}

static UiNode* btrc_Vector_UiNode_iterGet(btrc_Vector_UiNode* self, int i) {
    return self->data[i];
}

static void btrc_Vector_TrayItem_init(btrc_Vector_TrayItem* self) {
    self->__rc = 1;
    (self->data = NULL);
    (self->len = 0);
    (self->cap = 0);
}

static btrc_Vector_TrayItem* btrc_Vector_TrayItem_new(void) {
    btrc_Vector_TrayItem* self = ((btrc_Vector_TrayItem*)malloc(sizeof(btrc_Vector_TrayItem)));
    memset(self, 0, sizeof(btrc_Vector_TrayItem));
    btrc_Vector_TrayItem_init(self);
    return self;
}

static void btrc_Vector_TrayItem_destroy(btrc_Vector_TrayItem* self) {
    free(self);
}

static void btrc_Vector_TrayItem_push(btrc_Vector_TrayItem* self, TrayItem* val) {
    (val->__rc++);
    if (self->len >= self->cap) {
        (self->cap = ((self->cap == 0) ? 4 : (self->cap * 2)));
        (self->data = ((TrayItem**)__btrc_safe_realloc(self->data, (sizeof(TrayItem*) * self->cap))));
    }
    (self->data[self->len] = val);
    (self->len++);
}

static TrayItem* btrc_Vector_TrayItem_pop(btrc_Vector_TrayItem* self) {
    if (self->len <= 0) {
        fprintf(stderr, "Vector pop from empty list\n");
        exit(1);
    }
    (self->len--);
    return self->data[self->len];
}

static TrayItem* btrc_Vector_TrayItem_get(btrc_Vector_TrayItem* self, int i) {
    if ((i < 0) || (i >= self->len)) {
        fprintf(stderr, "Vector index out of bounds: %d (len=%d)\n", i, self->len);
        exit(1);
    }
    return self->data[i];
}

static void btrc_Vector_TrayItem_set(btrc_Vector_TrayItem* self, int i, TrayItem* val) {
    if ((i < 0) || (i >= self->len)) {
        fprintf(stderr, "Vector index out of bounds: %d (len=%d)\n", i, self->len);
        exit(1);
    }
    if (self->data[i]) {
        if ((--self->data[i]->__rc) <= 0) {
            TrayItem_destroy(self->data[i]);
        }
    }
    (val->__rc++);
    (self->data[i] = val);
}

static void btrc_Vector_TrayItem_free(btrc_Vector_TrayItem* self) {
    for (int i = 0; (i < self->len); (i++)) {
        if (self->data[i]) {
            if ((--self->data[i]->__rc) <= 0) {
                TrayItem_destroy(self->data[i]);
            }
        }
    }
    free(self->data);
    (self->data = NULL);
    (self->len = 0);
    (self->cap = 0);
}

static void btrc_Vector_TrayItem_remove(btrc_Vector_TrayItem* self, int idx) {
    if ((idx < 0) || (idx >= self->len)) {
        fprintf(stderr, "Vector remove index out of bounds: %d (len=%d)\n", idx, self->len);
        exit(1);
    }
    if (self->data[idx]) {
        if ((--self->data[idx]->__rc) <= 0) {
            TrayItem_destroy(self->data[idx]);
        }
    }
    for (int i = idx; (i < (self->len - 1)); (i++)) {
        (self->data[i] = self->data[(i + 1)]);
    }
    (self->len--);
}

static void btrc_Vector_TrayItem_reverse(btrc_Vector_TrayItem* self) {
    for (int i = 0; (i < (self->len / 2)); (i++)) {
        TrayItem* tmp = self->data[i];
        (self->data[i] = self->data[((self->len - 1) - i)]);
        (self->data[((self->len - 1) - i)] = tmp);
    }
}

static btrc_Vector_TrayItem* btrc_Vector_TrayItem_reversed(btrc_Vector_TrayItem* self) {
    btrc_Vector_TrayItem* result = btrc_Vector_TrayItem_new();
    for (int i = (self->len - 1); (i >= 0); (i--)) {
        btrc_Vector_TrayItem_push(result, self->data[i]);
    }
    return result;
}

static void btrc_Vector_TrayItem_swap(btrc_Vector_TrayItem* self, int i, int j) {
    if ((((i < 0) || (i >= self->len)) || (j < 0)) || (j >= self->len)) {
        fprintf(stderr, "Vector swap index out of bounds\n");
        exit(1);
    }
    TrayItem* tmp = self->data[i];
    (self->data[i] = self->data[j]);
    (self->data[j] = tmp);
}

static void btrc_Vector_TrayItem_clear(btrc_Vector_TrayItem* self) {
    for (int i = 0; (i < self->len); (i++)) {
        if (self->data[i]) {
            if ((--self->data[i]->__rc) <= 0) {
                TrayItem_destroy(self->data[i]);
            }
        }
    }
    (self->len = 0);
}

static void btrc_Vector_TrayItem_fill(btrc_Vector_TrayItem* self, TrayItem* val) {
    for (int i = 0; (i < self->len); (i++)) {
        if (self->data[i]) {
            if ((--self->data[i]->__rc) <= 0) {
                TrayItem_destroy(self->data[i]);
            }
        }
        (val->__rc++);
        (self->data[i] = val);
    }
}

static int btrc_Vector_TrayItem_size(btrc_Vector_TrayItem* self) {
    return self->len;
}

static bool btrc_Vector_TrayItem_isEmpty(btrc_Vector_TrayItem* self) {
    return (self->len == 0);
}

static TrayItem* btrc_Vector_TrayItem_first(btrc_Vector_TrayItem* self) {
    if (self->len == 0) {
        fprintf(stderr, "Vector.first() called on empty list\n");
        exit(1);
    }
    return self->data[0];
}

static TrayItem* btrc_Vector_TrayItem_last(btrc_Vector_TrayItem* self) {
    if (self->len == 0) {
        fprintf(stderr, "Vector.last() called on empty list\n");
        exit(1);
    }
    return self->data[(self->len - 1)];
}

static btrc_Vector_TrayItem* btrc_Vector_TrayItem_slice(btrc_Vector_TrayItem* self, int start, int end) {
    if (start < 0) {
        (start = (self->len + start));
    }
    if (end < 0) {
        (end = (self->len + end));
    }
    if (start < 0) {
        (start = 0);
    }
    if (end > self->len) {
        (end = self->len);
    }
    btrc_Vector_TrayItem* result = btrc_Vector_TrayItem_new();
    for (int i = start; (i < end); (i++)) {
        btrc_Vector_TrayItem_push(result, self->data[i]);
    }
    return result;
}

static btrc_Vector_TrayItem* btrc_Vector_TrayItem_take(btrc_Vector_TrayItem* self, int n) {
    if (n > self->len) {
        (n = self->len);
    }
    if (n < 0) {
        (n = 0);
    }
    return btrc_Vector_TrayItem_slice(self, 0, n);
}

static btrc_Vector_TrayItem* btrc_Vector_TrayItem_drop(btrc_Vector_TrayItem* self, int n) {
    if (n > self->len) {
        (n = self->len);
    }
    if (n < 0) {
        (n = 0);
    }
    return btrc_Vector_TrayItem_slice(self, n, self->len);
}

static void btrc_Vector_TrayItem_extend(btrc_Vector_TrayItem* self, btrc_Vector_TrayItem* other) {
    for (int i = 0; (i < other->len); (i++)) {
        btrc_Vector_TrayItem_push(self, other->data[i]);
    }
}

static void btrc_Vector_TrayItem_insert(btrc_Vector_TrayItem* self, int idx, TrayItem* val) {
    if ((idx < 0) || (idx > self->len)) {
        fprintf(stderr, "Vector insert index out of bounds: %d (size %d)\n", idx, self->len);
        exit(1);
    }
    (val->__rc++);
    if (self->len >= self->cap) {
        (self->cap = ((self->cap == 0) ? 4 : (self->cap * 2)));
        (self->data = ((TrayItem**)__btrc_safe_realloc(self->data, (sizeof(TrayItem*) * self->cap))));
    }
    for (int i = self->len; (i > idx); (i--)) {
        (self->data[i] = self->data[(i - 1)]);
    }
    (self->data[idx] = val);
    (self->len++);
}

static bool btrc_Vector_TrayItem_contains(btrc_Vector_TrayItem* self, TrayItem* val) {
    for (int i = 0; (i < self->len); (i++)) {
        if (__btrc_eq(self->data[i], val)) {
            return true;
        }
    }
    return false;
}

static int btrc_Vector_TrayItem_indexOf(btrc_Vector_TrayItem* self, TrayItem* val) {
    for (int i = 0; (i < self->len); (i++)) {
        if (__btrc_eq(self->data[i], val)) {
            return i;
        }
    }
    return (-1);
}

static int btrc_Vector_TrayItem_lastIndexOf(btrc_Vector_TrayItem* self, TrayItem* val) {
    for (int i = (self->len - 1); (i >= 0); (i--)) {
        if (__btrc_eq(self->data[i], val)) {
            return i;
        }
    }
    return (-1);
}

static int btrc_Vector_TrayItem_count(btrc_Vector_TrayItem* self, TrayItem* val) {
    int c = 0;
    for (int i = 0; (i < self->len); (i++)) {
        if (__btrc_eq(self->data[i], val)) {
            (c++);
        }
    }
    return c;
}

static void btrc_Vector_TrayItem_removeAll(btrc_Vector_TrayItem* self, TrayItem* val) {
    int j = 0;
    for (int i = 0; (i < self->len); (i++)) {
        if (!__btrc_eq(self->data[i], val)) {
            (self->data[j] = self->data[i]);
            (j++);
        } else if (self->data[i]) {
            if ((--self->data[i]->__rc) <= 0) {
                TrayItem_destroy(self->data[i]);
            }
        }
    }
    (self->len = j);
}

static btrc_Vector_TrayItem* btrc_Vector_TrayItem_distinct(btrc_Vector_TrayItem* self) {
    btrc_Vector_TrayItem* result = btrc_Vector_TrayItem_new();
    for (int i = 0; (i < self->len); (i++)) {
        if (!btrc_Vector_TrayItem_contains(result, self->data[i])) {
            btrc_Vector_TrayItem_push(result, self->data[i]);
        }
    }
    return result;
}

static void btrc_Vector_TrayItem_sort(btrc_Vector_TrayItem* self) {
    for (int i = 1; (i < self->len); (i++)) {
        TrayItem* key = self->data[i];
        int j = (i - 1);
        while ((j >= 0) && __btrc_lt(key, self->data[j])) {
            (self->data[(j + 1)] = self->data[j]);
            (j = (j - 1));
        }
        (self->data[(j + 1)] = key);
    }
}

static btrc_Vector_TrayItem* btrc_Vector_TrayItem_sorted(btrc_Vector_TrayItem* self) {
    btrc_Vector_TrayItem* result = btrc_Vector_TrayItem_new();
    for (int i = 0; (i < self->len); (i++)) {
        btrc_Vector_TrayItem_push(result, self->data[i]);
    }
    btrc_Vector_TrayItem_sort(result);
    return result;
}

static TrayItem* btrc_Vector_TrayItem_min(btrc_Vector_TrayItem* self) {
    if (self->len <= 0) {
        fprintf(stderr, "Vector min on empty list\n");
        exit(1);
    }
    TrayItem* m = self->data[0];
    for (int i = 1; (i < self->len); (i++)) {
        if (__btrc_lt(self->data[i], m)) {
            (m = self->data[i]);
        }
    }
    return m;
}

static TrayItem* btrc_Vector_TrayItem_max(btrc_Vector_TrayItem* self) {
    if (self->len <= 0) {
        fprintf(stderr, "Vector max on empty list\n");
        exit(1);
    }
    TrayItem* m = self->data[0];
    for (int i = 1; (i < self->len); (i++)) {
        if (__btrc_gt(self->data[i], m)) {
            (m = self->data[i]);
        }
    }
    return m;
}

static btrc_Vector_TrayItem* btrc_Vector_TrayItem_filter(btrc_Vector_TrayItem* self, __btrc_fn_bool_TrayItem pred) {
    btrc_Vector_TrayItem* result = btrc_Vector_TrayItem_new();
    for (int i = 0; (i < self->len); (i++)) {
        if (pred(self->data[i])) {
            btrc_Vector_TrayItem_push(result, self->data[i]);
        }
    }
    return result;
}

static int btrc_Vector_TrayItem_findIndex(btrc_Vector_TrayItem* self, __btrc_fn_bool_TrayItem pred) {
    for (int i = 0; (i < self->len); (i++)) {
        if (pred(self->data[i])) {
            return i;
        }
    }
    return (-1);
}

static void btrc_Vector_TrayItem_forEach(btrc_Vector_TrayItem* self, __btrc_fn_void_TrayItem fn) {
    for (int i = 0; (i < self->len); (i++)) {
        fn(self->data[i]);
    }
}

static btrc_Vector_TrayItem* btrc_Vector_TrayItem_map(btrc_Vector_TrayItem* self, __btrc_fn_TrayItem_TrayItem fn) {
    btrc_Vector_TrayItem* result = btrc_Vector_TrayItem_new();
    for (int i = 0; (i < self->len); (i++)) {
        btrc_Vector_TrayItem_push(result, fn(self->data[i]));
    }
    return result;
}

static bool btrc_Vector_TrayItem_any(btrc_Vector_TrayItem* self, __btrc_fn_bool_TrayItem pred) {
    for (int i = 0; (i < self->len); (i++)) {
        if (pred(self->data[i])) {
            return true;
        }
    }
    return false;
}

static bool btrc_Vector_TrayItem_all(btrc_Vector_TrayItem* self, __btrc_fn_bool_TrayItem pred) {
    for (int i = 0; (i < self->len); (i++)) {
        if (!pred(self->data[i])) {
            return false;
        }
    }
    return true;
}

static TrayItem* btrc_Vector_TrayItem_reduce(btrc_Vector_TrayItem* self, TrayItem* init, __btrc_fn_TrayItem_TrayItem_TrayItem fn) {
    TrayItem* acc = init;
    for (int i = 0; (i < self->len); (i++)) {
        (acc = fn(acc, self->data[i]));
    }
    return acc;
}

static btrc_Vector_TrayItem* btrc_Vector_TrayItem_copy(btrc_Vector_TrayItem* self) {
    btrc_Vector_TrayItem* result = btrc_Vector_TrayItem_new();
    for (int i = 0; (i < self->len); (i++)) {
        btrc_Vector_TrayItem_push(result, self->data[i]);
    }
    return result;
}

static void btrc_Vector_TrayItem_removeAt(btrc_Vector_TrayItem* self, int idx) {
    btrc_Vector_TrayItem_remove(self, idx);
}

static int btrc_Vector_TrayItem_iterLen(btrc_Vector_TrayItem* self) {
    return self->len;
}

static TrayItem* btrc_Vector_TrayItem_iterGet(btrc_Vector_TrayItem* self, int i) {
    return self->data[i];
}

static void btrc_Vector_bool_init(btrc_Vector_bool* self) {
    self->__rc = 1;
    (self->data = NULL);
    (self->len = 0);
    (self->cap = 0);
}

static btrc_Vector_bool* btrc_Vector_bool_new(void) {
    btrc_Vector_bool* self = ((btrc_Vector_bool*)malloc(sizeof(btrc_Vector_bool)));
    memset(self, 0, sizeof(btrc_Vector_bool));
    btrc_Vector_bool_init(self);
    return self;
}

static void btrc_Vector_bool_destroy(btrc_Vector_bool* self) {
    free(self);
}

static void btrc_Vector_bool_push(btrc_Vector_bool* self, bool val) {
    if (self->len >= self->cap) {
        (self->cap = ((self->cap == 0) ? 4 : (self->cap * 2)));
        (self->data = ((bool*)__btrc_safe_realloc(self->data, (sizeof(bool) * self->cap))));
    }
    (self->data[self->len] = val);
    (self->len++);
}

static bool btrc_Vector_bool_pop(btrc_Vector_bool* self) {
    if (self->len <= 0) {
        fprintf(stderr, "Vector pop from empty list\n");
        exit(1);
    }
    (self->len--);
    return self->data[self->len];
}

static bool btrc_Vector_bool_get(btrc_Vector_bool* self, int i) {
    if ((i < 0) || (i >= self->len)) {
        fprintf(stderr, "Vector index out of bounds: %d (len=%d)\n", i, self->len);
        exit(1);
    }
    return self->data[i];
}

static void btrc_Vector_bool_set(btrc_Vector_bool* self, int i, bool val) {
    if ((i < 0) || (i >= self->len)) {
        fprintf(stderr, "Vector index out of bounds: %d (len=%d)\n", i, self->len);
        exit(1);
    }
    (self->data[i] = val);
}

static void btrc_Vector_bool_free(btrc_Vector_bool* self) {
    for (int i = 0; (i < self->len); (i++)) {
    }
    free(self->data);
    (self->data = NULL);
    (self->len = 0);
    (self->cap = 0);
}

static void btrc_Vector_bool_remove(btrc_Vector_bool* self, int idx) {
    if ((idx < 0) || (idx >= self->len)) {
        fprintf(stderr, "Vector remove index out of bounds: %d (len=%d)\n", idx, self->len);
        exit(1);
    }
    for (int i = idx; (i < (self->len - 1)); (i++)) {
        (self->data[i] = self->data[(i + 1)]);
    }
    (self->len--);
}

static void btrc_Vector_bool_reverse(btrc_Vector_bool* self) {
    for (int i = 0; (i < (self->len / 2)); (i++)) {
        bool tmp = self->data[i];
        (self->data[i] = self->data[((self->len - 1) - i)]);
        (self->data[((self->len - 1) - i)] = tmp);
    }
}

static btrc_Vector_bool* btrc_Vector_bool_reversed(btrc_Vector_bool* self) {
    btrc_Vector_bool* result = btrc_Vector_bool_new();
    for (int i = (self->len - 1); (i >= 0); (i--)) {
        btrc_Vector_bool_push(result, self->data[i]);
    }
    return result;
}

static void btrc_Vector_bool_swap(btrc_Vector_bool* self, int i, int j) {
    if ((((i < 0) || (i >= self->len)) || (j < 0)) || (j >= self->len)) {
        fprintf(stderr, "Vector swap index out of bounds\n");
        exit(1);
    }
    bool tmp = self->data[i];
    (self->data[i] = self->data[j]);
    (self->data[j] = tmp);
}

static void btrc_Vector_bool_clear(btrc_Vector_bool* self) {
    for (int i = 0; (i < self->len); (i++)) {
    }
    (self->len = 0);
}

static void btrc_Vector_bool_fill(btrc_Vector_bool* self, bool val) {
    for (int i = 0; (i < self->len); (i++)) {
        (self->data[i] = val);
    }
}

static int btrc_Vector_bool_size(btrc_Vector_bool* self) {
    return self->len;
}

static bool btrc_Vector_bool_isEmpty(btrc_Vector_bool* self) {
    return (self->len == 0);
}

static bool btrc_Vector_bool_first(btrc_Vector_bool* self) {
    if (self->len == 0) {
        fprintf(stderr, "Vector.first() called on empty list\n");
        exit(1);
    }
    return self->data[0];
}

static bool btrc_Vector_bool_last(btrc_Vector_bool* self) {
    if (self->len == 0) {
        fprintf(stderr, "Vector.last() called on empty list\n");
        exit(1);
    }
    return self->data[(self->len - 1)];
}

static btrc_Vector_bool* btrc_Vector_bool_slice(btrc_Vector_bool* self, int start, int end) {
    if (start < 0) {
        (start = (self->len + start));
    }
    if (end < 0) {
        (end = (self->len + end));
    }
    if (start < 0) {
        (start = 0);
    }
    if (end > self->len) {
        (end = self->len);
    }
    btrc_Vector_bool* result = btrc_Vector_bool_new();
    for (int i = start; (i < end); (i++)) {
        btrc_Vector_bool_push(result, self->data[i]);
    }
    return result;
}

static btrc_Vector_bool* btrc_Vector_bool_take(btrc_Vector_bool* self, int n) {
    if (n > self->len) {
        (n = self->len);
    }
    if (n < 0) {
        (n = 0);
    }
    return btrc_Vector_bool_slice(self, 0, n);
}

static btrc_Vector_bool* btrc_Vector_bool_drop(btrc_Vector_bool* self, int n) {
    if (n > self->len) {
        (n = self->len);
    }
    if (n < 0) {
        (n = 0);
    }
    return btrc_Vector_bool_slice(self, n, self->len);
}

static void btrc_Vector_bool_extend(btrc_Vector_bool* self, btrc_Vector_bool* other) {
    for (int i = 0; (i < other->len); (i++)) {
        btrc_Vector_bool_push(self, other->data[i]);
    }
}

static void btrc_Vector_bool_insert(btrc_Vector_bool* self, int idx, bool val) {
    if ((idx < 0) || (idx > self->len)) {
        fprintf(stderr, "Vector insert index out of bounds: %d (size %d)\n", idx, self->len);
        exit(1);
    }
    if (self->len >= self->cap) {
        (self->cap = ((self->cap == 0) ? 4 : (self->cap * 2)));
        (self->data = ((bool*)__btrc_safe_realloc(self->data, (sizeof(bool) * self->cap))));
    }
    for (int i = self->len; (i > idx); (i--)) {
        (self->data[i] = self->data[(i - 1)]);
    }
    (self->data[idx] = val);
    (self->len++);
}

static bool btrc_Vector_bool_contains(btrc_Vector_bool* self, bool val) {
    for (int i = 0; (i < self->len); (i++)) {
        if (__btrc_eq(self->data[i], val)) {
            return true;
        }
    }
    return false;
}

static int btrc_Vector_bool_indexOf(btrc_Vector_bool* self, bool val) {
    for (int i = 0; (i < self->len); (i++)) {
        if (__btrc_eq(self->data[i], val)) {
            return i;
        }
    }
    return (-1);
}

static int btrc_Vector_bool_lastIndexOf(btrc_Vector_bool* self, bool val) {
    for (int i = (self->len - 1); (i >= 0); (i--)) {
        if (__btrc_eq(self->data[i], val)) {
            return i;
        }
    }
    return (-1);
}

static int btrc_Vector_bool_count(btrc_Vector_bool* self, bool val) {
    int c = 0;
    for (int i = 0; (i < self->len); (i++)) {
        if (__btrc_eq(self->data[i], val)) {
            (c++);
        }
    }
    return c;
}

static void btrc_Vector_bool_removeAll(btrc_Vector_bool* self, bool val) {
    int j = 0;
    for (int i = 0; (i < self->len); (i++)) {
        if (!__btrc_eq(self->data[i], val)) {
            (self->data[j] = self->data[i]);
            (j++);
        }
    }
    (self->len = j);
}

static btrc_Vector_bool* btrc_Vector_bool_distinct(btrc_Vector_bool* self) {
    btrc_Vector_bool* result = btrc_Vector_bool_new();
    for (int i = 0; (i < self->len); (i++)) {
        if (!btrc_Vector_bool_contains(result, self->data[i])) {
            btrc_Vector_bool_push(result, self->data[i]);
        }
    }
    return result;
}

static void btrc_Vector_bool_sort(btrc_Vector_bool* self) {
    for (int i = 1; (i < self->len); (i++)) {
        bool key = self->data[i];
        int j = (i - 1);
        while ((j >= 0) && __btrc_lt(key, self->data[j])) {
            (self->data[(j + 1)] = self->data[j]);
            (j = (j - 1));
        }
        (self->data[(j + 1)] = key);
    }
}

static btrc_Vector_bool* btrc_Vector_bool_sorted(btrc_Vector_bool* self) {
    btrc_Vector_bool* result = btrc_Vector_bool_new();
    for (int i = 0; (i < self->len); (i++)) {
        btrc_Vector_bool_push(result, self->data[i]);
    }
    btrc_Vector_bool_sort(result);
    return result;
}

static bool btrc_Vector_bool_min(btrc_Vector_bool* self) {
    if (self->len <= 0) {
        fprintf(stderr, "Vector min on empty list\n");
        exit(1);
    }
    bool m = self->data[0];
    for (int i = 1; (i < self->len); (i++)) {
        if (__btrc_lt(self->data[i], m)) {
            (m = self->data[i]);
        }
    }
    return m;
}

static bool btrc_Vector_bool_max(btrc_Vector_bool* self) {
    if (self->len <= 0) {
        fprintf(stderr, "Vector max on empty list\n");
        exit(1);
    }
    bool m = self->data[0];
    for (int i = 1; (i < self->len); (i++)) {
        if (__btrc_gt(self->data[i], m)) {
            (m = self->data[i]);
        }
    }
    return m;
}

static bool btrc_Vector_bool_sum(btrc_Vector_bool* self) {
    bool s = ((bool)0);
    for (int i = 0; (i < self->len); (i++)) {
        (s = (s + self->data[i]));
    }
    return s;
}

static btrc_Vector_bool* btrc_Vector_bool_filter(btrc_Vector_bool* self, __btrc_fn_bool_bool pred) {
    btrc_Vector_bool* result = btrc_Vector_bool_new();
    for (int i = 0; (i < self->len); (i++)) {
        if (pred(self->data[i])) {
            btrc_Vector_bool_push(result, self->data[i]);
        }
    }
    return result;
}

static int btrc_Vector_bool_findIndex(btrc_Vector_bool* self, __btrc_fn_bool_bool pred) {
    for (int i = 0; (i < self->len); (i++)) {
        if (pred(self->data[i])) {
            return i;
        }
    }
    return (-1);
}

static void btrc_Vector_bool_forEach(btrc_Vector_bool* self, __btrc_fn_void_bool fn) {
    for (int i = 0; (i < self->len); (i++)) {
        fn(self->data[i]);
    }
}

static btrc_Vector_bool* btrc_Vector_bool_map(btrc_Vector_bool* self, __btrc_fn_bool_bool fn) {
    btrc_Vector_bool* result = btrc_Vector_bool_new();
    for (int i = 0; (i < self->len); (i++)) {
        btrc_Vector_bool_push(result, fn(self->data[i]));
    }
    return result;
}

static bool btrc_Vector_bool_any(btrc_Vector_bool* self, __btrc_fn_bool_bool pred) {
    for (int i = 0; (i < self->len); (i++)) {
        if (pred(self->data[i])) {
            return true;
        }
    }
    return false;
}

static bool btrc_Vector_bool_all(btrc_Vector_bool* self, __btrc_fn_bool_bool pred) {
    for (int i = 0; (i < self->len); (i++)) {
        if (!pred(self->data[i])) {
            return false;
        }
    }
    return true;
}

static bool btrc_Vector_bool_reduce(btrc_Vector_bool* self, bool init, __btrc_fn_bool_bool_bool fn) {
    bool acc = init;
    for (int i = 0; (i < self->len); (i++)) {
        (acc = fn(acc, self->data[i]));
    }
    return acc;
}

static btrc_Vector_bool* btrc_Vector_bool_copy(btrc_Vector_bool* self) {
    btrc_Vector_bool* result = btrc_Vector_bool_new();
    for (int i = 0; (i < self->len); (i++)) {
        btrc_Vector_bool_push(result, self->data[i]);
    }
    return result;
}

static void btrc_Vector_bool_removeAt(btrc_Vector_bool* self, int idx) {
    btrc_Vector_bool_remove(self, idx);
}

static int btrc_Vector_bool_iterLen(btrc_Vector_bool* self) {
    return self->len;
}

static bool btrc_Vector_bool_iterGet(btrc_Vector_bool* self, int i) {
    return self->data[i];
}

static void btrc_Vector_int_init(btrc_Vector_int* self) {
    self->__rc = 1;
    (self->data = NULL);
    (self->len = 0);
    (self->cap = 0);
}

static btrc_Vector_int* btrc_Vector_int_new(void) {
    btrc_Vector_int* self = ((btrc_Vector_int*)malloc(sizeof(btrc_Vector_int)));
    memset(self, 0, sizeof(btrc_Vector_int));
    btrc_Vector_int_init(self);
    return self;
}

static void btrc_Vector_int_destroy(btrc_Vector_int* self) {
    free(self);
}

static void btrc_Vector_int_push(btrc_Vector_int* self, int val) {
    if (self->len >= self->cap) {
        (self->cap = ((self->cap == 0) ? 4 : (self->cap * 2)));
        (self->data = ((int*)__btrc_safe_realloc(self->data, (sizeof(int) * self->cap))));
    }
    (self->data[self->len] = val);
    (self->len++);
}

static int btrc_Vector_int_pop(btrc_Vector_int* self) {
    if (self->len <= 0) {
        fprintf(stderr, "Vector pop from empty list\n");
        exit(1);
    }
    (self->len--);
    return self->data[self->len];
}

static int btrc_Vector_int_get(btrc_Vector_int* self, int i) {
    if ((i < 0) || (i >= self->len)) {
        fprintf(stderr, "Vector index out of bounds: %d (len=%d)\n", i, self->len);
        exit(1);
    }
    return self->data[i];
}

static void btrc_Vector_int_set(btrc_Vector_int* self, int i, int val) {
    if ((i < 0) || (i >= self->len)) {
        fprintf(stderr, "Vector index out of bounds: %d (len=%d)\n", i, self->len);
        exit(1);
    }
    (self->data[i] = val);
}

static void btrc_Vector_int_free(btrc_Vector_int* self) {
    for (int i = 0; (i < self->len); (i++)) {
    }
    free(self->data);
    (self->data = NULL);
    (self->len = 0);
    (self->cap = 0);
}

static void btrc_Vector_int_remove(btrc_Vector_int* self, int idx) {
    if ((idx < 0) || (idx >= self->len)) {
        fprintf(stderr, "Vector remove index out of bounds: %d (len=%d)\n", idx, self->len);
        exit(1);
    }
    for (int i = idx; (i < (self->len - 1)); (i++)) {
        (self->data[i] = self->data[(i + 1)]);
    }
    (self->len--);
}

static void btrc_Vector_int_reverse(btrc_Vector_int* self) {
    for (int i = 0; (i < (self->len / 2)); (i++)) {
        int tmp = self->data[i];
        (self->data[i] = self->data[((self->len - 1) - i)]);
        (self->data[((self->len - 1) - i)] = tmp);
    }
}

static btrc_Vector_int* btrc_Vector_int_reversed(btrc_Vector_int* self) {
    btrc_Vector_int* result = btrc_Vector_int_new();
    for (int i = (self->len - 1); (i >= 0); (i--)) {
        btrc_Vector_int_push(result, self->data[i]);
    }
    return result;
}

static void btrc_Vector_int_swap(btrc_Vector_int* self, int i, int j) {
    if ((((i < 0) || (i >= self->len)) || (j < 0)) || (j >= self->len)) {
        fprintf(stderr, "Vector swap index out of bounds\n");
        exit(1);
    }
    int tmp = self->data[i];
    (self->data[i] = self->data[j]);
    (self->data[j] = tmp);
}

static void btrc_Vector_int_clear(btrc_Vector_int* self) {
    for (int i = 0; (i < self->len); (i++)) {
    }
    (self->len = 0);
}

static void btrc_Vector_int_fill(btrc_Vector_int* self, int val) {
    for (int i = 0; (i < self->len); (i++)) {
        (self->data[i] = val);
    }
}

static int btrc_Vector_int_size(btrc_Vector_int* self) {
    return self->len;
}

static bool btrc_Vector_int_isEmpty(btrc_Vector_int* self) {
    return (self->len == 0);
}

static int btrc_Vector_int_first(btrc_Vector_int* self) {
    if (self->len == 0) {
        fprintf(stderr, "Vector.first() called on empty list\n");
        exit(1);
    }
    return self->data[0];
}

static int btrc_Vector_int_last(btrc_Vector_int* self) {
    if (self->len == 0) {
        fprintf(stderr, "Vector.last() called on empty list\n");
        exit(1);
    }
    return self->data[(self->len - 1)];
}

static btrc_Vector_int* btrc_Vector_int_slice(btrc_Vector_int* self, int start, int end) {
    if (start < 0) {
        (start = (self->len + start));
    }
    if (end < 0) {
        (end = (self->len + end));
    }
    if (start < 0) {
        (start = 0);
    }
    if (end > self->len) {
        (end = self->len);
    }
    btrc_Vector_int* result = btrc_Vector_int_new();
    for (int i = start; (i < end); (i++)) {
        btrc_Vector_int_push(result, self->data[i]);
    }
    return result;
}

static btrc_Vector_int* btrc_Vector_int_take(btrc_Vector_int* self, int n) {
    if (n > self->len) {
        (n = self->len);
    }
    if (n < 0) {
        (n = 0);
    }
    return btrc_Vector_int_slice(self, 0, n);
}

static btrc_Vector_int* btrc_Vector_int_drop(btrc_Vector_int* self, int n) {
    if (n > self->len) {
        (n = self->len);
    }
    if (n < 0) {
        (n = 0);
    }
    return btrc_Vector_int_slice(self, n, self->len);
}

static void btrc_Vector_int_extend(btrc_Vector_int* self, btrc_Vector_int* other) {
    for (int i = 0; (i < other->len); (i++)) {
        btrc_Vector_int_push(self, other->data[i]);
    }
}

static void btrc_Vector_int_insert(btrc_Vector_int* self, int idx, int val) {
    if ((idx < 0) || (idx > self->len)) {
        fprintf(stderr, "Vector insert index out of bounds: %d (size %d)\n", idx, self->len);
        exit(1);
    }
    if (self->len >= self->cap) {
        (self->cap = ((self->cap == 0) ? 4 : (self->cap * 2)));
        (self->data = ((int*)__btrc_safe_realloc(self->data, (sizeof(int) * self->cap))));
    }
    for (int i = self->len; (i > idx); (i--)) {
        (self->data[i] = self->data[(i - 1)]);
    }
    (self->data[idx] = val);
    (self->len++);
}

static bool btrc_Vector_int_contains(btrc_Vector_int* self, int val) {
    for (int i = 0; (i < self->len); (i++)) {
        if (__btrc_eq(self->data[i], val)) {
            return true;
        }
    }
    return false;
}

static int btrc_Vector_int_indexOf(btrc_Vector_int* self, int val) {
    for (int i = 0; (i < self->len); (i++)) {
        if (__btrc_eq(self->data[i], val)) {
            return i;
        }
    }
    return (-1);
}

static int btrc_Vector_int_lastIndexOf(btrc_Vector_int* self, int val) {
    for (int i = (self->len - 1); (i >= 0); (i--)) {
        if (__btrc_eq(self->data[i], val)) {
            return i;
        }
    }
    return (-1);
}

static int btrc_Vector_int_count(btrc_Vector_int* self, int val) {
    int c = 0;
    for (int i = 0; (i < self->len); (i++)) {
        if (__btrc_eq(self->data[i], val)) {
            (c++);
        }
    }
    return c;
}

static void btrc_Vector_int_removeAll(btrc_Vector_int* self, int val) {
    int j = 0;
    for (int i = 0; (i < self->len); (i++)) {
        if (!__btrc_eq(self->data[i], val)) {
            (self->data[j] = self->data[i]);
            (j++);
        }
    }
    (self->len = j);
}

static btrc_Vector_int* btrc_Vector_int_distinct(btrc_Vector_int* self) {
    btrc_Vector_int* result = btrc_Vector_int_new();
    for (int i = 0; (i < self->len); (i++)) {
        if (!btrc_Vector_int_contains(result, self->data[i])) {
            btrc_Vector_int_push(result, self->data[i]);
        }
    }
    return result;
}

static void btrc_Vector_int_sort(btrc_Vector_int* self) {
    for (int i = 1; (i < self->len); (i++)) {
        int key = self->data[i];
        int j = (i - 1);
        while ((j >= 0) && __btrc_lt(key, self->data[j])) {
            (self->data[(j + 1)] = self->data[j]);
            (j = (j - 1));
        }
        (self->data[(j + 1)] = key);
    }
}

static btrc_Vector_int* btrc_Vector_int_sorted(btrc_Vector_int* self) {
    btrc_Vector_int* result = btrc_Vector_int_new();
    for (int i = 0; (i < self->len); (i++)) {
        btrc_Vector_int_push(result, self->data[i]);
    }
    btrc_Vector_int_sort(result);
    return result;
}

static int btrc_Vector_int_min(btrc_Vector_int* self) {
    if (self->len <= 0) {
        fprintf(stderr, "Vector min on empty list\n");
        exit(1);
    }
    int m = self->data[0];
    for (int i = 1; (i < self->len); (i++)) {
        if (__btrc_lt(self->data[i], m)) {
            (m = self->data[i]);
        }
    }
    return m;
}

static int btrc_Vector_int_max(btrc_Vector_int* self) {
    if (self->len <= 0) {
        fprintf(stderr, "Vector max on empty list\n");
        exit(1);
    }
    int m = self->data[0];
    for (int i = 1; (i < self->len); (i++)) {
        if (__btrc_gt(self->data[i], m)) {
            (m = self->data[i]);
        }
    }
    return m;
}

static int btrc_Vector_int_sum(btrc_Vector_int* self) {
    int s = ((int)0);
    for (int i = 0; (i < self->len); (i++)) {
        (s = (s + self->data[i]));
    }
    return s;
}

static btrc_Vector_int* btrc_Vector_int_filter(btrc_Vector_int* self, __btrc_fn_bool_int pred) {
    btrc_Vector_int* result = btrc_Vector_int_new();
    for (int i = 0; (i < self->len); (i++)) {
        if (pred(self->data[i])) {
            btrc_Vector_int_push(result, self->data[i]);
        }
    }
    return result;
}

static int btrc_Vector_int_findIndex(btrc_Vector_int* self, __btrc_fn_bool_int pred) {
    for (int i = 0; (i < self->len); (i++)) {
        if (pred(self->data[i])) {
            return i;
        }
    }
    return (-1);
}

static void btrc_Vector_int_forEach(btrc_Vector_int* self, __btrc_fn_void_int fn) {
    for (int i = 0; (i < self->len); (i++)) {
        fn(self->data[i]);
    }
}

static btrc_Vector_int* btrc_Vector_int_map(btrc_Vector_int* self, __btrc_fn_int_int fn) {
    btrc_Vector_int* result = btrc_Vector_int_new();
    for (int i = 0; (i < self->len); (i++)) {
        btrc_Vector_int_push(result, fn(self->data[i]));
    }
    return result;
}

static bool btrc_Vector_int_any(btrc_Vector_int* self, __btrc_fn_bool_int pred) {
    for (int i = 0; (i < self->len); (i++)) {
        if (pred(self->data[i])) {
            return true;
        }
    }
    return false;
}

static bool btrc_Vector_int_all(btrc_Vector_int* self, __btrc_fn_bool_int pred) {
    for (int i = 0; (i < self->len); (i++)) {
        if (!pred(self->data[i])) {
            return false;
        }
    }
    return true;
}

static int btrc_Vector_int_reduce(btrc_Vector_int* self, int init, __btrc_fn_int_int_int fn) {
    int acc = init;
    for (int i = 0; (i < self->len); (i++)) {
        (acc = fn(acc, self->data[i]));
    }
    return acc;
}

static btrc_Vector_int* btrc_Vector_int_copy(btrc_Vector_int* self) {
    btrc_Vector_int* result = btrc_Vector_int_new();
    for (int i = 0; (i < self->len); (i++)) {
        btrc_Vector_int_push(result, self->data[i]);
    }
    return result;
}

static void btrc_Vector_int_removeAt(btrc_Vector_int* self, int idx) {
    btrc_Vector_int_remove(self, idx);
}

static int btrc_Vector_int_iterLen(btrc_Vector_int* self) {
    return self->len;
}

static int btrc_Vector_int_iterGet(btrc_Vector_int* self, int i) {
    return self->data[i];
}

static void btrc_Vector_float_init(btrc_Vector_float* self) {
    self->__rc = 1;
    (self->data = NULL);
    (self->len = 0);
    (self->cap = 0);
}

static btrc_Vector_float* btrc_Vector_float_new(void) {
    btrc_Vector_float* self = ((btrc_Vector_float*)malloc(sizeof(btrc_Vector_float)));
    memset(self, 0, sizeof(btrc_Vector_float));
    btrc_Vector_float_init(self);
    return self;
}

static void btrc_Vector_float_destroy(btrc_Vector_float* self) {
    free(self);
}

static void btrc_Vector_float_push(btrc_Vector_float* self, float val) {
    if (self->len >= self->cap) {
        (self->cap = ((self->cap == 0) ? 4 : (self->cap * 2)));
        (self->data = ((float*)__btrc_safe_realloc(self->data, (sizeof(float) * self->cap))));
    }
    (self->data[self->len] = val);
    (self->len++);
}

static float btrc_Vector_float_pop(btrc_Vector_float* self) {
    if (self->len <= 0) {
        fprintf(stderr, "Vector pop from empty list\n");
        exit(1);
    }
    (self->len--);
    return self->data[self->len];
}

static float btrc_Vector_float_get(btrc_Vector_float* self, int i) {
    if ((i < 0) || (i >= self->len)) {
        fprintf(stderr, "Vector index out of bounds: %d (len=%d)\n", i, self->len);
        exit(1);
    }
    return self->data[i];
}

static void btrc_Vector_float_set(btrc_Vector_float* self, int i, float val) {
    if ((i < 0) || (i >= self->len)) {
        fprintf(stderr, "Vector index out of bounds: %d (len=%d)\n", i, self->len);
        exit(1);
    }
    (self->data[i] = val);
}

static void btrc_Vector_float_free(btrc_Vector_float* self) {
    for (int i = 0; (i < self->len); (i++)) {
    }
    free(self->data);
    (self->data = NULL);
    (self->len = 0);
    (self->cap = 0);
}

static void btrc_Vector_float_remove(btrc_Vector_float* self, int idx) {
    if ((idx < 0) || (idx >= self->len)) {
        fprintf(stderr, "Vector remove index out of bounds: %d (len=%d)\n", idx, self->len);
        exit(1);
    }
    for (int i = idx; (i < (self->len - 1)); (i++)) {
        (self->data[i] = self->data[(i + 1)]);
    }
    (self->len--);
}

static void btrc_Vector_float_reverse(btrc_Vector_float* self) {
    for (int i = 0; (i < (self->len / 2)); (i++)) {
        float tmp = self->data[i];
        (self->data[i] = self->data[((self->len - 1) - i)]);
        (self->data[((self->len - 1) - i)] = tmp);
    }
}

static btrc_Vector_float* btrc_Vector_float_reversed(btrc_Vector_float* self) {
    btrc_Vector_float* result = btrc_Vector_float_new();
    for (int i = (self->len - 1); (i >= 0); (i--)) {
        btrc_Vector_float_push(result, self->data[i]);
    }
    return result;
}

static void btrc_Vector_float_swap(btrc_Vector_float* self, int i, int j) {
    if ((((i < 0) || (i >= self->len)) || (j < 0)) || (j >= self->len)) {
        fprintf(stderr, "Vector swap index out of bounds\n");
        exit(1);
    }
    float tmp = self->data[i];
    (self->data[i] = self->data[j]);
    (self->data[j] = tmp);
}

static void btrc_Vector_float_clear(btrc_Vector_float* self) {
    for (int i = 0; (i < self->len); (i++)) {
    }
    (self->len = 0);
}

static void btrc_Vector_float_fill(btrc_Vector_float* self, float val) {
    for (int i = 0; (i < self->len); (i++)) {
        (self->data[i] = val);
    }
}

static int btrc_Vector_float_size(btrc_Vector_float* self) {
    return self->len;
}

static bool btrc_Vector_float_isEmpty(btrc_Vector_float* self) {
    return (self->len == 0);
}

static float btrc_Vector_float_first(btrc_Vector_float* self) {
    if (self->len == 0) {
        fprintf(stderr, "Vector.first() called on empty list\n");
        exit(1);
    }
    return self->data[0];
}

static float btrc_Vector_float_last(btrc_Vector_float* self) {
    if (self->len == 0) {
        fprintf(stderr, "Vector.last() called on empty list\n");
        exit(1);
    }
    return self->data[(self->len - 1)];
}

static btrc_Vector_float* btrc_Vector_float_slice(btrc_Vector_float* self, int start, int end) {
    if (start < 0) {
        (start = (self->len + start));
    }
    if (end < 0) {
        (end = (self->len + end));
    }
    if (start < 0) {
        (start = 0);
    }
    if (end > self->len) {
        (end = self->len);
    }
    btrc_Vector_float* result = btrc_Vector_float_new();
    for (int i = start; (i < end); (i++)) {
        btrc_Vector_float_push(result, self->data[i]);
    }
    return result;
}

static btrc_Vector_float* btrc_Vector_float_take(btrc_Vector_float* self, int n) {
    if (n > self->len) {
        (n = self->len);
    }
    if (n < 0) {
        (n = 0);
    }
    return btrc_Vector_float_slice(self, 0, n);
}

static btrc_Vector_float* btrc_Vector_float_drop(btrc_Vector_float* self, int n) {
    if (n > self->len) {
        (n = self->len);
    }
    if (n < 0) {
        (n = 0);
    }
    return btrc_Vector_float_slice(self, n, self->len);
}

static void btrc_Vector_float_extend(btrc_Vector_float* self, btrc_Vector_float* other) {
    for (int i = 0; (i < other->len); (i++)) {
        btrc_Vector_float_push(self, other->data[i]);
    }
}

static void btrc_Vector_float_insert(btrc_Vector_float* self, int idx, float val) {
    if ((idx < 0) || (idx > self->len)) {
        fprintf(stderr, "Vector insert index out of bounds: %d (size %d)\n", idx, self->len);
        exit(1);
    }
    if (self->len >= self->cap) {
        (self->cap = ((self->cap == 0) ? 4 : (self->cap * 2)));
        (self->data = ((float*)__btrc_safe_realloc(self->data, (sizeof(float) * self->cap))));
    }
    for (int i = self->len; (i > idx); (i--)) {
        (self->data[i] = self->data[(i - 1)]);
    }
    (self->data[idx] = val);
    (self->len++);
}

static bool btrc_Vector_float_contains(btrc_Vector_float* self, float val) {
    for (int i = 0; (i < self->len); (i++)) {
        if (__btrc_eq(self->data[i], val)) {
            return true;
        }
    }
    return false;
}

static int btrc_Vector_float_indexOf(btrc_Vector_float* self, float val) {
    for (int i = 0; (i < self->len); (i++)) {
        if (__btrc_eq(self->data[i], val)) {
            return i;
        }
    }
    return (-1);
}

static int btrc_Vector_float_lastIndexOf(btrc_Vector_float* self, float val) {
    for (int i = (self->len - 1); (i >= 0); (i--)) {
        if (__btrc_eq(self->data[i], val)) {
            return i;
        }
    }
    return (-1);
}

static int btrc_Vector_float_count(btrc_Vector_float* self, float val) {
    int c = 0;
    for (int i = 0; (i < self->len); (i++)) {
        if (__btrc_eq(self->data[i], val)) {
            (c++);
        }
    }
    return c;
}

static void btrc_Vector_float_removeAll(btrc_Vector_float* self, float val) {
    int j = 0;
    for (int i = 0; (i < self->len); (i++)) {
        if (!__btrc_eq(self->data[i], val)) {
            (self->data[j] = self->data[i]);
            (j++);
        }
    }
    (self->len = j);
}

static btrc_Vector_float* btrc_Vector_float_distinct(btrc_Vector_float* self) {
    btrc_Vector_float* result = btrc_Vector_float_new();
    for (int i = 0; (i < self->len); (i++)) {
        if (!btrc_Vector_float_contains(result, self->data[i])) {
            btrc_Vector_float_push(result, self->data[i]);
        }
    }
    return result;
}

static void btrc_Vector_float_sort(btrc_Vector_float* self) {
    for (int i = 1; (i < self->len); (i++)) {
        float key = self->data[i];
        int j = (i - 1);
        while ((j >= 0) && __btrc_lt(key, self->data[j])) {
            (self->data[(j + 1)] = self->data[j]);
            (j = (j - 1));
        }
        (self->data[(j + 1)] = key);
    }
}

static btrc_Vector_float* btrc_Vector_float_sorted(btrc_Vector_float* self) {
    btrc_Vector_float* result = btrc_Vector_float_new();
    for (int i = 0; (i < self->len); (i++)) {
        btrc_Vector_float_push(result, self->data[i]);
    }
    btrc_Vector_float_sort(result);
    return result;
}

static float btrc_Vector_float_min(btrc_Vector_float* self) {
    if (self->len <= 0) {
        fprintf(stderr, "Vector min on empty list\n");
        exit(1);
    }
    float m = self->data[0];
    for (int i = 1; (i < self->len); (i++)) {
        if (__btrc_lt(self->data[i], m)) {
            (m = self->data[i]);
        }
    }
    return m;
}

static float btrc_Vector_float_max(btrc_Vector_float* self) {
    if (self->len <= 0) {
        fprintf(stderr, "Vector max on empty list\n");
        exit(1);
    }
    float m = self->data[0];
    for (int i = 1; (i < self->len); (i++)) {
        if (__btrc_gt(self->data[i], m)) {
            (m = self->data[i]);
        }
    }
    return m;
}

static float btrc_Vector_float_sum(btrc_Vector_float* self) {
    float s = ((float)0);
    for (int i = 0; (i < self->len); (i++)) {
        (s = (s + self->data[i]));
    }
    return s;
}

static btrc_Vector_float* btrc_Vector_float_filter(btrc_Vector_float* self, __btrc_fn_bool_float pred) {
    btrc_Vector_float* result = btrc_Vector_float_new();
    for (int i = 0; (i < self->len); (i++)) {
        if (pred(self->data[i])) {
            btrc_Vector_float_push(result, self->data[i]);
        }
    }
    return result;
}

static int btrc_Vector_float_findIndex(btrc_Vector_float* self, __btrc_fn_bool_float pred) {
    for (int i = 0; (i < self->len); (i++)) {
        if (pred(self->data[i])) {
            return i;
        }
    }
    return (-1);
}

static void btrc_Vector_float_forEach(btrc_Vector_float* self, __btrc_fn_void_float fn) {
    for (int i = 0; (i < self->len); (i++)) {
        fn(self->data[i]);
    }
}

static btrc_Vector_float* btrc_Vector_float_map(btrc_Vector_float* self, __btrc_fn_float_float fn) {
    btrc_Vector_float* result = btrc_Vector_float_new();
    for (int i = 0; (i < self->len); (i++)) {
        btrc_Vector_float_push(result, fn(self->data[i]));
    }
    return result;
}

static bool btrc_Vector_float_any(btrc_Vector_float* self, __btrc_fn_bool_float pred) {
    for (int i = 0; (i < self->len); (i++)) {
        if (pred(self->data[i])) {
            return true;
        }
    }
    return false;
}

static bool btrc_Vector_float_all(btrc_Vector_float* self, __btrc_fn_bool_float pred) {
    for (int i = 0; (i < self->len); (i++)) {
        if (!pred(self->data[i])) {
            return false;
        }
    }
    return true;
}

static float btrc_Vector_float_reduce(btrc_Vector_float* self, float init, __btrc_fn_float_float_float fn) {
    float acc = init;
    for (int i = 0; (i < self->len); (i++)) {
        (acc = fn(acc, self->data[i]));
    }
    return acc;
}

static btrc_Vector_float* btrc_Vector_float_copy(btrc_Vector_float* self) {
    btrc_Vector_float* result = btrc_Vector_float_new();
    for (int i = 0; (i < self->len); (i++)) {
        btrc_Vector_float_push(result, self->data[i]);
    }
    return result;
}

static void btrc_Vector_float_removeAt(btrc_Vector_float* self, int idx) {
    btrc_Vector_float_remove(self, idx);
}

static int btrc_Vector_float_iterLen(btrc_Vector_float* self) {
    return self->len;
}

static float btrc_Vector_float_iterGet(btrc_Vector_float* self, int i) {
    return self->data[i];
}

static void btrc_Map_string_string_init(btrc_Map_string_string* self) {
    self->__rc = 1;
    (self->cap = 16);
    (self->len = 0);
    (self->keys = ((char**)__btrc_safe_calloc(16, sizeof(char*))));
    (self->values = ((char**)__btrc_safe_calloc(16, sizeof(char*))));
    (self->occupied = ((bool*)__btrc_safe_calloc(16, sizeof(bool))));
}

static btrc_Map_string_string* btrc_Map_string_string_new(void) {
    btrc_Map_string_string* self = ((btrc_Map_string_string*)malloc(sizeof(btrc_Map_string_string)));
    memset(self, 0, sizeof(btrc_Map_string_string));
    btrc_Map_string_string_init(self);
    return self;
}

static void btrc_Map_string_string_destroy(btrc_Map_string_string* self) {
    free(self);
}

static void btrc_Map_string_string_resize(btrc_Map_string_string* self) {
    int old_cap = self->cap;
    char** old_keys = self->keys;
    char** old_values = self->values;
    bool* old_occupied = self->occupied;
    (self->cap = (self->cap * 2));
    (self->len = 0);
    (self->keys = ((char**)__btrc_safe_calloc(self->cap, sizeof(char*))));
    (self->values = ((char**)__btrc_safe_calloc(self->cap, sizeof(char*))));
    (self->occupied = ((bool*)__btrc_safe_calloc(self->cap, sizeof(bool))));
    for (int i = 0; (i < old_cap); (i++)) {
        if (old_occupied[i]) {
            btrc_Map_string_string_put(self, old_keys[i], old_values[i]);
        }
    }
    free(old_keys);
    free(old_values);
    free(old_occupied);
}

static void btrc_Map_string_string_put(btrc_Map_string_string* self, char* key, char* value) {
    if ((self->len * 4) >= (self->cap * 3)) {
        btrc_Map_string_string_resize(self);
    }
    unsigned int idx = (__btrc_hash(key) % self->cap);
    while (self->occupied[idx]) {
        if (__btrc_eq(self->keys[idx], key)) {
            (self->values[idx] = value);
            return;
        }
        (idx = ((idx + 1) % self->cap));
    }
    (self->keys[idx] = key);
    (self->values[idx] = value);
    (self->occupied[idx] = true);
    (self->len++);
}

static char* btrc_Map_string_string_get(btrc_Map_string_string* self, char* key) {
    unsigned int idx = (__btrc_hash(key) % self->cap);
    while (self->occupied[idx]) {
        if (__btrc_eq(self->keys[idx], key)) {
            return self->values[idx];
        }
        (idx = ((idx + 1) % self->cap));
    }
    fprintf(stderr, "Map key not found\n");
    exit(1);
    return self->values[0];
}

static char* btrc_Map_string_string_getOrDefault(btrc_Map_string_string* self, char* key, char* fallback) {
    unsigned int idx = (__btrc_hash(key) % self->cap);
    while (self->occupied[idx]) {
        if (__btrc_eq(self->keys[idx], key)) {
            return self->values[idx];
        }
        (idx = ((idx + 1) % self->cap));
    }
    return fallback;
}

static bool btrc_Map_string_string_has(btrc_Map_string_string* self, char* key) {
    unsigned int idx = (__btrc_hash(key) % self->cap);
    while (self->occupied[idx]) {
        if (__btrc_eq(self->keys[idx], key)) {
            return true;
        }
        (idx = ((idx + 1) % self->cap));
    }
    return false;
}

static bool btrc_Map_string_string_contains(btrc_Map_string_string* self, char* key) {
    return btrc_Map_string_string_has(self, key);
}

static void btrc_Map_string_string_putIfAbsent(btrc_Map_string_string* self, char* key, char* value) {
    if (!btrc_Map_string_string_has(self, key)) {
        btrc_Map_string_string_put(self, key, value);
    }
}

static void btrc_Map_string_string_free(btrc_Map_string_string* self) {
    free(self->keys);
    free(self->values);
    free(self->occupied);
    (self->keys = NULL);
    (self->values = NULL);
    (self->occupied = NULL);
    (self->cap = 0);
    (self->len = 0);
}

static void btrc_Map_string_string_remove(btrc_Map_string_string* self, char* key) {
    unsigned int idx = (__btrc_hash(key) % self->cap);
    while (self->occupied[idx]) {
        if (__btrc_eq(self->keys[idx], key)) {
            (self->occupied[idx] = false);
            (self->len--);
            unsigned int j = ((idx + 1) % self->cap);
            while (self->occupied[j]) {
                char* rk = self->keys[j];
                char* rv = self->values[j];
                (self->occupied[j] = false);
                (self->len--);
                btrc_Map_string_string_put(self, rk, rv);
                (j = ((j + 1) % self->cap));
            }
            return;
        }
        (idx = ((idx + 1) % self->cap));
    }
}

static void btrc_Map_string_string_clear(btrc_Map_string_string* self) {
    for (int i = 0; (i < self->cap); (i++)) {
        (self->occupied[i] = false);
    }
    (self->len = 0);
}

static int btrc_Map_string_string_size(btrc_Map_string_string* self) {
    return self->len;
}

static bool btrc_Map_string_string_isEmpty(btrc_Map_string_string* self) {
    return (self->len == 0);
}

static btrc_Vector_string* btrc_Map_string_string_keys(btrc_Map_string_string* self) {
    btrc_Vector_string* result = btrc_Vector_string_new();
    for (int i = 0; (i < self->cap); (i++)) {
        if (self->occupied[i]) {
            btrc_Vector_string_push(result, self->keys[i]);
        }
    }
    return result;
}

static btrc_Vector_string* btrc_Map_string_string_values(btrc_Map_string_string* self) {
    btrc_Vector_string* result = btrc_Vector_string_new();
    for (int i = 0; (i < self->cap); (i++)) {
        if (self->occupied[i]) {
            btrc_Vector_string_push(result, self->values[i]);
        }
    }
    return result;
}

static bool btrc_Map_string_string_containsValue(btrc_Map_string_string* self, char* value) {
    for (int i = 0; (i < self->cap); (i++)) {
        if (self->occupied[i] && __btrc_eq(self->values[i], value)) {
            return true;
        }
    }
    return false;
}

static void btrc_Map_string_string_set(btrc_Map_string_string* self, char* key, char* value) {
    btrc_Map_string_string_put(self, key, value);
}

static void btrc_Map_string_string_merge(btrc_Map_string_string* self, btrc_Map_string_string* other) {
    for (int i = 0; (i < other->cap); (i++)) {
        if (other->occupied[i]) {
            btrc_Map_string_string_put(self, other->keys[i], other->values[i]);
        }
    }
}

static int btrc_Map_string_string_iterLen(btrc_Map_string_string* self) {
    return self->len;
}

static char* btrc_Map_string_string_iterGet(btrc_Map_string_string* self, int n) {
    int count = 0;
    for (int i = 0; (i < self->cap); (i++)) {
        if (self->occupied[i]) {
            if (count == n) {
                return self->keys[i];
            }
            (count++);
        }
    }
    fprintf(stderr, "Map iterGet: index out of bounds\n");
    exit(1);
    return self->keys[0];
}

static char* btrc_Map_string_string_iterValueAt(btrc_Map_string_string* self, int n) {
    int count = 0;
    for (int i = 0; (i < self->cap); (i++)) {
        if (self->occupied[i]) {
            if (count == n) {
                return self->values[i];
            }
            (count++);
        }
    }
    fprintf(stderr, "Map iterValueAt: index out of bounds\n");
    exit(1);
    return self->values[0];
}

static void btrc_Map_string_bool_init(btrc_Map_string_bool* self) {
    self->__rc = 1;
    (self->cap = 16);
    (self->len = 0);
    (self->keys = ((char**)__btrc_safe_calloc(16, sizeof(char*))));
    (self->values = ((bool*)__btrc_safe_calloc(16, sizeof(bool))));
    (self->occupied = ((bool*)__btrc_safe_calloc(16, sizeof(bool))));
}

static btrc_Map_string_bool* btrc_Map_string_bool_new(void) {
    btrc_Map_string_bool* self = ((btrc_Map_string_bool*)malloc(sizeof(btrc_Map_string_bool)));
    memset(self, 0, sizeof(btrc_Map_string_bool));
    btrc_Map_string_bool_init(self);
    return self;
}

static void btrc_Map_string_bool_destroy(btrc_Map_string_bool* self) {
    free(self);
}

static void btrc_Map_string_bool_resize(btrc_Map_string_bool* self) {
    int old_cap = self->cap;
    char** old_keys = self->keys;
    bool* old_values = self->values;
    bool* old_occupied = self->occupied;
    (self->cap = (self->cap * 2));
    (self->len = 0);
    (self->keys = ((char**)__btrc_safe_calloc(self->cap, sizeof(char*))));
    (self->values = ((bool*)__btrc_safe_calloc(self->cap, sizeof(bool))));
    (self->occupied = ((bool*)__btrc_safe_calloc(self->cap, sizeof(bool))));
    for (int i = 0; (i < old_cap); (i++)) {
        if (old_occupied[i]) {
            btrc_Map_string_bool_put(self, old_keys[i], old_values[i]);
        }
    }
    free(old_keys);
    free(old_values);
    free(old_occupied);
}

static void btrc_Map_string_bool_put(btrc_Map_string_bool* self, char* key, bool value) {
    if ((self->len * 4) >= (self->cap * 3)) {
        btrc_Map_string_bool_resize(self);
    }
    unsigned int idx = (__btrc_hash(key) % self->cap);
    while (self->occupied[idx]) {
        if (__btrc_eq(self->keys[idx], key)) {
            (self->values[idx] = value);
            return;
        }
        (idx = ((idx + 1) % self->cap));
    }
    (self->keys[idx] = key);
    (self->values[idx] = value);
    (self->occupied[idx] = true);
    (self->len++);
}

static bool btrc_Map_string_bool_get(btrc_Map_string_bool* self, char* key) {
    unsigned int idx = (__btrc_hash(key) % self->cap);
    while (self->occupied[idx]) {
        if (__btrc_eq(self->keys[idx], key)) {
            return self->values[idx];
        }
        (idx = ((idx + 1) % self->cap));
    }
    fprintf(stderr, "Map key not found\n");
    exit(1);
    return self->values[0];
}

static bool btrc_Map_string_bool_getOrDefault(btrc_Map_string_bool* self, char* key, bool fallback) {
    unsigned int idx = (__btrc_hash(key) % self->cap);
    while (self->occupied[idx]) {
        if (__btrc_eq(self->keys[idx], key)) {
            return self->values[idx];
        }
        (idx = ((idx + 1) % self->cap));
    }
    return fallback;
}

static bool btrc_Map_string_bool_has(btrc_Map_string_bool* self, char* key) {
    unsigned int idx = (__btrc_hash(key) % self->cap);
    while (self->occupied[idx]) {
        if (__btrc_eq(self->keys[idx], key)) {
            return true;
        }
        (idx = ((idx + 1) % self->cap));
    }
    return false;
}

static bool btrc_Map_string_bool_contains(btrc_Map_string_bool* self, char* key) {
    return btrc_Map_string_bool_has(self, key);
}

static void btrc_Map_string_bool_putIfAbsent(btrc_Map_string_bool* self, char* key, bool value) {
    if (!btrc_Map_string_bool_has(self, key)) {
        btrc_Map_string_bool_put(self, key, value);
    }
}

static void btrc_Map_string_bool_free(btrc_Map_string_bool* self) {
    free(self->keys);
    free(self->values);
    free(self->occupied);
    (self->keys = NULL);
    (self->values = NULL);
    (self->occupied = NULL);
    (self->cap = 0);
    (self->len = 0);
}

static void btrc_Map_string_bool_remove(btrc_Map_string_bool* self, char* key) {
    unsigned int idx = (__btrc_hash(key) % self->cap);
    while (self->occupied[idx]) {
        if (__btrc_eq(self->keys[idx], key)) {
            (self->occupied[idx] = false);
            (self->len--);
            unsigned int j = ((idx + 1) % self->cap);
            while (self->occupied[j]) {
                char* rk = self->keys[j];
                bool rv = self->values[j];
                (self->occupied[j] = false);
                (self->len--);
                btrc_Map_string_bool_put(self, rk, rv);
                (j = ((j + 1) % self->cap));
            }
            return;
        }
        (idx = ((idx + 1) % self->cap));
    }
}

static void btrc_Map_string_bool_clear(btrc_Map_string_bool* self) {
    for (int i = 0; (i < self->cap); (i++)) {
        (self->occupied[i] = false);
    }
    (self->len = 0);
}

static int btrc_Map_string_bool_size(btrc_Map_string_bool* self) {
    return self->len;
}

static bool btrc_Map_string_bool_isEmpty(btrc_Map_string_bool* self) {
    return (self->len == 0);
}

static btrc_Vector_string* btrc_Map_string_bool_keys(btrc_Map_string_bool* self) {
    btrc_Vector_string* result = btrc_Vector_string_new();
    for (int i = 0; (i < self->cap); (i++)) {
        if (self->occupied[i]) {
            btrc_Vector_string_push(result, self->keys[i]);
        }
    }
    return result;
}

static btrc_Vector_bool* btrc_Map_string_bool_values(btrc_Map_string_bool* self) {
    btrc_Vector_bool* result = btrc_Vector_bool_new();
    for (int i = 0; (i < self->cap); (i++)) {
        if (self->occupied[i]) {
            btrc_Vector_bool_push(result, self->values[i]);
        }
    }
    return result;
}

static bool btrc_Map_string_bool_containsValue(btrc_Map_string_bool* self, bool value) {
    for (int i = 0; (i < self->cap); (i++)) {
        if (self->occupied[i] && __btrc_eq(self->values[i], value)) {
            return true;
        }
    }
    return false;
}

static void btrc_Map_string_bool_set(btrc_Map_string_bool* self, char* key, bool value) {
    btrc_Map_string_bool_put(self, key, value);
}

static void btrc_Map_string_bool_merge(btrc_Map_string_bool* self, btrc_Map_string_bool* other) {
    for (int i = 0; (i < other->cap); (i++)) {
        if (other->occupied[i]) {
            btrc_Map_string_bool_put(self, other->keys[i], other->values[i]);
        }
    }
}

static int btrc_Map_string_bool_iterLen(btrc_Map_string_bool* self) {
    return self->len;
}

static char* btrc_Map_string_bool_iterGet(btrc_Map_string_bool* self, int n) {
    int count = 0;
    for (int i = 0; (i < self->cap); (i++)) {
        if (self->occupied[i]) {
            if (count == n) {
                return self->keys[i];
            }
            (count++);
        }
    }
    fprintf(stderr, "Map iterGet: index out of bounds\n");
    exit(1);
    return self->keys[0];
}

static bool btrc_Map_string_bool_iterValueAt(btrc_Map_string_bool* self, int n) {
    int count = 0;
    for (int i = 0; (i < self->cap); (i++)) {
        if (self->occupied[i]) {
            if (count == n) {
                return self->values[i];
            }
            (count++);
        }
    }
    fprintf(stderr, "Map iterValueAt: index out of bounds\n");
    exit(1);
    return self->values[0];
}

void Strings_init(Strings* self) {
    self->__rc = 1;
}

void Strings_destroy(Strings* self) {
    free(self);
}

char* Strings_copy(char* s) {
    int __fstr_1_len = snprintf(NULL, 0, "%s", s);
    char* __fstr_1_buf = __btrc_str_track(((char*)malloc((__fstr_1_len + 1))));
    snprintf(__fstr_1_buf, (__fstr_1_len + 1), "%s", s);
    __auto_type __btrc_ret_2 = __fstr_1_buf;
    return __btrc_ret_2;
}

char* Strings_replace(char* s, char* old, char* replacement) {
    if (s == NULL) {
        __auto_type __btrc_ret_4 = "";
        return __btrc_ret_4;
    }
    if ((old == NULL) || (replacement == NULL)) {
        __auto_type __btrc_ret_5 = Strings_copy(s);
        return __btrc_ret_5;
    }
    int slen = ((int)strlen(s));
    int oldlen = ((int)strlen(old));
    if (oldlen == 0) {
        __auto_type __btrc_ret_6 = Strings_copy(s);
        return __btrc_ret_6;
    }
    int replen = ((int)strlen(replacement));
    int cap = ((slen * 2) + 1);
    char* result = ((char*)malloc(cap));
    int rlen = 0;
    int i = 0;
    while (i < slen) {
        if (((i + oldlen) <= slen) && (strncmp((s + i), old, oldlen) == 0)) {
            while ((rlen + replen) >= cap) {
                (cap = (cap * 2));
                (result = ((char*)realloc(result, cap)));
            }
            memcpy((result + rlen), replacement, replen);
            (rlen = (rlen + replen));
            (i = (i + oldlen));
        } else {
            if ((rlen + 1) >= cap) {
                (cap = (cap * 2));
                (result = ((char*)realloc(result, cap)));
            }
            (result[rlen] = s[i]);
            (rlen++);
            (i++);
        }
    }
    (result[rlen] = '\0');
    return result;
}

bool Strings_isDigit(char c) {
    __auto_type __btrc_ret_7 = ((c >= '0') && (c <= '9'));
    return __btrc_ret_7;
}

bool Strings_isAlpha(char c) {
    __auto_type __btrc_ret_8 = (((c >= 'a') && (c <= 'z')) || ((c >= 'A') && (c <= 'Z')));
    return __btrc_ret_8;
}

int Strings_toInt(char* s) {
    if (s == NULL) {
        __auto_type __btrc_ret_11 = 0;
        return __btrc_ret_11;
    }
    char* value = __btrc_str_track(__btrc_trim(s));
    if (__btrc_isEmpty(value)) {
        __auto_type __btrc_ret_12 = 0;
        return __btrc_ret_12;
    }
    int sign = 1;
    int i = 0;
    if (__btrc_startsWith(value, "-")) {
        (sign = (-1));
        (i = 1);
    } else if (__btrc_startsWith(value, "+")) {
        (i = 1);
    }
    int result = 0;
    while ((i < ((int)strlen(value))) && Strings_isDigit(value[i])) {
        (result = ((result * 10) + (value[i] - '0')));
        (i++);
    }
    __auto_type __btrc_ret_13 = (result * sign);
    return __btrc_ret_13;
}

int Strings_find(char* s, char* sub, int start) {
    int slen = ((int)strlen(s));
    int sublen = ((int)strlen(sub));
    if (start < 0) {
        (start = 0);
    }
    if (sublen == 0) {
        return start;
    }
    int i = start;
    while ((i + sublen) <= slen) {
        if (strncmp((s + i), sub, sublen) == 0) {
            return i;
        }
        (i++);
    }
    __auto_type __btrc_ret_16 = (-1);
    return __btrc_ret_16;
}

int Strings_compare(char* left, char* right) {
    if ((left == NULL) && (right == NULL)) {
        __auto_type __btrc_ret_18 = 0;
        return __btrc_ret_18;
    }
    if (left == NULL) {
        __auto_type __btrc_ret_19 = (-1);
        return __btrc_ret_19;
    }
    if (right == NULL) {
        __auto_type __btrc_ret_20 = 1;
        return __btrc_ret_20;
    }
    int i = 0;
    while ((left[i] != '\0') && (right[i] != '\0')) {
        if (left[i] < right[i]) {
            __auto_type __btrc_ret_21 = (-1);
            return __btrc_ret_21;
        }
        if (left[i] > right[i]) {
            __auto_type __btrc_ret_22 = 1;
            return __btrc_ret_22;
        }
        (i++);
    }
    if ((left[i] == '\0') && (right[i] == '\0')) {
        __auto_type __btrc_ret_23 = 0;
        return __btrc_ret_23;
    }
    if (left[i] == '\0') {
        __auto_type __btrc_ret_24 = (-1);
        return __btrc_ret_24;
    }
    __auto_type __btrc_ret_25 = 1;
    return __btrc_ret_25;
}

char* Strings_fromInt(int n) {
    char* buf = ((char*)malloc(32));
    snprintf(buf, 32, "%d", n);
    return buf;
}

void Console_init(Console* self) {
    self->__rc = 1;
}

void Console_destroy(Console* self) {
    free(self);
}

void File_init(File* self, char* path, char* mode) {
    self->__rc = 1;
    (self->path = path);
    (self->mode = mode);
    (self->handle = fopen(path, mode));
    (self->is_open = (self->handle != NULL));
}

File* File_new(char* path, char* mode) {
    File* self = ((File*)malloc(sizeof(File)));
    memset(self, 0, sizeof(File));
    File_init(self, path, mode);
    return self;
}

void File_destroy(File* self) {
    File_close(self);
    free(self);
}

bool File_ok(File* self) {
    __auto_type __btrc_ret_40 = self->is_open;
    return __btrc_ret_40;
}

char* File_read(File* self) {
    if (!self->is_open) {
        __auto_type __btrc_ret_41 = "";
        return __btrc_ret_41;
    }
    fseek(self->handle, 0, SEEK_END);
    long size = ftell(self->handle);
    fseek(self->handle, 0, SEEK_SET);
    char* buf = ((char*)malloc((size + 1)));
    long n = ((long)fread(buf, 1, size, self->handle));
    (buf[n] = '\0');
    return buf;
}

void File_write(File* self, char* text) {
    if (!self->is_open) {
        return;
    }
    fputs(text, self->handle);
}

void File_close(File* self) {
    if (self->is_open) {
        if (((int)strlen(self->path)) > 0) {
            fclose(self->handle);
        }
        (self->is_open = false);
    }
}

void Path_init(Path* self) {
    self->__rc = 1;
}

void Path_destroy(Path* self) {
    free(self);
}

char* Path_readAll(char* path) {
    File* f = File_new(path, "r");
    if (!File_ok(f)) {
        __auto_type __btrc_ret_49 = "";
        if (f != NULL) {
            if ((--f->__rc) <= 0) {
                File_destroy(f);
            }
        }
        return __btrc_ret_49;
    }
    char* content = File_read(f);
    File_close(f);
    if (f != NULL) {
        if ((--f->__rc) <= 0) {
            File_destroy(f);
        }
    }
    return content;
    if (f != NULL) {
        if ((--f->__rc) <= 0) {
            File_destroy(f);
        }
    }
}

void Path_writeAll(char* path, char* content) {
    File* f = File_new(path, "w");
    if (!File_ok(f)) {
        if (f != NULL) {
            if ((--f->__rc) <= 0) {
                File_destroy(f);
            }
        }
        return;
    }
    File_write(f, content);
    File_close(f);
    if (f != NULL) {
        if ((--f->__rc) <= 0) {
            File_destroy(f);
        }
    }
}

void UnixPlatform_init(UnixPlatform* self) {
    self->__rc = 1;
}

void UnixPlatform_destroy(UnixPlatform* self) {
    free(self);
}

int UnixPlatform_pid(void) {
    __auto_type __btrc_ret_50 = ((int)getpid());
    return __btrc_ret_50;
}

int UnixPlatform_euid(void) {
    __auto_type __btrc_ret_51 = ((int)geteuid());
    return __btrc_ret_51;
}

void Platform_init(Platform* self) {
    self->__rc = 1;
}

void Platform_destroy(Platform* self) {
    free(self);
}

int Platform_pid(void) {
    __auto_type __btrc_ret_55 = UnixPlatform_pid();
    return __btrc_ret_55;
}

int Platform_euid(void) {
    __auto_type __btrc_ret_56 = UnixPlatform_euid();
    return __btrc_ret_56;
}

void Environment_init(Environment* self) {
    self->__rc = 1;
}

void Environment_destroy(Environment* self) {
    free(self);
}

char* Environment_get(char* name, char* fallback) {
    char* value = getenv(name);
    if ((value == NULL) || __btrc_isEmpty(value)) {
        return fallback;
    }
    __auto_type __btrc_ret_58 = Strings_copy(value);
    return __btrc_ret_58;
}

void ProcessStatus_init(ProcessStatus* self, int raw) {
    self->__rc = 1;
    (self->raw = raw);
}

ProcessStatus* ProcessStatus_new(int raw) {
    ProcessStatus* self = ((ProcessStatus*)malloc(sizeof(ProcessStatus)));
    memset(self, 0, sizeof(ProcessStatus));
    ProcessStatus_init(self, raw);
    return self;
}

void ProcessStatus_destroy(ProcessStatus* self) {
    free(self);
}

int ProcessStatus_code(ProcessStatus* self) {
    if (self->raw == (-1)) {
        __auto_type __btrc_ret_60 = 127;
        return __btrc_ret_60;
    }
    if (self->raw > 255) {
        __auto_type __btrc_ret_61 = __btrc_div_int(self->raw, 256);
        return __btrc_ret_61;
    }
    __auto_type __btrc_ret_62 = self->raw;
    return __btrc_ret_62;
}

void UnixPipe_init(UnixPipe* self, char* command) {
    self->__rc = 1;
    (self->command = command);
    (self->handle = popen(command, "r"));
}

UnixPipe* UnixPipe_new(char* command) {
    UnixPipe* self = ((UnixPipe*)malloc(sizeof(UnixPipe)));
    memset(self, 0, sizeof(UnixPipe));
    UnixPipe_init(self, command);
    return self;
}

void UnixPipe_destroy(UnixPipe* self) {
    if (self->handle != NULL) {
        pclose(self->handle);
        (self->handle = NULL);
    }
    free(self);
}

bool UnixPipe_ok(UnixPipe* self) {
    __auto_type __btrc_ret_64 = (self->handle != NULL);
    return __btrc_ret_64;
}

char* UnixPipe_readAll(UnixPipe* self) {
    if (!UnixPipe_ok(self)) {
        __auto_type __btrc_ret_65 = "";
        return __btrc_ret_65;
    }
    int cap = 4096;
    int len = 0;
    char* buffer = ((char*)malloc(cap));
    int ch = fgetc(self->handle);
    while (ch != EOF) {
        if ((len + 2) >= cap) {
            (cap = (cap * 2));
            (buffer = ((char*)realloc(buffer, cap)));
        }
        (buffer[len] = ((char)ch));
        (len++);
        (ch = fgetc(self->handle));
    }
    (buffer[len] = '\0');
    return buffer;
}

ProcessStatus* UnixPipe_close(UnixPipe* self) {
    if (!UnixPipe_ok(self)) {
        __auto_type __btrc_ret_66 = ProcessStatus_new((-1));
        return __btrc_ret_66;
    }
    int raw = pclose(self->handle);
    (self->handle = NULL);
    __auto_type __btrc_ret_67 = ProcessStatus_new(raw);
    return __btrc_ret_67;
}

void UnixProcess_init(UnixProcess* self) {
    self->__rc = 1;
}

void UnixProcess_destroy(UnixProcess* self) {
    free(self);
}

ProcessStatus* UnixProcess_system(char* command) {
    __auto_type __btrc_ret_68 = ProcessStatus_new(system(command));
    return __btrc_ret_68;
}

UnixPipe* UnixProcess_pipe(char* command) {
    __auto_type __btrc_ret_69 = UnixPipe_new(command);
    return __btrc_ret_69;
}

void ShellWords_init(ShellWords* self) {
    self->__rc = 1;
}

void ShellWords_destroy(ShellWords* self) {
    free(self);
}

bool ShellWords_isSafeArgChar(char c) {
    if ((c >= 'a') && (c <= 'z')) {
        __auto_type __btrc_ret_70 = true;
        return __btrc_ret_70;
    }
    if ((c >= 'A') && (c <= 'Z')) {
        __auto_type __btrc_ret_71 = true;
        return __btrc_ret_71;
    }
    if ((c >= '0') && (c <= '9')) {
        __auto_type __btrc_ret_72 = true;
        return __btrc_ret_72;
    }
    __auto_type __btrc_ret_73 = ((((((((c == '_') || (c == '-')) || (c == '.')) || (c == '/')) || (c == ':')) || (c == '=')) || (c == ',')) || (c == '+'));
    return __btrc_ret_73;
}

bool ShellWords_isSafeArg(char* raw) {
    int len = ((int)strlen(raw));
    if (len == 0) {
        __auto_type __btrc_ret_74 = false;
        return __btrc_ret_74;
    }
    for (int __i_75 = 0; (raw[__i_75] != '\0'); (__i_75++)) {
        char ch = raw[__i_75];
        if (!ShellWords_isSafeArgChar(ch)) {
            __auto_type __btrc_ret_76 = false;
            return __btrc_ret_76;
        }
    }
    __auto_type __btrc_ret_77 = true;
    return __btrc_ret_77;
}

char* ShellWords_quote(char* raw) {
    if (ShellWords_isSafeArg(raw)) {
        __auto_type __btrc_ret_78 = Strings_copy(raw);
        return __btrc_ret_78;
    }
    char* escaped = Strings_replace(raw, "'", "'\\''");
    __auto_type __btrc_ret_79 = __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("'", escaped)), "'"));
    return __btrc_ret_79;
}

char* ShellWords_redact(char* text, char* sensitive) {
    if (__btrc_isEmpty(sensitive)) {
        return text;
    }
    __auto_type __btrc_ret_80 = Strings_replace(text, sensitive, "***");
    return __btrc_ret_80;
}

void ExecResult_init(ExecResult* self, int code, char* out, char* err, char* command) {
    self->__rc = 1;
    (self->code = code);
    (self->out = out);
    (self->err = err);
    (self->command = command);
}

ExecResult* ExecResult_new(int code, char* out, char* err, char* command) {
    ExecResult* self = ((ExecResult*)malloc(sizeof(ExecResult)));
    memset(self, 0, sizeof(ExecResult));
    ExecResult_init(self, code, out, err, command);
    return self;
}

void ExecResult_destroy(ExecResult* self) {
    free(self);
}

char* ExecResult_stdout(ExecResult* self) {
    __auto_type __btrc_ret_82 = self->out;
    return __btrc_ret_82;
}

void Command_init(Command* self, char* executable) {
    self->__rc = 1;
    (self->executable = executable);
    if (self->args != NULL) {
        if ((--self->args->__rc) <= 0) {
            btrc_Vector_string_free(self->args);
        }
    }
    btrc_Vector_string* __list_85 = btrc_Vector_string_new();
    (self->args = __list_85);
    btrc_Vector_string* __list_84 = btrc_Vector_string_new();
    (__list_84->__rc++);
    if (self->env != NULL) {
        if ((--self->env->__rc) <= 0) {
            btrc_Vector_string_free(self->env);
        }
    }
    btrc_Vector_string* __list_87 = btrc_Vector_string_new();
    (self->env = __list_87);
    btrc_Vector_string* __list_86 = btrc_Vector_string_new();
    (__list_86->__rc++);
    (self->useSudo = false);
    (self->captureOutput = true);
    (self->checkStatus = true);
    (self->mergeStderr = true);
    (self->sensitive = "");
}

Command* Command_new(char* executable) {
    Command* self = ((Command*)malloc(sizeof(Command)));
    memset(self, 0, sizeof(Command));
    Command_init(self, executable);
    return self;
}

void Command_destroy(Command* self) {
    if (self->args != NULL) {
        if ((--self->args->__rc) <= 0) {
            btrc_Vector_string_free(self->args);
        }
    }
    if (self->env != NULL) {
        if ((--self->env->__rc) <= 0) {
            btrc_Vector_string_free(self->env);
        }
    }
    if (__btrc_tracking) {
        __btrc_mark_destroyed(self);
    }
    free(self);
}

Command* Command_arg(Command* self, char* value) {
    btrc_Vector_string_push(self->args, value);
    __auto_type __btrc_ret_88 = self;
    return __btrc_ret_88;
}

Command* Command_capture(Command* self, bool enabled) {
    (self->captureOutput = enabled);
    __auto_type __btrc_ret_94 = self;
    return __btrc_ret_94;
}

Command* Command_check(Command* self, bool enabled) {
    (self->checkStatus = enabled);
    __auto_type __btrc_ret_95 = self;
    return __btrc_ret_95;
}

char* Command_renderEnv(Command* self, char* item) {
    int split = Strings_find(item, "=", 0);
    if (split <= 0) {
        __auto_type __btrc_ret_98 = ShellWords_quote(item);
        return __btrc_ret_98;
    }
    char* name = __btrc_str_track(__btrc_substring(item, 0, split));
    char* value = __btrc_str_track(__btrc_substring(item, (split + 1), ((((int)strlen(item)) - split) - 1)));
    __auto_type __btrc_ret_99 = __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(name, "=")), ShellWords_quote(value)));
    return __btrc_ret_99;
}

char* Command_render(Command* self) {
    btrc_Vector_string* parts = btrc_Vector_string_new();
    int __n_101 = btrc_Vector_string_iterLen(self->env);
    for (int __i_100 = 0; (__i_100 < __n_101); (__i_100++)) {
        char* item = btrc_Vector_string_iterGet(self->env, __i_100);
        btrc_Vector_string_push(parts, Command_renderEnv(self, item));
    }
    if (self->useSudo) {
        btrc_Vector_string_push(parts, "sudo");
    }
    btrc_Vector_string_push(parts, ShellWords_quote(self->executable));
    int __n_103 = btrc_Vector_string_iterLen(self->args);
    for (int __i_102 = 0; (__i_102 < __n_103); (__i_102++)) {
        char* item = btrc_Vector_string_iterGet(self->args, __i_102);
        btrc_Vector_string_push(parts, ShellWords_quote(item));
    }
    if (self->mergeStderr) {
        btrc_Vector_string_push(parts, "2>&1");
    }
    __auto_type __btrc_ret_104 = btrc_Vector_string_join(parts, " ");
    return __btrc_ret_104;
}

void UnixShell_init(UnixShell* self) {
    self->__rc = 1;
    (self->logCommands = false);
    (self->chrootPath = "");
}

UnixShell* UnixShell_new(void) {
    UnixShell* self = ((UnixShell*)malloc(sizeof(UnixShell)));
    memset(self, 0, sizeof(UnixShell));
    UnixShell_init(self);
    return self;
}

void UnixShell_destroy(UnixShell* self) {
    free(self);
}

char* UnixShell_quote(char* raw) {
    __auto_type __btrc_ret_105 = ShellWords_quote(raw);
    return __btrc_ret_105;
}

char* UnixShell_redactText(char* text, char* sensitive) {
    __auto_type __btrc_ret_106 = ShellWords_redact(text, sensitive);
    return __btrc_ret_106;
}

ExecResult* UnixShell_run(UnixShell* self, char* command) {
    __auto_type __btrc_ret_107 = UnixShell_runRaw(self, command, true, true, "");
    return __btrc_ret_107;
}

ExecResult* UnixShell_runUnchecked(UnixShell* self, char* command) {
    __auto_type __btrc_ret_108 = UnixShell_runRaw(self, command, true, false, "");
    return __btrc_ret_108;
}

ExecResult* UnixShell_runCommand(UnixShell* self, Command* command) {
    __auto_type __btrc_ret_109 = UnixShell_runRaw(self, Command_render(command), command->captureOutput, command->checkStatus, command->sensitive);
    return __btrc_ret_109;
}

ExecResult* UnixShell_runRaw(UnixShell* self, char* command, bool captureOutput, bool checkStatus, char* sensitive) {
    char* rendered = command;
    if (((int)strlen(self->chrootPath)) > 0) {
        int __fstr_110_len = snprintf(NULL, 0, "nixos-enter --root %s --command %s", ShellWords_quote(self->chrootPath), ShellWords_quote(command));
        char* __fstr_110_buf = __btrc_str_track(((char*)malloc((__fstr_110_len + 1))));
        snprintf(__fstr_110_buf, (__fstr_110_len + 1), "nixos-enter --root %s --command %s", ShellWords_quote(self->chrootPath), ShellWords_quote(command));
        (rendered = __fstr_110_buf);
    }
    if (self->logCommands) {
        char* visible = UnixShell_redactText(rendered, sensitive);
        fprintf(stderr, "LOG: %s\n", visible);
    }
    if (!captureOutput) {
        ProcessStatus* status = UnixProcess_system(rendered);
        int code = ProcessStatus_code(status);
        if (checkStatus && (code != 0)) {
            fprintf(stderr, "Command failed (%d): %s\n", code, UnixShell_redactText(rendered, sensitive));
        }
        __auto_type __btrc_ret_111 = ExecResult_new(code, "", "", rendered);
        return __btrc_ret_111;
    }
    UnixPipe* pipe = UnixProcess_pipe(rendered);
    if (!UnixPipe_ok(pipe)) {
        __auto_type __btrc_ret_112 = ExecResult_new(127, "", "popen failed", rendered);
        return __btrc_ret_112;
    }
    char* output = UnixPipe_readAll(pipe);
    ProcessStatus* status = UnixPipe_close(pipe);
    int code = ProcessStatus_code(status);
    if (checkStatus && (code != 0)) {
        fprintf(stderr, "Command failed (%d): %s\n", code, UnixShell_redactText(rendered, sensitive));
    }
    __auto_type __btrc_ret_113 = ExecResult_new(code, output, "", rendered);
    return __btrc_ret_113;
}

void PowerShell_init(PowerShell* self) {
    self->__rc = 1;
}

void PowerShell_destroy(PowerShell* self) {
    free(self);
}

void UnixPamPassword_init(UnixPamPassword* self) {
    self->__rc = 1;
}

void UnixPamPassword_destroy(UnixPamPassword* self) {
    free(self);
}

void FileStatus_init(FileStatus* self, char* path) {
    self->__rc = 1;
    (self->path = path);
    struct stat st;
    (self->found = (stat(path, (&st)) == 0));
    (self->mode = (self->found ? ((int)st.st_mode) : 0));
    struct stat lst;
    (self->linkFound = (lstat(path, (&lst)) == 0));
    (self->linkMode = (self->linkFound ? ((int)lst.st_mode) : 0));
}

FileStatus* FileStatus_new(char* path) {
    FileStatus* self = ((FileStatus*)malloc(sizeof(FileStatus)));
    memset(self, 0, sizeof(FileStatus));
    FileStatus_init(self, path);
    return self;
}

void FileStatus_destroy(FileStatus* self) {
    free(self);
}

bool FileStatus_exists(FileStatus* self) {
    __auto_type __btrc_ret_118 = self->found;
    return __btrc_ret_118;
}

bool FileStatus_isDir(FileStatus* self) {
    __auto_type __btrc_ret_119 = (self->found && S_ISDIR(self->mode));
    return __btrc_ret_119;
}

bool FileStatus_isFile(FileStatus* self) {
    __auto_type __btrc_ret_120 = (self->found && S_ISREG(self->mode));
    return __btrc_ret_120;
}

bool FileStatus_isSymlink(FileStatus* self) {
    __auto_type __btrc_ret_121 = (self->linkFound && S_ISLNK(self->linkMode));
    return __btrc_ret_121;
}

void Directory_init(Directory* self, char* path) {
    self->__rc = 1;
    (self->path = path);
}

Directory* Directory_new(char* path) {
    Directory* self = ((Directory*)malloc(sizeof(Directory)));
    memset(self, 0, sizeof(Directory));
    Directory_init(self, path);
    return self;
}

void Directory_destroy(Directory* self) {
    free(self);
}

btrc_Vector_string* Directory_entries(Directory* self) {
    btrc_Vector_string* result = btrc_Vector_string_new();
    DIR* dir = opendir(self->path);
    if (dir == NULL) {
        return result;
    }
    struct dirent* entry = readdir(dir);
    while (entry != NULL) {
        char* name = entry->d_name;
        if ((!(strcmp(name, ".") == 0)) && (!(strcmp(name, "..") == 0))) {
            btrc_Vector_string_push(result, Strings_copy(name));
        }
        (entry = readdir(dir));
    }
    closedir(dir);
    return result;
}

void UnixFileSystem_init(UnixFileSystem* self) {
    self->__rc = 1;
}

void UnixFileSystem_destroy(UnixFileSystem* self) {
    free(self);
}

int UnixFileSystem_statusCode(int raw) {
    if (raw == (-1)) {
        __auto_type __btrc_ret_122 = 127;
        return __btrc_ret_122;
    }
    if (raw > 255) {
        __auto_type __btrc_ret_123 = __btrc_div_int(raw, 256);
        return __btrc_ret_123;
    }
    return raw;
}

int UnixFileSystem_chmodPath(char* path, int mode) {
    __auto_type __btrc_ret_124 = chmod(path, ((mode_t)mode));
    return __btrc_ret_124;
}

int UnixFileSystem_mkdirPath(char* path, int mode) {
    __auto_type __btrc_ret_125 = mkdir(path, ((mode_t)mode));
    return __btrc_ret_125;
}

int UnixFileSystem_runShell(char* command) {
    __auto_type __btrc_ret_126 = UnixFileSystem_statusCode(system(command));
    return __btrc_ret_126;
}

int UnixFileSystem_mkdirp(char* path) {
    char* quoted = PathTools_shellQuote(path);
    int __fstr_127_len = snprintf(NULL, 0, "mkdir -p %s", quoted);
    char* __fstr_127_buf = __btrc_str_track(((char*)malloc((__fstr_127_len + 1))));
    snprintf(__fstr_127_buf, (__fstr_127_len + 1), "mkdir -p %s", quoted);
    __auto_type __btrc_ret_128 = UnixFileSystem_runShell(__fstr_127_buf);
    return __btrc_ret_128;
}

int UnixFileSystem_removeRecursive(char* path) {
    char* quoted = PathTools_shellQuote(path);
    int __fstr_129_len = snprintf(NULL, 0, "rm -rf %s", quoted);
    char* __fstr_129_buf = __btrc_str_track(((char*)malloc((__fstr_129_len + 1))));
    snprintf(__fstr_129_buf, (__fstr_129_len + 1), "rm -rf %s", quoted);
    __auto_type __btrc_ret_130 = UnixFileSystem_runShell(__fstr_129_buf);
    return __btrc_ret_130;
}

int UnixFileSystem_symlinkPath(char* target, char* linkPath) {
    __auto_type __btrc_ret_131 = symlink(target, linkPath);
    return __btrc_ret_131;
}

char* UnixFileSystem_readLink(char* path) {
    char buffer[4096];
    ssize_t length = readlink(path, buffer, 4095);
    if (length < 0) {
        __auto_type __btrc_ret_132 = "";
        return __btrc_ret_132;
    }
    (buffer[length] = '\0');
    __auto_type __btrc_ret_133 = Strings_copy(buffer);
    return __btrc_ret_133;
}

char* UnixFileSystem_tempDir(char* prefix) {
    char* base = Environment_get("TMPDIR", "/tmp");
    char* templatePath = PathTools_join(base, __btrc_str_track(__btrc_strcat(prefix, ".XXXXXX")));
    char* raw = Strings_copy(templatePath);
    char* result = mkdtemp(raw);
    if (result == NULL) {
        __auto_type __btrc_ret_134 = "";
        return __btrc_ret_134;
    }
    __auto_type __btrc_ret_135 = Strings_copy(result);
    return __btrc_ret_135;
}

void PathTools_init(PathTools* self) {
    self->__rc = 1;
}

void PathTools_destroy(PathTools* self) {
    free(self);
}

char* PathTools_shellQuote(char* raw) {
    __auto_type __btrc_ret_136 = ShellWords_quote(raw);
    return __btrc_ret_136;
}

char* PathTools_join(char* left, char* right) {
    if (((int)strlen(left)) == 0) {
        __auto_type __btrc_ret_141 = Strings_copy(right);
        return __btrc_ret_141;
    }
    if (((int)strlen(right)) == 0) {
        __auto_type __btrc_ret_142 = Strings_copy(left);
        return __btrc_ret_142;
    }
    if (left[(((int)strlen(left)) - 1)] == '/') {
        int __fstr_143_len = snprintf(NULL, 0, "%s%s", left, right);
        char* __fstr_143_buf = __btrc_str_track(((char*)malloc((__fstr_143_len + 1))));
        snprintf(__fstr_143_buf, (__fstr_143_len + 1), "%s%s", left, right);
        __auto_type __btrc_ret_144 = __fstr_143_buf;
        return __btrc_ret_144;
    }
    int __fstr_145_len = snprintf(NULL, 0, "%s/%s", left, right);
    char* __fstr_145_buf = __btrc_str_track(((char*)malloc((__fstr_145_len + 1))));
    snprintf(__fstr_145_buf, (__fstr_145_len + 1), "%s/%s", left, right);
    __auto_type __btrc_ret_146 = __fstr_145_buf;
    return __btrc_ret_146;
}

void FileSystem_init(FileSystem* self) {
    self->__rc = 1;
}

void FileSystem_destroy(FileSystem* self) {
    free(self);
}

int FileSystem_chmod(char* path, int mode) {
    __auto_type __btrc_ret_147 = UnixFileSystem_chmodPath(path, mode);
    return __btrc_ret_147;
}

int FileSystem_mkdir(char* path, int mode) {
    __auto_type __btrc_ret_148 = UnixFileSystem_mkdirPath(path, mode);
    return __btrc_ret_148;
}

int FileSystem_mkdirp(char* path) {
    __auto_type __btrc_ret_149 = UnixFileSystem_mkdirp(path);
    return __btrc_ret_149;
}

int FileSystem_removeRecursive(char* path) {
    __auto_type __btrc_ret_150 = UnixFileSystem_removeRecursive(path);
    return __btrc_ret_150;
}

int FileSystem_symlink(char* target, char* linkPath) {
    __auto_type __btrc_ret_151 = UnixFileSystem_symlinkPath(target, linkPath);
    return __btrc_ret_151;
}

char* FileSystem_readLink(char* path) {
    __auto_type __btrc_ret_152 = UnixFileSystem_readLink(path);
    return __btrc_ret_152;
}

char* FileSystem_tempDir(char* prefix) {
    __auto_type __btrc_ret_153 = UnixFileSystem_tempDir(prefix);
    return __btrc_ret_153;
}

void FileSystem_writeText(char* path, char* content) {
    Path_writeAll(path, content);
}

void DaemonSpec_init(DaemonSpec* self, char* name, Command* command) {
    self->__rc = 1;
    (self->name = name);
    if (self->command != NULL) {
        if ((--self->command->__rc) <= 0) {
            Command_destroy(self->command);
        }
    }
    (self->command = command);
    (command->__rc++);
    int __fstr_155_len = snprintf(NULL, 0, "/tmp/%s.pid", name);
    char* __fstr_155_buf = __btrc_str_track(((char*)malloc((__fstr_155_len + 1))));
    snprintf(__fstr_155_buf, (__fstr_155_len + 1), "/tmp/%s.pid", name);
    (self->pidFile = __fstr_155_buf);
    int __fstr_156_len = snprintf(NULL, 0, "/tmp/%s.log", name);
    char* __fstr_156_buf = __btrc_str_track(((char*)malloc((__fstr_156_len + 1))));
    snprintf(__fstr_156_buf, (__fstr_156_len + 1), "/tmp/%s.log", name);
    (self->logFile = __fstr_156_buf);
    (self->workingDirectory = "");
    (self->autoRestart = false);
}

void DaemonSpec_destroy(DaemonSpec* self) {
    if (self->command != NULL) {
        if ((--self->command->__rc) <= 0) {
            Command_destroy(self->command);
        }
    }
    if (__btrc_tracking) {
        __btrc_mark_destroyed(self);
    }
    free(self);
}

char* DaemonSpec_renderStartCommand(DaemonSpec* self) {
    char* rendered = Command_render(self->command);
    int __fstr_161_len = snprintf(NULL, 0, "cd %s && ", UnixShell_quote(self->workingDirectory));
    char* __fstr_161_buf = __btrc_str_track(((char*)malloc((__fstr_161_len + 1))));
    snprintf(__fstr_161_buf, (__fstr_161_len + 1), "cd %s && ", UnixShell_quote(self->workingDirectory));
    char* prefix = (__btrc_isEmpty(self->workingDirectory) ? "" : __fstr_161_buf);
    int __fstr_162_len = snprintf(NULL, 0, "%snohup %s >> %s 2>&1 & echo $! > %s", prefix, rendered, UnixShell_quote(self->logFile), UnixShell_quote(self->pidFile));
    char* __fstr_162_buf = __btrc_str_track(((char*)malloc((__fstr_162_len + 1))));
    snprintf(__fstr_162_buf, (__fstr_162_len + 1), "%snohup %s >> %s 2>&1 & echo $! > %s", prefix, rendered, UnixShell_quote(self->logFile), UnixShell_quote(self->pidFile));
    __auto_type __btrc_ret_163 = __fstr_162_buf;
    return __btrc_ret_163;
}

void DaemonController_init(DaemonController* self) {
    self->__rc = 1;
    if (self->shell != NULL) {
        if ((--self->shell->__rc) <= 0) {
            UnixShell_destroy(self->shell);
        }
    }
    (self->shell = UnixShell_new());
    (UnixShell_new()->__rc++);
}

void DaemonController_destroy(DaemonController* self) {
    if (self->shell != NULL) {
        if ((--self->shell->__rc) <= 0) {
            UnixShell_destroy(self->shell);
        }
    }
    if (__btrc_tracking) {
        __btrc_mark_destroyed(self);
    }
    free(self);
}

void AppSpec_init(AppSpec* self, char* name) {
    self->__rc = 1;
    (self->name = name);
    (self->version = "0.0.0");
}

void AppSpec_destroy(AppSpec* self) {
    free(self);
}

AppSpec* AppSpec_withVersion(AppSpec* self, char* version) {
    (self->version = version);
    __auto_type __btrc_ret_169 = self;
    return __btrc_ret_169;
}

void DaemonApp_init(DaemonApp* self, char* name, DaemonSpec* daemon) {
    self->__rc = 1;
    (self->name = name);
    (self->version = "0.0.0");
    if (self->daemon != NULL) {
        if ((--self->daemon->__rc) <= 0) {
            DaemonSpec_destroy(self->daemon);
        }
    }
    (self->daemon = daemon);
    (daemon->__rc++);
}

void DaemonApp_destroy(DaemonApp* self) {
    if (self->daemon != NULL) {
        if ((--self->daemon->__rc) <= 0) {
            DaemonSpec_destroy(self->daemon);
        }
    }
    if (__btrc_tracking) {
        __btrc_mark_destroyed(self);
    }
    free(self);
}

void Html_init(Html* self) {
    self->__rc = 1;
}

void Html_destroy(Html* self) {
    free(self);
}

char* Html_escape(char* raw) {
    char* text = Strings_replace(raw, "&", "&amp;");
    (text = Strings_replace(text, "\"", "&quot;"));
    (text = Strings_replace(text, "<", "&lt;"));
    (text = Strings_replace(text, ">", "&gt;"));
    return text;
}

void UiNode_init(UiNode* self, char* tag) {
    self->__rc = 1;
    (self->tag = tag);
    (self->textContent = "");
    (self->rawText = false);
    if (self->attributes != NULL) {
        if ((--self->attributes->__rc) <= 0) {
            btrc_Vector_string_free(self->attributes);
        }
    }
    btrc_Vector_string* __list_171 = btrc_Vector_string_new();
    (self->attributes = __list_171);
    btrc_Vector_string* __list_170 = btrc_Vector_string_new();
    (__list_170->__rc++);
    if (self->children != NULL) {
        if ((--self->children->__rc) <= 0) {
            btrc_Vector_UiNode_free(self->children);
        }
    }
    btrc_Vector_UiNode* __list_173 = btrc_Vector_UiNode_new();
    (self->children = __list_173);
    btrc_Vector_UiNode* __list_172 = btrc_Vector_UiNode_new();
    (__list_172->__rc++);
}

UiNode* UiNode_new(char* tag) {
    UiNode* self = ((UiNode*)malloc(sizeof(UiNode)));
    memset(self, 0, sizeof(UiNode));
    UiNode_init(self, tag);
    return self;
}

void UiNode_destroy(UiNode* self) {
    if (self->attributes != NULL) {
        if ((--self->attributes->__rc) <= 0) {
            btrc_Vector_string_free(self->attributes);
        }
    }
    if (self->children != NULL) {
        if ((--self->children->__rc) <= 0) {
            btrc_Vector_UiNode_free(self->children);
        }
    }
    if (__btrc_tracking) {
        __btrc_mark_destroyed(self);
    }
    free(self);
}

UiNode* UiNode_text(UiNode* self, char* value) {
    (self->textContent = value);
    (self->rawText = false);
    __auto_type __btrc_ret_174 = self;
    return __btrc_ret_174;
}

UiNode* UiNode_raw(UiNode* self, char* value) {
    (self->textContent = value);
    (self->rawText = true);
    __auto_type __btrc_ret_175 = self;
    return __btrc_ret_175;
}

UiNode* UiNode_attr(UiNode* self, char* name, char* value) {
    int __fstr_177_len = snprintf(NULL, 0, "%s='%s'", name, Html_escape(value));
    char* __fstr_177_buf = __btrc_str_track(((char*)malloc((__fstr_177_len + 1))));
    snprintf(__fstr_177_buf, (__fstr_177_len + 1), "%s='%s'", name, Html_escape(value));
    btrc_Vector_string_push(self->attributes, __fstr_177_buf);
    __auto_type __btrc_ret_178 = self;
    return __btrc_ret_178;
}

char* UiNode_renderAttributes(UiNode* self) {
    if (self->attributes->len == 0) {
        __auto_type __btrc_ret_186 = "";
        return __btrc_ret_186;
    }
    __auto_type __btrc_ret_187 = __btrc_str_track(__btrc_strcat(" ", btrc_Vector_string_join(self->attributes, " ")));
    return __btrc_ret_187;
}

bool UiNode_isVoidElement(UiNode* self) {
    __auto_type __btrc_ret_188 = ((((((strcmp(self->tag, "br") == 0) || (strcmp(self->tag, "hr") == 0)) || (strcmp(self->tag, "img") == 0)) || (strcmp(self->tag, "input") == 0)) || (strcmp(self->tag, "link") == 0)) || (strcmp(self->tag, "meta") == 0));
    return __btrc_ret_188;
}

char* UiNode_renderHtml(UiNode* self) {
    char* attrs = UiNode_renderAttributes(self);
    if (UiNode_isVoidElement(self)) {
        int __fstr_189_len = snprintf(NULL, 0, "<%s%s>", self->tag, attrs);
        char* __fstr_189_buf = __btrc_str_track(((char*)malloc((__fstr_189_len + 1))));
        snprintf(__fstr_189_buf, (__fstr_189_len + 1), "<%s%s>", self->tag, attrs);
        __auto_type __btrc_ret_190 = __fstr_189_buf;
        return __btrc_ret_190;
    }
    char* body = "";
    if (!__btrc_isEmpty(self->textContent)) {
        (body = (self->rawText ? self->textContent : Html_escape(self->textContent)));
    }
    int __n_192 = btrc_Vector_UiNode_iterLen(self->children);
    for (int __i_191 = 0; (__i_191 < __n_192); (__i_191++)) {
        UiNode* node = btrc_Vector_UiNode_iterGet(self->children, __i_191);
        int __fstr_193_len = snprintf(NULL, 0, "%s%s", body, UiNode_renderHtml(node));
        char* __fstr_193_buf = __btrc_str_track(((char*)malloc((__fstr_193_len + 1))));
        snprintf(__fstr_193_buf, (__fstr_193_len + 1), "%s%s", body, UiNode_renderHtml(node));
        (body = __fstr_193_buf);
    }
    int __fstr_194_len = snprintf(NULL, 0, "<%s%s>%s</%s>", self->tag, attrs, body, self->tag);
    char* __fstr_194_buf = __btrc_str_track(((char*)malloc((__fstr_194_len + 1))));
    snprintf(__fstr_194_buf, (__fstr_194_len + 1), "<%s%s>%s</%s>", self->tag, attrs, body, self->tag);
    __auto_type __btrc_ret_195 = __fstr_194_buf;
    return __btrc_ret_195;
}

void UiDocument_init(UiDocument* self, char* title, UiNode* body) {
    self->__rc = 1;
    (self->title = title);
    (self->css = "");
    if (self->body != NULL) {
        if ((--self->body->__rc) <= 0) {
            UiNode_destroy(self->body);
        }
    }
    (self->body = body);
    (body->__rc++);
}

UiDocument* UiDocument_new(char* title, UiNode* body) {
    UiDocument* self = ((UiDocument*)malloc(sizeof(UiDocument)));
    memset(self, 0, sizeof(UiDocument));
    UiDocument_init(self, title, body);
    return self;
}

void UiDocument_destroy(UiDocument* self) {
    if (self->body != NULL) {
        if ((--self->body->__rc) <= 0) {
            UiNode_destroy(self->body);
        }
    }
    if (__btrc_tracking) {
        __btrc_mark_destroyed(self);
    }
    free(self);
}

char* UiDocument_renderHtml(UiDocument* self) {
    int __fstr_197_len = snprintf(NULL, 0, "<style>%s</style>", self->css);
    char* __fstr_197_buf = __btrc_str_track(((char*)malloc((__fstr_197_len + 1))));
    snprintf(__fstr_197_buf, (__fstr_197_len + 1), "<style>%s</style>", self->css);
    char* cssBlock = (__btrc_isEmpty(self->css) ? "" : __fstr_197_buf);
    int __fstr_198_len = snprintf(NULL, 0, "<!doctype html><html><head><meta charset=\"utf-8\"><title>%s</title>%s</head><body>%s</body></html>", Html_escape(self->title), cssBlock, UiNode_renderHtml(self->body));
    char* __fstr_198_buf = __btrc_str_track(((char*)malloc((__fstr_198_len + 1))));
    snprintf(__fstr_198_buf, (__fstr_198_len + 1), "<!doctype html><html><head><meta charset=\"utf-8\"><title>%s</title>%s</head><body>%s</body></html>", Html_escape(self->title), cssBlock, UiNode_renderHtml(self->body));
    __auto_type __btrc_ret_199 = __fstr_198_buf;
    return __btrc_ret_199;
}

void UiDocument_writeHtml(UiDocument* self, char* path) {
    FileSystem_writeText(path, UiDocument_renderHtml(self));
}

void HtmlView_init(HtmlView* self, UiDocument* document) {
    self->__rc = 1;
    if (self->document != NULL) {
        if ((--self->document->__rc) <= 0) {
            UiDocument_destroy(self->document);
        }
    }
    (self->document = document);
    (document->__rc++);
}

void HtmlView_destroy(HtmlView* self) {
    if (self->document != NULL) {
        if ((--self->document->__rc) <= 0) {
            UiDocument_destroy(self->document);
        }
    }
    if (__btrc_tracking) {
        __btrc_mark_destroyed(self);
    }
    free(self);
}

void HtmlView_write(HtmlView* self, char* path) {
    UiDocument_writeHtml(self->document, path);
}

void NativeView_init(NativeView* self, UiNode* root) {
    self->__rc = 1;
    if (self->root != NULL) {
        if ((--self->root->__rc) <= 0) {
            UiNode_destroy(self->root);
        }
    }
    (self->root = root);
    (root->__rc++);
}

void NativeView_destroy(NativeView* self) {
    if (self->root != NULL) {
        if ((--self->root->__rc) <= 0) {
            UiNode_destroy(self->root);
        }
    }
    if (__btrc_tracking) {
        __btrc_mark_destroyed(self);
    }
    free(self);
}

void Window_init(Window* self, char* title, int width, int height, HtmlView* html) {
    self->__rc = 1;
    (self->title = title);
    (self->width = width);
    (self->height = height);
    if (self->html != NULL) {
        if ((--self->html->__rc) <= 0) {
            HtmlView_destroy(self->html);
        }
    }
    (self->html = html);
    (html->__rc++);
}

void Window_destroy(Window* self) {
    if (self->html != NULL) {
        if ((--self->html->__rc) <= 0) {
            HtmlView_destroy(self->html);
        }
    }
    if (__btrc_tracking) {
        __btrc_mark_destroyed(self);
    }
    free(self);
}

void TrayItem_init(TrayItem* self, char* label, char* command) {
    self->__rc = 1;
    (self->label = label);
    (self->command = command);
    (self->enabled = true);
}

TrayItem* TrayItem_new(char* label, char* command) {
    TrayItem* self = ((TrayItem*)malloc(sizeof(TrayItem)));
    memset(self, 0, sizeof(TrayItem));
    TrayItem_init(self, label, command);
    return self;
}

void TrayItem_destroy(TrayItem* self) {
    free(self);
}

void Tray_init(Tray* self, char* title) {
    self->__rc = 1;
    (self->title = title);
    (self->tooltip = title);
    (self->iconPath = "");
    if (self->items != NULL) {
        if ((--self->items->__rc) <= 0) {
            btrc_Vector_TrayItem_free(self->items);
        }
    }
    btrc_Vector_TrayItem* __list_205 = btrc_Vector_TrayItem_new();
    (self->items = __list_205);
    btrc_Vector_TrayItem* __list_204 = btrc_Vector_TrayItem_new();
    (__list_204->__rc++);
}

Tray* Tray_new(char* title) {
    Tray* self = ((Tray*)malloc(sizeof(Tray)));
    memset(self, 0, sizeof(Tray));
    Tray_init(self, title);
    return self;
}

void Tray_destroy(Tray* self) {
    if (self->items != NULL) {
        if ((--self->items->__rc) <= 0) {
            btrc_Vector_TrayItem_free(self->items);
        }
    }
    if (__btrc_tracking) {
        __btrc_mark_destroyed(self);
    }
    free(self);
}

Tray* Tray_icon(Tray* self, char* path) {
    (self->iconPath = path);
    __auto_type __btrc_ret_206 = self;
    return __btrc_ret_206;
}

Tray* Tray_tip(Tray* self, char* text) {
    (self->tooltip = text);
    __auto_type __btrc_ret_207 = self;
    return __btrc_ret_207;
}

Tray* Tray_item(Tray* self, char* label, char* command) {
    btrc_Vector_TrayItem_push(self->items, TrayItem_new(label, command));
    __auto_type __btrc_ret_208 = self;
    return __btrc_ret_208;
}

Tray* Tray_add(Tray* self, TrayItem* item) {
    btrc_Vector_TrayItem_push(self->items, item);
    __auto_type __btrc_ret_209 = self;
    return __btrc_ret_209;
}

void HtmlUiBackend_init(HtmlUiBackend* self, char* opener) {
    self->__rc = 1;
    (self->opener = opener);
}

HtmlUiBackend* HtmlUiBackend_new(char* opener) {
    HtmlUiBackend* self = ((HtmlUiBackend*)malloc(sizeof(HtmlUiBackend)));
    memset(self, 0, sizeof(HtmlUiBackend));
    HtmlUiBackend_init(self, opener);
    return self;
}

void HtmlUiBackend_destroy(HtmlUiBackend* self) {
    free(self);
}

Command* HtmlUiBackend_openCommand(HtmlUiBackend* self, char* path) {
    __auto_type __btrc_ret_210 = Command_check(Command_capture(Command_arg(Command_new(self->opener), path), false), false);
    return __btrc_ret_210;
}

ExecResult* HtmlUiBackend_openFile(HtmlUiBackend* self, char* path) {
    UnixShell* shell = UnixShell_new();
    __auto_type __btrc_ret_211 = UnixShell_runCommand(shell, HtmlUiBackend_openCommand(self, path));
    if (shell != NULL) {
        if ((--shell->__rc) <= 0) {
            UnixShell_destroy(shell);
        }
    }
    return __btrc_ret_211;
    if (shell != NULL) {
        if ((--shell->__rc) <= 0) {
            UnixShell_destroy(shell);
        }
    }
}

ExecResult* HtmlUiBackend_openWindow(HtmlUiBackend* self, Window* window, char* path) {
    HtmlView_write(window->html, path);
    __auto_type __btrc_ret_212 = HtmlUiBackend_openFile(self, path);
    return __btrc_ret_212;
}

void NativeUiBackend_init(NativeUiBackend* self, char* name, HtmlUiBackend* htmlBackend) {
    self->__rc = 1;
    (self->name = name);
    if (self->htmlBackend != NULL) {
        if ((--self->htmlBackend->__rc) <= 0) {
            HtmlUiBackend_destroy(self->htmlBackend);
        }
    }
    (self->htmlBackend = htmlBackend);
    (htmlBackend->__rc++);
}

NativeUiBackend* NativeUiBackend_new(char* name, HtmlUiBackend* htmlBackend) {
    NativeUiBackend* self = ((NativeUiBackend*)malloc(sizeof(NativeUiBackend)));
    memset(self, 0, sizeof(NativeUiBackend));
    NativeUiBackend_init(self, name, htmlBackend);
    return self;
}

void NativeUiBackend_destroy(NativeUiBackend* self) {
    if (self->htmlBackend != NULL) {
        if ((--self->htmlBackend->__rc) <= 0) {
            HtmlUiBackend_destroy(self->htmlBackend);
        }
    }
    if (__btrc_tracking) {
        __btrc_mark_destroyed(self);
    }
    free(self);
}

bool NativeUiBackend_isMac(NativeUiBackend* self) {
    __auto_type __btrc_ret_213 = (strcmp(self->name, "macos") == 0);
    return __btrc_ret_213;
}

bool NativeUiBackend_isLinux(NativeUiBackend* self) {
    __auto_type __btrc_ret_214 = (strcmp(self->name, "linux") == 0);
    return __btrc_ret_214;
}

Command* NativeUiBackend_notifyCommand(NativeUiBackend* self, char* title, char* body) {
    if (NativeUiBackend_isMac(self)) {
        char* script = __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("display notification ", NativeUi_applescriptString(body))), " with title ")), NativeUi_applescriptString(title)));
        __auto_type __btrc_ret_216 = Command_check(Command_capture(Command_arg(Command_arg(Command_new("osascript"), "-e"), script), false), false);
        return __btrc_ret_216;
    }
    if (NativeUiBackend_isLinux(self)) {
        __auto_type __btrc_ret_217 = Command_check(Command_capture(Command_arg(Command_arg(Command_new("notify-send"), title), body), false), false);
        return __btrc_ret_217;
    }
    __auto_type __btrc_ret_218 = Command_check(Command_capture(Command_arg(Command_arg(Command_arg(Command_new("powershell"), "-NoProfile"), "-Command"), "Write-Error 'Native notifications are TODO for Windows'"), false), false);
    return __btrc_ret_218;
}

Command* NativeUiBackend_alertCommand(NativeUiBackend* self, char* title, char* body) {
    if (NativeUiBackend_isMac(self)) {
        char* script = __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("display dialog ", NativeUi_applescriptString(body))), " with title ")), NativeUi_applescriptString(title))), " buttons {\"OK\"} default button \"OK\""));
        __auto_type __btrc_ret_219 = Command_check(Command_capture(Command_arg(Command_arg(Command_new("osascript"), "-e"), script), false), false);
        return __btrc_ret_219;
    }
    if (NativeUiBackend_isLinux(self)) {
        __auto_type __btrc_ret_220 = Command_check(Command_capture(Command_arg(Command_arg(Command_arg(Command_arg(Command_arg(Command_new("zenity"), "--info"), "--title"), title), "--text"), body), false), false);
        return __btrc_ret_220;
    }
    __auto_type __btrc_ret_221 = Command_check(Command_capture(Command_arg(Command_arg(Command_arg(Command_new("powershell"), "-NoProfile"), "-Command"), "Write-Error 'Native dialogs are TODO for Windows'"), false), false);
    return __btrc_ret_221;
}

ExecResult* NativeUiBackend_notify(NativeUiBackend* self, char* title, char* body) {
    __auto_type __btrc_ret_222 = UnixShell_runCommand(UnixShell_new(), NativeUiBackend_notifyCommand(self, title, body));
    return __btrc_ret_222;
}

ExecResult* NativeUiBackend_alert(NativeUiBackend* self, char* title, char* body) {
    __auto_type __btrc_ret_223 = UnixShell_runCommand(UnixShell_new(), NativeUiBackend_alertCommand(self, title, body));
    return __btrc_ret_223;
}

void LinuxUiBuilder_init(LinuxUiBuilder* self) {
    self->__rc = 1;
}

void LinuxUiBuilder_destroy(LinuxUiBuilder* self) {
    free(self);
}

HtmlUiBackend* LinuxUiBuilder_html(void) {
    __auto_type __btrc_ret_226 = HtmlUiBackend_new("xdg-open");
    return __btrc_ret_226;
}

NativeUiBackend* LinuxUiBuilder_native(void) {
    __auto_type __btrc_ret_227 = NativeUiBackend_new("linux", LinuxUiBuilder_html());
    return __btrc_ret_227;
}

void MacUiBuilder_init(MacUiBuilder* self) {
    self->__rc = 1;
}

void MacUiBuilder_destroy(MacUiBuilder* self) {
    free(self);
}

HtmlUiBackend* MacUiBuilder_html(void) {
    __auto_type __btrc_ret_228 = HtmlUiBackend_new("open");
    return __btrc_ret_228;
}

NativeUiBackend* MacUiBuilder_native(void) {
    __auto_type __btrc_ret_229 = NativeUiBackend_new("macos", MacUiBuilder_html());
    return __btrc_ret_229;
}

void WindowsUiBuilder_init(WindowsUiBuilder* self) {
    self->__rc = 1;
}

void WindowsUiBuilder_destroy(WindowsUiBuilder* self) {
    free(self);
}

HtmlUiBackend* WindowsUiBuilder_html(void) {
    __auto_type __btrc_ret_230 = HtmlUiBackend_new("powershell");
    return __btrc_ret_230;
}

NativeUiBackend* WindowsUiBuilder_native(void) {
    __auto_type __btrc_ret_231 = NativeUiBackend_new("windows", WindowsUiBuilder_html());
    return __btrc_ret_231;
}

void Ui_init(Ui* self) {
    self->__rc = 1;
}

void Ui_destroy(Ui* self) {
    free(self);
}

void NativeUi_init(NativeUi* self) {
    self->__rc = 1;
}

void NativeUi_destroy(NativeUi* self) {
    free(self);
}

char* NativeUi_applescriptString(char* raw) {
    char* escaped = Strings_replace(raw, "\\", "\\\\");
    (escaped = Strings_replace(escaped, "\"", "\\\""));
    __auto_type __btrc_ret_239 = __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("\"", escaped)), "\""));
    return __btrc_ret_239;
}

void UiRuntime_init(UiRuntime* self) {
    self->__rc = 1;
}

void UiRuntime_destroy(UiRuntime* self) {
    free(self);
}

static void* __btrc_spawn_wrapper_1(void* __arg) {
    __btrc_spawn_env_1* __env = ((__btrc_spawn_env_1*)__arg);
    Command* command = __env->command;
    ExecResult* result = UnixShell_runCommand(UnixShell_new(), command);
    __auto_type __btrc_ret_243 = result->code;
    void* __result = ((void*)((intptr_t)__btrc_ret_243));
    if (command != NULL) {
        if ((--command->__rc) <= 0) {
            Command_destroy(command);
        }
    }
    free(__env);
    return __result;
}

static void* __btrc_spawn_wrapper_2(void* __arg) {
    __btrc_spawn_env_2* __env = ((__btrc_spawn_env_2*)__arg);
    NativeUiBackend* backend = __env->backend;
    char* body = __env->body;
    char* title = __env->title;
    ExecResult* result = NativeUiBackend_notify(backend, title, body);
    __auto_type __btrc_ret_245 = result->code;
    void* __result = ((void*)((intptr_t)__btrc_ret_245));
    if (backend != NULL) {
        if ((--backend->__rc) <= 0) {
            NativeUiBackend_destroy(backend);
        }
    }
    free(__env);
    return __result;
}

void Signal_init(Signal* self) {
    self->__rc = 1;
    if (self->events != NULL) {
        if ((--self->events->__rc) <= 0) {
            btrc_Vector_string_free(self->events);
        }
    }
    btrc_Vector_string* __list_248 = btrc_Vector_string_new();
    (self->events = __list_248);
    btrc_Vector_string* __list_247 = btrc_Vector_string_new();
    (__list_247->__rc++);
}

void Signal_destroy(Signal* self) {
    if (self->events != NULL) {
        if ((--self->events->__rc) <= 0) {
            btrc_Vector_string_free(self->events);
        }
    }
    if (__btrc_tracking) {
        __btrc_mark_destroyed(self);
    }
    free(self);
}

void JsonObject_init(JsonObject* self) {
    self->__rc = 1;
    if (self->values != NULL) {
        if ((--self->values->__rc) <= 0) {
            btrc_Map_string_string_free(self->values);
        }
    }
    (self->values = btrc_Map_string_string_new());
    (btrc_Map_string_string_new()->__rc++);
    if (self->quoted != NULL) {
        if ((--self->quoted->__rc) <= 0) {
            btrc_Map_string_bool_free(self->quoted);
        }
    }
    (self->quoted = btrc_Map_string_bool_new());
    (btrc_Map_string_bool_new()->__rc++);
}

JsonObject* JsonObject_new(void) {
    JsonObject* self = ((JsonObject*)malloc(sizeof(JsonObject)));
    memset(self, 0, sizeof(JsonObject));
    JsonObject_init(self);
    return self;
}

void JsonObject_destroy(JsonObject* self) {
    if (self->values != NULL) {
        if ((--self->values->__rc) <= 0) {
            btrc_Map_string_string_free(self->values);
        }
    }
    if (self->quoted != NULL) {
        if ((--self->quoted->__rc) <= 0) {
            btrc_Map_string_bool_free(self->quoted);
        }
    }
    if (__btrc_tracking) {
        __btrc_mark_destroyed(self);
    }
    free(self);
}

char* JsonObject_escape(char* text) {
    if (text == NULL) {
        __auto_type __btrc_ret_252 = "";
        return __btrc_ret_252;
    }
    char* escaped = Strings_replace(text, "\\", "\\\\");
    (escaped = Strings_replace(escaped, "\"", "\\\""));
    (escaped = Strings_replace(escaped, "\n", "\\n"));
    return escaped;
}

char* JsonObject_unescape(char* text) {
    char* result = "";
    bool escaped = false;
    for (int i = 0; (i < ((int)strlen(text))); (i++)) {
        char* current = __btrc_str_track(__btrc_substring(text, i, 1));
        if (escaped) {
            if (strcmp(current, "n") == 0) {
                (result = __btrc_str_track(__btrc_strcat(result, "\n")));
            } else if (strcmp(current, "r") == 0) {
                (result = __btrc_str_track(__btrc_strcat(result, "\r")));
            } else if (strcmp(current, "t") == 0) {
                (result = __btrc_str_track(__btrc_strcat(result, "\t")));
            } else {
                (result = __btrc_str_track(__btrc_strcat(result, current)));
            }
            (escaped = false);
            continue;
        }
        if (strcmp(current, "\\") == 0) {
            (escaped = true);
            continue;
        }
        (result = __btrc_str_track(__btrc_strcat(result, current)));
    }
    if (escaped) {
        (result = __btrc_str_track(__btrc_strcat(result, "\\")));
    }
    return result;
}

void JsonObject_setString(JsonObject* self, char* key, char* value) {
    btrc_Map_string_string_put(self->values, key, value);
    btrc_Map_string_bool_put(self->quoted, key, true);
}

void JsonObject_setRaw(JsonObject* self, char* key, char* value) {
    btrc_Map_string_string_put(self->values, key, value);
    btrc_Map_string_bool_put(self->quoted, key, false);
}

char* JsonObject_stringify(JsonObject* self) {
    btrc_Vector_string* fields = btrc_Vector_string_new();
    int __n_259 = btrc_Map_string_string_iterLen(self->values);
    for (int __i_258 = 0; (__i_258 < __n_259); (__i_258++)) {
        char* key = btrc_Map_string_string_iterGet(self->values, __i_258);
        char* value = btrc_Map_string_string_iterValueAt(self->values, __i_258);
        char* escapedKey = JsonObject_escape(key);
        char* field = __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("\"", escapedKey)), "\":"));
        if (btrc_Map_string_bool_getOrDefault(self->quoted, key, true)) {
            char* escapedValue = JsonObject_escape(value);
            (field = __btrc_str_track(__btrc_strcat(field, __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("\"", escapedValue)), "\"")))));
        } else {
            (field = __btrc_str_track(__btrc_strcat(field, value)));
        }
        btrc_Vector_string_push(fields, field);
    }
    __auto_type __btrc_ret_260 = __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("{", btrc_Vector_string_join(fields, ","))), "}"));
    return __btrc_ret_260;
}

int JsonObject_skipSpaces(char* text, int i) {
    int len = ((int)strlen(text));
    while ((i < len) && ((((text[i] == ' ') || (text[i] == '\n')) || (text[i] == '\t')) || (text[i] == '\r'))) {
        (i++);
    }
    return i;
}

char* JsonObject_slice(char* text, int start, int end) {
    int len = (end - start);
    char* result = ((char*)malloc((len + 1)));
    memcpy(result, (text + start), len);
    (result[len] = '\0');
    return result;
}

int JsonObject_stringEnd(char* text, int start) {
    int len = ((int)strlen(text));
    bool escaped = false;
    int i = start;
    while (i < len) {
        if ((!escaped) && (text[i] == ((char)34))) {
            return i;
        }
        (escaped = ((!escaped) && (text[i] == '\\')));
        if (text[i] != '\\') {
            (escaped = false);
        }
        (i++);
    }
    return len;
}

JsonObject* JsonObject_parse(char* text) {
    JsonObject* obj = JsonObject_new();
    int len = ((int)strlen(text));
    int i = 0;
    while (i < len) {
        (i = JsonObject_skipSpaces(text, i));
        if (i >= len) {
            break;
        }
        if (text[i] != ((char)34)) {
            (i++);
            continue;
        }
        (i++);
        int keyStart = i;
        (i = JsonObject_stringEnd(text, keyStart));
        char* key = JsonObject_unescape(JsonObject_slice(text, keyStart, i));
        (i++);
        (i = JsonObject_skipSpaces(text, i));
        if ((i < len) && (text[i] == ':')) {
            (i++);
        }
        (i = JsonObject_skipSpaces(text, i));
        if (i >= len) {
            break;
        }
        if (text[i] == ((char)34)) {
            (i++);
            int valueStart = i;
            (i = JsonObject_stringEnd(text, valueStart));
            char* value = JsonObject_unescape(JsonObject_slice(text, valueStart, i));
            JsonObject_setString(obj, key, value);
            (i++);
        } else {
            int valueStart = i;
            while (((i < len) && (text[i] != ',')) && (text[i] != '}')) {
                (i++);
            }
            int valueEnd = i;
            while ((valueEnd > valueStart) && ((((text[(valueEnd - 1)] == ' ') || (text[(valueEnd - 1)] == '\n')) || (text[(valueEnd - 1)] == '\t')) || (text[(valueEnd - 1)] == '\r'))) {
                (valueEnd--);
            }
            char* value = JsonObject_slice(text, valueStart, valueEnd);
            JsonObject_setRaw(obj, key, value);
        }
    }
    return obj;
    if (obj != NULL) {
        if ((--obj->__rc) <= 0) {
            JsonObject_destroy(obj);
        }
    }
}

void Toml_init(Toml* self) {
    self->__rc = 1;
}

void Toml_destroy(Toml* self) {
    free(self);
}

char* Toml_stripInlineComment(char* raw) {
    bool inString = false;
    bool escaped = false;
    int len = ((int)strlen(raw));
    for (int i = 0; (i < len); (i++)) {
        char c = raw[i];
        if (inString) {
            if ((!escaped) && (c == ((char)34))) {
                (inString = false);
            }
            (escaped = ((!escaped) && (c == '\\')));
            if (c != '\\') {
                (escaped = false);
            }
            continue;
        }
        if (c == ((char)34)) {
            (inString = true);
            (escaped = false);
            continue;
        }
        if (c == '#') {
            __auto_type __btrc_ret_262 = __btrc_str_track(__btrc_trim(__btrc_str_track(__btrc_substring(raw, 0, i))));
            return __btrc_ret_262;
        }
    }
    __auto_type __btrc_ret_263 = __btrc_str_track(__btrc_trim(raw));
    return __btrc_ret_263;
}

char* Toml_unquote(char* raw) {
    char* value = __btrc_str_track(__btrc_trim(raw));
    if ((__btrc_startsWith(value, "\"") && __btrc_endsWith(value, "\"")) && (((int)strlen(value)) >= 2)) {
        __auto_type __btrc_ret_264 = __btrc_str_track(__btrc_substring(value, 1, (((int)strlen(value)) - 2)));
        return __btrc_ret_264;
    }
    if ((__btrc_startsWith(value, "'") && __btrc_endsWith(value, "'")) && (((int)strlen(value)) >= 2)) {
        __auto_type __btrc_ret_265 = __btrc_str_track(__btrc_substring(value, 1, (((int)strlen(value)) - 2)));
        return __btrc_ret_265;
    }
    return value;
}

void UnixPattern_init(UnixPattern* self) {
    self->__rc = 1;
}

void UnixPattern_destroy(UnixPattern* self) {
    free(self);
}

bool UnixPattern_matches(char* pattern, char* text) {
    __auto_type __btrc_ret_275 = (fnmatch(pattern, text, 0) == 0);
    return __btrc_ret_275;
}

void Pattern_init(Pattern* self) {
    self->__rc = 1;
}

void Pattern_destroy(Pattern* self) {
    free(self);
}

bool Pattern_matches(char* pattern, char* text) {
    __auto_type __btrc_ret_276 = UnixPattern_matches(pattern, text);
    return __btrc_ret_276;
}

void Math_init(Math* self) {
    self->__rc = 1;
}

void Math_destroy(Math* self) {
    free(self);
}

int Math_abs(int x) {
    if (x < 0) {
        __auto_type __btrc_ret_285 = (-x);
        return __btrc_ret_285;
    }
    return x;
}

int Math_factorial(int n) {
    if (n <= 1) {
        __auto_type __btrc_ret_289 = 1;
        return __btrc_ret_289;
    }
    __auto_type __btrc_ret_290 = (n * Math_factorial((n - 1)));
    return __btrc_ret_290;
}

int Math_gcd(int a, int b) {
    while (b != 0) {
        int temp = b;
        (b = __btrc_mod_int(a, b));
        (a = temp);
    }
    return a;
}

void DateTime_init(DateTime* self, int year, int month, int day, int hour, int minute, int second) {
    self->__rc = 1;
    (self->year = year);
    (self->month = month);
    (self->day = day);
    (self->hour = hour);
    (self->minute = minute);
    (self->second = second);
}

DateTime* DateTime_new(int year, int month, int day, int hour, int minute, int second) {
    DateTime* self = ((DateTime*)malloc(sizeof(DateTime)));
    memset(self, 0, sizeof(DateTime));
    DateTime_init(self, year, month, day, hour, minute, second);
    return self;
}

void DateTime_destroy(DateTime* self) {
    free(self);
}

void Timer_init(Timer* self) {
    self->__rc = 1;
    (self->start_time = 0);
    (self->end_time = 0);
    (self->running = false);
}

void Timer_destroy(Timer* self) {
    free(self);
}

void Random_init(Random* self) {
    self->__rc = 1;
    (self->seeded = false);
}

void Random_destroy(Random* self) {
    free(self);
}

void Random_seed(Random* self, int s) {
    srand(s);
    (self->seeded = true);
}

void Random_seedTime(Random* self) {
    srand(((unsigned int)time(NULL)));
    (self->seeded = true);
}

int Random_randint(Random* self, int lo, int hi) {
    if (!self->seeded) {
        Random_seedTime(self);
    }
    __auto_type __btrc_ret_329 = (lo + (rand() % ((hi - lo) + 1)));
    return __btrc_ret_329;
}

float Random_random(Random* self) {
    if (!self->seeded) {
        Random_seedTime(self);
    }
    __auto_type __btrc_ret_330 = __btrc_div_double(((float)rand()), ((float)RAND_MAX));
    return __btrc_ret_330;
}

void Error_init(Error* self, char* message, int code) {
    self->__rc = 1;
    (self->message = message);
    (self->code = code);
}

void Error_destroy(Error* self) {
    free(self);
}

char* Error_toString(Error* self) {
    __auto_type __btrc_ret_333 = self->message;
    return __btrc_ret_333;
}

void ValueError_init(ValueError* self, char* message) {
    self->__rc = 1;
    (self->message = message);
    (self->code = 1);
}

void ValueError_destroy(ValueError* self) {
    free(self);
}

void IOError_init(IOError* self, char* message) {
    self->__rc = 1;
    (self->message = message);
    (self->code = 2);
}

void IOError_destroy(IOError* self) {
    free(self);
}

void TypeError_init(TypeError* self, char* message) {
    self->__rc = 1;
    (self->message = message);
    (self->code = 3);
}

void TypeError_destroy(TypeError* self) {
    free(self);
}

void IndexError_init(IndexError* self, char* message) {
    self->__rc = 1;
    (self->message = message);
    (self->code = 4);
}

void IndexError_destroy(IndexError* self) {
    free(self);
}

void KeyError_init(KeyError* self, char* message) {
    self->__rc = 1;
    (self->message = message);
    (self->code = 5);
}

void KeyError_destroy(KeyError* self) {
    free(self);
}

void CliArgs_init(CliArgs* self, int argc, char** argv) {
    self->__rc = 1;
    (self->program = ((argc > 0) ? Strings_copy(argv[0]) : ""));
    if (self->values != NULL) {
        if ((--self->values->__rc) <= 0) {
            btrc_Vector_string_free(self->values);
        }
    }
    btrc_Vector_string* __list_335 = btrc_Vector_string_new();
    (self->values = __list_335);
    btrc_Vector_string* __list_334 = btrc_Vector_string_new();
    (__list_334->__rc++);
    for (int i = 1; (i < argc); (i++)) {
        btrc_Vector_string_push(self->values, Strings_copy(argv[i]));
    }
}

void CliArgs_destroy(CliArgs* self) {
    if (self->values != NULL) {
        if ((--self->values->__rc) <= 0) {
            btrc_Vector_string_free(self->values);
        }
    }
    if (__btrc_tracking) {
        __btrc_mark_destroyed(self);
    }
    free(self);
}

char* CliArgs_command(CliArgs* self) {
    if (self->values->len == 0) {
        __auto_type __btrc_ret_338 = "";
        return __btrc_ret_338;
    }
    __auto_type __btrc_ret_339 = btrc_Vector_string_get(self->values, 0);
    return __btrc_ret_339;
}

void CliCommand_init(CliCommand* self, char* name) {
    self->__rc = 1;
    (self->name = name);
    if (self->aliases != NULL) {
        if ((--self->aliases->__rc) <= 0) {
            btrc_Vector_string_free(self->aliases);
        }
    }
    btrc_Vector_string* __list_350 = btrc_Vector_string_new();
    (self->aliases = __list_350);
    btrc_Vector_string* __list_349 = btrc_Vector_string_new();
    (__list_349->__rc++);
}

void CliCommand_destroy(CliCommand* self) {
    if (self->aliases != NULL) {
        if ((--self->aliases->__rc) <= 0) {
            btrc_Vector_string_free(self->aliases);
        }
    }
    if (__btrc_tracking) {
        __btrc_mark_destroyed(self);
    }
    free(self);
}

void TraySignal_init(TraySignal* self) {
    self->__rc = 1;
}

void TraySignal_destroy(TraySignal* self) {
    free(self);
}

char* TraySignal_quit(void) {
    __auto_type __btrc_ret_356 = "__quit__";
    return __btrc_ret_356;
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

void SystemTray_destroy(SystemTray* self) {
    if (self->handle != NULL) {
        btrc_tray_destroy(self->handle);
        (self->handle = NULL);
    }
    if (self->model != NULL) {
        if ((--self->model->__rc) <= 0) {
            Tray_destroy(self->model);
        }
    }
    if (self->shell != NULL) {
        if ((--self->shell->__rc) <= 0) {
            UnixShell_destroy(self->shell);
        }
    }
    if (__btrc_tracking) {
        __btrc_mark_destroyed(self);
    }
    free(self);
}

SystemTray* SystemTray_tip(SystemTray* self, char* text) {
    Tray_tip(self->model, text);
    __auto_type __btrc_ret_358 = self;
    return __btrc_ret_358;
}

SystemTray* SystemTray_item(SystemTray* self, char* label, char* command) {
    Tray_item(self->model, label, command);
    __auto_type __btrc_ret_359 = self;
    return __btrc_ret_359;
}

bool SystemTray_available(SystemTray* self) {
    __auto_type __btrc_ret_361 = (self->handle != NULL);
    return __btrc_ret_361;
}

bool SystemTray_show(SystemTray* self) {
    if (self->realized) {
        __auto_type __btrc_ret_362 = SystemTray_available(self);
        return __btrc_ret_362;
    }
    (self->handle = btrc_tray_create(self->model->title));
    if (self->handle == NULL) {
        __auto_type __btrc_ret_363 = false;
        return __btrc_ret_363;
    }
    if (!__btrc_isEmpty(self->model->iconPath)) {
        btrc_tray_set_icon(self->handle, self->model->iconPath);
    }
    btrc_tray_set_tooltip(self->handle, self->model->tooltip);
    int __n_365 = btrc_Vector_TrayItem_iterLen(self->model->items);
    for (int __i_364 = 0; (__i_364 < __n_365); (__i_364++)) {
        TrayItem* entry = btrc_Vector_TrayItem_iterGet(self->model->items, __i_364);
        btrc_tray_add_item(self->handle, entry->label, entry->command, entry->enabled);
    }
    btrc_tray_set_menu(self->handle);
    bool ok = btrc_tray_show(self->handle);
    (self->realized = ok);
    return ok;
}

bool SystemTray_pump(SystemTray* self, int timeoutMs) {
    if (self->handle == NULL) {
        __auto_type __btrc_ret_366 = false;
        return __btrc_ret_366;
    }
    bool alive = btrc_tray_run_iteration(self->handle, timeoutMs);
    char* command = btrc_tray_take_command(self->handle);
    if (command != NULL) {
        if (strcmp(command, TraySignal_quit()) == 0) {
            btrc_tray_request_quit(self->handle);
            __auto_type __btrc_ret_367 = false;
            return __btrc_ret_367;
        }
        UnixShell_runRaw(self->shell, command, false, false, "");
    }
    __auto_type __btrc_ret_368 = (alive && (!btrc_tray_should_quit(self->handle)));
    return __btrc_ret_368;
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
        __auto_type __btrc_ret_369 = 0;
        return __btrc_ret_369;
    }
    SystemTray_run(tray);
    __auto_type __btrc_ret_370 = 0;
    return __btrc_ret_370;
}

