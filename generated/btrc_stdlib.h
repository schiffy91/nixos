#ifndef BTRC_STDLIB_H
#define BTRC_STDLIB_H

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
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <fnmatch.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <pwd.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>

#define _DARWIN_C_SOURCE

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

static inline char* __btrc_toLower(const char* s) {
    if (!s) { char* r = (char*)malloc(1); r[0] = '\0'; return r; }
    int len = (int)strlen(s);
    char* result = (char*)malloc(len + 1);
    for (int i = 0; i < len; i++) result[i] = (char)tolower((unsigned char)s[i]);
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

/* shared destroyed-pointer guard (defined in the btrc stdlib archive) */
extern int __btrc_tracking;
extern void** __btrc_destroyed;
extern int __btrc_destroyed_count;
extern int __btrc_destroyed_cap;
void __btrc_mark_destroyed(void* ptr);
int __btrc_is_destroyed(void* ptr);

typedef struct {
    void* (*fn)(void*);
    void* arg;
    void* result;
    pthread_t handle;
} __btrc_thread_t;

static void* __btrc_thread_wrapper(void* raw) {
    __btrc_thread_t* t = (__btrc_thread_t*)raw;
    t->result = t->fn(t->arg);
    return NULL;
}

static __btrc_thread_t* __btrc_thread_spawn(void* (*fn)(void*), void* arg) {
    __btrc_thread_t* t = (__btrc_thread_t*)malloc(sizeof(__btrc_thread_t));
    if (!t) { fprintf(stderr, "btrc: thread alloc failed\n"); exit(1); }
    t->fn = fn;
    t->arg = arg;
    t->result = NULL;
    int err = pthread_create(&t->handle, NULL, __btrc_thread_wrapper, t);
    if (err != 0) { fprintf(stderr, "btrc: pthread_create failed\n"); free(t); exit(1); }
    return t;
}

typedef struct Strings Strings;
void Strings_destroy(Strings* self);
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
typedef struct CliArgs CliArgs;
void CliArgs_destroy(CliArgs* self);
typedef struct CliCommand CliCommand;
void CliCommand_destroy(CliCommand* self);
typedef struct Console Console;
void Console_destroy(Console* self);
typedef struct DateTime DateTime;
void DateTime_destroy(DateTime* self);
typedef struct Timer Timer;
void Timer_destroy(Timer* self);
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
typedef struct HttpRequest HttpRequest;
void HttpRequest_destroy(HttpRequest* self);
typedef struct HttpResponse HttpResponse;
void HttpResponse_destroy(HttpResponse* self);
typedef struct HttpServer HttpServer;
void HttpServer_destroy(HttpServer* self);
typedef struct HttpClientResponse HttpClientResponse;
void HttpClientResponse_destroy(HttpClientResponse* self);
typedef struct HttpClient HttpClient;
void HttpClient_destroy(HttpClient* self);
typedef struct Browser Browser;
void Browser_destroy(Browser* self);
typedef struct File File;
void File_destroy(File* self);
typedef struct Path Path;
void Path_destroy(Path* self);
typedef struct JsonObject JsonObject;
void JsonObject_destroy(JsonObject* self);
typedef struct Json Json;
void Json_destroy(Json* self);
typedef struct Math Math;
void Math_destroy(Math* self);
typedef struct UnixPattern UnixPattern;
void UnixPattern_destroy(UnixPattern* self);
typedef struct Pattern Pattern;
void Pattern_destroy(Pattern* self);
typedef struct Random Random;
void Random_destroy(Random* self);
typedef struct UnixPamPassword UnixPamPassword;
void UnixPamPassword_destroy(UnixPamPassword* self);
typedef struct Toml Toml;
void Toml_destroy(Toml* self);
typedef struct btrc_Vector_string btrc_Vector_string;
typedef struct btrc_Vector_UiNode btrc_Vector_UiNode;
typedef struct btrc_Vector_TrayItem btrc_Vector_TrayItem;
typedef struct btrc_Vector_bool btrc_Vector_bool;
typedef struct btrc_Vector_int btrc_Vector_int;
typedef struct btrc_Vector_float btrc_Vector_float;
typedef struct btrc_Vector_Map_string_string btrc_Vector_Map_string_string;
typedef struct btrc_Map_string_string btrc_Map_string_string;
typedef struct btrc_Map_string_bool btrc_Map_string_bool;
void Strings_init(Strings* self);
Strings* Strings_new(void);
char* Strings_copy(char* s);
char* Strings_repeat(char* s, int count);
char* Strings_join(btrc_Vector_string* items, char* sep);
char* Strings_replace(char* s, char* old, char* replacement);
btrc_Vector_string* Strings_split(char* s, char* delim);
bool Strings_isDigit(char c);
bool Strings_isAlpha(char c);
bool Strings_isAlnum(char c);
bool Strings_isSpace(char c);
int Strings_toInt(char* s);
float Strings_toFloat(char* s);
int Strings_count(char* s, char* sub);
int Strings_find(char* s, char* sub, int start);
int Strings_rfind(char* s, char* sub);
int Strings_compare(char* left, char* right);
bool Strings_lessThan(char* left, char* right);
char* Strings_capitalize(char* s);
char* Strings_title(char* s);
char* Strings_swapCase(char* s);
char* Strings_padLeft(char* s, int width, char fill);
char* Strings_padRight(char* s, int width, char fill);
char* Strings_center(char* s, int width, char fill);
char* Strings_lstrip(char* s);
char* Strings_rstrip(char* s);
char* Strings_removePrefix(char* s, char* prefix);
char* Strings_fromInt(int n);
char* Strings_fromFloat(float f);
bool Strings_isDigitStr(char* s);
bool Strings_isAlphaStr(char* s);
bool Strings_isBlank(char* s);
void UnixPlatform_init(UnixPlatform* self);
UnixPlatform* UnixPlatform_new(void);
int UnixPlatform_pid(void);
int UnixPlatform_euid(void);
void Platform_init(Platform* self);
Platform* Platform_new(void);
bool Platform_isUnix(void);
bool Platform_isWindows(void);
char* Platform_pathSeparator(void);
int Platform_pid(void);
int Platform_euid(void);
bool Platform_isRoot(void);
void Environment_init(Environment* self);
Environment* Environment_new(void);
char* Environment_get(char* name, char* fallback);
bool Environment_has(char* name);
FILE* popen(const char* command, const char* mode);
int pclose(FILE* stream);
void ProcessStatus_init(ProcessStatus* self, int raw);
ProcessStatus* ProcessStatus_new(int raw);
int ProcessStatus_code(ProcessStatus* self);
bool ProcessStatus_ok(ProcessStatus* self);
void UnixPipe_init(UnixPipe* self, char* command);
UnixPipe* UnixPipe_new(char* command);
bool UnixPipe_ok(UnixPipe* self);
char* UnixPipe_readAll(UnixPipe* self);
ProcessStatus* UnixPipe_close(UnixPipe* self);
void UnixProcess_init(UnixProcess* self);
UnixProcess* UnixProcess_new(void);
ProcessStatus* UnixProcess_system(char* command);
UnixPipe* UnixProcess_pipe(char* command);
void ShellWords_init(ShellWords* self);
ShellWords* ShellWords_new(void);
bool ShellWords_isSafeArgChar(char c);
bool ShellWords_isSafeArg(char* raw);
char* ShellWords_quote(char* raw);
char* ShellWords_redact(char* text, char* sensitive);
void ExecResult_init(ExecResult* self, int code, char* out, char* err, char* command);
ExecResult* ExecResult_new(int code, char* out, char* err, char* command);
bool ExecResult_ok(ExecResult* self);
char* ExecResult_stdout(ExecResult* self);
char* ExecResult_trimmed(ExecResult* self);
char* ExecResult_stderr(ExecResult* self);
void Command_init(Command* self, char* executable);
Command* Command_new(char* executable);
Command* Command_arg(Command* self, char* value);
Command* Command_flag(Command* self, char* name, char* value);
Command* Command_envVar(Command* self, char* name, char* value);
Command* Command_sudo(Command* self, bool enabled);
Command* Command_capture(Command* self, bool enabled);
Command* Command_check(Command* self, bool enabled);
Command* Command_mergeError(Command* self, bool enabled);
Command* Command_redact(Command* self, char* value);
char* Command_renderEnv(Command* self, char* item);
char* Command_render(Command* self);
void UnixShell_init(UnixShell* self);
UnixShell* UnixShell_new(void);
char* UnixShell_quote(char* raw);
char* UnixShell_redactText(char* text, char* sensitive);
int UnixShell_statusCode(int rawStatus);
void UnixShell_chroot(UnixShell* self, char* path);
void UnixShell_clearChroot(UnixShell* self);
ExecResult* UnixShell_run(UnixShell* self, char* command);
ExecResult* UnixShell_runUnchecked(UnixShell* self, char* command);
ExecResult* UnixShell_runCommand(UnixShell* self, Command* command);
char* UnixShell_capture(UnixShell* self, Command* command);
ExecResult* UnixShell_runRaw(UnixShell* self, char* command, bool captureOutput, bool checkStatus, char* sensitive);
void PowerShell_init(PowerShell* self);
PowerShell* PowerShell_new(void);
ExecResult* PowerShell_run(PowerShell* self, char* command);
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
UnixFileSystem* UnixFileSystem_new(void);
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
PathTools* PathTools_new(void);
char* PathTools_shellQuote(char* raw);
char* PathTools_basename(char* path);
char* PathTools_dirname(char* path);
char* PathTools_join(char* left, char* right);
void FileSystem_init(FileSystem* self);
FileSystem* FileSystem_new(void);
bool FileSystem_exists(char* path);
bool FileSystem_isDir(char* path);
bool FileSystem_isFile(char* path);
bool FileSystem_isSymlink(char* path);
int FileSystem_chmod(char* path, int mode);
int FileSystem_mkdir(char* path, int mode);
int FileSystem_mkdirp(char* path);
int FileSystem_removeRecursive(char* path);
int FileSystem_symlink(char* target, char* linkPath);
char* FileSystem_readLink(char* path);
char* FileSystem_tempDir(char* prefix);
btrc_Vector_string* FileSystem_listDir(char* path);
char* FileSystem_readText(char* path);
void FileSystem_writeText(char* path, char* content);
void DaemonSpec_init(DaemonSpec* self, char* name, Command* command);
DaemonSpec* DaemonSpec_new(char* name, Command* command);
DaemonSpec* DaemonSpec_pid(DaemonSpec* self, char* path);
DaemonSpec* DaemonSpec_log(DaemonSpec* self, char* path);
DaemonSpec* DaemonSpec_cwd(DaemonSpec* self, char* path);
DaemonSpec* DaemonSpec_restart(DaemonSpec* self, bool enabled);
char* DaemonSpec_renderStartCommand(DaemonSpec* self);
void DaemonController_init(DaemonController* self);
DaemonController* DaemonController_new(void);
ExecResult* DaemonController_start(DaemonController* self, DaemonSpec* spec);
ExecResult* DaemonController_stop(DaemonController* self, DaemonSpec* spec);
ExecResult* DaemonController_status(DaemonController* self, DaemonSpec* spec);
void AppSpec_init(AppSpec* self, char* name);
AppSpec* AppSpec_new(char* name);
AppSpec* AppSpec_withVersion(AppSpec* self, char* version);
void DaemonApp_init(DaemonApp* self, char* name, DaemonSpec* daemon);
DaemonApp* DaemonApp_new(char* name, DaemonSpec* daemon);
AppSpec* DaemonApp_withVersion(DaemonApp* self, char* version);
void Html_init(Html* self);
Html* Html_new(void);
char* Html_escape(char* raw);
void UiNode_init(UiNode* self, char* tag);
UiNode* UiNode_new(char* tag);
UiNode* UiNode_text(UiNode* self, char* value);
UiNode* UiNode_raw(UiNode* self, char* value);
UiNode* UiNode_attr(UiNode* self, char* name, char* value);
UiNode* UiNode_id(UiNode* self, char* value);
UiNode* UiNode_className(UiNode* self, char* value);
UiNode* UiNode_style(UiNode* self, char* value);
UiNode* UiNode_child(UiNode* self, UiNode* node);
UiNode* UiNode_childrenFrom(UiNode* self, btrc_Vector_UiNode* nodes);
char* UiNode_renderAttributes(UiNode* self);
bool UiNode_isVoidElement(UiNode* self);
char* UiNode_renderHtml(UiNode* self);
void UiDocument_init(UiDocument* self, char* title, UiNode* body);
UiDocument* UiDocument_new(char* title, UiNode* body);
UiDocument* UiDocument_style(UiDocument* self, char* css);
char* UiDocument_renderHtml(UiDocument* self);
void UiDocument_writeHtml(UiDocument* self, char* path);
void HtmlView_init(HtmlView* self, UiDocument* document);
HtmlView* HtmlView_new(UiDocument* document);
char* HtmlView_render(HtmlView* self);
void HtmlView_write(HtmlView* self, char* path);
void NativeView_init(NativeView* self, UiNode* root);
NativeView* NativeView_new(UiNode* root);
void Window_init(Window* self, char* title, int width, int height, HtmlView* html);
Window* Window_new(char* title, int width, int height, HtmlView* html);
void TrayItem_init(TrayItem* self, char* label, char* command);
TrayItem* TrayItem_new(char* label, char* command);
TrayItem* TrayItem_disabled(TrayItem* self);
char* TrayItem_renderLabel(TrayItem* self);
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
bool NativeUiBackend_isWindows(NativeUiBackend* self);
Command* NativeUiBackend_notifyCommand(NativeUiBackend* self, char* title, char* body);
Command* NativeUiBackend_alertCommand(NativeUiBackend* self, char* title, char* body);
ExecResult* NativeUiBackend_notify(NativeUiBackend* self, char* title, char* body);
ExecResult* NativeUiBackend_alert(NativeUiBackend* self, char* title, char* body);
ExecResult* NativeUiBackend_openFile(NativeUiBackend* self, char* path);
ExecResult* NativeUiBackend_openWindow(NativeUiBackend* self, Window* window, char* path);
void LinuxUiBuilder_init(LinuxUiBuilder* self);
LinuxUiBuilder* LinuxUiBuilder_new(void);
HtmlUiBackend* LinuxUiBuilder_html(void);
NativeUiBackend* LinuxUiBuilder_native(void);
void MacUiBuilder_init(MacUiBuilder* self);
MacUiBuilder* MacUiBuilder_new(void);
HtmlUiBackend* MacUiBuilder_html(void);
NativeUiBackend* MacUiBuilder_native(void);
void WindowsUiBuilder_init(WindowsUiBuilder* self);
WindowsUiBuilder* WindowsUiBuilder_new(void);
HtmlUiBackend* WindowsUiBuilder_html(void);
NativeUiBackend* WindowsUiBuilder_native(void);
void Ui_init(Ui* self);
Ui* Ui_new(void);
UiNode* Ui_node(char* tag);
UiNode* Ui_text(char* value);
UiNode* Ui_rawHtml(char* value);
UiNode* Ui_div(void);
UiNode* Ui_button(char* label);
UiNode* Ui_input(char* name, char* value);
UiDocument* Ui_document(char* title, UiNode* body);
void NativeUi_init(NativeUi* self);
NativeUi* NativeUi_new(void);
char* NativeUi_applescriptString(char* raw);
NativeUiBackend* NativeUi_detect(void);
void UiRuntime_init(UiRuntime* self);
UiRuntime* UiRuntime_new(void);
__btrc_thread_t* UiRuntime_runCommandAsync(Command* command);
__btrc_thread_t* UiRuntime_notifyAsync(NativeUiBackend* backend, char* title, char* body);
typedef struct __btrc_spawn_env_1 __btrc_spawn_env_1;
typedef struct __btrc_spawn_env_2 __btrc_spawn_env_2;
void Signal_init(Signal* self);
Signal* Signal_new(void);
Signal* Signal_emit(Signal* self, char* value);
bool Signal_hasEvents(Signal* self);
char* Signal_latest(Signal* self);
void Signal_clear(Signal* self);
void CliArgs_init(CliArgs* self, int argc, char** argv);
CliArgs* CliArgs_new(int argc, char** argv);
int CliArgs_count(CliArgs* self);
char* CliArgs_get(CliArgs* self, int index);
char* CliArgs_command(CliArgs* self);
bool CliArgs_has(CliArgs* self, char* flag);
char* CliArgs_valueAfter(CliArgs* self, char* flag, char* fallback);
bool CliArgs_commandIs(CliArgs* self, char* name);
char* CliArgs_valueAfterPrefix(CliArgs* self, char* prefix, char* fallback);
void CliCommand_init(CliCommand* self, char* name);
CliCommand* CliCommand_new(char* name);
void CliCommand_alias(CliCommand* self, char* name);
bool CliCommand_matches(CliCommand* self, char* value);
void Console_init(Console* self);
Console* Console_new(void);
void Console_log(char* msg);
void Console_error(char* msg);
void Console_write(char* msg);
void Console_writeLine(char* msg);
void DateTime_init(DateTime* self, int year, int month, int day, int hour, int minute, int second);
DateTime* DateTime_new(int year, int month, int day, int hour, int minute, int second);
DateTime* DateTime_now(void);
void DateTime_display(DateTime* self);
char* DateTime_format(DateTime* self);
char* DateTime_dateString(DateTime* self);
char* DateTime_timeString(DateTime* self);
void Timer_init(Timer* self);
Timer* Timer_new(void);
void Timer_start(Timer* self);
void Timer_stop(Timer* self);
float Timer_elapsed(Timer* self);
void Timer_reset(Timer* self);
void Error_init(Error* self, char* message, int code);
Error* Error_new(char* message, int code);
char* Error_toString(Error* self);
void ValueError_init(ValueError* self, char* message);
ValueError* ValueError_new(char* message);
char* ValueError_toString(ValueError* self);
void IOError_init(IOError* self, char* message);
IOError* IOError_new(char* message);
char* IOError_toString(IOError* self);
void TypeError_init(TypeError* self, char* message);
TypeError* TypeError_new(char* message);
char* TypeError_toString(TypeError* self);
void IndexError_init(IndexError* self, char* message);
IndexError* IndexError_new(char* message);
char* IndexError_toString(IndexError* self);
void KeyError_init(KeyError* self, char* message);
KeyError* KeyError_new(char* message);
char* KeyError_toString(KeyError* self);
void HttpRequest_init(HttpRequest* self);
HttpRequest* HttpRequest_new(void);
char* HttpRequest_header(HttpRequest* self, char* name);
bool HttpRequest_isGet(HttpRequest* self);
bool HttpRequest_isPost(HttpRequest* self);
void HttpResponse_init(HttpResponse* self, int status, char* contentType, char* body);
HttpResponse* HttpResponse_new(int status, char* contentType, char* body);
HttpResponse* HttpResponse_addHeader(HttpResponse* self, char* line);
HttpResponse* HttpResponse_text(char* body);
HttpResponse* HttpResponse_html(char* body);
HttpResponse* HttpResponse_json(char* body);
HttpResponse* HttpResponse_js(char* body);
HttpResponse* HttpResponse_css(char* body);
HttpResponse* HttpResponse_notFound(void);
HttpResponse* HttpResponse_error(int code, char* message);
void HttpServer_init(HttpServer* self, int port);
HttpServer* HttpServer_new(int port);
bool HttpServer_start(HttpServer* self);
int HttpServer_acceptConn(HttpServer* self);
void HttpServer_closeConn(HttpServer* self, int c);
int HttpServer_headerContentLength(char* head, int headerEnd);
char* HttpServer_readRequestRaw(HttpServer* self, int c);
HttpRequest* HttpServer_readRequest(HttpServer* self, int c);
HttpRequest* HttpServer_parse(HttpServer* self, char* raw);
char* HttpServer_reason(HttpServer* self, int status);
void HttpServer_sendAll(HttpServer* self, int c, char* data);
void HttpServer_respond(HttpServer* self, int c, HttpResponse* resp);
void HttpClientResponse_init(HttpClientResponse* self, int status, char* body, char* error);
HttpClientResponse* HttpClientResponse_new(int status, char* body, char* error);
bool HttpClientResponse_ok(HttpClientResponse* self);
void HttpClient_init(HttpClient* self);
HttpClient* HttpClient_new(void);
HttpClient* HttpClient_timeout(HttpClient* self, int secs);
HttpClientResponse* HttpClient_request(HttpClient* self, char* method, char* url, btrc_Vector_string* headers, char* body);
HttpClientResponse* HttpClient_post(HttpClient* self, char* url, btrc_Vector_string* headers, char* body);
HttpClientResponse* HttpClient_get(HttpClient* self, char* url, btrc_Vector_string* headers);
void Browser_init(Browser* self);
Browser* Browser_new(void);
void Browser_open(char* url);
void File_init(File* self, char* path, char* mode);
File* File_new(char* path, char* mode);
bool File_ok(File* self);
char* File_read(File* self);
char* File_readLine(File* self);
btrc_Vector_string* File_readLines(File* self);
void File_setHandle(File* self, FILE* h);
void File_write(File* self, char* text);
void File_writeLine(File* self, char* text);
void File_close(File* self);
bool File_eof(File* self);
void File_flush(File* self);
void Path_init(Path* self);
Path* Path_new(void);
bool Path_exists(char* path);
char* Path_readAll(char* path);
void Path_writeAll(char* path, char* content);
void JsonObject_init(JsonObject* self);
JsonObject* JsonObject_new(void);
char* JsonObject_escape(char* text);
char* JsonObject_unescape(char* text);
void JsonObject_setString(JsonObject* self, char* key, char* value);
void JsonObject_setRaw(JsonObject* self, char* key, char* value);
void JsonObject_setBool(JsonObject* self, char* key, bool value);
void JsonObject_setInt(JsonObject* self, char* key, int value);
bool JsonObject_has(JsonObject* self, char* key);
char* JsonObject_getString(JsonObject* self, char* key, char* fallback);
bool JsonObject_getBool(JsonObject* self, char* key, bool fallback);
int JsonObject_getInt(JsonObject* self, char* key, int fallback);
char* JsonObject_stringify(JsonObject* self);
int JsonObject_skipSpaces(char* text, int i);
char* JsonObject_slice(char* text, int start, int end);
int JsonObject_stringEnd(char* text, int start);
JsonObject* JsonObject_parse(char* text);
JsonObject* JsonObject_readFile(char* path);
void JsonObject_writeFile(JsonObject* self, char* path);
void Json_init(Json* self);
Json* Json_new(void);
char* Json_esc(char* s);
char* Json_str(char* s);
char* Json_getString(char* json, char* key);
char* Json_getStringAfter(char* json, char* anchor, char* key);
char* Json_getStringFrom(char* json, char* key, int from);
void Math_init(Math* self);
Math* Math_new(void);
float Math_PI(void);
float Math_E(void);
float Math_TAU(void);
float Math_INF(void);
int Math_abs(int x);
float Math_fabs(float x);
int Math_max(int a, int b);
int Math_min(int a, int b);
float Math_fmax(float a, float b);
float Math_fmin(float a, float b);
int Math_clamp(int x, int lo, int hi);
float Math_power(float base, int exp);
float Math_sqrt(float x);
int Math_factorial(int n);
int Math_gcd(int a, int b);
int Math_lcm(int a, int b);
int Math_fibonacci(int n);
bool Math_isPrime(int n);
bool Math_isEven(int n);
bool Math_isOdd(int n);
int Math_sum(btrc_Vector_int* items);
float Math_fsum(btrc_Vector_float* items);
float Math_sin(float x);
float Math_cos(float x);
float Math_tan(float x);
float Math_asin(float x);
float Math_acos(float x);
float Math_atan(float x);
float Math_atan2(float y, float x);
float Math_ceil(float x);
float Math_floor(float x);
int Math_round(float x);
int Math_truncate(float x);
float Math_log(float x);
float Math_log10(float x);
float Math_log2(float x);
float Math_exp(float x);
float Math_toRadians(float degrees);
float Math_toDegrees(float radians);
float Math_fclamp(float val, float lo, float hi);
int Math_sign(int x);
float Math_fsign(float x);
void UnixPattern_init(UnixPattern* self);
UnixPattern* UnixPattern_new(void);
bool UnixPattern_matches(char* pattern, char* text);
void Pattern_init(Pattern* self);
Pattern* Pattern_new(void);
bool Pattern_matches(char* pattern, char* text);
bool Pattern_anyMatches(btrc_Vector_string* patterns, char* text);
void Random_init(Random* self);
Random* Random_new(void);
void Random_seed(Random* self, int s);
void Random_seedTime(Random* self);
int Random_randint(Random* self, int lo, int hi);
float Random_random(Random* self);
float Random_uniform(Random* self, float lo, float hi);
int Random_choice(Random* self, btrc_Vector_int* items);
void Random_shuffle(Random* self, btrc_Vector_int* items);
int forkpty(int* amaster, char* name, void* termp, void* winp);
void UnixPamPassword_init(UnixPamPassword* self);
UnixPamPassword* UnixPamPassword_new(void);
bool UnixPamPassword_change(char* user, char* oldPassword, char* newPassword);
void Toml_init(Toml* self);
Toml* Toml_new(void);
char* Toml_stripInlineComment(char* raw);
char* Toml_unquote(char* raw);
char* Toml_key(char* line);
char* Toml_value(char* line);
char* Toml_sectionName(char* line);
char* Toml_tableArrayName(char* line);
btrc_Map_string_string* Toml_sectionMap(char* content, char* section);
btrc_Vector_Map_string_string* Toml_tableArrayBlocks(char* content, char* table);
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
typedef bool (*__btrc_fn_bool_Map_string_string)(btrc_Map_string_string*);
typedef void (*__btrc_fn_void_Map_string_string)(btrc_Map_string_string*);
typedef btrc_Map_string_string* (*__btrc_fn_Map_string_string_Map_string_string)(btrc_Map_string_string*);
typedef btrc_Map_string_string* (*__btrc_fn_Map_string_string_Map_string_string_Map_string_string)(btrc_Map_string_string*, btrc_Map_string_string*);

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

struct btrc_Vector_Map_string_string {
    int __rc;
    btrc_Map_string_string** data;
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

struct Console {
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

struct HttpRequest {
    int __rc;
    char* method;
    char* path;
    char* query;
    btrc_Map_string_string* headers;
    char* body;
};

struct HttpResponse {
    int __rc;
    int status;
    char* contentType;
    char* body;
    btrc_Vector_string* extraHeaders;
};

struct HttpServer {
    int __rc;
    int fd;
    int port;
    bool bindAny;
};

struct HttpClientResponse {
    int __rc;
    int status;
    char* body;
    char* error;
};

struct HttpClient {
    int __rc;
    int timeoutSecs;
};

struct Browser {
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

struct JsonObject {
    int __rc;
    btrc_Map_string_string* values;
    btrc_Map_string_bool* quoted;
};

struct Json {
    int __rc;
};

struct Math {
    int __rc;
};

struct UnixPattern {
    int __rc;
};

struct Pattern {
    int __rc;
};

struct Random {
    int __rc;
    bool seeded;
};

struct UnixPamPassword {
    int __rc;
};

struct Toml {
    int __rc;
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

void btrc_Vector_string_init(btrc_Vector_string* self);
btrc_Vector_string* btrc_Vector_string_new(void);
void btrc_Vector_string_destroy(btrc_Vector_string* self);
void btrc_Vector_string_push(btrc_Vector_string* self, char* val);
char* btrc_Vector_string_pop(btrc_Vector_string* self);
char* btrc_Vector_string_get(btrc_Vector_string* self, int i);
void btrc_Vector_string_set(btrc_Vector_string* self, int i, char* val);
void btrc_Vector_string_free(btrc_Vector_string* self);
void btrc_Vector_string_remove(btrc_Vector_string* self, int idx);
void btrc_Vector_string_reverse(btrc_Vector_string* self);
btrc_Vector_string* btrc_Vector_string_reversed(btrc_Vector_string* self);
void btrc_Vector_string_swap(btrc_Vector_string* self, int i, int j);
void btrc_Vector_string_clear(btrc_Vector_string* self);
void btrc_Vector_string_fill(btrc_Vector_string* self, char* val);
int btrc_Vector_string_size(btrc_Vector_string* self);
bool btrc_Vector_string_isEmpty(btrc_Vector_string* self);
char* btrc_Vector_string_first(btrc_Vector_string* self);
char* btrc_Vector_string_last(btrc_Vector_string* self);
btrc_Vector_string* btrc_Vector_string_slice(btrc_Vector_string* self, int start, int end);
btrc_Vector_string* btrc_Vector_string_take(btrc_Vector_string* self, int n);
btrc_Vector_string* btrc_Vector_string_drop(btrc_Vector_string* self, int n);
void btrc_Vector_string_extend(btrc_Vector_string* self, btrc_Vector_string* other);
void btrc_Vector_string_insert(btrc_Vector_string* self, int idx, char* val);
bool btrc_Vector_string_contains(btrc_Vector_string* self, char* val);
int btrc_Vector_string_indexOf(btrc_Vector_string* self, char* val);
int btrc_Vector_string_lastIndexOf(btrc_Vector_string* self, char* val);
int btrc_Vector_string_count(btrc_Vector_string* self, char* val);
void btrc_Vector_string_removeAll(btrc_Vector_string* self, char* val);
btrc_Vector_string* btrc_Vector_string_distinct(btrc_Vector_string* self);
void btrc_Vector_string_sort(btrc_Vector_string* self);
btrc_Vector_string* btrc_Vector_string_sorted(btrc_Vector_string* self);
char* btrc_Vector_string_min(btrc_Vector_string* self);
char* btrc_Vector_string_max(btrc_Vector_string* self);
char* btrc_Vector_string_sum(btrc_Vector_string* self);
char* btrc_Vector_string_join(btrc_Vector_string* self, char* sep);
char* btrc_Vector_string_joinToString(btrc_Vector_string* self, char* sep);
btrc_Vector_string* btrc_Vector_string_filter(btrc_Vector_string* self, __btrc_fn_bool_string pred);
int btrc_Vector_string_findIndex(btrc_Vector_string* self, __btrc_fn_bool_string pred);
void btrc_Vector_string_forEach(btrc_Vector_string* self, __btrc_fn_void_string fn);
btrc_Vector_string* btrc_Vector_string_map(btrc_Vector_string* self, __btrc_fn_string_string fn);
bool btrc_Vector_string_any(btrc_Vector_string* self, __btrc_fn_bool_string pred);
bool btrc_Vector_string_all(btrc_Vector_string* self, __btrc_fn_bool_string pred);
char* btrc_Vector_string_reduce(btrc_Vector_string* self, char* init, __btrc_fn_string_string_string fn);
btrc_Vector_string* btrc_Vector_string_copy(btrc_Vector_string* self);
void btrc_Vector_string_removeAt(btrc_Vector_string* self, int idx);
int btrc_Vector_string_iterLen(btrc_Vector_string* self);
char* btrc_Vector_string_iterGet(btrc_Vector_string* self, int i);

void btrc_Vector_UiNode_init(btrc_Vector_UiNode* self);
btrc_Vector_UiNode* btrc_Vector_UiNode_new(void);
void btrc_Vector_UiNode_destroy(btrc_Vector_UiNode* self);
void btrc_Vector_UiNode_push(btrc_Vector_UiNode* self, UiNode* val);
UiNode* btrc_Vector_UiNode_pop(btrc_Vector_UiNode* self);
UiNode* btrc_Vector_UiNode_get(btrc_Vector_UiNode* self, int i);
void btrc_Vector_UiNode_set(btrc_Vector_UiNode* self, int i, UiNode* val);
void btrc_Vector_UiNode_free(btrc_Vector_UiNode* self);
void btrc_Vector_UiNode_remove(btrc_Vector_UiNode* self, int idx);
void btrc_Vector_UiNode_reverse(btrc_Vector_UiNode* self);
btrc_Vector_UiNode* btrc_Vector_UiNode_reversed(btrc_Vector_UiNode* self);
void btrc_Vector_UiNode_swap(btrc_Vector_UiNode* self, int i, int j);
void btrc_Vector_UiNode_clear(btrc_Vector_UiNode* self);
void btrc_Vector_UiNode_fill(btrc_Vector_UiNode* self, UiNode* val);
int btrc_Vector_UiNode_size(btrc_Vector_UiNode* self);
bool btrc_Vector_UiNode_isEmpty(btrc_Vector_UiNode* self);
UiNode* btrc_Vector_UiNode_first(btrc_Vector_UiNode* self);
UiNode* btrc_Vector_UiNode_last(btrc_Vector_UiNode* self);
btrc_Vector_UiNode* btrc_Vector_UiNode_slice(btrc_Vector_UiNode* self, int start, int end);
btrc_Vector_UiNode* btrc_Vector_UiNode_take(btrc_Vector_UiNode* self, int n);
btrc_Vector_UiNode* btrc_Vector_UiNode_drop(btrc_Vector_UiNode* self, int n);
void btrc_Vector_UiNode_extend(btrc_Vector_UiNode* self, btrc_Vector_UiNode* other);
void btrc_Vector_UiNode_insert(btrc_Vector_UiNode* self, int idx, UiNode* val);
bool btrc_Vector_UiNode_contains(btrc_Vector_UiNode* self, UiNode* val);
int btrc_Vector_UiNode_indexOf(btrc_Vector_UiNode* self, UiNode* val);
int btrc_Vector_UiNode_lastIndexOf(btrc_Vector_UiNode* self, UiNode* val);
int btrc_Vector_UiNode_count(btrc_Vector_UiNode* self, UiNode* val);
void btrc_Vector_UiNode_removeAll(btrc_Vector_UiNode* self, UiNode* val);
btrc_Vector_UiNode* btrc_Vector_UiNode_distinct(btrc_Vector_UiNode* self);
void btrc_Vector_UiNode_sort(btrc_Vector_UiNode* self);
btrc_Vector_UiNode* btrc_Vector_UiNode_sorted(btrc_Vector_UiNode* self);
UiNode* btrc_Vector_UiNode_min(btrc_Vector_UiNode* self);
UiNode* btrc_Vector_UiNode_max(btrc_Vector_UiNode* self);
UiNode* btrc_Vector_UiNode_sum(btrc_Vector_UiNode* self);
char* btrc_Vector_UiNode_join(btrc_Vector_UiNode* self, char* sep);
char* btrc_Vector_UiNode_joinToString(btrc_Vector_UiNode* self, char* sep);
btrc_Vector_UiNode* btrc_Vector_UiNode_filter(btrc_Vector_UiNode* self, __btrc_fn_bool_UiNode pred);
int btrc_Vector_UiNode_findIndex(btrc_Vector_UiNode* self, __btrc_fn_bool_UiNode pred);
void btrc_Vector_UiNode_forEach(btrc_Vector_UiNode* self, __btrc_fn_void_UiNode fn);
btrc_Vector_UiNode* btrc_Vector_UiNode_map(btrc_Vector_UiNode* self, __btrc_fn_UiNode_UiNode fn);
bool btrc_Vector_UiNode_any(btrc_Vector_UiNode* self, __btrc_fn_bool_UiNode pred);
bool btrc_Vector_UiNode_all(btrc_Vector_UiNode* self, __btrc_fn_bool_UiNode pred);
UiNode* btrc_Vector_UiNode_reduce(btrc_Vector_UiNode* self, UiNode* init, __btrc_fn_UiNode_UiNode_UiNode fn);
btrc_Vector_UiNode* btrc_Vector_UiNode_copy(btrc_Vector_UiNode* self);
void btrc_Vector_UiNode_removeAt(btrc_Vector_UiNode* self, int idx);
int btrc_Vector_UiNode_iterLen(btrc_Vector_UiNode* self);
UiNode* btrc_Vector_UiNode_iterGet(btrc_Vector_UiNode* self, int i);

void btrc_Vector_TrayItem_init(btrc_Vector_TrayItem* self);
btrc_Vector_TrayItem* btrc_Vector_TrayItem_new(void);
void btrc_Vector_TrayItem_destroy(btrc_Vector_TrayItem* self);
void btrc_Vector_TrayItem_push(btrc_Vector_TrayItem* self, TrayItem* val);
TrayItem* btrc_Vector_TrayItem_pop(btrc_Vector_TrayItem* self);
TrayItem* btrc_Vector_TrayItem_get(btrc_Vector_TrayItem* self, int i);
void btrc_Vector_TrayItem_set(btrc_Vector_TrayItem* self, int i, TrayItem* val);
void btrc_Vector_TrayItem_free(btrc_Vector_TrayItem* self);
void btrc_Vector_TrayItem_remove(btrc_Vector_TrayItem* self, int idx);
void btrc_Vector_TrayItem_reverse(btrc_Vector_TrayItem* self);
btrc_Vector_TrayItem* btrc_Vector_TrayItem_reversed(btrc_Vector_TrayItem* self);
void btrc_Vector_TrayItem_swap(btrc_Vector_TrayItem* self, int i, int j);
void btrc_Vector_TrayItem_clear(btrc_Vector_TrayItem* self);
void btrc_Vector_TrayItem_fill(btrc_Vector_TrayItem* self, TrayItem* val);
int btrc_Vector_TrayItem_size(btrc_Vector_TrayItem* self);
bool btrc_Vector_TrayItem_isEmpty(btrc_Vector_TrayItem* self);
TrayItem* btrc_Vector_TrayItem_first(btrc_Vector_TrayItem* self);
TrayItem* btrc_Vector_TrayItem_last(btrc_Vector_TrayItem* self);
btrc_Vector_TrayItem* btrc_Vector_TrayItem_slice(btrc_Vector_TrayItem* self, int start, int end);
btrc_Vector_TrayItem* btrc_Vector_TrayItem_take(btrc_Vector_TrayItem* self, int n);
btrc_Vector_TrayItem* btrc_Vector_TrayItem_drop(btrc_Vector_TrayItem* self, int n);
void btrc_Vector_TrayItem_extend(btrc_Vector_TrayItem* self, btrc_Vector_TrayItem* other);
void btrc_Vector_TrayItem_insert(btrc_Vector_TrayItem* self, int idx, TrayItem* val);
bool btrc_Vector_TrayItem_contains(btrc_Vector_TrayItem* self, TrayItem* val);
int btrc_Vector_TrayItem_indexOf(btrc_Vector_TrayItem* self, TrayItem* val);
int btrc_Vector_TrayItem_lastIndexOf(btrc_Vector_TrayItem* self, TrayItem* val);
int btrc_Vector_TrayItem_count(btrc_Vector_TrayItem* self, TrayItem* val);
void btrc_Vector_TrayItem_removeAll(btrc_Vector_TrayItem* self, TrayItem* val);
btrc_Vector_TrayItem* btrc_Vector_TrayItem_distinct(btrc_Vector_TrayItem* self);
void btrc_Vector_TrayItem_sort(btrc_Vector_TrayItem* self);
btrc_Vector_TrayItem* btrc_Vector_TrayItem_sorted(btrc_Vector_TrayItem* self);
TrayItem* btrc_Vector_TrayItem_min(btrc_Vector_TrayItem* self);
TrayItem* btrc_Vector_TrayItem_max(btrc_Vector_TrayItem* self);
TrayItem* btrc_Vector_TrayItem_sum(btrc_Vector_TrayItem* self);
char* btrc_Vector_TrayItem_join(btrc_Vector_TrayItem* self, char* sep);
char* btrc_Vector_TrayItem_joinToString(btrc_Vector_TrayItem* self, char* sep);
btrc_Vector_TrayItem* btrc_Vector_TrayItem_filter(btrc_Vector_TrayItem* self, __btrc_fn_bool_TrayItem pred);
int btrc_Vector_TrayItem_findIndex(btrc_Vector_TrayItem* self, __btrc_fn_bool_TrayItem pred);
void btrc_Vector_TrayItem_forEach(btrc_Vector_TrayItem* self, __btrc_fn_void_TrayItem fn);
btrc_Vector_TrayItem* btrc_Vector_TrayItem_map(btrc_Vector_TrayItem* self, __btrc_fn_TrayItem_TrayItem fn);
bool btrc_Vector_TrayItem_any(btrc_Vector_TrayItem* self, __btrc_fn_bool_TrayItem pred);
bool btrc_Vector_TrayItem_all(btrc_Vector_TrayItem* self, __btrc_fn_bool_TrayItem pred);
TrayItem* btrc_Vector_TrayItem_reduce(btrc_Vector_TrayItem* self, TrayItem* init, __btrc_fn_TrayItem_TrayItem_TrayItem fn);
btrc_Vector_TrayItem* btrc_Vector_TrayItem_copy(btrc_Vector_TrayItem* self);
void btrc_Vector_TrayItem_removeAt(btrc_Vector_TrayItem* self, int idx);
int btrc_Vector_TrayItem_iterLen(btrc_Vector_TrayItem* self);
TrayItem* btrc_Vector_TrayItem_iterGet(btrc_Vector_TrayItem* self, int i);

void btrc_Vector_bool_init(btrc_Vector_bool* self);
btrc_Vector_bool* btrc_Vector_bool_new(void);
void btrc_Vector_bool_destroy(btrc_Vector_bool* self);
void btrc_Vector_bool_push(btrc_Vector_bool* self, bool val);
bool btrc_Vector_bool_pop(btrc_Vector_bool* self);
bool btrc_Vector_bool_get(btrc_Vector_bool* self, int i);
void btrc_Vector_bool_set(btrc_Vector_bool* self, int i, bool val);
void btrc_Vector_bool_free(btrc_Vector_bool* self);
void btrc_Vector_bool_remove(btrc_Vector_bool* self, int idx);
void btrc_Vector_bool_reverse(btrc_Vector_bool* self);
btrc_Vector_bool* btrc_Vector_bool_reversed(btrc_Vector_bool* self);
void btrc_Vector_bool_swap(btrc_Vector_bool* self, int i, int j);
void btrc_Vector_bool_clear(btrc_Vector_bool* self);
void btrc_Vector_bool_fill(btrc_Vector_bool* self, bool val);
int btrc_Vector_bool_size(btrc_Vector_bool* self);
bool btrc_Vector_bool_isEmpty(btrc_Vector_bool* self);
bool btrc_Vector_bool_first(btrc_Vector_bool* self);
bool btrc_Vector_bool_last(btrc_Vector_bool* self);
btrc_Vector_bool* btrc_Vector_bool_slice(btrc_Vector_bool* self, int start, int end);
btrc_Vector_bool* btrc_Vector_bool_take(btrc_Vector_bool* self, int n);
btrc_Vector_bool* btrc_Vector_bool_drop(btrc_Vector_bool* self, int n);
void btrc_Vector_bool_extend(btrc_Vector_bool* self, btrc_Vector_bool* other);
void btrc_Vector_bool_insert(btrc_Vector_bool* self, int idx, bool val);
bool btrc_Vector_bool_contains(btrc_Vector_bool* self, bool val);
int btrc_Vector_bool_indexOf(btrc_Vector_bool* self, bool val);
int btrc_Vector_bool_lastIndexOf(btrc_Vector_bool* self, bool val);
int btrc_Vector_bool_count(btrc_Vector_bool* self, bool val);
void btrc_Vector_bool_removeAll(btrc_Vector_bool* self, bool val);
btrc_Vector_bool* btrc_Vector_bool_distinct(btrc_Vector_bool* self);
void btrc_Vector_bool_sort(btrc_Vector_bool* self);
btrc_Vector_bool* btrc_Vector_bool_sorted(btrc_Vector_bool* self);
bool btrc_Vector_bool_min(btrc_Vector_bool* self);
bool btrc_Vector_bool_max(btrc_Vector_bool* self);
bool btrc_Vector_bool_sum(btrc_Vector_bool* self);
char* btrc_Vector_bool_join(btrc_Vector_bool* self, char* sep);
char* btrc_Vector_bool_joinToString(btrc_Vector_bool* self, char* sep);
btrc_Vector_bool* btrc_Vector_bool_filter(btrc_Vector_bool* self, __btrc_fn_bool_bool pred);
int btrc_Vector_bool_findIndex(btrc_Vector_bool* self, __btrc_fn_bool_bool pred);
void btrc_Vector_bool_forEach(btrc_Vector_bool* self, __btrc_fn_void_bool fn);
btrc_Vector_bool* btrc_Vector_bool_map(btrc_Vector_bool* self, __btrc_fn_bool_bool fn);
bool btrc_Vector_bool_any(btrc_Vector_bool* self, __btrc_fn_bool_bool pred);
bool btrc_Vector_bool_all(btrc_Vector_bool* self, __btrc_fn_bool_bool pred);
bool btrc_Vector_bool_reduce(btrc_Vector_bool* self, bool init, __btrc_fn_bool_bool_bool fn);
btrc_Vector_bool* btrc_Vector_bool_copy(btrc_Vector_bool* self);
void btrc_Vector_bool_removeAt(btrc_Vector_bool* self, int idx);
int btrc_Vector_bool_iterLen(btrc_Vector_bool* self);
bool btrc_Vector_bool_iterGet(btrc_Vector_bool* self, int i);

void btrc_Vector_int_init(btrc_Vector_int* self);
btrc_Vector_int* btrc_Vector_int_new(void);
void btrc_Vector_int_destroy(btrc_Vector_int* self);
void btrc_Vector_int_push(btrc_Vector_int* self, int val);
int btrc_Vector_int_pop(btrc_Vector_int* self);
int btrc_Vector_int_get(btrc_Vector_int* self, int i);
void btrc_Vector_int_set(btrc_Vector_int* self, int i, int val);
void btrc_Vector_int_free(btrc_Vector_int* self);
void btrc_Vector_int_remove(btrc_Vector_int* self, int idx);
void btrc_Vector_int_reverse(btrc_Vector_int* self);
btrc_Vector_int* btrc_Vector_int_reversed(btrc_Vector_int* self);
void btrc_Vector_int_swap(btrc_Vector_int* self, int i, int j);
void btrc_Vector_int_clear(btrc_Vector_int* self);
void btrc_Vector_int_fill(btrc_Vector_int* self, int val);
int btrc_Vector_int_size(btrc_Vector_int* self);
bool btrc_Vector_int_isEmpty(btrc_Vector_int* self);
int btrc_Vector_int_first(btrc_Vector_int* self);
int btrc_Vector_int_last(btrc_Vector_int* self);
btrc_Vector_int* btrc_Vector_int_slice(btrc_Vector_int* self, int start, int end);
btrc_Vector_int* btrc_Vector_int_take(btrc_Vector_int* self, int n);
btrc_Vector_int* btrc_Vector_int_drop(btrc_Vector_int* self, int n);
void btrc_Vector_int_extend(btrc_Vector_int* self, btrc_Vector_int* other);
void btrc_Vector_int_insert(btrc_Vector_int* self, int idx, int val);
bool btrc_Vector_int_contains(btrc_Vector_int* self, int val);
int btrc_Vector_int_indexOf(btrc_Vector_int* self, int val);
int btrc_Vector_int_lastIndexOf(btrc_Vector_int* self, int val);
int btrc_Vector_int_count(btrc_Vector_int* self, int val);
void btrc_Vector_int_removeAll(btrc_Vector_int* self, int val);
btrc_Vector_int* btrc_Vector_int_distinct(btrc_Vector_int* self);
void btrc_Vector_int_sort(btrc_Vector_int* self);
btrc_Vector_int* btrc_Vector_int_sorted(btrc_Vector_int* self);
int btrc_Vector_int_min(btrc_Vector_int* self);
int btrc_Vector_int_max(btrc_Vector_int* self);
int btrc_Vector_int_sum(btrc_Vector_int* self);
char* btrc_Vector_int_join(btrc_Vector_int* self, char* sep);
char* btrc_Vector_int_joinToString(btrc_Vector_int* self, char* sep);
btrc_Vector_int* btrc_Vector_int_filter(btrc_Vector_int* self, __btrc_fn_bool_int pred);
int btrc_Vector_int_findIndex(btrc_Vector_int* self, __btrc_fn_bool_int pred);
void btrc_Vector_int_forEach(btrc_Vector_int* self, __btrc_fn_void_int fn);
btrc_Vector_int* btrc_Vector_int_map(btrc_Vector_int* self, __btrc_fn_int_int fn);
bool btrc_Vector_int_any(btrc_Vector_int* self, __btrc_fn_bool_int pred);
bool btrc_Vector_int_all(btrc_Vector_int* self, __btrc_fn_bool_int pred);
int btrc_Vector_int_reduce(btrc_Vector_int* self, int init, __btrc_fn_int_int_int fn);
btrc_Vector_int* btrc_Vector_int_copy(btrc_Vector_int* self);
void btrc_Vector_int_removeAt(btrc_Vector_int* self, int idx);
int btrc_Vector_int_iterLen(btrc_Vector_int* self);
int btrc_Vector_int_iterGet(btrc_Vector_int* self, int i);

void btrc_Vector_float_init(btrc_Vector_float* self);
btrc_Vector_float* btrc_Vector_float_new(void);
void btrc_Vector_float_destroy(btrc_Vector_float* self);
void btrc_Vector_float_push(btrc_Vector_float* self, float val);
float btrc_Vector_float_pop(btrc_Vector_float* self);
float btrc_Vector_float_get(btrc_Vector_float* self, int i);
void btrc_Vector_float_set(btrc_Vector_float* self, int i, float val);
void btrc_Vector_float_free(btrc_Vector_float* self);
void btrc_Vector_float_remove(btrc_Vector_float* self, int idx);
void btrc_Vector_float_reverse(btrc_Vector_float* self);
btrc_Vector_float* btrc_Vector_float_reversed(btrc_Vector_float* self);
void btrc_Vector_float_swap(btrc_Vector_float* self, int i, int j);
void btrc_Vector_float_clear(btrc_Vector_float* self);
void btrc_Vector_float_fill(btrc_Vector_float* self, float val);
int btrc_Vector_float_size(btrc_Vector_float* self);
bool btrc_Vector_float_isEmpty(btrc_Vector_float* self);
float btrc_Vector_float_first(btrc_Vector_float* self);
float btrc_Vector_float_last(btrc_Vector_float* self);
btrc_Vector_float* btrc_Vector_float_slice(btrc_Vector_float* self, int start, int end);
btrc_Vector_float* btrc_Vector_float_take(btrc_Vector_float* self, int n);
btrc_Vector_float* btrc_Vector_float_drop(btrc_Vector_float* self, int n);
void btrc_Vector_float_extend(btrc_Vector_float* self, btrc_Vector_float* other);
void btrc_Vector_float_insert(btrc_Vector_float* self, int idx, float val);
bool btrc_Vector_float_contains(btrc_Vector_float* self, float val);
int btrc_Vector_float_indexOf(btrc_Vector_float* self, float val);
int btrc_Vector_float_lastIndexOf(btrc_Vector_float* self, float val);
int btrc_Vector_float_count(btrc_Vector_float* self, float val);
void btrc_Vector_float_removeAll(btrc_Vector_float* self, float val);
btrc_Vector_float* btrc_Vector_float_distinct(btrc_Vector_float* self);
void btrc_Vector_float_sort(btrc_Vector_float* self);
btrc_Vector_float* btrc_Vector_float_sorted(btrc_Vector_float* self);
float btrc_Vector_float_min(btrc_Vector_float* self);
float btrc_Vector_float_max(btrc_Vector_float* self);
float btrc_Vector_float_sum(btrc_Vector_float* self);
char* btrc_Vector_float_join(btrc_Vector_float* self, char* sep);
char* btrc_Vector_float_joinToString(btrc_Vector_float* self, char* sep);
btrc_Vector_float* btrc_Vector_float_filter(btrc_Vector_float* self, __btrc_fn_bool_float pred);
int btrc_Vector_float_findIndex(btrc_Vector_float* self, __btrc_fn_bool_float pred);
void btrc_Vector_float_forEach(btrc_Vector_float* self, __btrc_fn_void_float fn);
btrc_Vector_float* btrc_Vector_float_map(btrc_Vector_float* self, __btrc_fn_float_float fn);
bool btrc_Vector_float_any(btrc_Vector_float* self, __btrc_fn_bool_float pred);
bool btrc_Vector_float_all(btrc_Vector_float* self, __btrc_fn_bool_float pred);
float btrc_Vector_float_reduce(btrc_Vector_float* self, float init, __btrc_fn_float_float_float fn);
btrc_Vector_float* btrc_Vector_float_copy(btrc_Vector_float* self);
void btrc_Vector_float_removeAt(btrc_Vector_float* self, int idx);
int btrc_Vector_float_iterLen(btrc_Vector_float* self);
float btrc_Vector_float_iterGet(btrc_Vector_float* self, int i);

void btrc_Map_string_string_init(btrc_Map_string_string* self);
btrc_Map_string_string* btrc_Map_string_string_new(void);
void btrc_Map_string_string_destroy(btrc_Map_string_string* self);
void btrc_Map_string_string_resize(btrc_Map_string_string* self);
void btrc_Map_string_string_put(btrc_Map_string_string* self, char* key, char* value);
char* btrc_Map_string_string_get(btrc_Map_string_string* self, char* key);
char* btrc_Map_string_string_getOrDefault(btrc_Map_string_string* self, char* key, char* fallback);
bool btrc_Map_string_string_has(btrc_Map_string_string* self, char* key);
bool btrc_Map_string_string_contains(btrc_Map_string_string* self, char* key);
void btrc_Map_string_string_putIfAbsent(btrc_Map_string_string* self, char* key, char* value);
void btrc_Map_string_string_free(btrc_Map_string_string* self);
void btrc_Map_string_string_remove(btrc_Map_string_string* self, char* key);
void btrc_Map_string_string_clear(btrc_Map_string_string* self);
int btrc_Map_string_string_size(btrc_Map_string_string* self);
bool btrc_Map_string_string_isEmpty(btrc_Map_string_string* self);
btrc_Vector_string* btrc_Map_string_string_keys(btrc_Map_string_string* self);
btrc_Vector_string* btrc_Map_string_string_values(btrc_Map_string_string* self);
bool btrc_Map_string_string_containsValue(btrc_Map_string_string* self, char* value);
void btrc_Map_string_string_set(btrc_Map_string_string* self, char* key, char* value);
void btrc_Map_string_string_merge(btrc_Map_string_string* self, btrc_Map_string_string* other);
int btrc_Map_string_string_iterLen(btrc_Map_string_string* self);
char* btrc_Map_string_string_iterGet(btrc_Map_string_string* self, int n);
char* btrc_Map_string_string_iterValueAt(btrc_Map_string_string* self, int n);

void btrc_Vector_Map_string_string_init(btrc_Vector_Map_string_string* self);
btrc_Vector_Map_string_string* btrc_Vector_Map_string_string_new(void);
void btrc_Vector_Map_string_string_destroy(btrc_Vector_Map_string_string* self);
void btrc_Vector_Map_string_string_push(btrc_Vector_Map_string_string* self, btrc_Map_string_string* val);
btrc_Map_string_string* btrc_Vector_Map_string_string_pop(btrc_Vector_Map_string_string* self);
btrc_Map_string_string* btrc_Vector_Map_string_string_get(btrc_Vector_Map_string_string* self, int i);
void btrc_Vector_Map_string_string_set(btrc_Vector_Map_string_string* self, int i, btrc_Map_string_string* val);
void btrc_Vector_Map_string_string_free(btrc_Vector_Map_string_string* self);
void btrc_Vector_Map_string_string_remove(btrc_Vector_Map_string_string* self, int idx);
void btrc_Vector_Map_string_string_reverse(btrc_Vector_Map_string_string* self);
btrc_Vector_Map_string_string* btrc_Vector_Map_string_string_reversed(btrc_Vector_Map_string_string* self);
void btrc_Vector_Map_string_string_swap(btrc_Vector_Map_string_string* self, int i, int j);
void btrc_Vector_Map_string_string_clear(btrc_Vector_Map_string_string* self);
void btrc_Vector_Map_string_string_fill(btrc_Vector_Map_string_string* self, btrc_Map_string_string* val);
int btrc_Vector_Map_string_string_size(btrc_Vector_Map_string_string* self);
bool btrc_Vector_Map_string_string_isEmpty(btrc_Vector_Map_string_string* self);
btrc_Map_string_string* btrc_Vector_Map_string_string_first(btrc_Vector_Map_string_string* self);
btrc_Map_string_string* btrc_Vector_Map_string_string_last(btrc_Vector_Map_string_string* self);
btrc_Vector_Map_string_string* btrc_Vector_Map_string_string_slice(btrc_Vector_Map_string_string* self, int start, int end);
btrc_Vector_Map_string_string* btrc_Vector_Map_string_string_take(btrc_Vector_Map_string_string* self, int n);
btrc_Vector_Map_string_string* btrc_Vector_Map_string_string_drop(btrc_Vector_Map_string_string* self, int n);
void btrc_Vector_Map_string_string_extend(btrc_Vector_Map_string_string* self, btrc_Vector_Map_string_string* other);
void btrc_Vector_Map_string_string_insert(btrc_Vector_Map_string_string* self, int idx, btrc_Map_string_string* val);
bool btrc_Vector_Map_string_string_contains(btrc_Vector_Map_string_string* self, btrc_Map_string_string* val);
int btrc_Vector_Map_string_string_indexOf(btrc_Vector_Map_string_string* self, btrc_Map_string_string* val);
int btrc_Vector_Map_string_string_lastIndexOf(btrc_Vector_Map_string_string* self, btrc_Map_string_string* val);
int btrc_Vector_Map_string_string_count(btrc_Vector_Map_string_string* self, btrc_Map_string_string* val);
void btrc_Vector_Map_string_string_removeAll(btrc_Vector_Map_string_string* self, btrc_Map_string_string* val);
btrc_Vector_Map_string_string* btrc_Vector_Map_string_string_distinct(btrc_Vector_Map_string_string* self);
void btrc_Vector_Map_string_string_sort(btrc_Vector_Map_string_string* self);
btrc_Vector_Map_string_string* btrc_Vector_Map_string_string_sorted(btrc_Vector_Map_string_string* self);
btrc_Map_string_string* btrc_Vector_Map_string_string_min(btrc_Vector_Map_string_string* self);
btrc_Map_string_string* btrc_Vector_Map_string_string_max(btrc_Vector_Map_string_string* self);
btrc_Map_string_string* btrc_Vector_Map_string_string_sum(btrc_Vector_Map_string_string* self);
char* btrc_Vector_Map_string_string_join(btrc_Vector_Map_string_string* self, char* sep);
char* btrc_Vector_Map_string_string_joinToString(btrc_Vector_Map_string_string* self, char* sep);
btrc_Vector_Map_string_string* btrc_Vector_Map_string_string_filter(btrc_Vector_Map_string_string* self, __btrc_fn_bool_Map_string_string pred);
int btrc_Vector_Map_string_string_findIndex(btrc_Vector_Map_string_string* self, __btrc_fn_bool_Map_string_string pred);
void btrc_Vector_Map_string_string_forEach(btrc_Vector_Map_string_string* self, __btrc_fn_void_Map_string_string fn);
btrc_Vector_Map_string_string* btrc_Vector_Map_string_string_map(btrc_Vector_Map_string_string* self, __btrc_fn_Map_string_string_Map_string_string fn);
bool btrc_Vector_Map_string_string_any(btrc_Vector_Map_string_string* self, __btrc_fn_bool_Map_string_string pred);
bool btrc_Vector_Map_string_string_all(btrc_Vector_Map_string_string* self, __btrc_fn_bool_Map_string_string pred);
btrc_Map_string_string* btrc_Vector_Map_string_string_reduce(btrc_Vector_Map_string_string* self, btrc_Map_string_string* init, __btrc_fn_Map_string_string_Map_string_string_Map_string_string fn);
btrc_Vector_Map_string_string* btrc_Vector_Map_string_string_copy(btrc_Vector_Map_string_string* self);
void btrc_Vector_Map_string_string_removeAt(btrc_Vector_Map_string_string* self, int idx);
int btrc_Vector_Map_string_string_iterLen(btrc_Vector_Map_string_string* self);
btrc_Map_string_string* btrc_Vector_Map_string_string_iterGet(btrc_Vector_Map_string_string* self, int i);

void btrc_Map_string_bool_init(btrc_Map_string_bool* self);
btrc_Map_string_bool* btrc_Map_string_bool_new(void);
void btrc_Map_string_bool_destroy(btrc_Map_string_bool* self);
void btrc_Map_string_bool_resize(btrc_Map_string_bool* self);
void btrc_Map_string_bool_put(btrc_Map_string_bool* self, char* key, bool value);
bool btrc_Map_string_bool_get(btrc_Map_string_bool* self, char* key);
bool btrc_Map_string_bool_getOrDefault(btrc_Map_string_bool* self, char* key, bool fallback);
bool btrc_Map_string_bool_has(btrc_Map_string_bool* self, char* key);
bool btrc_Map_string_bool_contains(btrc_Map_string_bool* self, char* key);
void btrc_Map_string_bool_putIfAbsent(btrc_Map_string_bool* self, char* key, bool value);
void btrc_Map_string_bool_free(btrc_Map_string_bool* self);
void btrc_Map_string_bool_remove(btrc_Map_string_bool* self, char* key);
void btrc_Map_string_bool_clear(btrc_Map_string_bool* self);
int btrc_Map_string_bool_size(btrc_Map_string_bool* self);
bool btrc_Map_string_bool_isEmpty(btrc_Map_string_bool* self);
btrc_Vector_string* btrc_Map_string_bool_keys(btrc_Map_string_bool* self);
btrc_Vector_bool* btrc_Map_string_bool_values(btrc_Map_string_bool* self);
bool btrc_Map_string_bool_containsValue(btrc_Map_string_bool* self, bool value);
void btrc_Map_string_bool_set(btrc_Map_string_bool* self, char* key, bool value);
void btrc_Map_string_bool_merge(btrc_Map_string_bool* self, btrc_Map_string_bool* other);
int btrc_Map_string_bool_iterLen(btrc_Map_string_bool* self);
char* btrc_Map_string_bool_iterGet(btrc_Map_string_bool* self, int n);
bool btrc_Map_string_bool_iterValueAt(btrc_Map_string_bool* self, int n);

void UiNode_visit(UiNode* self, void (*fn)(void**));
#endif
