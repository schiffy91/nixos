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
typedef struct NixosLog NixosLog;
void NixosLog_destroy(NixosLog* self);
typedef struct NixosPaths NixosPaths;
void NixosPaths_destroy(NixosPaths* self);
typedef struct LocalConfigFile LocalConfigFile;
void LocalConfigFile_destroy(LocalConfigFile* self);
typedef struct Interactive Interactive;
void Interactive_destroy(Interactive* self);
typedef struct NixEvalCache NixEvalCache;
void NixEvalCache_destroy(NixEvalCache* self);
typedef struct NixosConfig NixosConfig;
void NixosConfig_destroy(NixosConfig* self);
typedef struct SecretsManager SecretsManager;
void SecretsManager_destroy(SecretsManager* self);
typedef struct PermissionsManager PermissionsManager;
void PermissionsManager_destroy(PermissionsManager* self);
typedef struct ResetSubvolume ResetSubvolume;
void ResetSubvolume_destroy(ResetSubvolume* self);
typedef struct SnapshotManager SnapshotManager;
void SnapshotManager_destroy(SnapshotManager* self);
typedef struct RebuildOptions RebuildOptions;
void RebuildOptions_destroy(RebuildOptions* self);
typedef struct NixosRebuilder NixosRebuilder;
void NixosRebuilder_destroy(NixosRebuilder* self);
typedef struct DiffOptions DiffOptions;
void DiffOptions_destroy(DiffOptions* self);
typedef struct DiffScanner DiffScanner;
void DiffScanner_destroy(DiffScanner* self);
typedef struct Installer Installer;
void Installer_destroy(Installer* self);
typedef struct SecureBootManager SecureBootManager;
void SecureBootManager_destroy(SecureBootManager* self);
typedef struct Tpm2Manager Tpm2Manager;
void Tpm2Manager_destroy(Tpm2Manager* self);
typedef struct PasswordManager PasswordManager;
void PasswordManager_destroy(PasswordManager* self);
typedef struct DisplayLayoutRule DisplayLayoutRule;
void DisplayLayoutRule_destroy(DisplayLayoutRule* self);
typedef struct AudioPreset AudioPreset;
void AudioPreset_destroy(AudioPreset* self);
typedef struct LabelsConfig LabelsConfig;
void LabelsConfig_destroy(LabelsConfig* self);
typedef struct AudioSink AudioSink;
void AudioSink_destroy(AudioSink* self);
typedef struct AudioManager AudioManager;
void AudioManager_destroy(AudioManager* self);
typedef struct CaffeineManager CaffeineManager;
void CaffeineManager_destroy(CaffeineManager* self);
typedef struct DisplayOutput DisplayOutput;
void DisplayOutput_destroy(DisplayOutput* self);
typedef struct DisplayManager DisplayManager;
void DisplayManager_destroy(DisplayManager* self);
typedef struct SystemUi SystemUi;
void SystemUi_destroy(SystemUi* self);
typedef struct VmOperation VmOperation;
void VmOperation_destroy(VmOperation* self);
typedef struct VmTestSpec VmTestSpec;
void VmTestSpec_destroy(VmTestSpec* self);
typedef struct VmSpecParser VmSpecParser;
void VmSpecParser_destroy(VmSpecParser* self);
typedef struct QemuE2eHarness QemuE2eHarness;
void QemuE2eHarness_destroy(QemuE2eHarness* self);
typedef struct VmOperationCatalog VmOperationCatalog;
void VmOperationCatalog_destroy(VmOperationCatalog* self);
typedef struct VmTestRunner VmTestRunner;
void VmTestRunner_destroy(VmTestRunner* self);
typedef struct VmGraphNode VmGraphNode;
void VmGraphNode_destroy(VmGraphNode* self);
typedef struct VmTestGraph VmTestGraph;
void VmTestGraph_destroy(VmTestGraph* self);
typedef struct VmGraphParser VmGraphParser;
void VmGraphParser_destroy(VmGraphParser* self);
typedef struct VmGraphRunner VmGraphRunner;
void VmGraphRunner_destroy(VmGraphRunner* self);
typedef struct NixosCtl NixosCtl;
void NixosCtl_destroy(NixosCtl* self);
typedef struct btrc_Vector_string btrc_Vector_string;
typedef struct btrc_Vector_UiNode btrc_Vector_UiNode;
typedef struct btrc_Vector_TrayItem btrc_Vector_TrayItem;
typedef struct btrc_Vector_bool btrc_Vector_bool;
typedef struct btrc_Vector_int btrc_Vector_int;
typedef struct btrc_Vector_float btrc_Vector_float;
typedef struct btrc_Vector_ResetSubvolume btrc_Vector_ResetSubvolume;
typedef struct btrc_Vector_DisplayLayoutRule btrc_Vector_DisplayLayoutRule;
typedef struct btrc_Vector_AudioPreset btrc_Vector_AudioPreset;
typedef struct btrc_Vector_AudioSink btrc_Vector_AudioSink;
typedef struct btrc_Vector_DisplayOutput btrc_Vector_DisplayOutput;
typedef struct btrc_Vector_VmOperation btrc_Vector_VmOperation;
typedef struct btrc_Vector_VmGraphNode btrc_Vector_VmGraphNode;
typedef struct btrc_Map_string_string btrc_Map_string_string;
typedef struct btrc_Map_string_bool btrc_Map_string_bool;
void Strings_init(Strings* self);
char* Strings_copy(char* s);
char* Strings_replace(char* s, char* old, char* replacement);
btrc_Vector_string* Strings_split(char* s, char* delim);
bool Strings_isDigit(char c);
bool Strings_isAlpha(char c);
int Strings_toInt(char* s);
int Strings_count(char* s, char* sub);
int Strings_find(char* s, char* sub, int start);
int Strings_compare(char* left, char* right);
char* Strings_removePrefix(char* s, char* prefix);
char* Strings_fromInt(int n);
void Console_init(Console* self);
void Console_log(char* msg);
void Console_error(char* msg);
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
bool Platform_isRoot(void);
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
bool ExecResult_ok(ExecResult* self);
char* ExecResult_stdout(ExecResult* self);
void Command_init(Command* self, char* executable);
Command* Command_new(char* executable);
Command* Command_arg(Command* self, char* value);
Command* Command_flag(Command* self, char* name, char* value);
Command* Command_envVar(Command* self, char* name, char* value);
Command* Command_capture(Command* self, bool enabled);
Command* Command_check(Command* self, bool enabled);
Command* Command_redact(Command* self, char* value);
char* Command_renderEnv(Command* self, char* item);
char* Command_render(Command* self);
void UnixShell_init(UnixShell* self);
UnixShell* UnixShell_new(void);
char* UnixShell_quote(char* raw);
char* UnixShell_redactText(char* text, char* sensitive);
void UnixShell_chroot(UnixShell* self, char* path);
void UnixShell_clearChroot(UnixShell* self);
ExecResult* UnixShell_run(UnixShell* self, char* command);
ExecResult* UnixShell_runUnchecked(UnixShell* self, char* command);
ExecResult* UnixShell_runCommand(UnixShell* self, Command* command);
ExecResult* UnixShell_runRaw(UnixShell* self, char* command, bool captureOutput, bool checkStatus, char* sensitive);
void PowerShell_init(PowerShell* self);
int forkpty(int* amaster, char* name, void* termp, void* winp);
void UnixPamPassword_init(UnixPamPassword* self);
bool UnixPamPassword_change(char* user, char* oldPassword, char* newPassword);
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
char* PathTools_basename(char* path);
char* PathTools_dirname(char* path);
char* PathTools_join(char* left, char* right);
void FileSystem_init(FileSystem* self);
bool FileSystem_exists(char* path);
bool FileSystem_isSymlink(char* path);
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
Ui* Ui_new(void);
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
void JsonObject_setBool(JsonObject* self, char* key, bool value);
void JsonObject_setInt(JsonObject* self, char* key, int value);
char* JsonObject_getString(JsonObject* self, char* key, char* fallback);
char* JsonObject_stringify(JsonObject* self);
int JsonObject_skipSpaces(char* text, int i);
char* JsonObject_slice(char* text, int start, int end);
int JsonObject_stringEnd(char* text, int start);
JsonObject* JsonObject_parse(char* text);
JsonObject* JsonObject_readFile(char* path);
void JsonObject_writeFile(JsonObject* self, char* path);
void Toml_init(Toml* self);
char* Toml_stripInlineComment(char* raw);
char* Toml_unquote(char* raw);
char* Toml_key(char* line);
char* Toml_value(char* line);
char* Toml_sectionName(char* line);
char* Toml_tableArrayName(char* line);
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
CliArgs* CliArgs_new(int argc, char** argv);
int CliArgs_count(CliArgs* self);
char* CliArgs_get(CliArgs* self, int index);
char* CliArgs_command(CliArgs* self);
bool CliArgs_has(CliArgs* self, char* flag);
char* CliArgs_valueAfter(CliArgs* self, char* flag, char* fallback);
void CliCommand_init(CliCommand* self, char* name);
void NixosLog_init(NixosLog* self);
char* NixosLog_gray(void);
char* NixosLog_orange(void);
char* NixosLog_red(void);
char* NixosLog_reset(void);
void NixosLog_info(char* message);
void NixosLog_error(char* message);
void NixosLog_fatal(char* message);
void NixosPaths_init(NixosPaths* self, char* root);
NixosPaths* NixosPaths_new(char* root);
char* NixosPaths_configPath(NixosPaths* self);
char* NixosPaths_hostsPath(NixosPaths* self);
char* NixosPaths_secretsPathFallback(NixosPaths* self);
void LocalConfigFile_init(LocalConfigFile* self, char* path);
LocalConfigFile* LocalConfigFile_new(char* path);
bool LocalConfigFile_exists(LocalConfigFile* self);
JsonObject* LocalConfigFile_read(LocalConfigFile* self);
void LocalConfigFile_overwrite(LocalConfigFile* self, JsonObject* data);
char* LocalConfigFile_getString(LocalConfigFile* self, char* key, char* fallback);
void LocalConfigFile_setString(LocalConfigFile* self, char* key, char* value);
void Interactive_init(Interactive* self);
Interactive* Interactive_new(void);
char* Interactive_ask(Interactive* self, char* prompt);
bool Interactive_confirm(Interactive* self, char* prompt);
char* Interactive_askPassword(Interactive* self, char* prompt);
char* Interactive_askPasswordConfirmed(Interactive* self, char* prompt);
char* Interactive_askHostPath(Interactive* self, char* hostsPath);
void Interactive_askToReboot(Interactive* self);
void NixEvalCache_init(NixEvalCache* self);
NixEvalCache* NixEvalCache_new(void);
bool NixEvalCache_has(NixEvalCache* self, char* key);
char* NixEvalCache_get(NixEvalCache* self, char* key);
void NixEvalCache_put(NixEvalCache* self, char* key, char* value);
void NixosConfig_init(NixosConfig* self, char* root);
NixosConfig* NixosConfig_new(char* root);
bool NixosConfig_exists(NixosConfig* self);
char* NixosConfig_hostPath(NixosConfig* self);
char* NixosConfig_target(NixosConfig* self);
void NixosConfig_reset(NixosConfig* self, char* hostPath, char* target);
char* NixosConfig_host(NixosConfig* self);
char* NixosConfig_flakeRef(NixosConfig* self);
char* NixosConfig_inputLockedRev(NixosConfig* self, char* inputName);
char* NixosConfig_evalRaw(NixosConfig* self, char* attribute);
bool NixosConfig_evalBool(NixosConfig* self, char* attribute);
char* NixosConfig_standardTarget(NixosConfig* self);
char* NixosConfig_secureBootTarget(NixosConfig* self);
char* NixosConfig_diskOperationTarget(NixosConfig* self);
char* NixosConfig_username(NixosConfig* self);
char* NixosConfig_secretsPath(NixosConfig* self);
char* NixosConfig_hashedPasswordPath(NixosConfig* self);
char* NixosConfig_diskDevice(NixosConfig* self);
char* NixosConfig_rootPartLabelPath(NixosConfig* self);
char* NixosConfig_tpmDevice(NixosConfig* self);
char* NixosConfig_tpmVersionPath(NixosConfig* self);
void SecretsManager_init(SecretsManager* self, NixosConfig* config);
SecretsManager* SecretsManager_new(NixosConfig* config);
bool SecretsManager_hasHashedPassword(SecretsManager* self);
void SecretsManager_writeHashedPassword(SecretsManager* self, char* hashed);
bool SecretsManager_needsPassword(SecretsManager* self, char* plainTextPasswordPath);
char* SecretsManager_hashPassword(SecretsManager* self, char* password);
void SecretsManager_createIfMissing(SecretsManager* self, char* plainTextPasswordPath);
void SecretsManager_secure(SecretsManager* self);
void PermissionsManager_init(PermissionsManager* self, NixosConfig* config);
PermissionsManager* PermissionsManager_new(NixosConfig* config);
char* PermissionsManager_ignorePredicate(PermissionsManager* self, char* quotedRoot);
void PermissionsManager_secureSecretsPath(PermissionsManager* self, char* path);
void PermissionsManager_secureTree(PermissionsManager* self, char* username);
void ResetSubvolume_init(ResetSubvolume* self, char* name, char* mountPoint);
ResetSubvolume* ResetSubvolume_new(char* name, char* mountPoint);
void SnapshotManager_init(SnapshotManager* self, NixosConfig* config);
SnapshotManager* SnapshotManager_new(NixosConfig* config);
char* SnapshotManager_hostPath(SnapshotManager* self, char* path);
char* SnapshotManager_snapshotsPath(SnapshotManager* self);
char* SnapshotManager_cleanName(SnapshotManager* self);
btrc_Vector_ResetSubvolume* SnapshotManager_resetSubvolumes(SnapshotManager* self);
char* SnapshotManager_cleanSnapshotPath(SnapshotManager* self, char* subvolumeName);
bool SnapshotManager_isSubvolume(SnapshotManager* self, char* path);
bool SnapshotManager_isReadonly(SnapshotManager* self, char* path);
btrc_Vector_string* SnapshotManager_childSubvolumes(SnapshotManager* self, char* path);
void SnapshotManager_deleteSubvolume(SnapshotManager* self, char* path);
void SnapshotManager_createReadonlySnapshot(SnapshotManager* self, char* source, char* destination);
void SnapshotManager_createCleanSnapshot(SnapshotManager* self, ResetSubvolume* volume);
void SnapshotManager_createInitialSnapshots(SnapshotManager* self);
void RebuildOptions_init(RebuildOptions* self);
RebuildOptions* RebuildOptions_new(void);
void NixosRebuilder_init(NixosRebuilder* self, NixosConfig* config);
NixosRebuilder* NixosRebuilder_new(NixosConfig* config);
ExecResult* NixosRebuilder_runNixCollectGarbage(NixosRebuilder* self);
ExecResult* NixosRebuilder_verifyStore(NixosRebuilder* self);
ExecResult* NixosRebuilder_updateFlake(NixosRebuilder* self);
ExecResult* NixosRebuilder_switchSystem(NixosRebuilder* self, RebuildOptions* options);
char* NixosRebuilder_immutabilityDevice(NixosRebuilder* self);
void NixosRebuilder_bootstrapConfigIfMissing(NixosRebuilder* self, RebuildOptions* options);
char* NixosRebuilder_plainTextPasswordPath(NixosRebuilder* self);
char* NixosRebuilder_homeManagerLogs(NixosRebuilder* self);
void NixosRebuilder_refreshImmutableSnapshots(NixosRebuilder* self);
void NixosRebuilder_update(NixosRebuilder* self, RebuildOptions* options);
void DiffOptions_init(DiffOptions* self);
DiffOptions* DiffOptions_new(void);
void DiffScanner_init(DiffScanner* self, NixosConfig* config);
DiffScanner* DiffScanner_new(NixosConfig* config);
btrc_Vector_string* DiffScanner_keepPaths(DiffScanner* self);
btrc_Vector_string* DiffScanner_ignorePatterns(DiffScanner* self, char* path);
btrc_Vector_string* DiffScanner_mountPoints(DiffScanner* self);
btrc_Vector_string* DiffScanner_changedFiles(DiffScanner* self);
bool DiffScanner_isPersisted(DiffScanner* self, char* path, btrc_Vector_string* keepPaths);
bool DiffScanner_matchesPattern(DiffScanner* self, char* path, char* pattern);
bool DiffScanner_ignored(DiffScanner* self, char* path, btrc_Vector_string* patterns);
char* DiffScanner_topAncestor(DiffScanner* self, char* path, btrc_Vector_string* keepList, btrc_Vector_string* mounts);
btrc_Vector_string* DiffScanner_collapse(DiffScanner* self, btrc_Vector_string* paths, btrc_Vector_string* keepList, btrc_Vector_string* mounts);
btrc_Vector_string* DiffScanner_collapseToPersist(DiffScanner* self, btrc_Vector_string* persisted, btrc_Vector_string* keepList);
btrc_Vector_string* DiffScanner_atDepth(DiffScanner* self, btrc_Vector_string* bases, btrc_Vector_string* source, int depth);
btrc_Vector_string* DiffScanner_filterPattern(DiffScanner* self, btrc_Vector_string* input, char* pattern);
btrc_Vector_string* DiffScanner_previousCache(DiffScanner* self, char* cachePath);
void DiffScanner_writeCache(DiffScanner* self, char* cachePath, btrc_Vector_string* paths);
void DiffScanner_print(DiffScanner* self, DiffOptions* options);
void Installer_init(Installer* self, NixosConfig* config);
Installer* Installer_new(NixosConfig* config);
ExecResult* Installer_runDisko(Installer* self, char* mode, char* extraArgs);
ExecResult* Installer_mountDisk(Installer* self);
ExecResult* Installer_eraseAndMountDisk(Installer* self);
ExecResult* Installer_installNixos(Installer* self);
void Installer_permissionNixos(Installer* self);
char* Installer_plainTextPasswordPath(Installer* self);
void Installer_bootstrapConfigIfMissing(Installer* self);
void Installer_collectGarbage(Installer* self);
void Installer_debugShell(Installer* self);
void SecureBootManager_init(SecureBootManager* self, NixosConfig* config);
SecureBootManager* SecureBootManager_new(NixosConfig* config);
void SecureBootManager_removeOldEntries(SecureBootManager* self);
void SecureBootManager_createKeys(SecureBootManager* self);
void SecureBootManager_enrollKeys(SecureBootManager* self, bool microsoft);
char* SecureBootManager_compactJson(SecureBootManager* self, char* text);
btrc_Vector_string* SecureBootManager_unsignedFromJson(SecureBootManager* self, char* json);
void SecureBootManager_verify(SecureBootManager* self);
void SecureBootManager_enable(SecureBootManager* self, bool microsoft);
void SecureBootManager_disable(SecureBootManager* self);
void SecureBootManager_status(SecureBootManager* self);
void Tpm2Manager_init(Tpm2Manager* self, NixosConfig* config);
Tpm2Manager* Tpm2Manager_new(NixosConfig* config);
bool Tpm2Manager_exists(Tpm2Manager* self);
bool Tpm2Manager_diskEncrypted(Tpm2Manager* self);
bool Tpm2Manager_enroll(Tpm2Manager* self);
bool Tpm2Manager_wipe(Tpm2Manager* self);
void Tpm2Manager_status(Tpm2Manager* self);
void PasswordManager_init(PasswordManager* self, NixosConfig* config);
PasswordManager* PasswordManager_new(NixosConfig* config);
bool PasswordManager_changeLuksPassword(PasswordManager* self, char* oldPassword, char* newPassword);
void PasswordManager_fallbackChangeUserPassword(PasswordManager* self, char* user, char* password);
void PasswordManager_changeUserPassword(PasswordManager* self, char* oldPassword, char* password);
void PasswordManager_updateTpm2(PasswordManager* self);
void PasswordManager_change(PasswordManager* self, char* oldPassword, char* newPassword, bool changeFde, bool changeUser, bool updateTpm);
void DisplayLayoutRule_init(DisplayLayoutRule* self);
DisplayLayoutRule* DisplayLayoutRule_new(void);
void AudioPreset_init(AudioPreset* self);
AudioPreset* AudioPreset_new(void);
void LabelsConfig_init(LabelsConfig* self);
LabelsConfig* LabelsConfig_new(void);
char* LabelsConfig_displayLabel(LabelsConfig* self, char* name);
btrc_Vector_DisplayLayoutRule* LabelsConfig_layoutRules(LabelsConfig* self);
btrc_Vector_AudioPreset* LabelsConfig_audioPresets(LabelsConfig* self);
void AudioSink_init(AudioSink* self);
AudioSink* AudioSink_new(void);
void AudioManager_init(AudioManager* self);
AudioManager* AudioManager_new(void);
char* AudioManager_current(AudioManager* self);
char* AudioManager_jsonStringValue(AudioManager* self, char* line, char* key);
btrc_Vector_AudioSink* AudioManager_sinks(AudioManager* self);
char* AudioManager_entryJson(AudioManager* self, char* name, char* label, char* description, bool isDefault);
void AudioManager_list(AudioManager* self);
AudioPreset* AudioManager_findPreset(AudioManager* self, char* selector);
void AudioManager_applyPreset(AudioManager* self, AudioPreset* preset);
void AudioManager_set(AudioManager* self, char* selector);
void CaffeineManager_init(CaffeineManager* self);
CaffeineManager* CaffeineManager_new(void);
int CaffeineManager_pid(CaffeineManager* self);
bool CaffeineManager_enabled(CaffeineManager* self);
void CaffeineManager_enable(CaffeineManager* self);
void CaffeineManager_disable(CaffeineManager* self);
void CaffeineManager_toggle(CaffeineManager* self);
void DisplayOutput_init(DisplayOutput* self);
DisplayOutput* DisplayOutput_new(void);
char* DisplayOutput_json(DisplayOutput* self);
void DisplayManager_init(DisplayManager* self);
DisplayManager* DisplayManager_new(void);
char* DisplayManager_kscreen(DisplayManager* self);
bool DisplayManager_drmConnected(DisplayManager* self, char* name);
char* DisplayManager_lineValue(DisplayManager* self, char* block, char* prefix);
DisplayOutput* DisplayManager_parseBlock(DisplayManager* self, char* block);
btrc_Vector_DisplayOutput* DisplayManager_outputs(DisplayManager* self);
DisplayOutput* DisplayManager_findOutput(DisplayManager* self, char* name, btrc_Vector_DisplayOutput* outputs);
void DisplayManager_run(DisplayManager* self, char* arg);
void DisplayManager_enable(DisplayManager* self, char* name);
void DisplayManager_disable(DisplayManager* self, char* name);
void DisplayManager_primary(DisplayManager* self, char* name);
void DisplayManager_dpms(DisplayManager* self, char* state);
void DisplayManager_layout(DisplayManager* self);
void DisplayManager_applyLayout(DisplayManager* self, char* name);
void DisplayManager_list(DisplayManager* self);
void SystemUi_init(SystemUi* self);
SystemUi* SystemUi_new(void);
void SystemUi_terminal(SystemUi* self, char* command);
void SystemUi_update(SystemUi* self);
void SystemUi_upgrade(SystemUi* self);
void VmOperation_init(VmOperation* self);
VmOperation* VmOperation_new(void);
void VmOperation_expandArgs(VmOperation* self, btrc_Map_string_string* args);
void VmTestSpec_init(VmTestSpec* self);
VmTestSpec* VmTestSpec_new(void);
void VmTestSpec_setArg(VmTestSpec* self, char* key, char* value);
void VmTestSpec_setArgPair(VmTestSpec* self, char* pair);
void VmTestSpec_setDerivedArgs(VmTestSpec* self);
void VmTestSpec_refreshDerivedArgs(VmTestSpec* self);
char* VmTestSpec_stateDir(VmTestSpec* self);
char* VmTestSpec_stateHashFile(VmTestSpec* self);
char* VmTestSpec_parentHashFile(VmTestSpec* self);
char* VmTestSpec_resolveParentHash(VmTestSpec* self);
char* VmTestSpec_operationsMaterial(VmTestSpec* self);
char* VmTestSpec_hashMaterial(VmTestSpec* self);
void VmTestSpec_computeStateHash(VmTestSpec* self);
void VmTestSpec_expandArgs(VmTestSpec* self);
void VmSpecParser_init(VmSpecParser* self);
char* VmSpecParser_hostArch(void);
char* VmSpecParser_defaultIsoUrl(char* arch);
char* VmSpecParser_expandArgs(char* text, btrc_Map_string_string* args);
int VmSpecParser_skipSpaces(char* text, int i);
int VmSpecParser_keyPosition(char* text, char* key);
char* VmSpecParser_objectField(char* text, char* key);
btrc_Map_string_string* VmSpecParser_argsObject(char* text);
char* VmSpecParser_parseStringValue(char* text, int i, char* fallback);
char* VmSpecParser_field(char* text, char* key, char* fallback);
int VmSpecParser_intField(char* text, char* key, int fallback);
VmOperation* VmSpecParser_operation(char* objectText);
btrc_Vector_VmOperation* VmSpecParser_operations(char* text);
VmTestSpec* VmSpecParser_parse(char* text);
void VmSpecParser_applySpecField(VmTestSpec* spec, char* key, char* value);
void VmSpecParser_applyOperationField(VmOperation* op, char* key, char* value);
bool VmSpecParser_hasOperation(VmOperation* op);
char* VmSpecParser_yamlKey(char* line);
char* VmSpecParser_yamlValue(char* line);
VmTestSpec* VmSpecParser_parseToml(char* text);
VmTestSpec* VmSpecParser_parseYaml(char* text);
VmTestSpec* VmSpecParser_readFile(char* path);
void QemuE2eHarness_init(QemuE2eHarness* self, VmTestSpec* spec);
QemuE2eHarness* QemuE2eHarness_new(VmTestSpec* spec);
char* QemuE2eHarness_diskPath(QemuE2eHarness* self);
char* QemuE2eHarness_pidPath(QemuE2eHarness* self);
char* QemuE2eHarness_monitorPath(QemuE2eHarness* self);
char* QemuE2eHarness_qmpPath(QemuE2eHarness* self);
char* QemuE2eHarness_serialBasePath(QemuE2eHarness* self);
char* QemuE2eHarness_serialInPath(QemuE2eHarness* self);
char* QemuE2eHarness_serialOutPath(QemuE2eHarness* self);
char* QemuE2eHarness_serialLogPath(QemuE2eHarness* self);
char* QemuE2eHarness_serialReaderPidPath(QemuE2eHarness* self);
char* QemuE2eHarness_tpmDir(QemuE2eHarness* self);
char* QemuE2eHarness_tpmStateDir(QemuE2eHarness* self);
char* QemuE2eHarness_tpmSocketPath(QemuE2eHarness* self);
char* QemuE2eHarness_tpmPidPath(QemuE2eHarness* self);
char* QemuE2eHarness_tpmLogPath(QemuE2eHarness* self);
char* QemuE2eHarness_sshKeyPath(QemuE2eHarness* self);
char* QemuE2eHarness_sshPubKeyPath(QemuE2eHarness* self);
char* QemuE2eHarness_bootDir(QemuE2eHarness* self);
char* QemuE2eHarness_bootKernelFile(QemuE2eHarness* self);
char* QemuE2eHarness_bootInitrdFile(QemuE2eHarness* self);
char* QemuE2eHarness_bootAppendFile(QemuE2eHarness* self);
char* QemuE2eHarness_firmwareVarsPath(QemuE2eHarness* self);
char* QemuE2eHarness_firmwareVarsSnapshotPath(QemuE2eHarness* self, char* name);
char* QemuE2eHarness_parentStateDir(QemuE2eHarness* self);
char* QemuE2eHarness_parentWorkDirFile(QemuE2eHarness* self);
char* QemuE2eHarness_backingDiskFile(QemuE2eHarness* self);
char* QemuE2eHarness_absolutePath(QemuE2eHarness* self, char* path);
void QemuE2eHarness_ensureWorkDir(QemuE2eHarness* self);
char* QemuE2eHarness_qemuBinary(QemuE2eHarness* self);
bool QemuE2eHarness_argEnabled(QemuE2eHarness* self, char* key);
bool QemuE2eHarness_tpm2Enabled(QemuE2eHarness* self);
bool QemuE2eHarness_secureBootEnabled(QemuE2eHarness* self);
bool QemuE2eHarness_uefiEnabled(QemuE2eHarness* self);
bool QemuE2eHarness_shouldUseUefi(QemuE2eHarness* self, bool fromIso);
bool QemuE2eHarness_hostArchMatchesGuest(QemuE2eHarness* self);
char* QemuE2eHarness_qemuSharePath(QemuE2eHarness* self, char* fileName);
char* QemuE2eHarness_firmwareCodePath(QemuE2eHarness* self);
char* QemuE2eHarness_secureFirmwareCodePath(QemuE2eHarness* self);
char* QemuE2eHarness_firmwareVarsTemplatePath(QemuE2eHarness* self);
void QemuE2eHarness_makeFirmwareVarsWritable(QemuE2eHarness* self);
void QemuE2eHarness_setupFirmwareVars(QemuE2eHarness* self);
bool QemuE2eHarness_commandExists(QemuE2eHarness* self, char* name);
void QemuE2eHarness_requireCommand(QemuE2eHarness* self, char* name);
bool QemuE2eHarness_qemuDeviceAvailable(QemuE2eHarness* self, char* deviceName);
char* QemuE2eHarness_tpmQemuDevice(QemuE2eHarness* self);
void QemuE2eHarness_requireTpm2Capability(QemuE2eHarness* self);
void QemuE2eHarness_requireUefiCapability(QemuE2eHarness* self);
void QemuE2eHarness_requireSecureBootCapability(QemuE2eHarness* self);
char* QemuE2eHarness_secureBootCapabilityReport(QemuE2eHarness* self);
bool QemuE2eHarness_isDarwin(QemuE2eHarness* self);
char* QemuE2eHarness_stripLeadingSlash(QemuE2eHarness* self, char* path);
char* QemuE2eHarness_valueAfterLinePrefix(QemuE2eHarness* self, char* text, char* prefix, int start);
void QemuE2eHarness_extractBootSerial(QemuE2eHarness* self);
void QemuE2eHarness_downloadIso(QemuE2eHarness* self);
void QemuE2eHarness_createSshKey(QemuE2eHarness* self);
char* QemuE2eHarness_sshPubKey(QemuE2eHarness* self);
void QemuE2eHarness_createDisk(QemuE2eHarness* self);
void QemuE2eHarness_setup(QemuE2eHarness* self);
void QemuE2eHarness_resetState(QemuE2eHarness* self);
void QemuE2eHarness_cleanStateRecord(QemuE2eHarness* self);
void QemuE2eHarness_requireParentState(QemuE2eHarness* self);
void QemuE2eHarness_copyIfExists(QemuE2eHarness* self, char* source, char* target);
void QemuE2eHarness_copyTreeIfExists(QemuE2eHarness* self, char* source, char* target);
void QemuE2eHarness_inheritState(QemuE2eHarness* self);
void QemuE2eHarness_recordState(QemuE2eHarness* self);
bool QemuE2eHarness_isRunning(QemuE2eHarness* self);
bool QemuE2eHarness_hasSnapshot(QemuE2eHarness* self, char* name);
bool QemuE2eHarness_hasBackingDisk(QemuE2eHarness* self);
char* QemuE2eHarness_sshOptionsForShell(QemuE2eHarness* self);
void QemuE2eHarness_printStatus(QemuE2eHarness* self);
void QemuE2eHarness_prepareSerialPipe(QemuE2eHarness* self);
void QemuE2eHarness_startSerialReader(QemuE2eHarness* self);
void QemuE2eHarness_stopSerialReader(QemuE2eHarness* self);
void QemuE2eHarness_startSwtpm(QemuE2eHarness* self);
void QemuE2eHarness_stopSwtpm(QemuE2eHarness* self);
void QemuE2eHarness_addFirmware(QemuE2eHarness* self, Command* cmd, bool fromIso);
void QemuE2eHarness_addTpm2(QemuE2eHarness* self, Command* cmd);
void QemuE2eHarness_start(QemuE2eHarness* self, bool fromIso);
void QemuE2eHarness_stop(QemuE2eHarness* self);
void QemuE2eHarness_sleepSeconds(QemuE2eHarness* self, int seconds);
void QemuE2eHarness_serialSend(QemuE2eHarness* self, char* command);
void QemuE2eHarness_bootstrapSsh(QemuE2eHarness* self);
void QemuE2eHarness_upFromIso(QemuE2eHarness* self);
void QemuE2eHarness_rebootDisk(QemuE2eHarness* self);
Command* QemuE2eHarness_addSshOptions(QemuE2eHarness* self, Command* cmd);
ExecResult* QemuE2eHarness_sshWithTimeout(QemuE2eHarness* self, char* command, bool checkStatus, int timeoutSeconds);
ExecResult* QemuE2eHarness_ssh(QemuE2eHarness* self, char* command, bool checkStatus);
bool QemuE2eHarness_waitForSsh(QemuE2eHarness* self, int timeout);
ExecResult* QemuE2eHarness_host(QemuE2eHarness* self, char* command, bool checkStatus);
char* QemuE2eHarness_workspaceRoot(QemuE2eHarness* self);
ExecResult* QemuE2eHarness_workspaceFileExists(QemuE2eHarness* self, char* relativePath);
ExecResult* QemuE2eHarness_nixEval(QemuE2eHarness* self, char* attribute, int timeoutSeconds);
ExecResult* QemuE2eHarness_qmp(QemuE2eHarness* self, char* command, int timeoutSeconds);
void QemuE2eHarness_copyWorkspace(QemuE2eHarness* self, char* localPath, char* remotePath);
void QemuE2eHarness_configureVmHost(QemuE2eHarness* self);
void QemuE2eHarness_installNixosGuest(QemuE2eHarness* self);
void QemuE2eHarness_snapshot(QemuE2eHarness* self, char* name);
void QemuE2eHarness_restore(QemuE2eHarness* self, char* name);
void QemuE2eHarness_copyTo(QemuE2eHarness* self, char* localPath, char* remotePath);
void QemuE2eHarness_copyFrom(QemuE2eHarness* self, char* remotePath, char* localPath);
void VmOperationCatalog_init(VmOperationCatalog* self);
btrc_Vector_string* VmOperationCatalog_all(void);
void VmTestRunner_init(VmTestRunner* self, VmTestSpec* spec);
VmTestRunner* VmTestRunner_new(VmTestSpec* spec);
void VmTestRunner_fail(VmTestRunner* self, char* message);
bool VmTestRunner_outputMatches(VmTestRunner* self, ExecResult* result, char* expect);
void VmTestRunner_assertResult(VmTestRunner* self, char* label, ExecResult* result, char* expect);
void VmTestRunner_runOperation(VmTestRunner* self, VmOperation* op);
int VmTestRunner_run(VmTestRunner* self);
void VmGraphNode_init(VmGraphNode* self);
VmGraphNode* VmGraphNode_new(void);
void VmTestGraph_init(VmTestGraph* self);
VmTestGraph* VmTestGraph_new(void);
VmGraphNode* VmTestGraph_node(VmTestGraph* self, char* id);
char* VmTestGraph_resolvedSpecPath(VmTestGraph* self, VmGraphNode* node);
char* VmTestGraph_resolvedWorkspaceRoot(VmTestGraph* self);
btrc_Vector_string* VmTestGraph_defaultTargets(VmTestGraph* self);
void VmGraphParser_init(VmGraphParser* self);
btrc_Vector_string* VmGraphParser_stringArray(char* text, char* key);
btrc_Vector_string* VmGraphParser_objectArray(char* text, char* key);
VmGraphNode* VmGraphParser_node(char* objectText);
VmTestGraph* VmGraphParser_readFile(char* path);
void VmGraphRunner_init(VmGraphRunner* self, VmTestGraph* graph, btrc_Map_string_string* args);
VmGraphRunner* VmGraphRunner_new(VmTestGraph* graph, btrc_Map_string_string* args);
char* VmGraphRunner_sourceHash(VmGraphRunner* self);
VmTestSpec* VmGraphRunner_specFor(VmGraphRunner* self, VmGraphNode* node);
void VmGraphRunner_applyStructuralOverrides(VmGraphRunner* self, VmTestSpec* spec, btrc_Map_string_string* overrides);
void VmGraphRunner_list(VmGraphRunner* self);
void VmGraphRunner_status(VmGraphRunner* self);
int VmGraphRunner_operationCoverage(VmGraphRunner* self);
bool VmGraphRunner_force(VmGraphRunner* self);
bool VmGraphRunner_ready(VmGraphRunner* self, VmTestSpec* spec);
int VmGraphRunner_runNode(VmGraphRunner* self, char* id);
int VmGraphRunner_run(VmGraphRunner* self, btrc_Vector_string* targets);
void NixosCtl_init(NixosCtl* self);
NixosCtl* NixosCtl_new(void);
char* NixosCtl_env(char* name, char* fallback);
void NixosCtl_usage(NixosCtl* self);
void NixosCtl_applySpecArgs(NixosCtl* self, VmTestSpec* spec, CliArgs* args, int startIndex);
char* NixosCtl_tail(NixosCtl* self, CliArgs* args, int startIndex);
bool NixosCtl_needsRoot(NixosCtl* self, char* command);
int NixosCtl_sudoSelf(NixosCtl* self, CliArgs* args);
int NixosCtl_runVm(NixosCtl* self, CliArgs* args);
btrc_Map_string_string* NixosCtl_graphArgs(NixosCtl* self, CliArgs* args, int startIndex);
btrc_Vector_string* NixosCtl_graphTargets(NixosCtl* self, CliArgs* args, int startIndex);
int NixosCtl_runGraph(NixosCtl* self, CliArgs* args);
int NixosCtl_run(NixosCtl* self, CliArgs* args);
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
typedef bool (*__btrc_fn_bool_ResetSubvolume)(ResetSubvolume*);
typedef void (*__btrc_fn_void_ResetSubvolume)(ResetSubvolume*);
typedef ResetSubvolume* (*__btrc_fn_ResetSubvolume_ResetSubvolume)(ResetSubvolume*);
typedef ResetSubvolume* (*__btrc_fn_ResetSubvolume_ResetSubvolume_ResetSubvolume)(ResetSubvolume*, ResetSubvolume*);
typedef bool (*__btrc_fn_bool_DisplayLayoutRule)(DisplayLayoutRule*);
typedef void (*__btrc_fn_void_DisplayLayoutRule)(DisplayLayoutRule*);
typedef DisplayLayoutRule* (*__btrc_fn_DisplayLayoutRule_DisplayLayoutRule)(DisplayLayoutRule*);
typedef DisplayLayoutRule* (*__btrc_fn_DisplayLayoutRule_DisplayLayoutRule_DisplayLayoutRule)(DisplayLayoutRule*, DisplayLayoutRule*);
typedef bool (*__btrc_fn_bool_AudioPreset)(AudioPreset*);
typedef void (*__btrc_fn_void_AudioPreset)(AudioPreset*);
typedef AudioPreset* (*__btrc_fn_AudioPreset_AudioPreset)(AudioPreset*);
typedef AudioPreset* (*__btrc_fn_AudioPreset_AudioPreset_AudioPreset)(AudioPreset*, AudioPreset*);
typedef bool (*__btrc_fn_bool_AudioSink)(AudioSink*);
typedef void (*__btrc_fn_void_AudioSink)(AudioSink*);
typedef AudioSink* (*__btrc_fn_AudioSink_AudioSink)(AudioSink*);
typedef AudioSink* (*__btrc_fn_AudioSink_AudioSink_AudioSink)(AudioSink*, AudioSink*);
typedef bool (*__btrc_fn_bool_DisplayOutput)(DisplayOutput*);
typedef void (*__btrc_fn_void_DisplayOutput)(DisplayOutput*);
typedef DisplayOutput* (*__btrc_fn_DisplayOutput_DisplayOutput)(DisplayOutput*);
typedef DisplayOutput* (*__btrc_fn_DisplayOutput_DisplayOutput_DisplayOutput)(DisplayOutput*, DisplayOutput*);
typedef bool (*__btrc_fn_bool_VmOperation)(VmOperation*);
typedef void (*__btrc_fn_void_VmOperation)(VmOperation*);
typedef VmOperation* (*__btrc_fn_VmOperation_VmOperation)(VmOperation*);
typedef VmOperation* (*__btrc_fn_VmOperation_VmOperation_VmOperation)(VmOperation*, VmOperation*);
typedef bool (*__btrc_fn_bool_VmGraphNode)(VmGraphNode*);
typedef void (*__btrc_fn_void_VmGraphNode)(VmGraphNode*);
typedef VmGraphNode* (*__btrc_fn_VmGraphNode_VmGraphNode)(VmGraphNode*);
typedef VmGraphNode* (*__btrc_fn_VmGraphNode_VmGraphNode_VmGraphNode)(VmGraphNode*, VmGraphNode*);

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

struct btrc_Vector_ResetSubvolume {
    int __rc;
    ResetSubvolume** data;
    int len;
    int cap;
};

struct btrc_Vector_DisplayLayoutRule {
    int __rc;
    DisplayLayoutRule** data;
    int len;
    int cap;
};

struct btrc_Vector_AudioPreset {
    int __rc;
    AudioPreset** data;
    int len;
    int cap;
};

struct btrc_Vector_AudioSink {
    int __rc;
    AudioSink** data;
    int len;
    int cap;
};

struct btrc_Vector_DisplayOutput {
    int __rc;
    DisplayOutput** data;
    int len;
    int cap;
};

struct btrc_Vector_VmOperation {
    int __rc;
    VmOperation** data;
    int len;
    int cap;
};

struct btrc_Vector_VmGraphNode {
    int __rc;
    VmGraphNode** data;
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

struct NixosLog {
    int __rc;
};

struct NixosPaths {
    int __rc;
    char* root;
};

struct LocalConfigFile {
    int __rc;
    char* path;
};

struct Interactive {
    int __rc;
    UnixShell* shell;
};

struct NixEvalCache {
    int __rc;
    btrc_Map_string_string* values;
};

struct NixosConfig {
    int __rc;
    NixosPaths* paths;
    LocalConfigFile* local;
    UnixShell* shell;
    NixEvalCache* cache;
};

struct SecretsManager {
    int __rc;
    NixosConfig* config;
    UnixShell* shell;
    Interactive* interactive;
};

struct PermissionsManager {
    int __rc;
    NixosConfig* config;
    UnixShell* shell;
};

struct ResetSubvolume {
    int __rc;
    char* name;
    char* mountPoint;
};

struct SnapshotManager {
    int __rc;
    NixosConfig* config;
    UnixShell* shell;
    char* rootPrefix;
};

struct RebuildOptions {
    int __rc;
    bool rebuildFileSystem;
    bool reboot;
    bool clean;
    bool upgrade;
};

struct NixosRebuilder {
    int __rc;
    NixosConfig* config;
    UnixShell* shell;
    Interactive* interactive;
};

struct DiffOptions {
    int __rc;
    bool recent;
    bool showSymlinks;
    bool showPersistPaths;
    char* showChildren;
    int depth;
    char* pattern;
    char* diffignore;
};

struct DiffScanner {
    int __rc;
    NixosConfig* config;
    SnapshotManager* snapshots;
    UnixShell* shell;
};

struct Installer {
    int __rc;
    NixosConfig* config;
    UnixShell* shell;
    Interactive* interactive;
    char* mountPoint;
};

struct SecureBootManager {
    int __rc;
    NixosConfig* config;
    UnixShell* shell;
};

struct Tpm2Manager {
    int __rc;
    NixosConfig* config;
    UnixShell* shell;
};

struct PasswordManager {
    int __rc;
    NixosConfig* config;
    UnixShell* shell;
    SecretsManager* secrets;
};

struct DisplayLayoutRule {
    int __rc;
    char* display;
    char* position;
    char* relativeTo;
};

struct AudioPreset {
    int __rc;
    char* label;
    char* card;
    char* profile;
    char* sink;
    char* volume;
};

struct LabelsConfig {
    int __rc;
    char* path;
    char* content;
};

struct AudioSink {
    int __rc;
    char* name;
    char* description;
};

struct AudioManager {
    int __rc;
    UnixShell* shell;
    LabelsConfig* labels;
};

struct CaffeineManager {
    int __rc;
    UnixShell* shell;
    char* pidFile;
};

struct DisplayOutput {
    int __rc;
    char* name;
    char* label;
    char* kind;
    int priority;
    int x;
    int y;
    int width;
    int height;
    bool enabled;
    bool connected;
};

struct DisplayManager {
    int __rc;
    UnixShell* shell;
    LabelsConfig* labels;
};

struct SystemUi {
    int __rc;
    UnixShell* shell;
};

struct VmOperation {
    int __rc;
    char* kind;
    char* command;
    char* expect;
    char* name;
    char* localPath;
    char* remotePath;
    int timeout;
};

struct VmTestSpec {
    int __rc;
    char* name;
    char* workDir;
    char* arch;
    char* iso;
    char* isoUrl;
    char* diskSize;
    char* memory;
    int cpus;
    int sshPort;
    char* state;
    char* parentState;
    char* stateRoot;
    char* stateMaterial;
    char* parentHash;
    char* stateHash;
    char* stateHashShort;
    btrc_Map_string_string* args;
    btrc_Vector_VmOperation* operations;
};

struct VmSpecParser {
    int __rc;
};

struct QemuE2eHarness {
    int __rc;
    VmTestSpec* spec;
    UnixShell* shell;
};

struct VmOperationCatalog {
    int __rc;
};

struct VmTestRunner {
    int __rc;
    VmTestSpec* spec;
    QemuE2eHarness* vm;
    int failures;
};

struct VmGraphNode {
    int __rc;
    char* id;
    char* specPath;
    btrc_Vector_string* after;
    btrc_Map_string_string* args;
};

struct VmTestGraph {
    int __rc;
    char* name;
    char* path;
    char* baseDir;
    char* workspaceRoot;
    btrc_Vector_string* defaults;
    btrc_Vector_VmGraphNode* nodes;
};

struct VmGraphParser {
    int __rc;
};

struct VmGraphRunner {
    int __rc;
    VmTestGraph* graph;
    btrc_Map_string_string* args;
    btrc_Vector_string* done;
    btrc_Vector_string* visiting;
    char* sourceHashValue;
};

struct NixosCtl {
    int __rc;
    NixosConfig* config;
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

static void btrc_Vector_ResetSubvolume_init(btrc_Vector_ResetSubvolume* self);
static btrc_Vector_ResetSubvolume* btrc_Vector_ResetSubvolume_new(void);
static void btrc_Vector_ResetSubvolume_destroy(btrc_Vector_ResetSubvolume* self);
static void btrc_Vector_ResetSubvolume_push(btrc_Vector_ResetSubvolume* self, ResetSubvolume* val);
static ResetSubvolume* btrc_Vector_ResetSubvolume_pop(btrc_Vector_ResetSubvolume* self);
static ResetSubvolume* btrc_Vector_ResetSubvolume_get(btrc_Vector_ResetSubvolume* self, int i);
static void btrc_Vector_ResetSubvolume_set(btrc_Vector_ResetSubvolume* self, int i, ResetSubvolume* val);
static void btrc_Vector_ResetSubvolume_free(btrc_Vector_ResetSubvolume* self);
static void btrc_Vector_ResetSubvolume_remove(btrc_Vector_ResetSubvolume* self, int idx);
static void btrc_Vector_ResetSubvolume_reverse(btrc_Vector_ResetSubvolume* self);
static btrc_Vector_ResetSubvolume* btrc_Vector_ResetSubvolume_reversed(btrc_Vector_ResetSubvolume* self);
static void btrc_Vector_ResetSubvolume_swap(btrc_Vector_ResetSubvolume* self, int i, int j);
static void btrc_Vector_ResetSubvolume_clear(btrc_Vector_ResetSubvolume* self);
static void btrc_Vector_ResetSubvolume_fill(btrc_Vector_ResetSubvolume* self, ResetSubvolume* val);
static int btrc_Vector_ResetSubvolume_size(btrc_Vector_ResetSubvolume* self);
static bool btrc_Vector_ResetSubvolume_isEmpty(btrc_Vector_ResetSubvolume* self);
static ResetSubvolume* btrc_Vector_ResetSubvolume_first(btrc_Vector_ResetSubvolume* self);
static ResetSubvolume* btrc_Vector_ResetSubvolume_last(btrc_Vector_ResetSubvolume* self);
static btrc_Vector_ResetSubvolume* btrc_Vector_ResetSubvolume_slice(btrc_Vector_ResetSubvolume* self, int start, int end);
static btrc_Vector_ResetSubvolume* btrc_Vector_ResetSubvolume_take(btrc_Vector_ResetSubvolume* self, int n);
static btrc_Vector_ResetSubvolume* btrc_Vector_ResetSubvolume_drop(btrc_Vector_ResetSubvolume* self, int n);
static void btrc_Vector_ResetSubvolume_extend(btrc_Vector_ResetSubvolume* self, btrc_Vector_ResetSubvolume* other);
static void btrc_Vector_ResetSubvolume_insert(btrc_Vector_ResetSubvolume* self, int idx, ResetSubvolume* val);
static bool btrc_Vector_ResetSubvolume_contains(btrc_Vector_ResetSubvolume* self, ResetSubvolume* val);
static int btrc_Vector_ResetSubvolume_indexOf(btrc_Vector_ResetSubvolume* self, ResetSubvolume* val);
static int btrc_Vector_ResetSubvolume_lastIndexOf(btrc_Vector_ResetSubvolume* self, ResetSubvolume* val);
static int btrc_Vector_ResetSubvolume_count(btrc_Vector_ResetSubvolume* self, ResetSubvolume* val);
static void btrc_Vector_ResetSubvolume_removeAll(btrc_Vector_ResetSubvolume* self, ResetSubvolume* val);
static btrc_Vector_ResetSubvolume* btrc_Vector_ResetSubvolume_distinct(btrc_Vector_ResetSubvolume* self);
static void btrc_Vector_ResetSubvolume_sort(btrc_Vector_ResetSubvolume* self);
static btrc_Vector_ResetSubvolume* btrc_Vector_ResetSubvolume_sorted(btrc_Vector_ResetSubvolume* self);
static ResetSubvolume* btrc_Vector_ResetSubvolume_min(btrc_Vector_ResetSubvolume* self);
static ResetSubvolume* btrc_Vector_ResetSubvolume_max(btrc_Vector_ResetSubvolume* self);
static ResetSubvolume* btrc_Vector_ResetSubvolume_sum(btrc_Vector_ResetSubvolume* self);
static char* btrc_Vector_ResetSubvolume_join(btrc_Vector_ResetSubvolume* self, char* sep);
static char* btrc_Vector_ResetSubvolume_joinToString(btrc_Vector_ResetSubvolume* self, char* sep);
static btrc_Vector_ResetSubvolume* btrc_Vector_ResetSubvolume_filter(btrc_Vector_ResetSubvolume* self, __btrc_fn_bool_ResetSubvolume pred);
static int btrc_Vector_ResetSubvolume_findIndex(btrc_Vector_ResetSubvolume* self, __btrc_fn_bool_ResetSubvolume pred);
static void btrc_Vector_ResetSubvolume_forEach(btrc_Vector_ResetSubvolume* self, __btrc_fn_void_ResetSubvolume fn);
static btrc_Vector_ResetSubvolume* btrc_Vector_ResetSubvolume_map(btrc_Vector_ResetSubvolume* self, __btrc_fn_ResetSubvolume_ResetSubvolume fn);
static bool btrc_Vector_ResetSubvolume_any(btrc_Vector_ResetSubvolume* self, __btrc_fn_bool_ResetSubvolume pred);
static bool btrc_Vector_ResetSubvolume_all(btrc_Vector_ResetSubvolume* self, __btrc_fn_bool_ResetSubvolume pred);
static ResetSubvolume* btrc_Vector_ResetSubvolume_reduce(btrc_Vector_ResetSubvolume* self, ResetSubvolume* init, __btrc_fn_ResetSubvolume_ResetSubvolume_ResetSubvolume fn);
static btrc_Vector_ResetSubvolume* btrc_Vector_ResetSubvolume_copy(btrc_Vector_ResetSubvolume* self);
static void btrc_Vector_ResetSubvolume_removeAt(btrc_Vector_ResetSubvolume* self, int idx);
static int btrc_Vector_ResetSubvolume_iterLen(btrc_Vector_ResetSubvolume* self);
static ResetSubvolume* btrc_Vector_ResetSubvolume_iterGet(btrc_Vector_ResetSubvolume* self, int i);

static void btrc_Vector_DisplayLayoutRule_init(btrc_Vector_DisplayLayoutRule* self);
static btrc_Vector_DisplayLayoutRule* btrc_Vector_DisplayLayoutRule_new(void);
static void btrc_Vector_DisplayLayoutRule_destroy(btrc_Vector_DisplayLayoutRule* self);
static void btrc_Vector_DisplayLayoutRule_push(btrc_Vector_DisplayLayoutRule* self, DisplayLayoutRule* val);
static DisplayLayoutRule* btrc_Vector_DisplayLayoutRule_pop(btrc_Vector_DisplayLayoutRule* self);
static DisplayLayoutRule* btrc_Vector_DisplayLayoutRule_get(btrc_Vector_DisplayLayoutRule* self, int i);
static void btrc_Vector_DisplayLayoutRule_set(btrc_Vector_DisplayLayoutRule* self, int i, DisplayLayoutRule* val);
static void btrc_Vector_DisplayLayoutRule_free(btrc_Vector_DisplayLayoutRule* self);
static void btrc_Vector_DisplayLayoutRule_remove(btrc_Vector_DisplayLayoutRule* self, int idx);
static void btrc_Vector_DisplayLayoutRule_reverse(btrc_Vector_DisplayLayoutRule* self);
static btrc_Vector_DisplayLayoutRule* btrc_Vector_DisplayLayoutRule_reversed(btrc_Vector_DisplayLayoutRule* self);
static void btrc_Vector_DisplayLayoutRule_swap(btrc_Vector_DisplayLayoutRule* self, int i, int j);
static void btrc_Vector_DisplayLayoutRule_clear(btrc_Vector_DisplayLayoutRule* self);
static void btrc_Vector_DisplayLayoutRule_fill(btrc_Vector_DisplayLayoutRule* self, DisplayLayoutRule* val);
static int btrc_Vector_DisplayLayoutRule_size(btrc_Vector_DisplayLayoutRule* self);
static bool btrc_Vector_DisplayLayoutRule_isEmpty(btrc_Vector_DisplayLayoutRule* self);
static DisplayLayoutRule* btrc_Vector_DisplayLayoutRule_first(btrc_Vector_DisplayLayoutRule* self);
static DisplayLayoutRule* btrc_Vector_DisplayLayoutRule_last(btrc_Vector_DisplayLayoutRule* self);
static btrc_Vector_DisplayLayoutRule* btrc_Vector_DisplayLayoutRule_slice(btrc_Vector_DisplayLayoutRule* self, int start, int end);
static btrc_Vector_DisplayLayoutRule* btrc_Vector_DisplayLayoutRule_take(btrc_Vector_DisplayLayoutRule* self, int n);
static btrc_Vector_DisplayLayoutRule* btrc_Vector_DisplayLayoutRule_drop(btrc_Vector_DisplayLayoutRule* self, int n);
static void btrc_Vector_DisplayLayoutRule_extend(btrc_Vector_DisplayLayoutRule* self, btrc_Vector_DisplayLayoutRule* other);
static void btrc_Vector_DisplayLayoutRule_insert(btrc_Vector_DisplayLayoutRule* self, int idx, DisplayLayoutRule* val);
static bool btrc_Vector_DisplayLayoutRule_contains(btrc_Vector_DisplayLayoutRule* self, DisplayLayoutRule* val);
static int btrc_Vector_DisplayLayoutRule_indexOf(btrc_Vector_DisplayLayoutRule* self, DisplayLayoutRule* val);
static int btrc_Vector_DisplayLayoutRule_lastIndexOf(btrc_Vector_DisplayLayoutRule* self, DisplayLayoutRule* val);
static int btrc_Vector_DisplayLayoutRule_count(btrc_Vector_DisplayLayoutRule* self, DisplayLayoutRule* val);
static void btrc_Vector_DisplayLayoutRule_removeAll(btrc_Vector_DisplayLayoutRule* self, DisplayLayoutRule* val);
static btrc_Vector_DisplayLayoutRule* btrc_Vector_DisplayLayoutRule_distinct(btrc_Vector_DisplayLayoutRule* self);
static void btrc_Vector_DisplayLayoutRule_sort(btrc_Vector_DisplayLayoutRule* self);
static btrc_Vector_DisplayLayoutRule* btrc_Vector_DisplayLayoutRule_sorted(btrc_Vector_DisplayLayoutRule* self);
static DisplayLayoutRule* btrc_Vector_DisplayLayoutRule_min(btrc_Vector_DisplayLayoutRule* self);
static DisplayLayoutRule* btrc_Vector_DisplayLayoutRule_max(btrc_Vector_DisplayLayoutRule* self);
static DisplayLayoutRule* btrc_Vector_DisplayLayoutRule_sum(btrc_Vector_DisplayLayoutRule* self);
static char* btrc_Vector_DisplayLayoutRule_join(btrc_Vector_DisplayLayoutRule* self, char* sep);
static char* btrc_Vector_DisplayLayoutRule_joinToString(btrc_Vector_DisplayLayoutRule* self, char* sep);
static btrc_Vector_DisplayLayoutRule* btrc_Vector_DisplayLayoutRule_filter(btrc_Vector_DisplayLayoutRule* self, __btrc_fn_bool_DisplayLayoutRule pred);
static int btrc_Vector_DisplayLayoutRule_findIndex(btrc_Vector_DisplayLayoutRule* self, __btrc_fn_bool_DisplayLayoutRule pred);
static void btrc_Vector_DisplayLayoutRule_forEach(btrc_Vector_DisplayLayoutRule* self, __btrc_fn_void_DisplayLayoutRule fn);
static btrc_Vector_DisplayLayoutRule* btrc_Vector_DisplayLayoutRule_map(btrc_Vector_DisplayLayoutRule* self, __btrc_fn_DisplayLayoutRule_DisplayLayoutRule fn);
static bool btrc_Vector_DisplayLayoutRule_any(btrc_Vector_DisplayLayoutRule* self, __btrc_fn_bool_DisplayLayoutRule pred);
static bool btrc_Vector_DisplayLayoutRule_all(btrc_Vector_DisplayLayoutRule* self, __btrc_fn_bool_DisplayLayoutRule pred);
static DisplayLayoutRule* btrc_Vector_DisplayLayoutRule_reduce(btrc_Vector_DisplayLayoutRule* self, DisplayLayoutRule* init, __btrc_fn_DisplayLayoutRule_DisplayLayoutRule_DisplayLayoutRule fn);
static btrc_Vector_DisplayLayoutRule* btrc_Vector_DisplayLayoutRule_copy(btrc_Vector_DisplayLayoutRule* self);
static void btrc_Vector_DisplayLayoutRule_removeAt(btrc_Vector_DisplayLayoutRule* self, int idx);
static int btrc_Vector_DisplayLayoutRule_iterLen(btrc_Vector_DisplayLayoutRule* self);
static DisplayLayoutRule* btrc_Vector_DisplayLayoutRule_iterGet(btrc_Vector_DisplayLayoutRule* self, int i);

static void btrc_Vector_AudioPreset_init(btrc_Vector_AudioPreset* self);
static btrc_Vector_AudioPreset* btrc_Vector_AudioPreset_new(void);
static void btrc_Vector_AudioPreset_destroy(btrc_Vector_AudioPreset* self);
static void btrc_Vector_AudioPreset_push(btrc_Vector_AudioPreset* self, AudioPreset* val);
static AudioPreset* btrc_Vector_AudioPreset_pop(btrc_Vector_AudioPreset* self);
static AudioPreset* btrc_Vector_AudioPreset_get(btrc_Vector_AudioPreset* self, int i);
static void btrc_Vector_AudioPreset_set(btrc_Vector_AudioPreset* self, int i, AudioPreset* val);
static void btrc_Vector_AudioPreset_free(btrc_Vector_AudioPreset* self);
static void btrc_Vector_AudioPreset_remove(btrc_Vector_AudioPreset* self, int idx);
static void btrc_Vector_AudioPreset_reverse(btrc_Vector_AudioPreset* self);
static btrc_Vector_AudioPreset* btrc_Vector_AudioPreset_reversed(btrc_Vector_AudioPreset* self);
static void btrc_Vector_AudioPreset_swap(btrc_Vector_AudioPreset* self, int i, int j);
static void btrc_Vector_AudioPreset_clear(btrc_Vector_AudioPreset* self);
static void btrc_Vector_AudioPreset_fill(btrc_Vector_AudioPreset* self, AudioPreset* val);
static int btrc_Vector_AudioPreset_size(btrc_Vector_AudioPreset* self);
static bool btrc_Vector_AudioPreset_isEmpty(btrc_Vector_AudioPreset* self);
static AudioPreset* btrc_Vector_AudioPreset_first(btrc_Vector_AudioPreset* self);
static AudioPreset* btrc_Vector_AudioPreset_last(btrc_Vector_AudioPreset* self);
static btrc_Vector_AudioPreset* btrc_Vector_AudioPreset_slice(btrc_Vector_AudioPreset* self, int start, int end);
static btrc_Vector_AudioPreset* btrc_Vector_AudioPreset_take(btrc_Vector_AudioPreset* self, int n);
static btrc_Vector_AudioPreset* btrc_Vector_AudioPreset_drop(btrc_Vector_AudioPreset* self, int n);
static void btrc_Vector_AudioPreset_extend(btrc_Vector_AudioPreset* self, btrc_Vector_AudioPreset* other);
static void btrc_Vector_AudioPreset_insert(btrc_Vector_AudioPreset* self, int idx, AudioPreset* val);
static bool btrc_Vector_AudioPreset_contains(btrc_Vector_AudioPreset* self, AudioPreset* val);
static int btrc_Vector_AudioPreset_indexOf(btrc_Vector_AudioPreset* self, AudioPreset* val);
static int btrc_Vector_AudioPreset_lastIndexOf(btrc_Vector_AudioPreset* self, AudioPreset* val);
static int btrc_Vector_AudioPreset_count(btrc_Vector_AudioPreset* self, AudioPreset* val);
static void btrc_Vector_AudioPreset_removeAll(btrc_Vector_AudioPreset* self, AudioPreset* val);
static btrc_Vector_AudioPreset* btrc_Vector_AudioPreset_distinct(btrc_Vector_AudioPreset* self);
static void btrc_Vector_AudioPreset_sort(btrc_Vector_AudioPreset* self);
static btrc_Vector_AudioPreset* btrc_Vector_AudioPreset_sorted(btrc_Vector_AudioPreset* self);
static AudioPreset* btrc_Vector_AudioPreset_min(btrc_Vector_AudioPreset* self);
static AudioPreset* btrc_Vector_AudioPreset_max(btrc_Vector_AudioPreset* self);
static AudioPreset* btrc_Vector_AudioPreset_sum(btrc_Vector_AudioPreset* self);
static char* btrc_Vector_AudioPreset_join(btrc_Vector_AudioPreset* self, char* sep);
static char* btrc_Vector_AudioPreset_joinToString(btrc_Vector_AudioPreset* self, char* sep);
static btrc_Vector_AudioPreset* btrc_Vector_AudioPreset_filter(btrc_Vector_AudioPreset* self, __btrc_fn_bool_AudioPreset pred);
static int btrc_Vector_AudioPreset_findIndex(btrc_Vector_AudioPreset* self, __btrc_fn_bool_AudioPreset pred);
static void btrc_Vector_AudioPreset_forEach(btrc_Vector_AudioPreset* self, __btrc_fn_void_AudioPreset fn);
static btrc_Vector_AudioPreset* btrc_Vector_AudioPreset_map(btrc_Vector_AudioPreset* self, __btrc_fn_AudioPreset_AudioPreset fn);
static bool btrc_Vector_AudioPreset_any(btrc_Vector_AudioPreset* self, __btrc_fn_bool_AudioPreset pred);
static bool btrc_Vector_AudioPreset_all(btrc_Vector_AudioPreset* self, __btrc_fn_bool_AudioPreset pred);
static AudioPreset* btrc_Vector_AudioPreset_reduce(btrc_Vector_AudioPreset* self, AudioPreset* init, __btrc_fn_AudioPreset_AudioPreset_AudioPreset fn);
static btrc_Vector_AudioPreset* btrc_Vector_AudioPreset_copy(btrc_Vector_AudioPreset* self);
static void btrc_Vector_AudioPreset_removeAt(btrc_Vector_AudioPreset* self, int idx);
static int btrc_Vector_AudioPreset_iterLen(btrc_Vector_AudioPreset* self);
static AudioPreset* btrc_Vector_AudioPreset_iterGet(btrc_Vector_AudioPreset* self, int i);

static void btrc_Vector_AudioSink_init(btrc_Vector_AudioSink* self);
static btrc_Vector_AudioSink* btrc_Vector_AudioSink_new(void);
static void btrc_Vector_AudioSink_destroy(btrc_Vector_AudioSink* self);
static void btrc_Vector_AudioSink_push(btrc_Vector_AudioSink* self, AudioSink* val);
static AudioSink* btrc_Vector_AudioSink_pop(btrc_Vector_AudioSink* self);
static AudioSink* btrc_Vector_AudioSink_get(btrc_Vector_AudioSink* self, int i);
static void btrc_Vector_AudioSink_set(btrc_Vector_AudioSink* self, int i, AudioSink* val);
static void btrc_Vector_AudioSink_free(btrc_Vector_AudioSink* self);
static void btrc_Vector_AudioSink_remove(btrc_Vector_AudioSink* self, int idx);
static void btrc_Vector_AudioSink_reverse(btrc_Vector_AudioSink* self);
static btrc_Vector_AudioSink* btrc_Vector_AudioSink_reversed(btrc_Vector_AudioSink* self);
static void btrc_Vector_AudioSink_swap(btrc_Vector_AudioSink* self, int i, int j);
static void btrc_Vector_AudioSink_clear(btrc_Vector_AudioSink* self);
static void btrc_Vector_AudioSink_fill(btrc_Vector_AudioSink* self, AudioSink* val);
static int btrc_Vector_AudioSink_size(btrc_Vector_AudioSink* self);
static bool btrc_Vector_AudioSink_isEmpty(btrc_Vector_AudioSink* self);
static AudioSink* btrc_Vector_AudioSink_first(btrc_Vector_AudioSink* self);
static AudioSink* btrc_Vector_AudioSink_last(btrc_Vector_AudioSink* self);
static btrc_Vector_AudioSink* btrc_Vector_AudioSink_slice(btrc_Vector_AudioSink* self, int start, int end);
static btrc_Vector_AudioSink* btrc_Vector_AudioSink_take(btrc_Vector_AudioSink* self, int n);
static btrc_Vector_AudioSink* btrc_Vector_AudioSink_drop(btrc_Vector_AudioSink* self, int n);
static void btrc_Vector_AudioSink_extend(btrc_Vector_AudioSink* self, btrc_Vector_AudioSink* other);
static void btrc_Vector_AudioSink_insert(btrc_Vector_AudioSink* self, int idx, AudioSink* val);
static bool btrc_Vector_AudioSink_contains(btrc_Vector_AudioSink* self, AudioSink* val);
static int btrc_Vector_AudioSink_indexOf(btrc_Vector_AudioSink* self, AudioSink* val);
static int btrc_Vector_AudioSink_lastIndexOf(btrc_Vector_AudioSink* self, AudioSink* val);
static int btrc_Vector_AudioSink_count(btrc_Vector_AudioSink* self, AudioSink* val);
static void btrc_Vector_AudioSink_removeAll(btrc_Vector_AudioSink* self, AudioSink* val);
static btrc_Vector_AudioSink* btrc_Vector_AudioSink_distinct(btrc_Vector_AudioSink* self);
static void btrc_Vector_AudioSink_sort(btrc_Vector_AudioSink* self);
static btrc_Vector_AudioSink* btrc_Vector_AudioSink_sorted(btrc_Vector_AudioSink* self);
static AudioSink* btrc_Vector_AudioSink_min(btrc_Vector_AudioSink* self);
static AudioSink* btrc_Vector_AudioSink_max(btrc_Vector_AudioSink* self);
static AudioSink* btrc_Vector_AudioSink_sum(btrc_Vector_AudioSink* self);
static char* btrc_Vector_AudioSink_join(btrc_Vector_AudioSink* self, char* sep);
static char* btrc_Vector_AudioSink_joinToString(btrc_Vector_AudioSink* self, char* sep);
static btrc_Vector_AudioSink* btrc_Vector_AudioSink_filter(btrc_Vector_AudioSink* self, __btrc_fn_bool_AudioSink pred);
static int btrc_Vector_AudioSink_findIndex(btrc_Vector_AudioSink* self, __btrc_fn_bool_AudioSink pred);
static void btrc_Vector_AudioSink_forEach(btrc_Vector_AudioSink* self, __btrc_fn_void_AudioSink fn);
static btrc_Vector_AudioSink* btrc_Vector_AudioSink_map(btrc_Vector_AudioSink* self, __btrc_fn_AudioSink_AudioSink fn);
static bool btrc_Vector_AudioSink_any(btrc_Vector_AudioSink* self, __btrc_fn_bool_AudioSink pred);
static bool btrc_Vector_AudioSink_all(btrc_Vector_AudioSink* self, __btrc_fn_bool_AudioSink pred);
static AudioSink* btrc_Vector_AudioSink_reduce(btrc_Vector_AudioSink* self, AudioSink* init, __btrc_fn_AudioSink_AudioSink_AudioSink fn);
static btrc_Vector_AudioSink* btrc_Vector_AudioSink_copy(btrc_Vector_AudioSink* self);
static void btrc_Vector_AudioSink_removeAt(btrc_Vector_AudioSink* self, int idx);
static int btrc_Vector_AudioSink_iterLen(btrc_Vector_AudioSink* self);
static AudioSink* btrc_Vector_AudioSink_iterGet(btrc_Vector_AudioSink* self, int i);

static void btrc_Vector_DisplayOutput_init(btrc_Vector_DisplayOutput* self);
static btrc_Vector_DisplayOutput* btrc_Vector_DisplayOutput_new(void);
static void btrc_Vector_DisplayOutput_destroy(btrc_Vector_DisplayOutput* self);
static void btrc_Vector_DisplayOutput_push(btrc_Vector_DisplayOutput* self, DisplayOutput* val);
static DisplayOutput* btrc_Vector_DisplayOutput_pop(btrc_Vector_DisplayOutput* self);
static DisplayOutput* btrc_Vector_DisplayOutput_get(btrc_Vector_DisplayOutput* self, int i);
static void btrc_Vector_DisplayOutput_set(btrc_Vector_DisplayOutput* self, int i, DisplayOutput* val);
static void btrc_Vector_DisplayOutput_free(btrc_Vector_DisplayOutput* self);
static void btrc_Vector_DisplayOutput_remove(btrc_Vector_DisplayOutput* self, int idx);
static void btrc_Vector_DisplayOutput_reverse(btrc_Vector_DisplayOutput* self);
static btrc_Vector_DisplayOutput* btrc_Vector_DisplayOutput_reversed(btrc_Vector_DisplayOutput* self);
static void btrc_Vector_DisplayOutput_swap(btrc_Vector_DisplayOutput* self, int i, int j);
static void btrc_Vector_DisplayOutput_clear(btrc_Vector_DisplayOutput* self);
static void btrc_Vector_DisplayOutput_fill(btrc_Vector_DisplayOutput* self, DisplayOutput* val);
static int btrc_Vector_DisplayOutput_size(btrc_Vector_DisplayOutput* self);
static bool btrc_Vector_DisplayOutput_isEmpty(btrc_Vector_DisplayOutput* self);
static DisplayOutput* btrc_Vector_DisplayOutput_first(btrc_Vector_DisplayOutput* self);
static DisplayOutput* btrc_Vector_DisplayOutput_last(btrc_Vector_DisplayOutput* self);
static btrc_Vector_DisplayOutput* btrc_Vector_DisplayOutput_slice(btrc_Vector_DisplayOutput* self, int start, int end);
static btrc_Vector_DisplayOutput* btrc_Vector_DisplayOutput_take(btrc_Vector_DisplayOutput* self, int n);
static btrc_Vector_DisplayOutput* btrc_Vector_DisplayOutput_drop(btrc_Vector_DisplayOutput* self, int n);
static void btrc_Vector_DisplayOutput_extend(btrc_Vector_DisplayOutput* self, btrc_Vector_DisplayOutput* other);
static void btrc_Vector_DisplayOutput_insert(btrc_Vector_DisplayOutput* self, int idx, DisplayOutput* val);
static bool btrc_Vector_DisplayOutput_contains(btrc_Vector_DisplayOutput* self, DisplayOutput* val);
static int btrc_Vector_DisplayOutput_indexOf(btrc_Vector_DisplayOutput* self, DisplayOutput* val);
static int btrc_Vector_DisplayOutput_lastIndexOf(btrc_Vector_DisplayOutput* self, DisplayOutput* val);
static int btrc_Vector_DisplayOutput_count(btrc_Vector_DisplayOutput* self, DisplayOutput* val);
static void btrc_Vector_DisplayOutput_removeAll(btrc_Vector_DisplayOutput* self, DisplayOutput* val);
static btrc_Vector_DisplayOutput* btrc_Vector_DisplayOutput_distinct(btrc_Vector_DisplayOutput* self);
static void btrc_Vector_DisplayOutput_sort(btrc_Vector_DisplayOutput* self);
static btrc_Vector_DisplayOutput* btrc_Vector_DisplayOutput_sorted(btrc_Vector_DisplayOutput* self);
static DisplayOutput* btrc_Vector_DisplayOutput_min(btrc_Vector_DisplayOutput* self);
static DisplayOutput* btrc_Vector_DisplayOutput_max(btrc_Vector_DisplayOutput* self);
static DisplayOutput* btrc_Vector_DisplayOutput_sum(btrc_Vector_DisplayOutput* self);
static char* btrc_Vector_DisplayOutput_join(btrc_Vector_DisplayOutput* self, char* sep);
static char* btrc_Vector_DisplayOutput_joinToString(btrc_Vector_DisplayOutput* self, char* sep);
static btrc_Vector_DisplayOutput* btrc_Vector_DisplayOutput_filter(btrc_Vector_DisplayOutput* self, __btrc_fn_bool_DisplayOutput pred);
static int btrc_Vector_DisplayOutput_findIndex(btrc_Vector_DisplayOutput* self, __btrc_fn_bool_DisplayOutput pred);
static void btrc_Vector_DisplayOutput_forEach(btrc_Vector_DisplayOutput* self, __btrc_fn_void_DisplayOutput fn);
static btrc_Vector_DisplayOutput* btrc_Vector_DisplayOutput_map(btrc_Vector_DisplayOutput* self, __btrc_fn_DisplayOutput_DisplayOutput fn);
static bool btrc_Vector_DisplayOutput_any(btrc_Vector_DisplayOutput* self, __btrc_fn_bool_DisplayOutput pred);
static bool btrc_Vector_DisplayOutput_all(btrc_Vector_DisplayOutput* self, __btrc_fn_bool_DisplayOutput pred);
static DisplayOutput* btrc_Vector_DisplayOutput_reduce(btrc_Vector_DisplayOutput* self, DisplayOutput* init, __btrc_fn_DisplayOutput_DisplayOutput_DisplayOutput fn);
static btrc_Vector_DisplayOutput* btrc_Vector_DisplayOutput_copy(btrc_Vector_DisplayOutput* self);
static void btrc_Vector_DisplayOutput_removeAt(btrc_Vector_DisplayOutput* self, int idx);
static int btrc_Vector_DisplayOutput_iterLen(btrc_Vector_DisplayOutput* self);
static DisplayOutput* btrc_Vector_DisplayOutput_iterGet(btrc_Vector_DisplayOutput* self, int i);

static void btrc_Vector_VmOperation_init(btrc_Vector_VmOperation* self);
static btrc_Vector_VmOperation* btrc_Vector_VmOperation_new(void);
static void btrc_Vector_VmOperation_destroy(btrc_Vector_VmOperation* self);
static void btrc_Vector_VmOperation_push(btrc_Vector_VmOperation* self, VmOperation* val);
static VmOperation* btrc_Vector_VmOperation_pop(btrc_Vector_VmOperation* self);
static VmOperation* btrc_Vector_VmOperation_get(btrc_Vector_VmOperation* self, int i);
static void btrc_Vector_VmOperation_set(btrc_Vector_VmOperation* self, int i, VmOperation* val);
static void btrc_Vector_VmOperation_free(btrc_Vector_VmOperation* self);
static void btrc_Vector_VmOperation_remove(btrc_Vector_VmOperation* self, int idx);
static void btrc_Vector_VmOperation_reverse(btrc_Vector_VmOperation* self);
static btrc_Vector_VmOperation* btrc_Vector_VmOperation_reversed(btrc_Vector_VmOperation* self);
static void btrc_Vector_VmOperation_swap(btrc_Vector_VmOperation* self, int i, int j);
static void btrc_Vector_VmOperation_clear(btrc_Vector_VmOperation* self);
static void btrc_Vector_VmOperation_fill(btrc_Vector_VmOperation* self, VmOperation* val);
static int btrc_Vector_VmOperation_size(btrc_Vector_VmOperation* self);
static bool btrc_Vector_VmOperation_isEmpty(btrc_Vector_VmOperation* self);
static VmOperation* btrc_Vector_VmOperation_first(btrc_Vector_VmOperation* self);
static VmOperation* btrc_Vector_VmOperation_last(btrc_Vector_VmOperation* self);
static btrc_Vector_VmOperation* btrc_Vector_VmOperation_slice(btrc_Vector_VmOperation* self, int start, int end);
static btrc_Vector_VmOperation* btrc_Vector_VmOperation_take(btrc_Vector_VmOperation* self, int n);
static btrc_Vector_VmOperation* btrc_Vector_VmOperation_drop(btrc_Vector_VmOperation* self, int n);
static void btrc_Vector_VmOperation_extend(btrc_Vector_VmOperation* self, btrc_Vector_VmOperation* other);
static void btrc_Vector_VmOperation_insert(btrc_Vector_VmOperation* self, int idx, VmOperation* val);
static bool btrc_Vector_VmOperation_contains(btrc_Vector_VmOperation* self, VmOperation* val);
static int btrc_Vector_VmOperation_indexOf(btrc_Vector_VmOperation* self, VmOperation* val);
static int btrc_Vector_VmOperation_lastIndexOf(btrc_Vector_VmOperation* self, VmOperation* val);
static int btrc_Vector_VmOperation_count(btrc_Vector_VmOperation* self, VmOperation* val);
static void btrc_Vector_VmOperation_removeAll(btrc_Vector_VmOperation* self, VmOperation* val);
static btrc_Vector_VmOperation* btrc_Vector_VmOperation_distinct(btrc_Vector_VmOperation* self);
static void btrc_Vector_VmOperation_sort(btrc_Vector_VmOperation* self);
static btrc_Vector_VmOperation* btrc_Vector_VmOperation_sorted(btrc_Vector_VmOperation* self);
static VmOperation* btrc_Vector_VmOperation_min(btrc_Vector_VmOperation* self);
static VmOperation* btrc_Vector_VmOperation_max(btrc_Vector_VmOperation* self);
static VmOperation* btrc_Vector_VmOperation_sum(btrc_Vector_VmOperation* self);
static char* btrc_Vector_VmOperation_join(btrc_Vector_VmOperation* self, char* sep);
static char* btrc_Vector_VmOperation_joinToString(btrc_Vector_VmOperation* self, char* sep);
static btrc_Vector_VmOperation* btrc_Vector_VmOperation_filter(btrc_Vector_VmOperation* self, __btrc_fn_bool_VmOperation pred);
static int btrc_Vector_VmOperation_findIndex(btrc_Vector_VmOperation* self, __btrc_fn_bool_VmOperation pred);
static void btrc_Vector_VmOperation_forEach(btrc_Vector_VmOperation* self, __btrc_fn_void_VmOperation fn);
static btrc_Vector_VmOperation* btrc_Vector_VmOperation_map(btrc_Vector_VmOperation* self, __btrc_fn_VmOperation_VmOperation fn);
static bool btrc_Vector_VmOperation_any(btrc_Vector_VmOperation* self, __btrc_fn_bool_VmOperation pred);
static bool btrc_Vector_VmOperation_all(btrc_Vector_VmOperation* self, __btrc_fn_bool_VmOperation pred);
static VmOperation* btrc_Vector_VmOperation_reduce(btrc_Vector_VmOperation* self, VmOperation* init, __btrc_fn_VmOperation_VmOperation_VmOperation fn);
static btrc_Vector_VmOperation* btrc_Vector_VmOperation_copy(btrc_Vector_VmOperation* self);
static void btrc_Vector_VmOperation_removeAt(btrc_Vector_VmOperation* self, int idx);
static int btrc_Vector_VmOperation_iterLen(btrc_Vector_VmOperation* self);
static VmOperation* btrc_Vector_VmOperation_iterGet(btrc_Vector_VmOperation* self, int i);

static void btrc_Vector_VmGraphNode_init(btrc_Vector_VmGraphNode* self);
static btrc_Vector_VmGraphNode* btrc_Vector_VmGraphNode_new(void);
static void btrc_Vector_VmGraphNode_destroy(btrc_Vector_VmGraphNode* self);
static void btrc_Vector_VmGraphNode_push(btrc_Vector_VmGraphNode* self, VmGraphNode* val);
static VmGraphNode* btrc_Vector_VmGraphNode_pop(btrc_Vector_VmGraphNode* self);
static VmGraphNode* btrc_Vector_VmGraphNode_get(btrc_Vector_VmGraphNode* self, int i);
static void btrc_Vector_VmGraphNode_set(btrc_Vector_VmGraphNode* self, int i, VmGraphNode* val);
static void btrc_Vector_VmGraphNode_free(btrc_Vector_VmGraphNode* self);
static void btrc_Vector_VmGraphNode_remove(btrc_Vector_VmGraphNode* self, int idx);
static void btrc_Vector_VmGraphNode_reverse(btrc_Vector_VmGraphNode* self);
static btrc_Vector_VmGraphNode* btrc_Vector_VmGraphNode_reversed(btrc_Vector_VmGraphNode* self);
static void btrc_Vector_VmGraphNode_swap(btrc_Vector_VmGraphNode* self, int i, int j);
static void btrc_Vector_VmGraphNode_clear(btrc_Vector_VmGraphNode* self);
static void btrc_Vector_VmGraphNode_fill(btrc_Vector_VmGraphNode* self, VmGraphNode* val);
static int btrc_Vector_VmGraphNode_size(btrc_Vector_VmGraphNode* self);
static bool btrc_Vector_VmGraphNode_isEmpty(btrc_Vector_VmGraphNode* self);
static VmGraphNode* btrc_Vector_VmGraphNode_first(btrc_Vector_VmGraphNode* self);
static VmGraphNode* btrc_Vector_VmGraphNode_last(btrc_Vector_VmGraphNode* self);
static btrc_Vector_VmGraphNode* btrc_Vector_VmGraphNode_slice(btrc_Vector_VmGraphNode* self, int start, int end);
static btrc_Vector_VmGraphNode* btrc_Vector_VmGraphNode_take(btrc_Vector_VmGraphNode* self, int n);
static btrc_Vector_VmGraphNode* btrc_Vector_VmGraphNode_drop(btrc_Vector_VmGraphNode* self, int n);
static void btrc_Vector_VmGraphNode_extend(btrc_Vector_VmGraphNode* self, btrc_Vector_VmGraphNode* other);
static void btrc_Vector_VmGraphNode_insert(btrc_Vector_VmGraphNode* self, int idx, VmGraphNode* val);
static bool btrc_Vector_VmGraphNode_contains(btrc_Vector_VmGraphNode* self, VmGraphNode* val);
static int btrc_Vector_VmGraphNode_indexOf(btrc_Vector_VmGraphNode* self, VmGraphNode* val);
static int btrc_Vector_VmGraphNode_lastIndexOf(btrc_Vector_VmGraphNode* self, VmGraphNode* val);
static int btrc_Vector_VmGraphNode_count(btrc_Vector_VmGraphNode* self, VmGraphNode* val);
static void btrc_Vector_VmGraphNode_removeAll(btrc_Vector_VmGraphNode* self, VmGraphNode* val);
static btrc_Vector_VmGraphNode* btrc_Vector_VmGraphNode_distinct(btrc_Vector_VmGraphNode* self);
static void btrc_Vector_VmGraphNode_sort(btrc_Vector_VmGraphNode* self);
static btrc_Vector_VmGraphNode* btrc_Vector_VmGraphNode_sorted(btrc_Vector_VmGraphNode* self);
static VmGraphNode* btrc_Vector_VmGraphNode_min(btrc_Vector_VmGraphNode* self);
static VmGraphNode* btrc_Vector_VmGraphNode_max(btrc_Vector_VmGraphNode* self);
static VmGraphNode* btrc_Vector_VmGraphNode_sum(btrc_Vector_VmGraphNode* self);
static char* btrc_Vector_VmGraphNode_join(btrc_Vector_VmGraphNode* self, char* sep);
static char* btrc_Vector_VmGraphNode_joinToString(btrc_Vector_VmGraphNode* self, char* sep);
static btrc_Vector_VmGraphNode* btrc_Vector_VmGraphNode_filter(btrc_Vector_VmGraphNode* self, __btrc_fn_bool_VmGraphNode pred);
static int btrc_Vector_VmGraphNode_findIndex(btrc_Vector_VmGraphNode* self, __btrc_fn_bool_VmGraphNode pred);
static void btrc_Vector_VmGraphNode_forEach(btrc_Vector_VmGraphNode* self, __btrc_fn_void_VmGraphNode fn);
static btrc_Vector_VmGraphNode* btrc_Vector_VmGraphNode_map(btrc_Vector_VmGraphNode* self, __btrc_fn_VmGraphNode_VmGraphNode fn);
static bool btrc_Vector_VmGraphNode_any(btrc_Vector_VmGraphNode* self, __btrc_fn_bool_VmGraphNode pred);
static bool btrc_Vector_VmGraphNode_all(btrc_Vector_VmGraphNode* self, __btrc_fn_bool_VmGraphNode pred);
static VmGraphNode* btrc_Vector_VmGraphNode_reduce(btrc_Vector_VmGraphNode* self, VmGraphNode* init, __btrc_fn_VmGraphNode_VmGraphNode_VmGraphNode fn);
static btrc_Vector_VmGraphNode* btrc_Vector_VmGraphNode_copy(btrc_Vector_VmGraphNode* self);
static void btrc_Vector_VmGraphNode_removeAt(btrc_Vector_VmGraphNode* self, int idx);
static int btrc_Vector_VmGraphNode_iterLen(btrc_Vector_VmGraphNode* self);
static VmGraphNode* btrc_Vector_VmGraphNode_iterGet(btrc_Vector_VmGraphNode* self, int i);

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

static void btrc_Vector_ResetSubvolume_init(btrc_Vector_ResetSubvolume* self) {
    self->__rc = 1;
    (self->data = NULL);
    (self->len = 0);
    (self->cap = 0);
}

static btrc_Vector_ResetSubvolume* btrc_Vector_ResetSubvolume_new(void) {
    btrc_Vector_ResetSubvolume* self = ((btrc_Vector_ResetSubvolume*)malloc(sizeof(btrc_Vector_ResetSubvolume)));
    memset(self, 0, sizeof(btrc_Vector_ResetSubvolume));
    btrc_Vector_ResetSubvolume_init(self);
    return self;
}

static void btrc_Vector_ResetSubvolume_destroy(btrc_Vector_ResetSubvolume* self) {
    free(self);
}

static void btrc_Vector_ResetSubvolume_push(btrc_Vector_ResetSubvolume* self, ResetSubvolume* val) {
    (val->__rc++);
    if (self->len >= self->cap) {
        (self->cap = ((self->cap == 0) ? 4 : (self->cap * 2)));
        (self->data = ((ResetSubvolume**)__btrc_safe_realloc(self->data, (sizeof(ResetSubvolume*) * self->cap))));
    }
    (self->data[self->len] = val);
    (self->len++);
}

static ResetSubvolume* btrc_Vector_ResetSubvolume_pop(btrc_Vector_ResetSubvolume* self) {
    if (self->len <= 0) {
        fprintf(stderr, "Vector pop from empty list\n");
        exit(1);
    }
    (self->len--);
    return self->data[self->len];
}

static ResetSubvolume* btrc_Vector_ResetSubvolume_get(btrc_Vector_ResetSubvolume* self, int i) {
    if ((i < 0) || (i >= self->len)) {
        fprintf(stderr, "Vector index out of bounds: %d (len=%d)\n", i, self->len);
        exit(1);
    }
    return self->data[i];
}

static void btrc_Vector_ResetSubvolume_set(btrc_Vector_ResetSubvolume* self, int i, ResetSubvolume* val) {
    if ((i < 0) || (i >= self->len)) {
        fprintf(stderr, "Vector index out of bounds: %d (len=%d)\n", i, self->len);
        exit(1);
    }
    if (self->data[i]) {
        if ((--self->data[i]->__rc) <= 0) {
            ResetSubvolume_destroy(self->data[i]);
        }
    }
    (val->__rc++);
    (self->data[i] = val);
}

static void btrc_Vector_ResetSubvolume_free(btrc_Vector_ResetSubvolume* self) {
    for (int i = 0; (i < self->len); (i++)) {
        if (self->data[i]) {
            if ((--self->data[i]->__rc) <= 0) {
                ResetSubvolume_destroy(self->data[i]);
            }
        }
    }
    free(self->data);
    (self->data = NULL);
    (self->len = 0);
    (self->cap = 0);
}

static void btrc_Vector_ResetSubvolume_remove(btrc_Vector_ResetSubvolume* self, int idx) {
    if ((idx < 0) || (idx >= self->len)) {
        fprintf(stderr, "Vector remove index out of bounds: %d (len=%d)\n", idx, self->len);
        exit(1);
    }
    if (self->data[idx]) {
        if ((--self->data[idx]->__rc) <= 0) {
            ResetSubvolume_destroy(self->data[idx]);
        }
    }
    for (int i = idx; (i < (self->len - 1)); (i++)) {
        (self->data[i] = self->data[(i + 1)]);
    }
    (self->len--);
}

static void btrc_Vector_ResetSubvolume_reverse(btrc_Vector_ResetSubvolume* self) {
    for (int i = 0; (i < (self->len / 2)); (i++)) {
        ResetSubvolume* tmp = self->data[i];
        (self->data[i] = self->data[((self->len - 1) - i)]);
        (self->data[((self->len - 1) - i)] = tmp);
    }
}

static btrc_Vector_ResetSubvolume* btrc_Vector_ResetSubvolume_reversed(btrc_Vector_ResetSubvolume* self) {
    btrc_Vector_ResetSubvolume* result = btrc_Vector_ResetSubvolume_new();
    for (int i = (self->len - 1); (i >= 0); (i--)) {
        btrc_Vector_ResetSubvolume_push(result, self->data[i]);
    }
    return result;
}

static void btrc_Vector_ResetSubvolume_swap(btrc_Vector_ResetSubvolume* self, int i, int j) {
    if ((((i < 0) || (i >= self->len)) || (j < 0)) || (j >= self->len)) {
        fprintf(stderr, "Vector swap index out of bounds\n");
        exit(1);
    }
    ResetSubvolume* tmp = self->data[i];
    (self->data[i] = self->data[j]);
    (self->data[j] = tmp);
}

static void btrc_Vector_ResetSubvolume_clear(btrc_Vector_ResetSubvolume* self) {
    for (int i = 0; (i < self->len); (i++)) {
        if (self->data[i]) {
            if ((--self->data[i]->__rc) <= 0) {
                ResetSubvolume_destroy(self->data[i]);
            }
        }
    }
    (self->len = 0);
}

static void btrc_Vector_ResetSubvolume_fill(btrc_Vector_ResetSubvolume* self, ResetSubvolume* val) {
    for (int i = 0; (i < self->len); (i++)) {
        if (self->data[i]) {
            if ((--self->data[i]->__rc) <= 0) {
                ResetSubvolume_destroy(self->data[i]);
            }
        }
        (val->__rc++);
        (self->data[i] = val);
    }
}

static int btrc_Vector_ResetSubvolume_size(btrc_Vector_ResetSubvolume* self) {
    return self->len;
}

static bool btrc_Vector_ResetSubvolume_isEmpty(btrc_Vector_ResetSubvolume* self) {
    return (self->len == 0);
}

static ResetSubvolume* btrc_Vector_ResetSubvolume_first(btrc_Vector_ResetSubvolume* self) {
    if (self->len == 0) {
        fprintf(stderr, "Vector.first() called on empty list\n");
        exit(1);
    }
    return self->data[0];
}

static ResetSubvolume* btrc_Vector_ResetSubvolume_last(btrc_Vector_ResetSubvolume* self) {
    if (self->len == 0) {
        fprintf(stderr, "Vector.last() called on empty list\n");
        exit(1);
    }
    return self->data[(self->len - 1)];
}

static btrc_Vector_ResetSubvolume* btrc_Vector_ResetSubvolume_slice(btrc_Vector_ResetSubvolume* self, int start, int end) {
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
    btrc_Vector_ResetSubvolume* result = btrc_Vector_ResetSubvolume_new();
    for (int i = start; (i < end); (i++)) {
        btrc_Vector_ResetSubvolume_push(result, self->data[i]);
    }
    return result;
}

static btrc_Vector_ResetSubvolume* btrc_Vector_ResetSubvolume_take(btrc_Vector_ResetSubvolume* self, int n) {
    if (n > self->len) {
        (n = self->len);
    }
    if (n < 0) {
        (n = 0);
    }
    return btrc_Vector_ResetSubvolume_slice(self, 0, n);
}

static btrc_Vector_ResetSubvolume* btrc_Vector_ResetSubvolume_drop(btrc_Vector_ResetSubvolume* self, int n) {
    if (n > self->len) {
        (n = self->len);
    }
    if (n < 0) {
        (n = 0);
    }
    return btrc_Vector_ResetSubvolume_slice(self, n, self->len);
}

static void btrc_Vector_ResetSubvolume_extend(btrc_Vector_ResetSubvolume* self, btrc_Vector_ResetSubvolume* other) {
    for (int i = 0; (i < other->len); (i++)) {
        btrc_Vector_ResetSubvolume_push(self, other->data[i]);
    }
}

static void btrc_Vector_ResetSubvolume_insert(btrc_Vector_ResetSubvolume* self, int idx, ResetSubvolume* val) {
    if ((idx < 0) || (idx > self->len)) {
        fprintf(stderr, "Vector insert index out of bounds: %d (size %d)\n", idx, self->len);
        exit(1);
    }
    (val->__rc++);
    if (self->len >= self->cap) {
        (self->cap = ((self->cap == 0) ? 4 : (self->cap * 2)));
        (self->data = ((ResetSubvolume**)__btrc_safe_realloc(self->data, (sizeof(ResetSubvolume*) * self->cap))));
    }
    for (int i = self->len; (i > idx); (i--)) {
        (self->data[i] = self->data[(i - 1)]);
    }
    (self->data[idx] = val);
    (self->len++);
}

static bool btrc_Vector_ResetSubvolume_contains(btrc_Vector_ResetSubvolume* self, ResetSubvolume* val) {
    for (int i = 0; (i < self->len); (i++)) {
        if (__btrc_eq(self->data[i], val)) {
            return true;
        }
    }
    return false;
}

static int btrc_Vector_ResetSubvolume_indexOf(btrc_Vector_ResetSubvolume* self, ResetSubvolume* val) {
    for (int i = 0; (i < self->len); (i++)) {
        if (__btrc_eq(self->data[i], val)) {
            return i;
        }
    }
    return (-1);
}

static int btrc_Vector_ResetSubvolume_lastIndexOf(btrc_Vector_ResetSubvolume* self, ResetSubvolume* val) {
    for (int i = (self->len - 1); (i >= 0); (i--)) {
        if (__btrc_eq(self->data[i], val)) {
            return i;
        }
    }
    return (-1);
}

static int btrc_Vector_ResetSubvolume_count(btrc_Vector_ResetSubvolume* self, ResetSubvolume* val) {
    int c = 0;
    for (int i = 0; (i < self->len); (i++)) {
        if (__btrc_eq(self->data[i], val)) {
            (c++);
        }
    }
    return c;
}

static void btrc_Vector_ResetSubvolume_removeAll(btrc_Vector_ResetSubvolume* self, ResetSubvolume* val) {
    int j = 0;
    for (int i = 0; (i < self->len); (i++)) {
        if (!__btrc_eq(self->data[i], val)) {
            (self->data[j] = self->data[i]);
            (j++);
        } else if (self->data[i]) {
            if ((--self->data[i]->__rc) <= 0) {
                ResetSubvolume_destroy(self->data[i]);
            }
        }
    }
    (self->len = j);
}

static btrc_Vector_ResetSubvolume* btrc_Vector_ResetSubvolume_distinct(btrc_Vector_ResetSubvolume* self) {
    btrc_Vector_ResetSubvolume* result = btrc_Vector_ResetSubvolume_new();
    for (int i = 0; (i < self->len); (i++)) {
        if (!btrc_Vector_ResetSubvolume_contains(result, self->data[i])) {
            btrc_Vector_ResetSubvolume_push(result, self->data[i]);
        }
    }
    return result;
}

static void btrc_Vector_ResetSubvolume_sort(btrc_Vector_ResetSubvolume* self) {
    for (int i = 1; (i < self->len); (i++)) {
        ResetSubvolume* key = self->data[i];
        int j = (i - 1);
        while ((j >= 0) && __btrc_lt(key, self->data[j])) {
            (self->data[(j + 1)] = self->data[j]);
            (j = (j - 1));
        }
        (self->data[(j + 1)] = key);
    }
}

static btrc_Vector_ResetSubvolume* btrc_Vector_ResetSubvolume_sorted(btrc_Vector_ResetSubvolume* self) {
    btrc_Vector_ResetSubvolume* result = btrc_Vector_ResetSubvolume_new();
    for (int i = 0; (i < self->len); (i++)) {
        btrc_Vector_ResetSubvolume_push(result, self->data[i]);
    }
    btrc_Vector_ResetSubvolume_sort(result);
    return result;
}

static ResetSubvolume* btrc_Vector_ResetSubvolume_min(btrc_Vector_ResetSubvolume* self) {
    if (self->len <= 0) {
        fprintf(stderr, "Vector min on empty list\n");
        exit(1);
    }
    ResetSubvolume* m = self->data[0];
    for (int i = 1; (i < self->len); (i++)) {
        if (__btrc_lt(self->data[i], m)) {
            (m = self->data[i]);
        }
    }
    return m;
}

static ResetSubvolume* btrc_Vector_ResetSubvolume_max(btrc_Vector_ResetSubvolume* self) {
    if (self->len <= 0) {
        fprintf(stderr, "Vector max on empty list\n");
        exit(1);
    }
    ResetSubvolume* m = self->data[0];
    for (int i = 1; (i < self->len); (i++)) {
        if (__btrc_gt(self->data[i], m)) {
            (m = self->data[i]);
        }
    }
    return m;
}

static btrc_Vector_ResetSubvolume* btrc_Vector_ResetSubvolume_filter(btrc_Vector_ResetSubvolume* self, __btrc_fn_bool_ResetSubvolume pred) {
    btrc_Vector_ResetSubvolume* result = btrc_Vector_ResetSubvolume_new();
    for (int i = 0; (i < self->len); (i++)) {
        if (pred(self->data[i])) {
            btrc_Vector_ResetSubvolume_push(result, self->data[i]);
        }
    }
    return result;
}

static int btrc_Vector_ResetSubvolume_findIndex(btrc_Vector_ResetSubvolume* self, __btrc_fn_bool_ResetSubvolume pred) {
    for (int i = 0; (i < self->len); (i++)) {
        if (pred(self->data[i])) {
            return i;
        }
    }
    return (-1);
}

static void btrc_Vector_ResetSubvolume_forEach(btrc_Vector_ResetSubvolume* self, __btrc_fn_void_ResetSubvolume fn) {
    for (int i = 0; (i < self->len); (i++)) {
        fn(self->data[i]);
    }
}

static btrc_Vector_ResetSubvolume* btrc_Vector_ResetSubvolume_map(btrc_Vector_ResetSubvolume* self, __btrc_fn_ResetSubvolume_ResetSubvolume fn) {
    btrc_Vector_ResetSubvolume* result = btrc_Vector_ResetSubvolume_new();
    for (int i = 0; (i < self->len); (i++)) {
        btrc_Vector_ResetSubvolume_push(result, fn(self->data[i]));
    }
    return result;
}

static bool btrc_Vector_ResetSubvolume_any(btrc_Vector_ResetSubvolume* self, __btrc_fn_bool_ResetSubvolume pred) {
    for (int i = 0; (i < self->len); (i++)) {
        if (pred(self->data[i])) {
            return true;
        }
    }
    return false;
}

static bool btrc_Vector_ResetSubvolume_all(btrc_Vector_ResetSubvolume* self, __btrc_fn_bool_ResetSubvolume pred) {
    for (int i = 0; (i < self->len); (i++)) {
        if (!pred(self->data[i])) {
            return false;
        }
    }
    return true;
}

static ResetSubvolume* btrc_Vector_ResetSubvolume_reduce(btrc_Vector_ResetSubvolume* self, ResetSubvolume* init, __btrc_fn_ResetSubvolume_ResetSubvolume_ResetSubvolume fn) {
    ResetSubvolume* acc = init;
    for (int i = 0; (i < self->len); (i++)) {
        (acc = fn(acc, self->data[i]));
    }
    return acc;
}

static btrc_Vector_ResetSubvolume* btrc_Vector_ResetSubvolume_copy(btrc_Vector_ResetSubvolume* self) {
    btrc_Vector_ResetSubvolume* result = btrc_Vector_ResetSubvolume_new();
    for (int i = 0; (i < self->len); (i++)) {
        btrc_Vector_ResetSubvolume_push(result, self->data[i]);
    }
    return result;
}

static void btrc_Vector_ResetSubvolume_removeAt(btrc_Vector_ResetSubvolume* self, int idx) {
    btrc_Vector_ResetSubvolume_remove(self, idx);
}

static int btrc_Vector_ResetSubvolume_iterLen(btrc_Vector_ResetSubvolume* self) {
    return self->len;
}

static ResetSubvolume* btrc_Vector_ResetSubvolume_iterGet(btrc_Vector_ResetSubvolume* self, int i) {
    return self->data[i];
}

static void btrc_Vector_DisplayLayoutRule_init(btrc_Vector_DisplayLayoutRule* self) {
    self->__rc = 1;
    (self->data = NULL);
    (self->len = 0);
    (self->cap = 0);
}

static btrc_Vector_DisplayLayoutRule* btrc_Vector_DisplayLayoutRule_new(void) {
    btrc_Vector_DisplayLayoutRule* self = ((btrc_Vector_DisplayLayoutRule*)malloc(sizeof(btrc_Vector_DisplayLayoutRule)));
    memset(self, 0, sizeof(btrc_Vector_DisplayLayoutRule));
    btrc_Vector_DisplayLayoutRule_init(self);
    return self;
}

static void btrc_Vector_DisplayLayoutRule_destroy(btrc_Vector_DisplayLayoutRule* self) {
    free(self);
}

static void btrc_Vector_DisplayLayoutRule_push(btrc_Vector_DisplayLayoutRule* self, DisplayLayoutRule* val) {
    (val->__rc++);
    if (self->len >= self->cap) {
        (self->cap = ((self->cap == 0) ? 4 : (self->cap * 2)));
        (self->data = ((DisplayLayoutRule**)__btrc_safe_realloc(self->data, (sizeof(DisplayLayoutRule*) * self->cap))));
    }
    (self->data[self->len] = val);
    (self->len++);
}

static DisplayLayoutRule* btrc_Vector_DisplayLayoutRule_pop(btrc_Vector_DisplayLayoutRule* self) {
    if (self->len <= 0) {
        fprintf(stderr, "Vector pop from empty list\n");
        exit(1);
    }
    (self->len--);
    return self->data[self->len];
}

static DisplayLayoutRule* btrc_Vector_DisplayLayoutRule_get(btrc_Vector_DisplayLayoutRule* self, int i) {
    if ((i < 0) || (i >= self->len)) {
        fprintf(stderr, "Vector index out of bounds: %d (len=%d)\n", i, self->len);
        exit(1);
    }
    return self->data[i];
}

static void btrc_Vector_DisplayLayoutRule_set(btrc_Vector_DisplayLayoutRule* self, int i, DisplayLayoutRule* val) {
    if ((i < 0) || (i >= self->len)) {
        fprintf(stderr, "Vector index out of bounds: %d (len=%d)\n", i, self->len);
        exit(1);
    }
    if (self->data[i]) {
        if ((--self->data[i]->__rc) <= 0) {
            DisplayLayoutRule_destroy(self->data[i]);
        }
    }
    (val->__rc++);
    (self->data[i] = val);
}

static void btrc_Vector_DisplayLayoutRule_free(btrc_Vector_DisplayLayoutRule* self) {
    for (int i = 0; (i < self->len); (i++)) {
        if (self->data[i]) {
            if ((--self->data[i]->__rc) <= 0) {
                DisplayLayoutRule_destroy(self->data[i]);
            }
        }
    }
    free(self->data);
    (self->data = NULL);
    (self->len = 0);
    (self->cap = 0);
}

static void btrc_Vector_DisplayLayoutRule_remove(btrc_Vector_DisplayLayoutRule* self, int idx) {
    if ((idx < 0) || (idx >= self->len)) {
        fprintf(stderr, "Vector remove index out of bounds: %d (len=%d)\n", idx, self->len);
        exit(1);
    }
    if (self->data[idx]) {
        if ((--self->data[idx]->__rc) <= 0) {
            DisplayLayoutRule_destroy(self->data[idx]);
        }
    }
    for (int i = idx; (i < (self->len - 1)); (i++)) {
        (self->data[i] = self->data[(i + 1)]);
    }
    (self->len--);
}

static void btrc_Vector_DisplayLayoutRule_reverse(btrc_Vector_DisplayLayoutRule* self) {
    for (int i = 0; (i < (self->len / 2)); (i++)) {
        DisplayLayoutRule* tmp = self->data[i];
        (self->data[i] = self->data[((self->len - 1) - i)]);
        (self->data[((self->len - 1) - i)] = tmp);
    }
}

static btrc_Vector_DisplayLayoutRule* btrc_Vector_DisplayLayoutRule_reversed(btrc_Vector_DisplayLayoutRule* self) {
    btrc_Vector_DisplayLayoutRule* result = btrc_Vector_DisplayLayoutRule_new();
    for (int i = (self->len - 1); (i >= 0); (i--)) {
        btrc_Vector_DisplayLayoutRule_push(result, self->data[i]);
    }
    return result;
}

static void btrc_Vector_DisplayLayoutRule_swap(btrc_Vector_DisplayLayoutRule* self, int i, int j) {
    if ((((i < 0) || (i >= self->len)) || (j < 0)) || (j >= self->len)) {
        fprintf(stderr, "Vector swap index out of bounds\n");
        exit(1);
    }
    DisplayLayoutRule* tmp = self->data[i];
    (self->data[i] = self->data[j]);
    (self->data[j] = tmp);
}

static void btrc_Vector_DisplayLayoutRule_clear(btrc_Vector_DisplayLayoutRule* self) {
    for (int i = 0; (i < self->len); (i++)) {
        if (self->data[i]) {
            if ((--self->data[i]->__rc) <= 0) {
                DisplayLayoutRule_destroy(self->data[i]);
            }
        }
    }
    (self->len = 0);
}

static void btrc_Vector_DisplayLayoutRule_fill(btrc_Vector_DisplayLayoutRule* self, DisplayLayoutRule* val) {
    for (int i = 0; (i < self->len); (i++)) {
        if (self->data[i]) {
            if ((--self->data[i]->__rc) <= 0) {
                DisplayLayoutRule_destroy(self->data[i]);
            }
        }
        (val->__rc++);
        (self->data[i] = val);
    }
}

static int btrc_Vector_DisplayLayoutRule_size(btrc_Vector_DisplayLayoutRule* self) {
    return self->len;
}

static bool btrc_Vector_DisplayLayoutRule_isEmpty(btrc_Vector_DisplayLayoutRule* self) {
    return (self->len == 0);
}

static DisplayLayoutRule* btrc_Vector_DisplayLayoutRule_first(btrc_Vector_DisplayLayoutRule* self) {
    if (self->len == 0) {
        fprintf(stderr, "Vector.first() called on empty list\n");
        exit(1);
    }
    return self->data[0];
}

static DisplayLayoutRule* btrc_Vector_DisplayLayoutRule_last(btrc_Vector_DisplayLayoutRule* self) {
    if (self->len == 0) {
        fprintf(stderr, "Vector.last() called on empty list\n");
        exit(1);
    }
    return self->data[(self->len - 1)];
}

static btrc_Vector_DisplayLayoutRule* btrc_Vector_DisplayLayoutRule_slice(btrc_Vector_DisplayLayoutRule* self, int start, int end) {
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
    btrc_Vector_DisplayLayoutRule* result = btrc_Vector_DisplayLayoutRule_new();
    for (int i = start; (i < end); (i++)) {
        btrc_Vector_DisplayLayoutRule_push(result, self->data[i]);
    }
    return result;
}

static btrc_Vector_DisplayLayoutRule* btrc_Vector_DisplayLayoutRule_take(btrc_Vector_DisplayLayoutRule* self, int n) {
    if (n > self->len) {
        (n = self->len);
    }
    if (n < 0) {
        (n = 0);
    }
    return btrc_Vector_DisplayLayoutRule_slice(self, 0, n);
}

static btrc_Vector_DisplayLayoutRule* btrc_Vector_DisplayLayoutRule_drop(btrc_Vector_DisplayLayoutRule* self, int n) {
    if (n > self->len) {
        (n = self->len);
    }
    if (n < 0) {
        (n = 0);
    }
    return btrc_Vector_DisplayLayoutRule_slice(self, n, self->len);
}

static void btrc_Vector_DisplayLayoutRule_extend(btrc_Vector_DisplayLayoutRule* self, btrc_Vector_DisplayLayoutRule* other) {
    for (int i = 0; (i < other->len); (i++)) {
        btrc_Vector_DisplayLayoutRule_push(self, other->data[i]);
    }
}

static void btrc_Vector_DisplayLayoutRule_insert(btrc_Vector_DisplayLayoutRule* self, int idx, DisplayLayoutRule* val) {
    if ((idx < 0) || (idx > self->len)) {
        fprintf(stderr, "Vector insert index out of bounds: %d (size %d)\n", idx, self->len);
        exit(1);
    }
    (val->__rc++);
    if (self->len >= self->cap) {
        (self->cap = ((self->cap == 0) ? 4 : (self->cap * 2)));
        (self->data = ((DisplayLayoutRule**)__btrc_safe_realloc(self->data, (sizeof(DisplayLayoutRule*) * self->cap))));
    }
    for (int i = self->len; (i > idx); (i--)) {
        (self->data[i] = self->data[(i - 1)]);
    }
    (self->data[idx] = val);
    (self->len++);
}

static bool btrc_Vector_DisplayLayoutRule_contains(btrc_Vector_DisplayLayoutRule* self, DisplayLayoutRule* val) {
    for (int i = 0; (i < self->len); (i++)) {
        if (__btrc_eq(self->data[i], val)) {
            return true;
        }
    }
    return false;
}

static int btrc_Vector_DisplayLayoutRule_indexOf(btrc_Vector_DisplayLayoutRule* self, DisplayLayoutRule* val) {
    for (int i = 0; (i < self->len); (i++)) {
        if (__btrc_eq(self->data[i], val)) {
            return i;
        }
    }
    return (-1);
}

static int btrc_Vector_DisplayLayoutRule_lastIndexOf(btrc_Vector_DisplayLayoutRule* self, DisplayLayoutRule* val) {
    for (int i = (self->len - 1); (i >= 0); (i--)) {
        if (__btrc_eq(self->data[i], val)) {
            return i;
        }
    }
    return (-1);
}

static int btrc_Vector_DisplayLayoutRule_count(btrc_Vector_DisplayLayoutRule* self, DisplayLayoutRule* val) {
    int c = 0;
    for (int i = 0; (i < self->len); (i++)) {
        if (__btrc_eq(self->data[i], val)) {
            (c++);
        }
    }
    return c;
}

static void btrc_Vector_DisplayLayoutRule_removeAll(btrc_Vector_DisplayLayoutRule* self, DisplayLayoutRule* val) {
    int j = 0;
    for (int i = 0; (i < self->len); (i++)) {
        if (!__btrc_eq(self->data[i], val)) {
            (self->data[j] = self->data[i]);
            (j++);
        } else if (self->data[i]) {
            if ((--self->data[i]->__rc) <= 0) {
                DisplayLayoutRule_destroy(self->data[i]);
            }
        }
    }
    (self->len = j);
}

static btrc_Vector_DisplayLayoutRule* btrc_Vector_DisplayLayoutRule_distinct(btrc_Vector_DisplayLayoutRule* self) {
    btrc_Vector_DisplayLayoutRule* result = btrc_Vector_DisplayLayoutRule_new();
    for (int i = 0; (i < self->len); (i++)) {
        if (!btrc_Vector_DisplayLayoutRule_contains(result, self->data[i])) {
            btrc_Vector_DisplayLayoutRule_push(result, self->data[i]);
        }
    }
    return result;
}

static void btrc_Vector_DisplayLayoutRule_sort(btrc_Vector_DisplayLayoutRule* self) {
    for (int i = 1; (i < self->len); (i++)) {
        DisplayLayoutRule* key = self->data[i];
        int j = (i - 1);
        while ((j >= 0) && __btrc_lt(key, self->data[j])) {
            (self->data[(j + 1)] = self->data[j]);
            (j = (j - 1));
        }
        (self->data[(j + 1)] = key);
    }
}

static btrc_Vector_DisplayLayoutRule* btrc_Vector_DisplayLayoutRule_sorted(btrc_Vector_DisplayLayoutRule* self) {
    btrc_Vector_DisplayLayoutRule* result = btrc_Vector_DisplayLayoutRule_new();
    for (int i = 0; (i < self->len); (i++)) {
        btrc_Vector_DisplayLayoutRule_push(result, self->data[i]);
    }
    btrc_Vector_DisplayLayoutRule_sort(result);
    return result;
}

static DisplayLayoutRule* btrc_Vector_DisplayLayoutRule_min(btrc_Vector_DisplayLayoutRule* self) {
    if (self->len <= 0) {
        fprintf(stderr, "Vector min on empty list\n");
        exit(1);
    }
    DisplayLayoutRule* m = self->data[0];
    for (int i = 1; (i < self->len); (i++)) {
        if (__btrc_lt(self->data[i], m)) {
            (m = self->data[i]);
        }
    }
    return m;
}

static DisplayLayoutRule* btrc_Vector_DisplayLayoutRule_max(btrc_Vector_DisplayLayoutRule* self) {
    if (self->len <= 0) {
        fprintf(stderr, "Vector max on empty list\n");
        exit(1);
    }
    DisplayLayoutRule* m = self->data[0];
    for (int i = 1; (i < self->len); (i++)) {
        if (__btrc_gt(self->data[i], m)) {
            (m = self->data[i]);
        }
    }
    return m;
}

static btrc_Vector_DisplayLayoutRule* btrc_Vector_DisplayLayoutRule_filter(btrc_Vector_DisplayLayoutRule* self, __btrc_fn_bool_DisplayLayoutRule pred) {
    btrc_Vector_DisplayLayoutRule* result = btrc_Vector_DisplayLayoutRule_new();
    for (int i = 0; (i < self->len); (i++)) {
        if (pred(self->data[i])) {
            btrc_Vector_DisplayLayoutRule_push(result, self->data[i]);
        }
    }
    return result;
}

static int btrc_Vector_DisplayLayoutRule_findIndex(btrc_Vector_DisplayLayoutRule* self, __btrc_fn_bool_DisplayLayoutRule pred) {
    for (int i = 0; (i < self->len); (i++)) {
        if (pred(self->data[i])) {
            return i;
        }
    }
    return (-1);
}

static void btrc_Vector_DisplayLayoutRule_forEach(btrc_Vector_DisplayLayoutRule* self, __btrc_fn_void_DisplayLayoutRule fn) {
    for (int i = 0; (i < self->len); (i++)) {
        fn(self->data[i]);
    }
}

static btrc_Vector_DisplayLayoutRule* btrc_Vector_DisplayLayoutRule_map(btrc_Vector_DisplayLayoutRule* self, __btrc_fn_DisplayLayoutRule_DisplayLayoutRule fn) {
    btrc_Vector_DisplayLayoutRule* result = btrc_Vector_DisplayLayoutRule_new();
    for (int i = 0; (i < self->len); (i++)) {
        btrc_Vector_DisplayLayoutRule_push(result, fn(self->data[i]));
    }
    return result;
}

static bool btrc_Vector_DisplayLayoutRule_any(btrc_Vector_DisplayLayoutRule* self, __btrc_fn_bool_DisplayLayoutRule pred) {
    for (int i = 0; (i < self->len); (i++)) {
        if (pred(self->data[i])) {
            return true;
        }
    }
    return false;
}

static bool btrc_Vector_DisplayLayoutRule_all(btrc_Vector_DisplayLayoutRule* self, __btrc_fn_bool_DisplayLayoutRule pred) {
    for (int i = 0; (i < self->len); (i++)) {
        if (!pred(self->data[i])) {
            return false;
        }
    }
    return true;
}

static DisplayLayoutRule* btrc_Vector_DisplayLayoutRule_reduce(btrc_Vector_DisplayLayoutRule* self, DisplayLayoutRule* init, __btrc_fn_DisplayLayoutRule_DisplayLayoutRule_DisplayLayoutRule fn) {
    DisplayLayoutRule* acc = init;
    for (int i = 0; (i < self->len); (i++)) {
        (acc = fn(acc, self->data[i]));
    }
    return acc;
}

static btrc_Vector_DisplayLayoutRule* btrc_Vector_DisplayLayoutRule_copy(btrc_Vector_DisplayLayoutRule* self) {
    btrc_Vector_DisplayLayoutRule* result = btrc_Vector_DisplayLayoutRule_new();
    for (int i = 0; (i < self->len); (i++)) {
        btrc_Vector_DisplayLayoutRule_push(result, self->data[i]);
    }
    return result;
}

static void btrc_Vector_DisplayLayoutRule_removeAt(btrc_Vector_DisplayLayoutRule* self, int idx) {
    btrc_Vector_DisplayLayoutRule_remove(self, idx);
}

static int btrc_Vector_DisplayLayoutRule_iterLen(btrc_Vector_DisplayLayoutRule* self) {
    return self->len;
}

static DisplayLayoutRule* btrc_Vector_DisplayLayoutRule_iterGet(btrc_Vector_DisplayLayoutRule* self, int i) {
    return self->data[i];
}

static void btrc_Vector_AudioPreset_init(btrc_Vector_AudioPreset* self) {
    self->__rc = 1;
    (self->data = NULL);
    (self->len = 0);
    (self->cap = 0);
}

static btrc_Vector_AudioPreset* btrc_Vector_AudioPreset_new(void) {
    btrc_Vector_AudioPreset* self = ((btrc_Vector_AudioPreset*)malloc(sizeof(btrc_Vector_AudioPreset)));
    memset(self, 0, sizeof(btrc_Vector_AudioPreset));
    btrc_Vector_AudioPreset_init(self);
    return self;
}

static void btrc_Vector_AudioPreset_destroy(btrc_Vector_AudioPreset* self) {
    free(self);
}

static void btrc_Vector_AudioPreset_push(btrc_Vector_AudioPreset* self, AudioPreset* val) {
    (val->__rc++);
    if (self->len >= self->cap) {
        (self->cap = ((self->cap == 0) ? 4 : (self->cap * 2)));
        (self->data = ((AudioPreset**)__btrc_safe_realloc(self->data, (sizeof(AudioPreset*) * self->cap))));
    }
    (self->data[self->len] = val);
    (self->len++);
}

static AudioPreset* btrc_Vector_AudioPreset_pop(btrc_Vector_AudioPreset* self) {
    if (self->len <= 0) {
        fprintf(stderr, "Vector pop from empty list\n");
        exit(1);
    }
    (self->len--);
    return self->data[self->len];
}

static AudioPreset* btrc_Vector_AudioPreset_get(btrc_Vector_AudioPreset* self, int i) {
    if ((i < 0) || (i >= self->len)) {
        fprintf(stderr, "Vector index out of bounds: %d (len=%d)\n", i, self->len);
        exit(1);
    }
    return self->data[i];
}

static void btrc_Vector_AudioPreset_set(btrc_Vector_AudioPreset* self, int i, AudioPreset* val) {
    if ((i < 0) || (i >= self->len)) {
        fprintf(stderr, "Vector index out of bounds: %d (len=%d)\n", i, self->len);
        exit(1);
    }
    if (self->data[i]) {
        if ((--self->data[i]->__rc) <= 0) {
            AudioPreset_destroy(self->data[i]);
        }
    }
    (val->__rc++);
    (self->data[i] = val);
}

static void btrc_Vector_AudioPreset_free(btrc_Vector_AudioPreset* self) {
    for (int i = 0; (i < self->len); (i++)) {
        if (self->data[i]) {
            if ((--self->data[i]->__rc) <= 0) {
                AudioPreset_destroy(self->data[i]);
            }
        }
    }
    free(self->data);
    (self->data = NULL);
    (self->len = 0);
    (self->cap = 0);
}

static void btrc_Vector_AudioPreset_remove(btrc_Vector_AudioPreset* self, int idx) {
    if ((idx < 0) || (idx >= self->len)) {
        fprintf(stderr, "Vector remove index out of bounds: %d (len=%d)\n", idx, self->len);
        exit(1);
    }
    if (self->data[idx]) {
        if ((--self->data[idx]->__rc) <= 0) {
            AudioPreset_destroy(self->data[idx]);
        }
    }
    for (int i = idx; (i < (self->len - 1)); (i++)) {
        (self->data[i] = self->data[(i + 1)]);
    }
    (self->len--);
}

static void btrc_Vector_AudioPreset_reverse(btrc_Vector_AudioPreset* self) {
    for (int i = 0; (i < (self->len / 2)); (i++)) {
        AudioPreset* tmp = self->data[i];
        (self->data[i] = self->data[((self->len - 1) - i)]);
        (self->data[((self->len - 1) - i)] = tmp);
    }
}

static btrc_Vector_AudioPreset* btrc_Vector_AudioPreset_reversed(btrc_Vector_AudioPreset* self) {
    btrc_Vector_AudioPreset* result = btrc_Vector_AudioPreset_new();
    for (int i = (self->len - 1); (i >= 0); (i--)) {
        btrc_Vector_AudioPreset_push(result, self->data[i]);
    }
    return result;
}

static void btrc_Vector_AudioPreset_swap(btrc_Vector_AudioPreset* self, int i, int j) {
    if ((((i < 0) || (i >= self->len)) || (j < 0)) || (j >= self->len)) {
        fprintf(stderr, "Vector swap index out of bounds\n");
        exit(1);
    }
    AudioPreset* tmp = self->data[i];
    (self->data[i] = self->data[j]);
    (self->data[j] = tmp);
}

static void btrc_Vector_AudioPreset_clear(btrc_Vector_AudioPreset* self) {
    for (int i = 0; (i < self->len); (i++)) {
        if (self->data[i]) {
            if ((--self->data[i]->__rc) <= 0) {
                AudioPreset_destroy(self->data[i]);
            }
        }
    }
    (self->len = 0);
}

static void btrc_Vector_AudioPreset_fill(btrc_Vector_AudioPreset* self, AudioPreset* val) {
    for (int i = 0; (i < self->len); (i++)) {
        if (self->data[i]) {
            if ((--self->data[i]->__rc) <= 0) {
                AudioPreset_destroy(self->data[i]);
            }
        }
        (val->__rc++);
        (self->data[i] = val);
    }
}

static int btrc_Vector_AudioPreset_size(btrc_Vector_AudioPreset* self) {
    return self->len;
}

static bool btrc_Vector_AudioPreset_isEmpty(btrc_Vector_AudioPreset* self) {
    return (self->len == 0);
}

static AudioPreset* btrc_Vector_AudioPreset_first(btrc_Vector_AudioPreset* self) {
    if (self->len == 0) {
        fprintf(stderr, "Vector.first() called on empty list\n");
        exit(1);
    }
    return self->data[0];
}

static AudioPreset* btrc_Vector_AudioPreset_last(btrc_Vector_AudioPreset* self) {
    if (self->len == 0) {
        fprintf(stderr, "Vector.last() called on empty list\n");
        exit(1);
    }
    return self->data[(self->len - 1)];
}

static btrc_Vector_AudioPreset* btrc_Vector_AudioPreset_slice(btrc_Vector_AudioPreset* self, int start, int end) {
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
    btrc_Vector_AudioPreset* result = btrc_Vector_AudioPreset_new();
    for (int i = start; (i < end); (i++)) {
        btrc_Vector_AudioPreset_push(result, self->data[i]);
    }
    return result;
}

static btrc_Vector_AudioPreset* btrc_Vector_AudioPreset_take(btrc_Vector_AudioPreset* self, int n) {
    if (n > self->len) {
        (n = self->len);
    }
    if (n < 0) {
        (n = 0);
    }
    return btrc_Vector_AudioPreset_slice(self, 0, n);
}

static btrc_Vector_AudioPreset* btrc_Vector_AudioPreset_drop(btrc_Vector_AudioPreset* self, int n) {
    if (n > self->len) {
        (n = self->len);
    }
    if (n < 0) {
        (n = 0);
    }
    return btrc_Vector_AudioPreset_slice(self, n, self->len);
}

static void btrc_Vector_AudioPreset_extend(btrc_Vector_AudioPreset* self, btrc_Vector_AudioPreset* other) {
    for (int i = 0; (i < other->len); (i++)) {
        btrc_Vector_AudioPreset_push(self, other->data[i]);
    }
}

static void btrc_Vector_AudioPreset_insert(btrc_Vector_AudioPreset* self, int idx, AudioPreset* val) {
    if ((idx < 0) || (idx > self->len)) {
        fprintf(stderr, "Vector insert index out of bounds: %d (size %d)\n", idx, self->len);
        exit(1);
    }
    (val->__rc++);
    if (self->len >= self->cap) {
        (self->cap = ((self->cap == 0) ? 4 : (self->cap * 2)));
        (self->data = ((AudioPreset**)__btrc_safe_realloc(self->data, (sizeof(AudioPreset*) * self->cap))));
    }
    for (int i = self->len; (i > idx); (i--)) {
        (self->data[i] = self->data[(i - 1)]);
    }
    (self->data[idx] = val);
    (self->len++);
}

static bool btrc_Vector_AudioPreset_contains(btrc_Vector_AudioPreset* self, AudioPreset* val) {
    for (int i = 0; (i < self->len); (i++)) {
        if (__btrc_eq(self->data[i], val)) {
            return true;
        }
    }
    return false;
}

static int btrc_Vector_AudioPreset_indexOf(btrc_Vector_AudioPreset* self, AudioPreset* val) {
    for (int i = 0; (i < self->len); (i++)) {
        if (__btrc_eq(self->data[i], val)) {
            return i;
        }
    }
    return (-1);
}

static int btrc_Vector_AudioPreset_lastIndexOf(btrc_Vector_AudioPreset* self, AudioPreset* val) {
    for (int i = (self->len - 1); (i >= 0); (i--)) {
        if (__btrc_eq(self->data[i], val)) {
            return i;
        }
    }
    return (-1);
}

static int btrc_Vector_AudioPreset_count(btrc_Vector_AudioPreset* self, AudioPreset* val) {
    int c = 0;
    for (int i = 0; (i < self->len); (i++)) {
        if (__btrc_eq(self->data[i], val)) {
            (c++);
        }
    }
    return c;
}

static void btrc_Vector_AudioPreset_removeAll(btrc_Vector_AudioPreset* self, AudioPreset* val) {
    int j = 0;
    for (int i = 0; (i < self->len); (i++)) {
        if (!__btrc_eq(self->data[i], val)) {
            (self->data[j] = self->data[i]);
            (j++);
        } else if (self->data[i]) {
            if ((--self->data[i]->__rc) <= 0) {
                AudioPreset_destroy(self->data[i]);
            }
        }
    }
    (self->len = j);
}

static btrc_Vector_AudioPreset* btrc_Vector_AudioPreset_distinct(btrc_Vector_AudioPreset* self) {
    btrc_Vector_AudioPreset* result = btrc_Vector_AudioPreset_new();
    for (int i = 0; (i < self->len); (i++)) {
        if (!btrc_Vector_AudioPreset_contains(result, self->data[i])) {
            btrc_Vector_AudioPreset_push(result, self->data[i]);
        }
    }
    return result;
}

static void btrc_Vector_AudioPreset_sort(btrc_Vector_AudioPreset* self) {
    for (int i = 1; (i < self->len); (i++)) {
        AudioPreset* key = self->data[i];
        int j = (i - 1);
        while ((j >= 0) && __btrc_lt(key, self->data[j])) {
            (self->data[(j + 1)] = self->data[j]);
            (j = (j - 1));
        }
        (self->data[(j + 1)] = key);
    }
}

static btrc_Vector_AudioPreset* btrc_Vector_AudioPreset_sorted(btrc_Vector_AudioPreset* self) {
    btrc_Vector_AudioPreset* result = btrc_Vector_AudioPreset_new();
    for (int i = 0; (i < self->len); (i++)) {
        btrc_Vector_AudioPreset_push(result, self->data[i]);
    }
    btrc_Vector_AudioPreset_sort(result);
    return result;
}

static AudioPreset* btrc_Vector_AudioPreset_min(btrc_Vector_AudioPreset* self) {
    if (self->len <= 0) {
        fprintf(stderr, "Vector min on empty list\n");
        exit(1);
    }
    AudioPreset* m = self->data[0];
    for (int i = 1; (i < self->len); (i++)) {
        if (__btrc_lt(self->data[i], m)) {
            (m = self->data[i]);
        }
    }
    return m;
}

static AudioPreset* btrc_Vector_AudioPreset_max(btrc_Vector_AudioPreset* self) {
    if (self->len <= 0) {
        fprintf(stderr, "Vector max on empty list\n");
        exit(1);
    }
    AudioPreset* m = self->data[0];
    for (int i = 1; (i < self->len); (i++)) {
        if (__btrc_gt(self->data[i], m)) {
            (m = self->data[i]);
        }
    }
    return m;
}

static btrc_Vector_AudioPreset* btrc_Vector_AudioPreset_filter(btrc_Vector_AudioPreset* self, __btrc_fn_bool_AudioPreset pred) {
    btrc_Vector_AudioPreset* result = btrc_Vector_AudioPreset_new();
    for (int i = 0; (i < self->len); (i++)) {
        if (pred(self->data[i])) {
            btrc_Vector_AudioPreset_push(result, self->data[i]);
        }
    }
    return result;
}

static int btrc_Vector_AudioPreset_findIndex(btrc_Vector_AudioPreset* self, __btrc_fn_bool_AudioPreset pred) {
    for (int i = 0; (i < self->len); (i++)) {
        if (pred(self->data[i])) {
            return i;
        }
    }
    return (-1);
}

static void btrc_Vector_AudioPreset_forEach(btrc_Vector_AudioPreset* self, __btrc_fn_void_AudioPreset fn) {
    for (int i = 0; (i < self->len); (i++)) {
        fn(self->data[i]);
    }
}

static btrc_Vector_AudioPreset* btrc_Vector_AudioPreset_map(btrc_Vector_AudioPreset* self, __btrc_fn_AudioPreset_AudioPreset fn) {
    btrc_Vector_AudioPreset* result = btrc_Vector_AudioPreset_new();
    for (int i = 0; (i < self->len); (i++)) {
        btrc_Vector_AudioPreset_push(result, fn(self->data[i]));
    }
    return result;
}

static bool btrc_Vector_AudioPreset_any(btrc_Vector_AudioPreset* self, __btrc_fn_bool_AudioPreset pred) {
    for (int i = 0; (i < self->len); (i++)) {
        if (pred(self->data[i])) {
            return true;
        }
    }
    return false;
}

static bool btrc_Vector_AudioPreset_all(btrc_Vector_AudioPreset* self, __btrc_fn_bool_AudioPreset pred) {
    for (int i = 0; (i < self->len); (i++)) {
        if (!pred(self->data[i])) {
            return false;
        }
    }
    return true;
}

static AudioPreset* btrc_Vector_AudioPreset_reduce(btrc_Vector_AudioPreset* self, AudioPreset* init, __btrc_fn_AudioPreset_AudioPreset_AudioPreset fn) {
    AudioPreset* acc = init;
    for (int i = 0; (i < self->len); (i++)) {
        (acc = fn(acc, self->data[i]));
    }
    return acc;
}

static btrc_Vector_AudioPreset* btrc_Vector_AudioPreset_copy(btrc_Vector_AudioPreset* self) {
    btrc_Vector_AudioPreset* result = btrc_Vector_AudioPreset_new();
    for (int i = 0; (i < self->len); (i++)) {
        btrc_Vector_AudioPreset_push(result, self->data[i]);
    }
    return result;
}

static void btrc_Vector_AudioPreset_removeAt(btrc_Vector_AudioPreset* self, int idx) {
    btrc_Vector_AudioPreset_remove(self, idx);
}

static int btrc_Vector_AudioPreset_iterLen(btrc_Vector_AudioPreset* self) {
    return self->len;
}

static AudioPreset* btrc_Vector_AudioPreset_iterGet(btrc_Vector_AudioPreset* self, int i) {
    return self->data[i];
}

static void btrc_Vector_AudioSink_init(btrc_Vector_AudioSink* self) {
    self->__rc = 1;
    (self->data = NULL);
    (self->len = 0);
    (self->cap = 0);
}

static btrc_Vector_AudioSink* btrc_Vector_AudioSink_new(void) {
    btrc_Vector_AudioSink* self = ((btrc_Vector_AudioSink*)malloc(sizeof(btrc_Vector_AudioSink)));
    memset(self, 0, sizeof(btrc_Vector_AudioSink));
    btrc_Vector_AudioSink_init(self);
    return self;
}

static void btrc_Vector_AudioSink_destroy(btrc_Vector_AudioSink* self) {
    free(self);
}

static void btrc_Vector_AudioSink_push(btrc_Vector_AudioSink* self, AudioSink* val) {
    (val->__rc++);
    if (self->len >= self->cap) {
        (self->cap = ((self->cap == 0) ? 4 : (self->cap * 2)));
        (self->data = ((AudioSink**)__btrc_safe_realloc(self->data, (sizeof(AudioSink*) * self->cap))));
    }
    (self->data[self->len] = val);
    (self->len++);
}

static AudioSink* btrc_Vector_AudioSink_pop(btrc_Vector_AudioSink* self) {
    if (self->len <= 0) {
        fprintf(stderr, "Vector pop from empty list\n");
        exit(1);
    }
    (self->len--);
    return self->data[self->len];
}

static AudioSink* btrc_Vector_AudioSink_get(btrc_Vector_AudioSink* self, int i) {
    if ((i < 0) || (i >= self->len)) {
        fprintf(stderr, "Vector index out of bounds: %d (len=%d)\n", i, self->len);
        exit(1);
    }
    return self->data[i];
}

static void btrc_Vector_AudioSink_set(btrc_Vector_AudioSink* self, int i, AudioSink* val) {
    if ((i < 0) || (i >= self->len)) {
        fprintf(stderr, "Vector index out of bounds: %d (len=%d)\n", i, self->len);
        exit(1);
    }
    if (self->data[i]) {
        if ((--self->data[i]->__rc) <= 0) {
            AudioSink_destroy(self->data[i]);
        }
    }
    (val->__rc++);
    (self->data[i] = val);
}

static void btrc_Vector_AudioSink_free(btrc_Vector_AudioSink* self) {
    for (int i = 0; (i < self->len); (i++)) {
        if (self->data[i]) {
            if ((--self->data[i]->__rc) <= 0) {
                AudioSink_destroy(self->data[i]);
            }
        }
    }
    free(self->data);
    (self->data = NULL);
    (self->len = 0);
    (self->cap = 0);
}

static void btrc_Vector_AudioSink_remove(btrc_Vector_AudioSink* self, int idx) {
    if ((idx < 0) || (idx >= self->len)) {
        fprintf(stderr, "Vector remove index out of bounds: %d (len=%d)\n", idx, self->len);
        exit(1);
    }
    if (self->data[idx]) {
        if ((--self->data[idx]->__rc) <= 0) {
            AudioSink_destroy(self->data[idx]);
        }
    }
    for (int i = idx; (i < (self->len - 1)); (i++)) {
        (self->data[i] = self->data[(i + 1)]);
    }
    (self->len--);
}

static void btrc_Vector_AudioSink_reverse(btrc_Vector_AudioSink* self) {
    for (int i = 0; (i < (self->len / 2)); (i++)) {
        AudioSink* tmp = self->data[i];
        (self->data[i] = self->data[((self->len - 1) - i)]);
        (self->data[((self->len - 1) - i)] = tmp);
    }
}

static btrc_Vector_AudioSink* btrc_Vector_AudioSink_reversed(btrc_Vector_AudioSink* self) {
    btrc_Vector_AudioSink* result = btrc_Vector_AudioSink_new();
    for (int i = (self->len - 1); (i >= 0); (i--)) {
        btrc_Vector_AudioSink_push(result, self->data[i]);
    }
    return result;
}

static void btrc_Vector_AudioSink_swap(btrc_Vector_AudioSink* self, int i, int j) {
    if ((((i < 0) || (i >= self->len)) || (j < 0)) || (j >= self->len)) {
        fprintf(stderr, "Vector swap index out of bounds\n");
        exit(1);
    }
    AudioSink* tmp = self->data[i];
    (self->data[i] = self->data[j]);
    (self->data[j] = tmp);
}

static void btrc_Vector_AudioSink_clear(btrc_Vector_AudioSink* self) {
    for (int i = 0; (i < self->len); (i++)) {
        if (self->data[i]) {
            if ((--self->data[i]->__rc) <= 0) {
                AudioSink_destroy(self->data[i]);
            }
        }
    }
    (self->len = 0);
}

static void btrc_Vector_AudioSink_fill(btrc_Vector_AudioSink* self, AudioSink* val) {
    for (int i = 0; (i < self->len); (i++)) {
        if (self->data[i]) {
            if ((--self->data[i]->__rc) <= 0) {
                AudioSink_destroy(self->data[i]);
            }
        }
        (val->__rc++);
        (self->data[i] = val);
    }
}

static int btrc_Vector_AudioSink_size(btrc_Vector_AudioSink* self) {
    return self->len;
}

static bool btrc_Vector_AudioSink_isEmpty(btrc_Vector_AudioSink* self) {
    return (self->len == 0);
}

static AudioSink* btrc_Vector_AudioSink_first(btrc_Vector_AudioSink* self) {
    if (self->len == 0) {
        fprintf(stderr, "Vector.first() called on empty list\n");
        exit(1);
    }
    return self->data[0];
}

static AudioSink* btrc_Vector_AudioSink_last(btrc_Vector_AudioSink* self) {
    if (self->len == 0) {
        fprintf(stderr, "Vector.last() called on empty list\n");
        exit(1);
    }
    return self->data[(self->len - 1)];
}

static btrc_Vector_AudioSink* btrc_Vector_AudioSink_slice(btrc_Vector_AudioSink* self, int start, int end) {
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
    btrc_Vector_AudioSink* result = btrc_Vector_AudioSink_new();
    for (int i = start; (i < end); (i++)) {
        btrc_Vector_AudioSink_push(result, self->data[i]);
    }
    return result;
}

static btrc_Vector_AudioSink* btrc_Vector_AudioSink_take(btrc_Vector_AudioSink* self, int n) {
    if (n > self->len) {
        (n = self->len);
    }
    if (n < 0) {
        (n = 0);
    }
    return btrc_Vector_AudioSink_slice(self, 0, n);
}

static btrc_Vector_AudioSink* btrc_Vector_AudioSink_drop(btrc_Vector_AudioSink* self, int n) {
    if (n > self->len) {
        (n = self->len);
    }
    if (n < 0) {
        (n = 0);
    }
    return btrc_Vector_AudioSink_slice(self, n, self->len);
}

static void btrc_Vector_AudioSink_extend(btrc_Vector_AudioSink* self, btrc_Vector_AudioSink* other) {
    for (int i = 0; (i < other->len); (i++)) {
        btrc_Vector_AudioSink_push(self, other->data[i]);
    }
}

static void btrc_Vector_AudioSink_insert(btrc_Vector_AudioSink* self, int idx, AudioSink* val) {
    if ((idx < 0) || (idx > self->len)) {
        fprintf(stderr, "Vector insert index out of bounds: %d (size %d)\n", idx, self->len);
        exit(1);
    }
    (val->__rc++);
    if (self->len >= self->cap) {
        (self->cap = ((self->cap == 0) ? 4 : (self->cap * 2)));
        (self->data = ((AudioSink**)__btrc_safe_realloc(self->data, (sizeof(AudioSink*) * self->cap))));
    }
    for (int i = self->len; (i > idx); (i--)) {
        (self->data[i] = self->data[(i - 1)]);
    }
    (self->data[idx] = val);
    (self->len++);
}

static bool btrc_Vector_AudioSink_contains(btrc_Vector_AudioSink* self, AudioSink* val) {
    for (int i = 0; (i < self->len); (i++)) {
        if (__btrc_eq(self->data[i], val)) {
            return true;
        }
    }
    return false;
}

static int btrc_Vector_AudioSink_indexOf(btrc_Vector_AudioSink* self, AudioSink* val) {
    for (int i = 0; (i < self->len); (i++)) {
        if (__btrc_eq(self->data[i], val)) {
            return i;
        }
    }
    return (-1);
}

static int btrc_Vector_AudioSink_lastIndexOf(btrc_Vector_AudioSink* self, AudioSink* val) {
    for (int i = (self->len - 1); (i >= 0); (i--)) {
        if (__btrc_eq(self->data[i], val)) {
            return i;
        }
    }
    return (-1);
}

static int btrc_Vector_AudioSink_count(btrc_Vector_AudioSink* self, AudioSink* val) {
    int c = 0;
    for (int i = 0; (i < self->len); (i++)) {
        if (__btrc_eq(self->data[i], val)) {
            (c++);
        }
    }
    return c;
}

static void btrc_Vector_AudioSink_removeAll(btrc_Vector_AudioSink* self, AudioSink* val) {
    int j = 0;
    for (int i = 0; (i < self->len); (i++)) {
        if (!__btrc_eq(self->data[i], val)) {
            (self->data[j] = self->data[i]);
            (j++);
        } else if (self->data[i]) {
            if ((--self->data[i]->__rc) <= 0) {
                AudioSink_destroy(self->data[i]);
            }
        }
    }
    (self->len = j);
}

static btrc_Vector_AudioSink* btrc_Vector_AudioSink_distinct(btrc_Vector_AudioSink* self) {
    btrc_Vector_AudioSink* result = btrc_Vector_AudioSink_new();
    for (int i = 0; (i < self->len); (i++)) {
        if (!btrc_Vector_AudioSink_contains(result, self->data[i])) {
            btrc_Vector_AudioSink_push(result, self->data[i]);
        }
    }
    return result;
}

static void btrc_Vector_AudioSink_sort(btrc_Vector_AudioSink* self) {
    for (int i = 1; (i < self->len); (i++)) {
        AudioSink* key = self->data[i];
        int j = (i - 1);
        while ((j >= 0) && __btrc_lt(key, self->data[j])) {
            (self->data[(j + 1)] = self->data[j]);
            (j = (j - 1));
        }
        (self->data[(j + 1)] = key);
    }
}

static btrc_Vector_AudioSink* btrc_Vector_AudioSink_sorted(btrc_Vector_AudioSink* self) {
    btrc_Vector_AudioSink* result = btrc_Vector_AudioSink_new();
    for (int i = 0; (i < self->len); (i++)) {
        btrc_Vector_AudioSink_push(result, self->data[i]);
    }
    btrc_Vector_AudioSink_sort(result);
    return result;
}

static AudioSink* btrc_Vector_AudioSink_min(btrc_Vector_AudioSink* self) {
    if (self->len <= 0) {
        fprintf(stderr, "Vector min on empty list\n");
        exit(1);
    }
    AudioSink* m = self->data[0];
    for (int i = 1; (i < self->len); (i++)) {
        if (__btrc_lt(self->data[i], m)) {
            (m = self->data[i]);
        }
    }
    return m;
}

static AudioSink* btrc_Vector_AudioSink_max(btrc_Vector_AudioSink* self) {
    if (self->len <= 0) {
        fprintf(stderr, "Vector max on empty list\n");
        exit(1);
    }
    AudioSink* m = self->data[0];
    for (int i = 1; (i < self->len); (i++)) {
        if (__btrc_gt(self->data[i], m)) {
            (m = self->data[i]);
        }
    }
    return m;
}

static btrc_Vector_AudioSink* btrc_Vector_AudioSink_filter(btrc_Vector_AudioSink* self, __btrc_fn_bool_AudioSink pred) {
    btrc_Vector_AudioSink* result = btrc_Vector_AudioSink_new();
    for (int i = 0; (i < self->len); (i++)) {
        if (pred(self->data[i])) {
            btrc_Vector_AudioSink_push(result, self->data[i]);
        }
    }
    return result;
}

static int btrc_Vector_AudioSink_findIndex(btrc_Vector_AudioSink* self, __btrc_fn_bool_AudioSink pred) {
    for (int i = 0; (i < self->len); (i++)) {
        if (pred(self->data[i])) {
            return i;
        }
    }
    return (-1);
}

static void btrc_Vector_AudioSink_forEach(btrc_Vector_AudioSink* self, __btrc_fn_void_AudioSink fn) {
    for (int i = 0; (i < self->len); (i++)) {
        fn(self->data[i]);
    }
}

static btrc_Vector_AudioSink* btrc_Vector_AudioSink_map(btrc_Vector_AudioSink* self, __btrc_fn_AudioSink_AudioSink fn) {
    btrc_Vector_AudioSink* result = btrc_Vector_AudioSink_new();
    for (int i = 0; (i < self->len); (i++)) {
        btrc_Vector_AudioSink_push(result, fn(self->data[i]));
    }
    return result;
}

static bool btrc_Vector_AudioSink_any(btrc_Vector_AudioSink* self, __btrc_fn_bool_AudioSink pred) {
    for (int i = 0; (i < self->len); (i++)) {
        if (pred(self->data[i])) {
            return true;
        }
    }
    return false;
}

static bool btrc_Vector_AudioSink_all(btrc_Vector_AudioSink* self, __btrc_fn_bool_AudioSink pred) {
    for (int i = 0; (i < self->len); (i++)) {
        if (!pred(self->data[i])) {
            return false;
        }
    }
    return true;
}

static AudioSink* btrc_Vector_AudioSink_reduce(btrc_Vector_AudioSink* self, AudioSink* init, __btrc_fn_AudioSink_AudioSink_AudioSink fn) {
    AudioSink* acc = init;
    for (int i = 0; (i < self->len); (i++)) {
        (acc = fn(acc, self->data[i]));
    }
    return acc;
}

static btrc_Vector_AudioSink* btrc_Vector_AudioSink_copy(btrc_Vector_AudioSink* self) {
    btrc_Vector_AudioSink* result = btrc_Vector_AudioSink_new();
    for (int i = 0; (i < self->len); (i++)) {
        btrc_Vector_AudioSink_push(result, self->data[i]);
    }
    return result;
}

static void btrc_Vector_AudioSink_removeAt(btrc_Vector_AudioSink* self, int idx) {
    btrc_Vector_AudioSink_remove(self, idx);
}

static int btrc_Vector_AudioSink_iterLen(btrc_Vector_AudioSink* self) {
    return self->len;
}

static AudioSink* btrc_Vector_AudioSink_iterGet(btrc_Vector_AudioSink* self, int i) {
    return self->data[i];
}

static void btrc_Vector_DisplayOutput_init(btrc_Vector_DisplayOutput* self) {
    self->__rc = 1;
    (self->data = NULL);
    (self->len = 0);
    (self->cap = 0);
}

static btrc_Vector_DisplayOutput* btrc_Vector_DisplayOutput_new(void) {
    btrc_Vector_DisplayOutput* self = ((btrc_Vector_DisplayOutput*)malloc(sizeof(btrc_Vector_DisplayOutput)));
    memset(self, 0, sizeof(btrc_Vector_DisplayOutput));
    btrc_Vector_DisplayOutput_init(self);
    return self;
}

static void btrc_Vector_DisplayOutput_destroy(btrc_Vector_DisplayOutput* self) {
    free(self);
}

static void btrc_Vector_DisplayOutput_push(btrc_Vector_DisplayOutput* self, DisplayOutput* val) {
    (val->__rc++);
    if (self->len >= self->cap) {
        (self->cap = ((self->cap == 0) ? 4 : (self->cap * 2)));
        (self->data = ((DisplayOutput**)__btrc_safe_realloc(self->data, (sizeof(DisplayOutput*) * self->cap))));
    }
    (self->data[self->len] = val);
    (self->len++);
}

static DisplayOutput* btrc_Vector_DisplayOutput_pop(btrc_Vector_DisplayOutput* self) {
    if (self->len <= 0) {
        fprintf(stderr, "Vector pop from empty list\n");
        exit(1);
    }
    (self->len--);
    return self->data[self->len];
}

static DisplayOutput* btrc_Vector_DisplayOutput_get(btrc_Vector_DisplayOutput* self, int i) {
    if ((i < 0) || (i >= self->len)) {
        fprintf(stderr, "Vector index out of bounds: %d (len=%d)\n", i, self->len);
        exit(1);
    }
    return self->data[i];
}

static void btrc_Vector_DisplayOutput_set(btrc_Vector_DisplayOutput* self, int i, DisplayOutput* val) {
    if ((i < 0) || (i >= self->len)) {
        fprintf(stderr, "Vector index out of bounds: %d (len=%d)\n", i, self->len);
        exit(1);
    }
    if (self->data[i]) {
        if ((--self->data[i]->__rc) <= 0) {
            DisplayOutput_destroy(self->data[i]);
        }
    }
    (val->__rc++);
    (self->data[i] = val);
}

static void btrc_Vector_DisplayOutput_free(btrc_Vector_DisplayOutput* self) {
    for (int i = 0; (i < self->len); (i++)) {
        if (self->data[i]) {
            if ((--self->data[i]->__rc) <= 0) {
                DisplayOutput_destroy(self->data[i]);
            }
        }
    }
    free(self->data);
    (self->data = NULL);
    (self->len = 0);
    (self->cap = 0);
}

static void btrc_Vector_DisplayOutput_remove(btrc_Vector_DisplayOutput* self, int idx) {
    if ((idx < 0) || (idx >= self->len)) {
        fprintf(stderr, "Vector remove index out of bounds: %d (len=%d)\n", idx, self->len);
        exit(1);
    }
    if (self->data[idx]) {
        if ((--self->data[idx]->__rc) <= 0) {
            DisplayOutput_destroy(self->data[idx]);
        }
    }
    for (int i = idx; (i < (self->len - 1)); (i++)) {
        (self->data[i] = self->data[(i + 1)]);
    }
    (self->len--);
}

static void btrc_Vector_DisplayOutput_reverse(btrc_Vector_DisplayOutput* self) {
    for (int i = 0; (i < (self->len / 2)); (i++)) {
        DisplayOutput* tmp = self->data[i];
        (self->data[i] = self->data[((self->len - 1) - i)]);
        (self->data[((self->len - 1) - i)] = tmp);
    }
}

static btrc_Vector_DisplayOutput* btrc_Vector_DisplayOutput_reversed(btrc_Vector_DisplayOutput* self) {
    btrc_Vector_DisplayOutput* result = btrc_Vector_DisplayOutput_new();
    for (int i = (self->len - 1); (i >= 0); (i--)) {
        btrc_Vector_DisplayOutput_push(result, self->data[i]);
    }
    return result;
}

static void btrc_Vector_DisplayOutput_swap(btrc_Vector_DisplayOutput* self, int i, int j) {
    if ((((i < 0) || (i >= self->len)) || (j < 0)) || (j >= self->len)) {
        fprintf(stderr, "Vector swap index out of bounds\n");
        exit(1);
    }
    DisplayOutput* tmp = self->data[i];
    (self->data[i] = self->data[j]);
    (self->data[j] = tmp);
}

static void btrc_Vector_DisplayOutput_clear(btrc_Vector_DisplayOutput* self) {
    for (int i = 0; (i < self->len); (i++)) {
        if (self->data[i]) {
            if ((--self->data[i]->__rc) <= 0) {
                DisplayOutput_destroy(self->data[i]);
            }
        }
    }
    (self->len = 0);
}

static void btrc_Vector_DisplayOutput_fill(btrc_Vector_DisplayOutput* self, DisplayOutput* val) {
    for (int i = 0; (i < self->len); (i++)) {
        if (self->data[i]) {
            if ((--self->data[i]->__rc) <= 0) {
                DisplayOutput_destroy(self->data[i]);
            }
        }
        (val->__rc++);
        (self->data[i] = val);
    }
}

static int btrc_Vector_DisplayOutput_size(btrc_Vector_DisplayOutput* self) {
    return self->len;
}

static bool btrc_Vector_DisplayOutput_isEmpty(btrc_Vector_DisplayOutput* self) {
    return (self->len == 0);
}

static DisplayOutput* btrc_Vector_DisplayOutput_first(btrc_Vector_DisplayOutput* self) {
    if (self->len == 0) {
        fprintf(stderr, "Vector.first() called on empty list\n");
        exit(1);
    }
    return self->data[0];
}

static DisplayOutput* btrc_Vector_DisplayOutput_last(btrc_Vector_DisplayOutput* self) {
    if (self->len == 0) {
        fprintf(stderr, "Vector.last() called on empty list\n");
        exit(1);
    }
    return self->data[(self->len - 1)];
}

static btrc_Vector_DisplayOutput* btrc_Vector_DisplayOutput_slice(btrc_Vector_DisplayOutput* self, int start, int end) {
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
    btrc_Vector_DisplayOutput* result = btrc_Vector_DisplayOutput_new();
    for (int i = start; (i < end); (i++)) {
        btrc_Vector_DisplayOutput_push(result, self->data[i]);
    }
    return result;
}

static btrc_Vector_DisplayOutput* btrc_Vector_DisplayOutput_take(btrc_Vector_DisplayOutput* self, int n) {
    if (n > self->len) {
        (n = self->len);
    }
    if (n < 0) {
        (n = 0);
    }
    return btrc_Vector_DisplayOutput_slice(self, 0, n);
}

static btrc_Vector_DisplayOutput* btrc_Vector_DisplayOutput_drop(btrc_Vector_DisplayOutput* self, int n) {
    if (n > self->len) {
        (n = self->len);
    }
    if (n < 0) {
        (n = 0);
    }
    return btrc_Vector_DisplayOutput_slice(self, n, self->len);
}

static void btrc_Vector_DisplayOutput_extend(btrc_Vector_DisplayOutput* self, btrc_Vector_DisplayOutput* other) {
    for (int i = 0; (i < other->len); (i++)) {
        btrc_Vector_DisplayOutput_push(self, other->data[i]);
    }
}

static void btrc_Vector_DisplayOutput_insert(btrc_Vector_DisplayOutput* self, int idx, DisplayOutput* val) {
    if ((idx < 0) || (idx > self->len)) {
        fprintf(stderr, "Vector insert index out of bounds: %d (size %d)\n", idx, self->len);
        exit(1);
    }
    (val->__rc++);
    if (self->len >= self->cap) {
        (self->cap = ((self->cap == 0) ? 4 : (self->cap * 2)));
        (self->data = ((DisplayOutput**)__btrc_safe_realloc(self->data, (sizeof(DisplayOutput*) * self->cap))));
    }
    for (int i = self->len; (i > idx); (i--)) {
        (self->data[i] = self->data[(i - 1)]);
    }
    (self->data[idx] = val);
    (self->len++);
}

static bool btrc_Vector_DisplayOutput_contains(btrc_Vector_DisplayOutput* self, DisplayOutput* val) {
    for (int i = 0; (i < self->len); (i++)) {
        if (__btrc_eq(self->data[i], val)) {
            return true;
        }
    }
    return false;
}

static int btrc_Vector_DisplayOutput_indexOf(btrc_Vector_DisplayOutput* self, DisplayOutput* val) {
    for (int i = 0; (i < self->len); (i++)) {
        if (__btrc_eq(self->data[i], val)) {
            return i;
        }
    }
    return (-1);
}

static int btrc_Vector_DisplayOutput_lastIndexOf(btrc_Vector_DisplayOutput* self, DisplayOutput* val) {
    for (int i = (self->len - 1); (i >= 0); (i--)) {
        if (__btrc_eq(self->data[i], val)) {
            return i;
        }
    }
    return (-1);
}

static int btrc_Vector_DisplayOutput_count(btrc_Vector_DisplayOutput* self, DisplayOutput* val) {
    int c = 0;
    for (int i = 0; (i < self->len); (i++)) {
        if (__btrc_eq(self->data[i], val)) {
            (c++);
        }
    }
    return c;
}

static void btrc_Vector_DisplayOutput_removeAll(btrc_Vector_DisplayOutput* self, DisplayOutput* val) {
    int j = 0;
    for (int i = 0; (i < self->len); (i++)) {
        if (!__btrc_eq(self->data[i], val)) {
            (self->data[j] = self->data[i]);
            (j++);
        } else if (self->data[i]) {
            if ((--self->data[i]->__rc) <= 0) {
                DisplayOutput_destroy(self->data[i]);
            }
        }
    }
    (self->len = j);
}

static btrc_Vector_DisplayOutput* btrc_Vector_DisplayOutput_distinct(btrc_Vector_DisplayOutput* self) {
    btrc_Vector_DisplayOutput* result = btrc_Vector_DisplayOutput_new();
    for (int i = 0; (i < self->len); (i++)) {
        if (!btrc_Vector_DisplayOutput_contains(result, self->data[i])) {
            btrc_Vector_DisplayOutput_push(result, self->data[i]);
        }
    }
    return result;
}

static void btrc_Vector_DisplayOutput_sort(btrc_Vector_DisplayOutput* self) {
    for (int i = 1; (i < self->len); (i++)) {
        DisplayOutput* key = self->data[i];
        int j = (i - 1);
        while ((j >= 0) && __btrc_lt(key, self->data[j])) {
            (self->data[(j + 1)] = self->data[j]);
            (j = (j - 1));
        }
        (self->data[(j + 1)] = key);
    }
}

static btrc_Vector_DisplayOutput* btrc_Vector_DisplayOutput_sorted(btrc_Vector_DisplayOutput* self) {
    btrc_Vector_DisplayOutput* result = btrc_Vector_DisplayOutput_new();
    for (int i = 0; (i < self->len); (i++)) {
        btrc_Vector_DisplayOutput_push(result, self->data[i]);
    }
    btrc_Vector_DisplayOutput_sort(result);
    return result;
}

static DisplayOutput* btrc_Vector_DisplayOutput_min(btrc_Vector_DisplayOutput* self) {
    if (self->len <= 0) {
        fprintf(stderr, "Vector min on empty list\n");
        exit(1);
    }
    DisplayOutput* m = self->data[0];
    for (int i = 1; (i < self->len); (i++)) {
        if (__btrc_lt(self->data[i], m)) {
            (m = self->data[i]);
        }
    }
    return m;
}

static DisplayOutput* btrc_Vector_DisplayOutput_max(btrc_Vector_DisplayOutput* self) {
    if (self->len <= 0) {
        fprintf(stderr, "Vector max on empty list\n");
        exit(1);
    }
    DisplayOutput* m = self->data[0];
    for (int i = 1; (i < self->len); (i++)) {
        if (__btrc_gt(self->data[i], m)) {
            (m = self->data[i]);
        }
    }
    return m;
}

static btrc_Vector_DisplayOutput* btrc_Vector_DisplayOutput_filter(btrc_Vector_DisplayOutput* self, __btrc_fn_bool_DisplayOutput pred) {
    btrc_Vector_DisplayOutput* result = btrc_Vector_DisplayOutput_new();
    for (int i = 0; (i < self->len); (i++)) {
        if (pred(self->data[i])) {
            btrc_Vector_DisplayOutput_push(result, self->data[i]);
        }
    }
    return result;
}

static int btrc_Vector_DisplayOutput_findIndex(btrc_Vector_DisplayOutput* self, __btrc_fn_bool_DisplayOutput pred) {
    for (int i = 0; (i < self->len); (i++)) {
        if (pred(self->data[i])) {
            return i;
        }
    }
    return (-1);
}

static void btrc_Vector_DisplayOutput_forEach(btrc_Vector_DisplayOutput* self, __btrc_fn_void_DisplayOutput fn) {
    for (int i = 0; (i < self->len); (i++)) {
        fn(self->data[i]);
    }
}

static btrc_Vector_DisplayOutput* btrc_Vector_DisplayOutput_map(btrc_Vector_DisplayOutput* self, __btrc_fn_DisplayOutput_DisplayOutput fn) {
    btrc_Vector_DisplayOutput* result = btrc_Vector_DisplayOutput_new();
    for (int i = 0; (i < self->len); (i++)) {
        btrc_Vector_DisplayOutput_push(result, fn(self->data[i]));
    }
    return result;
}

static bool btrc_Vector_DisplayOutput_any(btrc_Vector_DisplayOutput* self, __btrc_fn_bool_DisplayOutput pred) {
    for (int i = 0; (i < self->len); (i++)) {
        if (pred(self->data[i])) {
            return true;
        }
    }
    return false;
}

static bool btrc_Vector_DisplayOutput_all(btrc_Vector_DisplayOutput* self, __btrc_fn_bool_DisplayOutput pred) {
    for (int i = 0; (i < self->len); (i++)) {
        if (!pred(self->data[i])) {
            return false;
        }
    }
    return true;
}

static DisplayOutput* btrc_Vector_DisplayOutput_reduce(btrc_Vector_DisplayOutput* self, DisplayOutput* init, __btrc_fn_DisplayOutput_DisplayOutput_DisplayOutput fn) {
    DisplayOutput* acc = init;
    for (int i = 0; (i < self->len); (i++)) {
        (acc = fn(acc, self->data[i]));
    }
    return acc;
}

static btrc_Vector_DisplayOutput* btrc_Vector_DisplayOutput_copy(btrc_Vector_DisplayOutput* self) {
    btrc_Vector_DisplayOutput* result = btrc_Vector_DisplayOutput_new();
    for (int i = 0; (i < self->len); (i++)) {
        btrc_Vector_DisplayOutput_push(result, self->data[i]);
    }
    return result;
}

static void btrc_Vector_DisplayOutput_removeAt(btrc_Vector_DisplayOutput* self, int idx) {
    btrc_Vector_DisplayOutput_remove(self, idx);
}

static int btrc_Vector_DisplayOutput_iterLen(btrc_Vector_DisplayOutput* self) {
    return self->len;
}

static DisplayOutput* btrc_Vector_DisplayOutput_iterGet(btrc_Vector_DisplayOutput* self, int i) {
    return self->data[i];
}

static void btrc_Vector_VmOperation_init(btrc_Vector_VmOperation* self) {
    self->__rc = 1;
    (self->data = NULL);
    (self->len = 0);
    (self->cap = 0);
}

static btrc_Vector_VmOperation* btrc_Vector_VmOperation_new(void) {
    btrc_Vector_VmOperation* self = ((btrc_Vector_VmOperation*)malloc(sizeof(btrc_Vector_VmOperation)));
    memset(self, 0, sizeof(btrc_Vector_VmOperation));
    btrc_Vector_VmOperation_init(self);
    return self;
}

static void btrc_Vector_VmOperation_destroy(btrc_Vector_VmOperation* self) {
    free(self);
}

static void btrc_Vector_VmOperation_push(btrc_Vector_VmOperation* self, VmOperation* val) {
    (val->__rc++);
    if (self->len >= self->cap) {
        (self->cap = ((self->cap == 0) ? 4 : (self->cap * 2)));
        (self->data = ((VmOperation**)__btrc_safe_realloc(self->data, (sizeof(VmOperation*) * self->cap))));
    }
    (self->data[self->len] = val);
    (self->len++);
}

static VmOperation* btrc_Vector_VmOperation_pop(btrc_Vector_VmOperation* self) {
    if (self->len <= 0) {
        fprintf(stderr, "Vector pop from empty list\n");
        exit(1);
    }
    (self->len--);
    return self->data[self->len];
}

static VmOperation* btrc_Vector_VmOperation_get(btrc_Vector_VmOperation* self, int i) {
    if ((i < 0) || (i >= self->len)) {
        fprintf(stderr, "Vector index out of bounds: %d (len=%d)\n", i, self->len);
        exit(1);
    }
    return self->data[i];
}

static void btrc_Vector_VmOperation_set(btrc_Vector_VmOperation* self, int i, VmOperation* val) {
    if ((i < 0) || (i >= self->len)) {
        fprintf(stderr, "Vector index out of bounds: %d (len=%d)\n", i, self->len);
        exit(1);
    }
    if (self->data[i]) {
        if ((--self->data[i]->__rc) <= 0) {
            VmOperation_destroy(self->data[i]);
        }
    }
    (val->__rc++);
    (self->data[i] = val);
}

static void btrc_Vector_VmOperation_free(btrc_Vector_VmOperation* self) {
    for (int i = 0; (i < self->len); (i++)) {
        if (self->data[i]) {
            if ((--self->data[i]->__rc) <= 0) {
                VmOperation_destroy(self->data[i]);
            }
        }
    }
    free(self->data);
    (self->data = NULL);
    (self->len = 0);
    (self->cap = 0);
}

static void btrc_Vector_VmOperation_remove(btrc_Vector_VmOperation* self, int idx) {
    if ((idx < 0) || (idx >= self->len)) {
        fprintf(stderr, "Vector remove index out of bounds: %d (len=%d)\n", idx, self->len);
        exit(1);
    }
    if (self->data[idx]) {
        if ((--self->data[idx]->__rc) <= 0) {
            VmOperation_destroy(self->data[idx]);
        }
    }
    for (int i = idx; (i < (self->len - 1)); (i++)) {
        (self->data[i] = self->data[(i + 1)]);
    }
    (self->len--);
}

static void btrc_Vector_VmOperation_reverse(btrc_Vector_VmOperation* self) {
    for (int i = 0; (i < (self->len / 2)); (i++)) {
        VmOperation* tmp = self->data[i];
        (self->data[i] = self->data[((self->len - 1) - i)]);
        (self->data[((self->len - 1) - i)] = tmp);
    }
}

static btrc_Vector_VmOperation* btrc_Vector_VmOperation_reversed(btrc_Vector_VmOperation* self) {
    btrc_Vector_VmOperation* result = btrc_Vector_VmOperation_new();
    for (int i = (self->len - 1); (i >= 0); (i--)) {
        btrc_Vector_VmOperation_push(result, self->data[i]);
    }
    return result;
}

static void btrc_Vector_VmOperation_swap(btrc_Vector_VmOperation* self, int i, int j) {
    if ((((i < 0) || (i >= self->len)) || (j < 0)) || (j >= self->len)) {
        fprintf(stderr, "Vector swap index out of bounds\n");
        exit(1);
    }
    VmOperation* tmp = self->data[i];
    (self->data[i] = self->data[j]);
    (self->data[j] = tmp);
}

static void btrc_Vector_VmOperation_clear(btrc_Vector_VmOperation* self) {
    for (int i = 0; (i < self->len); (i++)) {
        if (self->data[i]) {
            if ((--self->data[i]->__rc) <= 0) {
                VmOperation_destroy(self->data[i]);
            }
        }
    }
    (self->len = 0);
}

static void btrc_Vector_VmOperation_fill(btrc_Vector_VmOperation* self, VmOperation* val) {
    for (int i = 0; (i < self->len); (i++)) {
        if (self->data[i]) {
            if ((--self->data[i]->__rc) <= 0) {
                VmOperation_destroy(self->data[i]);
            }
        }
        (val->__rc++);
        (self->data[i] = val);
    }
}

static int btrc_Vector_VmOperation_size(btrc_Vector_VmOperation* self) {
    return self->len;
}

static bool btrc_Vector_VmOperation_isEmpty(btrc_Vector_VmOperation* self) {
    return (self->len == 0);
}

static VmOperation* btrc_Vector_VmOperation_first(btrc_Vector_VmOperation* self) {
    if (self->len == 0) {
        fprintf(stderr, "Vector.first() called on empty list\n");
        exit(1);
    }
    return self->data[0];
}

static VmOperation* btrc_Vector_VmOperation_last(btrc_Vector_VmOperation* self) {
    if (self->len == 0) {
        fprintf(stderr, "Vector.last() called on empty list\n");
        exit(1);
    }
    return self->data[(self->len - 1)];
}

static btrc_Vector_VmOperation* btrc_Vector_VmOperation_slice(btrc_Vector_VmOperation* self, int start, int end) {
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
    btrc_Vector_VmOperation* result = btrc_Vector_VmOperation_new();
    for (int i = start; (i < end); (i++)) {
        btrc_Vector_VmOperation_push(result, self->data[i]);
    }
    return result;
}

static btrc_Vector_VmOperation* btrc_Vector_VmOperation_take(btrc_Vector_VmOperation* self, int n) {
    if (n > self->len) {
        (n = self->len);
    }
    if (n < 0) {
        (n = 0);
    }
    return btrc_Vector_VmOperation_slice(self, 0, n);
}

static btrc_Vector_VmOperation* btrc_Vector_VmOperation_drop(btrc_Vector_VmOperation* self, int n) {
    if (n > self->len) {
        (n = self->len);
    }
    if (n < 0) {
        (n = 0);
    }
    return btrc_Vector_VmOperation_slice(self, n, self->len);
}

static void btrc_Vector_VmOperation_extend(btrc_Vector_VmOperation* self, btrc_Vector_VmOperation* other) {
    for (int i = 0; (i < other->len); (i++)) {
        btrc_Vector_VmOperation_push(self, other->data[i]);
    }
}

static void btrc_Vector_VmOperation_insert(btrc_Vector_VmOperation* self, int idx, VmOperation* val) {
    if ((idx < 0) || (idx > self->len)) {
        fprintf(stderr, "Vector insert index out of bounds: %d (size %d)\n", idx, self->len);
        exit(1);
    }
    (val->__rc++);
    if (self->len >= self->cap) {
        (self->cap = ((self->cap == 0) ? 4 : (self->cap * 2)));
        (self->data = ((VmOperation**)__btrc_safe_realloc(self->data, (sizeof(VmOperation*) * self->cap))));
    }
    for (int i = self->len; (i > idx); (i--)) {
        (self->data[i] = self->data[(i - 1)]);
    }
    (self->data[idx] = val);
    (self->len++);
}

static bool btrc_Vector_VmOperation_contains(btrc_Vector_VmOperation* self, VmOperation* val) {
    for (int i = 0; (i < self->len); (i++)) {
        if (__btrc_eq(self->data[i], val)) {
            return true;
        }
    }
    return false;
}

static int btrc_Vector_VmOperation_indexOf(btrc_Vector_VmOperation* self, VmOperation* val) {
    for (int i = 0; (i < self->len); (i++)) {
        if (__btrc_eq(self->data[i], val)) {
            return i;
        }
    }
    return (-1);
}

static int btrc_Vector_VmOperation_lastIndexOf(btrc_Vector_VmOperation* self, VmOperation* val) {
    for (int i = (self->len - 1); (i >= 0); (i--)) {
        if (__btrc_eq(self->data[i], val)) {
            return i;
        }
    }
    return (-1);
}

static int btrc_Vector_VmOperation_count(btrc_Vector_VmOperation* self, VmOperation* val) {
    int c = 0;
    for (int i = 0; (i < self->len); (i++)) {
        if (__btrc_eq(self->data[i], val)) {
            (c++);
        }
    }
    return c;
}

static void btrc_Vector_VmOperation_removeAll(btrc_Vector_VmOperation* self, VmOperation* val) {
    int j = 0;
    for (int i = 0; (i < self->len); (i++)) {
        if (!__btrc_eq(self->data[i], val)) {
            (self->data[j] = self->data[i]);
            (j++);
        } else if (self->data[i]) {
            if ((--self->data[i]->__rc) <= 0) {
                VmOperation_destroy(self->data[i]);
            }
        }
    }
    (self->len = j);
}

static btrc_Vector_VmOperation* btrc_Vector_VmOperation_distinct(btrc_Vector_VmOperation* self) {
    btrc_Vector_VmOperation* result = btrc_Vector_VmOperation_new();
    for (int i = 0; (i < self->len); (i++)) {
        if (!btrc_Vector_VmOperation_contains(result, self->data[i])) {
            btrc_Vector_VmOperation_push(result, self->data[i]);
        }
    }
    return result;
}

static void btrc_Vector_VmOperation_sort(btrc_Vector_VmOperation* self) {
    for (int i = 1; (i < self->len); (i++)) {
        VmOperation* key = self->data[i];
        int j = (i - 1);
        while ((j >= 0) && __btrc_lt(key, self->data[j])) {
            (self->data[(j + 1)] = self->data[j]);
            (j = (j - 1));
        }
        (self->data[(j + 1)] = key);
    }
}

static btrc_Vector_VmOperation* btrc_Vector_VmOperation_sorted(btrc_Vector_VmOperation* self) {
    btrc_Vector_VmOperation* result = btrc_Vector_VmOperation_new();
    for (int i = 0; (i < self->len); (i++)) {
        btrc_Vector_VmOperation_push(result, self->data[i]);
    }
    btrc_Vector_VmOperation_sort(result);
    return result;
}

static VmOperation* btrc_Vector_VmOperation_min(btrc_Vector_VmOperation* self) {
    if (self->len <= 0) {
        fprintf(stderr, "Vector min on empty list\n");
        exit(1);
    }
    VmOperation* m = self->data[0];
    for (int i = 1; (i < self->len); (i++)) {
        if (__btrc_lt(self->data[i], m)) {
            (m = self->data[i]);
        }
    }
    return m;
}

static VmOperation* btrc_Vector_VmOperation_max(btrc_Vector_VmOperation* self) {
    if (self->len <= 0) {
        fprintf(stderr, "Vector max on empty list\n");
        exit(1);
    }
    VmOperation* m = self->data[0];
    for (int i = 1; (i < self->len); (i++)) {
        if (__btrc_gt(self->data[i], m)) {
            (m = self->data[i]);
        }
    }
    return m;
}

static btrc_Vector_VmOperation* btrc_Vector_VmOperation_filter(btrc_Vector_VmOperation* self, __btrc_fn_bool_VmOperation pred) {
    btrc_Vector_VmOperation* result = btrc_Vector_VmOperation_new();
    for (int i = 0; (i < self->len); (i++)) {
        if (pred(self->data[i])) {
            btrc_Vector_VmOperation_push(result, self->data[i]);
        }
    }
    return result;
}

static int btrc_Vector_VmOperation_findIndex(btrc_Vector_VmOperation* self, __btrc_fn_bool_VmOperation pred) {
    for (int i = 0; (i < self->len); (i++)) {
        if (pred(self->data[i])) {
            return i;
        }
    }
    return (-1);
}

static void btrc_Vector_VmOperation_forEach(btrc_Vector_VmOperation* self, __btrc_fn_void_VmOperation fn) {
    for (int i = 0; (i < self->len); (i++)) {
        fn(self->data[i]);
    }
}

static btrc_Vector_VmOperation* btrc_Vector_VmOperation_map(btrc_Vector_VmOperation* self, __btrc_fn_VmOperation_VmOperation fn) {
    btrc_Vector_VmOperation* result = btrc_Vector_VmOperation_new();
    for (int i = 0; (i < self->len); (i++)) {
        btrc_Vector_VmOperation_push(result, fn(self->data[i]));
    }
    return result;
}

static bool btrc_Vector_VmOperation_any(btrc_Vector_VmOperation* self, __btrc_fn_bool_VmOperation pred) {
    for (int i = 0; (i < self->len); (i++)) {
        if (pred(self->data[i])) {
            return true;
        }
    }
    return false;
}

static bool btrc_Vector_VmOperation_all(btrc_Vector_VmOperation* self, __btrc_fn_bool_VmOperation pred) {
    for (int i = 0; (i < self->len); (i++)) {
        if (!pred(self->data[i])) {
            return false;
        }
    }
    return true;
}

static VmOperation* btrc_Vector_VmOperation_reduce(btrc_Vector_VmOperation* self, VmOperation* init, __btrc_fn_VmOperation_VmOperation_VmOperation fn) {
    VmOperation* acc = init;
    for (int i = 0; (i < self->len); (i++)) {
        (acc = fn(acc, self->data[i]));
    }
    return acc;
}

static btrc_Vector_VmOperation* btrc_Vector_VmOperation_copy(btrc_Vector_VmOperation* self) {
    btrc_Vector_VmOperation* result = btrc_Vector_VmOperation_new();
    for (int i = 0; (i < self->len); (i++)) {
        btrc_Vector_VmOperation_push(result, self->data[i]);
    }
    return result;
}

static void btrc_Vector_VmOperation_removeAt(btrc_Vector_VmOperation* self, int idx) {
    btrc_Vector_VmOperation_remove(self, idx);
}

static int btrc_Vector_VmOperation_iterLen(btrc_Vector_VmOperation* self) {
    return self->len;
}

static VmOperation* btrc_Vector_VmOperation_iterGet(btrc_Vector_VmOperation* self, int i) {
    return self->data[i];
}

static void btrc_Vector_VmGraphNode_init(btrc_Vector_VmGraphNode* self) {
    self->__rc = 1;
    (self->data = NULL);
    (self->len = 0);
    (self->cap = 0);
}

static btrc_Vector_VmGraphNode* btrc_Vector_VmGraphNode_new(void) {
    btrc_Vector_VmGraphNode* self = ((btrc_Vector_VmGraphNode*)malloc(sizeof(btrc_Vector_VmGraphNode)));
    memset(self, 0, sizeof(btrc_Vector_VmGraphNode));
    btrc_Vector_VmGraphNode_init(self);
    return self;
}

static void btrc_Vector_VmGraphNode_destroy(btrc_Vector_VmGraphNode* self) {
    free(self);
}

static void btrc_Vector_VmGraphNode_push(btrc_Vector_VmGraphNode* self, VmGraphNode* val) {
    (val->__rc++);
    if (self->len >= self->cap) {
        (self->cap = ((self->cap == 0) ? 4 : (self->cap * 2)));
        (self->data = ((VmGraphNode**)__btrc_safe_realloc(self->data, (sizeof(VmGraphNode*) * self->cap))));
    }
    (self->data[self->len] = val);
    (self->len++);
}

static VmGraphNode* btrc_Vector_VmGraphNode_pop(btrc_Vector_VmGraphNode* self) {
    if (self->len <= 0) {
        fprintf(stderr, "Vector pop from empty list\n");
        exit(1);
    }
    (self->len--);
    return self->data[self->len];
}

static VmGraphNode* btrc_Vector_VmGraphNode_get(btrc_Vector_VmGraphNode* self, int i) {
    if ((i < 0) || (i >= self->len)) {
        fprintf(stderr, "Vector index out of bounds: %d (len=%d)\n", i, self->len);
        exit(1);
    }
    return self->data[i];
}

static void btrc_Vector_VmGraphNode_set(btrc_Vector_VmGraphNode* self, int i, VmGraphNode* val) {
    if ((i < 0) || (i >= self->len)) {
        fprintf(stderr, "Vector index out of bounds: %d (len=%d)\n", i, self->len);
        exit(1);
    }
    if (self->data[i]) {
        if ((--self->data[i]->__rc) <= 0) {
            VmGraphNode_destroy(self->data[i]);
        }
    }
    (val->__rc++);
    (self->data[i] = val);
}

static void btrc_Vector_VmGraphNode_free(btrc_Vector_VmGraphNode* self) {
    for (int i = 0; (i < self->len); (i++)) {
        if (self->data[i]) {
            if ((--self->data[i]->__rc) <= 0) {
                VmGraphNode_destroy(self->data[i]);
            }
        }
    }
    free(self->data);
    (self->data = NULL);
    (self->len = 0);
    (self->cap = 0);
}

static void btrc_Vector_VmGraphNode_remove(btrc_Vector_VmGraphNode* self, int idx) {
    if ((idx < 0) || (idx >= self->len)) {
        fprintf(stderr, "Vector remove index out of bounds: %d (len=%d)\n", idx, self->len);
        exit(1);
    }
    if (self->data[idx]) {
        if ((--self->data[idx]->__rc) <= 0) {
            VmGraphNode_destroy(self->data[idx]);
        }
    }
    for (int i = idx; (i < (self->len - 1)); (i++)) {
        (self->data[i] = self->data[(i + 1)]);
    }
    (self->len--);
}

static void btrc_Vector_VmGraphNode_reverse(btrc_Vector_VmGraphNode* self) {
    for (int i = 0; (i < (self->len / 2)); (i++)) {
        VmGraphNode* tmp = self->data[i];
        (self->data[i] = self->data[((self->len - 1) - i)]);
        (self->data[((self->len - 1) - i)] = tmp);
    }
}

static btrc_Vector_VmGraphNode* btrc_Vector_VmGraphNode_reversed(btrc_Vector_VmGraphNode* self) {
    btrc_Vector_VmGraphNode* result = btrc_Vector_VmGraphNode_new();
    for (int i = (self->len - 1); (i >= 0); (i--)) {
        btrc_Vector_VmGraphNode_push(result, self->data[i]);
    }
    return result;
}

static void btrc_Vector_VmGraphNode_swap(btrc_Vector_VmGraphNode* self, int i, int j) {
    if ((((i < 0) || (i >= self->len)) || (j < 0)) || (j >= self->len)) {
        fprintf(stderr, "Vector swap index out of bounds\n");
        exit(1);
    }
    VmGraphNode* tmp = self->data[i];
    (self->data[i] = self->data[j]);
    (self->data[j] = tmp);
}

static void btrc_Vector_VmGraphNode_clear(btrc_Vector_VmGraphNode* self) {
    for (int i = 0; (i < self->len); (i++)) {
        if (self->data[i]) {
            if ((--self->data[i]->__rc) <= 0) {
                VmGraphNode_destroy(self->data[i]);
            }
        }
    }
    (self->len = 0);
}

static void btrc_Vector_VmGraphNode_fill(btrc_Vector_VmGraphNode* self, VmGraphNode* val) {
    for (int i = 0; (i < self->len); (i++)) {
        if (self->data[i]) {
            if ((--self->data[i]->__rc) <= 0) {
                VmGraphNode_destroy(self->data[i]);
            }
        }
        (val->__rc++);
        (self->data[i] = val);
    }
}

static int btrc_Vector_VmGraphNode_size(btrc_Vector_VmGraphNode* self) {
    return self->len;
}

static bool btrc_Vector_VmGraphNode_isEmpty(btrc_Vector_VmGraphNode* self) {
    return (self->len == 0);
}

static VmGraphNode* btrc_Vector_VmGraphNode_first(btrc_Vector_VmGraphNode* self) {
    if (self->len == 0) {
        fprintf(stderr, "Vector.first() called on empty list\n");
        exit(1);
    }
    return self->data[0];
}

static VmGraphNode* btrc_Vector_VmGraphNode_last(btrc_Vector_VmGraphNode* self) {
    if (self->len == 0) {
        fprintf(stderr, "Vector.last() called on empty list\n");
        exit(1);
    }
    return self->data[(self->len - 1)];
}

static btrc_Vector_VmGraphNode* btrc_Vector_VmGraphNode_slice(btrc_Vector_VmGraphNode* self, int start, int end) {
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
    btrc_Vector_VmGraphNode* result = btrc_Vector_VmGraphNode_new();
    for (int i = start; (i < end); (i++)) {
        btrc_Vector_VmGraphNode_push(result, self->data[i]);
    }
    return result;
}

static btrc_Vector_VmGraphNode* btrc_Vector_VmGraphNode_take(btrc_Vector_VmGraphNode* self, int n) {
    if (n > self->len) {
        (n = self->len);
    }
    if (n < 0) {
        (n = 0);
    }
    return btrc_Vector_VmGraphNode_slice(self, 0, n);
}

static btrc_Vector_VmGraphNode* btrc_Vector_VmGraphNode_drop(btrc_Vector_VmGraphNode* self, int n) {
    if (n > self->len) {
        (n = self->len);
    }
    if (n < 0) {
        (n = 0);
    }
    return btrc_Vector_VmGraphNode_slice(self, n, self->len);
}

static void btrc_Vector_VmGraphNode_extend(btrc_Vector_VmGraphNode* self, btrc_Vector_VmGraphNode* other) {
    for (int i = 0; (i < other->len); (i++)) {
        btrc_Vector_VmGraphNode_push(self, other->data[i]);
    }
}

static void btrc_Vector_VmGraphNode_insert(btrc_Vector_VmGraphNode* self, int idx, VmGraphNode* val) {
    if ((idx < 0) || (idx > self->len)) {
        fprintf(stderr, "Vector insert index out of bounds: %d (size %d)\n", idx, self->len);
        exit(1);
    }
    (val->__rc++);
    if (self->len >= self->cap) {
        (self->cap = ((self->cap == 0) ? 4 : (self->cap * 2)));
        (self->data = ((VmGraphNode**)__btrc_safe_realloc(self->data, (sizeof(VmGraphNode*) * self->cap))));
    }
    for (int i = self->len; (i > idx); (i--)) {
        (self->data[i] = self->data[(i - 1)]);
    }
    (self->data[idx] = val);
    (self->len++);
}

static bool btrc_Vector_VmGraphNode_contains(btrc_Vector_VmGraphNode* self, VmGraphNode* val) {
    for (int i = 0; (i < self->len); (i++)) {
        if (__btrc_eq(self->data[i], val)) {
            return true;
        }
    }
    return false;
}

static int btrc_Vector_VmGraphNode_indexOf(btrc_Vector_VmGraphNode* self, VmGraphNode* val) {
    for (int i = 0; (i < self->len); (i++)) {
        if (__btrc_eq(self->data[i], val)) {
            return i;
        }
    }
    return (-1);
}

static int btrc_Vector_VmGraphNode_lastIndexOf(btrc_Vector_VmGraphNode* self, VmGraphNode* val) {
    for (int i = (self->len - 1); (i >= 0); (i--)) {
        if (__btrc_eq(self->data[i], val)) {
            return i;
        }
    }
    return (-1);
}

static int btrc_Vector_VmGraphNode_count(btrc_Vector_VmGraphNode* self, VmGraphNode* val) {
    int c = 0;
    for (int i = 0; (i < self->len); (i++)) {
        if (__btrc_eq(self->data[i], val)) {
            (c++);
        }
    }
    return c;
}

static void btrc_Vector_VmGraphNode_removeAll(btrc_Vector_VmGraphNode* self, VmGraphNode* val) {
    int j = 0;
    for (int i = 0; (i < self->len); (i++)) {
        if (!__btrc_eq(self->data[i], val)) {
            (self->data[j] = self->data[i]);
            (j++);
        } else if (self->data[i]) {
            if ((--self->data[i]->__rc) <= 0) {
                VmGraphNode_destroy(self->data[i]);
            }
        }
    }
    (self->len = j);
}

static btrc_Vector_VmGraphNode* btrc_Vector_VmGraphNode_distinct(btrc_Vector_VmGraphNode* self) {
    btrc_Vector_VmGraphNode* result = btrc_Vector_VmGraphNode_new();
    for (int i = 0; (i < self->len); (i++)) {
        if (!btrc_Vector_VmGraphNode_contains(result, self->data[i])) {
            btrc_Vector_VmGraphNode_push(result, self->data[i]);
        }
    }
    return result;
}

static void btrc_Vector_VmGraphNode_sort(btrc_Vector_VmGraphNode* self) {
    for (int i = 1; (i < self->len); (i++)) {
        VmGraphNode* key = self->data[i];
        int j = (i - 1);
        while ((j >= 0) && __btrc_lt(key, self->data[j])) {
            (self->data[(j + 1)] = self->data[j]);
            (j = (j - 1));
        }
        (self->data[(j + 1)] = key);
    }
}

static btrc_Vector_VmGraphNode* btrc_Vector_VmGraphNode_sorted(btrc_Vector_VmGraphNode* self) {
    btrc_Vector_VmGraphNode* result = btrc_Vector_VmGraphNode_new();
    for (int i = 0; (i < self->len); (i++)) {
        btrc_Vector_VmGraphNode_push(result, self->data[i]);
    }
    btrc_Vector_VmGraphNode_sort(result);
    return result;
}

static VmGraphNode* btrc_Vector_VmGraphNode_min(btrc_Vector_VmGraphNode* self) {
    if (self->len <= 0) {
        fprintf(stderr, "Vector min on empty list\n");
        exit(1);
    }
    VmGraphNode* m = self->data[0];
    for (int i = 1; (i < self->len); (i++)) {
        if (__btrc_lt(self->data[i], m)) {
            (m = self->data[i]);
        }
    }
    return m;
}

static VmGraphNode* btrc_Vector_VmGraphNode_max(btrc_Vector_VmGraphNode* self) {
    if (self->len <= 0) {
        fprintf(stderr, "Vector max on empty list\n");
        exit(1);
    }
    VmGraphNode* m = self->data[0];
    for (int i = 1; (i < self->len); (i++)) {
        if (__btrc_gt(self->data[i], m)) {
            (m = self->data[i]);
        }
    }
    return m;
}

static btrc_Vector_VmGraphNode* btrc_Vector_VmGraphNode_filter(btrc_Vector_VmGraphNode* self, __btrc_fn_bool_VmGraphNode pred) {
    btrc_Vector_VmGraphNode* result = btrc_Vector_VmGraphNode_new();
    for (int i = 0; (i < self->len); (i++)) {
        if (pred(self->data[i])) {
            btrc_Vector_VmGraphNode_push(result, self->data[i]);
        }
    }
    return result;
}

static int btrc_Vector_VmGraphNode_findIndex(btrc_Vector_VmGraphNode* self, __btrc_fn_bool_VmGraphNode pred) {
    for (int i = 0; (i < self->len); (i++)) {
        if (pred(self->data[i])) {
            return i;
        }
    }
    return (-1);
}

static void btrc_Vector_VmGraphNode_forEach(btrc_Vector_VmGraphNode* self, __btrc_fn_void_VmGraphNode fn) {
    for (int i = 0; (i < self->len); (i++)) {
        fn(self->data[i]);
    }
}

static btrc_Vector_VmGraphNode* btrc_Vector_VmGraphNode_map(btrc_Vector_VmGraphNode* self, __btrc_fn_VmGraphNode_VmGraphNode fn) {
    btrc_Vector_VmGraphNode* result = btrc_Vector_VmGraphNode_new();
    for (int i = 0; (i < self->len); (i++)) {
        btrc_Vector_VmGraphNode_push(result, fn(self->data[i]));
    }
    return result;
}

static bool btrc_Vector_VmGraphNode_any(btrc_Vector_VmGraphNode* self, __btrc_fn_bool_VmGraphNode pred) {
    for (int i = 0; (i < self->len); (i++)) {
        if (pred(self->data[i])) {
            return true;
        }
    }
    return false;
}

static bool btrc_Vector_VmGraphNode_all(btrc_Vector_VmGraphNode* self, __btrc_fn_bool_VmGraphNode pred) {
    for (int i = 0; (i < self->len); (i++)) {
        if (!pred(self->data[i])) {
            return false;
        }
    }
    return true;
}

static VmGraphNode* btrc_Vector_VmGraphNode_reduce(btrc_Vector_VmGraphNode* self, VmGraphNode* init, __btrc_fn_VmGraphNode_VmGraphNode_VmGraphNode fn) {
    VmGraphNode* acc = init;
    for (int i = 0; (i < self->len); (i++)) {
        (acc = fn(acc, self->data[i]));
    }
    return acc;
}

static btrc_Vector_VmGraphNode* btrc_Vector_VmGraphNode_copy(btrc_Vector_VmGraphNode* self) {
    btrc_Vector_VmGraphNode* result = btrc_Vector_VmGraphNode_new();
    for (int i = 0; (i < self->len); (i++)) {
        btrc_Vector_VmGraphNode_push(result, self->data[i]);
    }
    return result;
}

static void btrc_Vector_VmGraphNode_removeAt(btrc_Vector_VmGraphNode* self, int idx) {
    btrc_Vector_VmGraphNode_remove(self, idx);
}

static int btrc_Vector_VmGraphNode_iterLen(btrc_Vector_VmGraphNode* self) {
    return self->len;
}

static VmGraphNode* btrc_Vector_VmGraphNode_iterGet(btrc_Vector_VmGraphNode* self, int i) {
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

btrc_Vector_string* Strings_split(char* s, char* delim) {
    btrc_Vector_string* result = btrc_Vector_string_new();
    if ((s == NULL) || (delim == NULL)) {
        return result;
    }
    int dlen = ((int)strlen(delim));
    if (dlen == 0) {
        return result;
    }
    char* p = s;
    while (*p) {
        char* found = strstr(p, delim);
        int seglen = ((found != NULL) ? ((int)(found - p)) : ((int)strlen(p)));
        char* item = ((char*)malloc((seglen + 1)));
        memcpy(item, p, seglen);
        (item[seglen] = '\0');
        btrc_Vector_string_push(result, item);
        if (found == NULL) {
            break;
        }
        (p = (found + dlen));
    }
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

int Strings_count(char* s, char* sub) {
    int slen = ((int)strlen(s));
    int sublen = ((int)strlen(sub));
    if (sublen == 0) {
        __auto_type __btrc_ret_15 = 0;
        return __btrc_ret_15;
    }
    int n = 0;
    int i = 0;
    while ((i + sublen) <= slen) {
        if (strncmp((s + i), sub, sublen) == 0) {
            (n++);
            (i = (i + sublen));
        } else {
            (i++);
        }
    }
    return n;
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

char* Strings_removePrefix(char* s, char* prefix) {
    if (!__btrc_startsWith(s, prefix)) {
        __auto_type __btrc_ret_30 = Strings_copy(s);
        return __btrc_ret_30;
    }
    __auto_type __btrc_ret_31 = __btrc_str_track(__btrc_substring(s, ((int)strlen(prefix)), (((int)strlen(s)) - ((int)strlen(prefix)))));
    return __btrc_ret_31;
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

void Console_log(char* msg) {
    printf("%s\n", msg);
}

void Console_error(char* msg) {
    fprintf(stderr, "%s\n", msg);
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

bool Platform_isRoot(void) {
    __auto_type __btrc_ret_57 = (Platform_euid() == 0);
    return __btrc_ret_57;
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

bool ExecResult_ok(ExecResult* self) {
    __auto_type __btrc_ret_81 = (self->code == 0);
    return __btrc_ret_81;
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

Command* Command_flag(Command* self, char* name, char* value) {
    btrc_Vector_string_push(self->args, name);
    btrc_Vector_string_push(self->args, value);
    __auto_type __btrc_ret_89 = self;
    return __btrc_ret_89;
}

Command* Command_envVar(Command* self, char* name, char* value) {
    int __fstr_91_len = snprintf(NULL, 0, "%s=%s", name, value);
    char* __fstr_91_buf = __btrc_str_track(((char*)malloc((__fstr_91_len + 1))));
    snprintf(__fstr_91_buf, (__fstr_91_len + 1), "%s=%s", name, value);
    btrc_Vector_string_push(self->env, __fstr_91_buf);
    __auto_type __btrc_ret_92 = self;
    return __btrc_ret_92;
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

Command* Command_redact(Command* self, char* value) {
    (self->sensitive = value);
    __auto_type __btrc_ret_97 = self;
    return __btrc_ret_97;
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

void UnixShell_chroot(UnixShell* self, char* path) {
    (self->chrootPath = path);
}

void UnixShell_clearChroot(UnixShell* self) {
    (self->chrootPath = "");
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

bool UnixPamPassword_change(char* user, char* oldPassword, char* newPassword) {
    struct passwd* pw = getpwnam(user);
    if (pw == NULL) {
        __auto_type __btrc_ret_115 = false;
        return __btrc_ret_115;
    }
    int fd = (-1);
    pid_t pid = forkpty((&fd), NULL, NULL, NULL);
    if (pid < 0) {
        __auto_type __btrc_ret_116 = false;
        return __btrc_ret_116;
    }
    if (pid == 0) {
        setgid(pw->pw_gid);
        setuid(pw->pw_uid);
        setenv("HOME", pw->pw_dir, 1);
        setenv("USER", user, 1);
        setenv("LOGNAME", user, 1);
        setenv("PATH", "/run/wrappers/bin:/run/current-system/sw/bin:/usr/bin:/bin", 1);
        execlp("passwd", "passwd", ((char*)NULL));
        _exit(127);
    }
    char* responses[3];
    (responses[0] = oldPassword);
    (responses[1] = newPassword);
    (responses[2] = newPassword);
    int response = 0;
    time_t deadline = (time(NULL) + 30);
    char buffer[4096];
    while ((time(NULL) < deadline) && (response < 3)) {
        fd_set readfds;
        FD_ZERO((&readfds));
        FD_SET(fd, (&readfds));
        struct timeval timeout;
        (timeout.tv_sec = 5);
        (timeout.tv_usec = 0);
        int ready = select((fd + 1), (&readfds), NULL, NULL, (&timeout));
        if (ready < 0) {
            break;
        }
        if (ready == 0) {
            continue;
        }
        ssize_t n = read(fd, buffer, sizeof(buffer));
        if (n <= 0) {
            break;
        }
        bool prompt = false;
        for (ssize_t i = 0; (i < n); (i++)) {
            if (buffer[i] == ':') {
                (prompt = true);
            }
        }
        if (prompt) {
            write(fd, responses[response], strlen(responses[response]));
            write(fd, "\n", 1);
            (response++);
            usleep(200000);
        }
    }
    close(fd);
    int status = 0;
    waitpid(pid, (&status), 0);
    __auto_type __btrc_ret_117 = (WIFEXITED(status) && (WEXITSTATUS(status) == 0));
    return __btrc_ret_117;
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

char* PathTools_basename(char* path) {
    int len = ((int)strlen(path));
    if (len == 0) {
        __auto_type __btrc_ret_137 = "";
        return __btrc_ret_137;
    }
    int end = (len - 1);
    while ((end > 0) && (path[end] == '/')) {
        (end--);
    }
    int start = end;
    while ((start > 0) && (path[(start - 1)] != '/')) {
        (start--);
    }
    int outLen = ((end - start) + 1);
    char* result = ((char*)malloc((outLen + 1)));
    memcpy(result, (path + start), outLen);
    (result[outLen] = '\0');
    return result;
}

char* PathTools_dirname(char* path) {
    int len = ((int)strlen(path));
    if (len == 0) {
        __auto_type __btrc_ret_138 = ".";
        return __btrc_ret_138;
    }
    int end = (len - 1);
    while ((end > 0) && (path[end] == '/')) {
        (end--);
    }
    while ((end > 0) && (path[end] != '/')) {
        (end--);
    }
    if (end == 0) {
        if (path[0] == '/') {
            __auto_type __btrc_ret_139 = "/";
            return __btrc_ret_139;
        }
        __auto_type __btrc_ret_140 = ".";
        return __btrc_ret_140;
    }
    char* result = ((char*)malloc((end + 1)));
    memcpy(result, path, end);
    (result[end] = '\0');
    return result;
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

bool FileSystem_exists(char* path) {
    FileStatus* status = FileStatus_new(path);
    bool result = FileStatus_exists(status);
    if (status != NULL) {
        if ((--status->__rc) <= 0) {
            FileStatus_destroy(status);
        }
    }
    return result;
    if (status != NULL) {
        if ((--status->__rc) <= 0) {
            FileStatus_destroy(status);
        }
    }
}

bool FileSystem_isSymlink(char* path) {
    FileStatus* status = FileStatus_new(path);
    bool result = FileStatus_isSymlink(status);
    if (status != NULL) {
        if ((--status->__rc) <= 0) {
            FileStatus_destroy(status);
        }
    }
    return result;
    if (status != NULL) {
        if ((--status->__rc) <= 0) {
            FileStatus_destroy(status);
        }
    }
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

Ui* Ui_new(void) {
    Ui* self = ((Ui*)malloc(sizeof(Ui)));
    memset(self, 0, sizeof(Ui));
    Ui_init(self);
    return self;
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

void JsonObject_setBool(JsonObject* self, char* key, bool value) {
    btrc_Map_string_string_put(self->values, key, (value ? "true" : "false"));
    btrc_Map_string_bool_put(self->quoted, key, false);
}

void JsonObject_setInt(JsonObject* self, char* key, int value) {
    btrc_Map_string_string_put(self->values, key, Strings_fromInt(value));
    btrc_Map_string_bool_put(self->quoted, key, false);
}

char* JsonObject_getString(JsonObject* self, char* key, char* fallback) {
    if (!btrc_Map_string_string_has(self->values, key)) {
        return fallback;
    }
    __auto_type __btrc_ret_254 = btrc_Map_string_string_get(self->values, key);
    return __btrc_ret_254;
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

JsonObject* JsonObject_readFile(char* path) {
    __auto_type __btrc_ret_261 = JsonObject_parse(Path_readAll(path));
    return __btrc_ret_261;
}

void JsonObject_writeFile(JsonObject* self, char* path) {
    Path_writeAll(path, JsonObject_stringify(self));
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

char* Toml_key(char* line) {
    char* cleaned = Toml_stripInlineComment(line);
    int pos = Strings_find(cleaned, "=", 0);
    if (pos < 0) {
        __auto_type __btrc_ret_266 = "";
        return __btrc_ret_266;
    }
    __auto_type __btrc_ret_267 = Toml_unquote(__btrc_str_track(__btrc_trim(__btrc_str_track(__btrc_substring(cleaned, 0, pos)))));
    return __btrc_ret_267;
}

char* Toml_value(char* line) {
    char* cleaned = Toml_stripInlineComment(line);
    int pos = Strings_find(cleaned, "=", 0);
    if (pos < 0) {
        __auto_type __btrc_ret_268 = "";
        return __btrc_ret_268;
    }
    __auto_type __btrc_ret_269 = Toml_unquote(__btrc_str_track(__btrc_trim(__btrc_str_track(__btrc_substring(cleaned, (pos + 1), ((((int)strlen(cleaned)) - pos) - 1))))));
    return __btrc_ret_269;
}

char* Toml_sectionName(char* line) {
    char* cleaned = Toml_stripInlineComment(line);
    if ((!__btrc_startsWith(cleaned, "[")) || (!__btrc_endsWith(cleaned, "]"))) {
        __auto_type __btrc_ret_270 = "";
        return __btrc_ret_270;
    }
    if (__btrc_startsWith(cleaned, "[[")) {
        __auto_type __btrc_ret_271 = "";
        return __btrc_ret_271;
    }
    __auto_type __btrc_ret_272 = __btrc_str_track(__btrc_trim(__btrc_str_track(__btrc_substring(cleaned, 1, (((int)strlen(cleaned)) - 2)))));
    return __btrc_ret_272;
}

char* Toml_tableArrayName(char* line) {
    char* cleaned = Toml_stripInlineComment(line);
    if ((!__btrc_startsWith(cleaned, "[[")) || (!__btrc_endsWith(cleaned, "]]"))) {
        __auto_type __btrc_ret_273 = "";
        return __btrc_ret_273;
    }
    __auto_type __btrc_ret_274 = __btrc_str_track(__btrc_trim(__btrc_str_track(__btrc_substring(cleaned, 2, (((int)strlen(cleaned)) - 4)))));
    return __btrc_ret_274;
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

CliArgs* CliArgs_new(int argc, char** argv) {
    CliArgs* self = ((CliArgs*)malloc(sizeof(CliArgs)));
    memset(self, 0, sizeof(CliArgs));
    CliArgs_init(self, argc, argv);
    return self;
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

int CliArgs_count(CliArgs* self) {
    __auto_type __btrc_ret_336 = self->values->len;
    return __btrc_ret_336;
}

char* CliArgs_get(CliArgs* self, int index) {
    __auto_type __btrc_ret_337 = btrc_Vector_string_get(self->values, index);
    return __btrc_ret_337;
}

char* CliArgs_command(CliArgs* self) {
    if (self->values->len == 0) {
        __auto_type __btrc_ret_338 = "";
        return __btrc_ret_338;
    }
    __auto_type __btrc_ret_339 = btrc_Vector_string_get(self->values, 0);
    return __btrc_ret_339;
}

bool CliArgs_has(CliArgs* self, char* flag) {
    int __n_341 = btrc_Vector_string_iterLen(self->values);
    for (int __i_340 = 0; (__i_340 < __n_341); (__i_340++)) {
        char* value = btrc_Vector_string_iterGet(self->values, __i_340);
        if (strcmp(value, flag) == 0) {
            __auto_type __btrc_ret_342 = true;
            return __btrc_ret_342;
        }
    }
    __auto_type __btrc_ret_343 = false;
    return __btrc_ret_343;
}

char* CliArgs_valueAfter(CliArgs* self, char* flag, char* fallback) {
    for (int i = 0; (i < (self->values->len - 1)); (i++)) {
        if (strcmp(btrc_Vector_string_get(self->values, i), flag) == 0) {
            __auto_type __btrc_ret_344 = btrc_Vector_string_get(self->values, (i + 1));
            return __btrc_ret_344;
        }
    }
    return fallback;
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

void NixosLog_init(NixosLog* self) {
    self->__rc = 1;
}

void NixosLog_destroy(NixosLog* self) {
    free(self);
}

char* NixosLog_gray(void) {
    __auto_type __btrc_ret_356 = "\033[90m";
    return __btrc_ret_356;
}

char* NixosLog_orange(void) {
    __auto_type __btrc_ret_357 = "\033[38;5;208m";
    return __btrc_ret_357;
}

char* NixosLog_red(void) {
    __auto_type __btrc_ret_358 = "\033[31m";
    return __btrc_ret_358;
}

char* NixosLog_reset(void) {
    __auto_type __btrc_ret_359 = "\033[0m";
    return __btrc_ret_359;
}

void NixosLog_info(char* message) {
    Console_error(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(NixosLog_gray(), "LOG: ")), message)), NixosLog_reset())));
}

void NixosLog_error(char* message) {
    Console_error(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(NixosLog_orange(), "ERROR: ")), message)), NixosLog_reset())));
}

void NixosLog_fatal(char* message) {
    Console_error(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(NixosLog_red(), message)), NixosLog_reset())));
    exit(1);
}

void NixosPaths_init(NixosPaths* self, char* root) {
    self->__rc = 1;
    (self->root = root);
}

NixosPaths* NixosPaths_new(char* root) {
    NixosPaths* self = ((NixosPaths*)malloc(sizeof(NixosPaths)));
    memset(self, 0, sizeof(NixosPaths));
    NixosPaths_init(self, root);
    return self;
}

void NixosPaths_destroy(NixosPaths* self) {
    free(self);
}

char* NixosPaths_configPath(NixosPaths* self) {
    __auto_type __btrc_ret_361 = PathTools_join(self->root, "config.json");
    return __btrc_ret_361;
}

char* NixosPaths_hostsPath(NixosPaths* self) {
    __auto_type __btrc_ret_364 = PathTools_join(self->root, "modules/hosts");
    return __btrc_ret_364;
}

char* NixosPaths_secretsPathFallback(NixosPaths* self) {
    __auto_type __btrc_ret_365 = PathTools_join(self->root, "secrets");
    return __btrc_ret_365;
}

void LocalConfigFile_init(LocalConfigFile* self, char* path) {
    self->__rc = 1;
    (self->path = path);
}

LocalConfigFile* LocalConfigFile_new(char* path) {
    LocalConfigFile* self = ((LocalConfigFile*)malloc(sizeof(LocalConfigFile)));
    memset(self, 0, sizeof(LocalConfigFile));
    LocalConfigFile_init(self, path);
    return self;
}

void LocalConfigFile_destroy(LocalConfigFile* self) {
    free(self);
}

bool LocalConfigFile_exists(LocalConfigFile* self) {
    __auto_type __btrc_ret_366 = FileSystem_exists(self->path);
    return __btrc_ret_366;
}

JsonObject* LocalConfigFile_read(LocalConfigFile* self) {
    if (!LocalConfigFile_exists(self)) {
        __auto_type __btrc_ret_367 = JsonObject_new();
        return __btrc_ret_367;
    }
    __auto_type __btrc_ret_368 = JsonObject_readFile(self->path);
    return __btrc_ret_368;
}

void LocalConfigFile_overwrite(LocalConfigFile* self, JsonObject* data) {
    char* dir = PathTools_dirname(self->path);
    FileSystem_mkdirp(dir);
    JsonObject_writeFile(data, self->path);
}

char* LocalConfigFile_getString(LocalConfigFile* self, char* key, char* fallback) {
    JsonObject* data = LocalConfigFile_read(self);
    char* value = JsonObject_getString(data, key, fallback);
    __auto_type __btrc_ret_369 = Strings_copy(value);
    return __btrc_ret_369;
}

void LocalConfigFile_setString(LocalConfigFile* self, char* key, char* value) {
    JsonObject* data = LocalConfigFile_read(self);
    JsonObject_setString(data, key, value);
    LocalConfigFile_overwrite(self, data);
}

void Interactive_init(Interactive* self) {
    self->__rc = 1;
    if (self->shell != NULL) {
        if ((--self->shell->__rc) <= 0) {
            UnixShell_destroy(self->shell);
        }
    }
    (self->shell = UnixShell_new());
    (UnixShell_new()->__rc++);
}

Interactive* Interactive_new(void) {
    Interactive* self = ((Interactive*)malloc(sizeof(Interactive)));
    memset(self, 0, sizeof(Interactive));
    Interactive_init(self);
    return self;
}

void Interactive_destroy(Interactive* self) {
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

char* Interactive_ask(Interactive* self, char* prompt) {
    char* command = __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("printf ", UnixShell_quote(__btrc_str_track(__btrc_strcat(prompt, " "))))), " >&2; IFS= read -r value; printf '%s' \"$value\""));
    ExecResult* result = UnixShell_runRaw(self->shell, command, true, false, "");
    __auto_type __btrc_ret_370 = ExecResult_stdout(result);
    return __btrc_ret_370;
}

bool Interactive_confirm(Interactive* self, char* prompt) {
    while (true) {
        char* response = __btrc_str_track(__btrc_trim(__btrc_str_track(__btrc_toLower(Interactive_ask(self, __btrc_str_track(__btrc_strcat(prompt, " (y/n):")))))));
        if ((strcmp(response, "y") == 0) || (strcmp(response, "yes") == 0)) {
            __auto_type __btrc_ret_371 = true;
            return __btrc_ret_371;
        }
        if ((strcmp(response, "n") == 0) || (strcmp(response, "no") == 0)) {
            __auto_type __btrc_ret_372 = false;
            return __btrc_ret_372;
        }
        Console_error("Invalid input. Enter 'y' or 'n'.");
    }
    __auto_type __btrc_ret_373 = false;
    return __btrc_ret_373;
}

char* Interactive_askPassword(Interactive* self, char* prompt) {
    char* command = __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("printf ", UnixShell_quote(__btrc_str_track(__btrc_strcat(prompt, ": "))))), " >&2; stty -echo; IFS= read -r value; stty echo; printf '\\n' >&2; printf '%s' \"$value\""));
    ExecResult* result = UnixShell_runRaw(self->shell, command, true, false, "");
    __auto_type __btrc_ret_374 = ExecResult_stdout(result);
    return __btrc_ret_374;
}

char* Interactive_askPasswordConfirmed(Interactive* self, char* prompt) {
    while (true) {
        char* first = Interactive_askPassword(self, prompt);
        char* second = Interactive_askPassword(self, "Confirm password");
        if (strcmp(first, second) == 0) {
            return first;
        }
        NixosLog_error("Passwords do not match.");
    }
    __auto_type __btrc_ret_375 = "";
    return __btrc_ret_375;
}

char* Interactive_askHostPath(Interactive* self, char* hostsPath) {
    ExecResult* found = UnixShell_runUnchecked(self->shell, __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("find ", UnixShell_quote(hostsPath))), " -type f -name '*.nix' | sort")));
    btrc_Vector_string* candidates = btrc_Vector_string_new();
    int __n_377 = btrc_Vector_string_iterLen(Strings_split(ExecResult_stdout(found), "\n"));
    for (int __i_376 = 0; (__i_376 < __n_377); (__i_376++)) {
        char* line = btrc_Vector_string_iterGet(Strings_split(ExecResult_stdout(found), "\n"), __i_376);
        char* path = __btrc_str_track(__btrc_trim(line));
        if (__btrc_isEmpty(path)) {
            continue;
        }
        char* hostName = Strings_replace(PathTools_basename(path), ".nix", "");
        char* hostDir = PathTools_basename(PathTools_dirname(path));
        if (strcmp(hostName, hostDir) == 0) {
            btrc_Vector_string_push(candidates, path);
        }
    }
    if (candidates->len == 0) {
        NixosLog_fatal(__btrc_str_track(__btrc_strcat("No host files found under ", hostsPath)));
    }
    while (true) {
        for (int i = 0; (i < candidates->len); (i++)) {
            char* path = btrc_Vector_string_get(candidates, i);
            char* hostName = Strings_replace(PathTools_basename(path), ".nix", "");
            char* arch = PathTools_basename(PathTools_dirname(PathTools_dirname(path)));
            Console_error(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(Strings_fromInt((i + 1)), ") ")), hostName)), " (")), arch)), ")")));
        }
        int selected = (Strings_toInt(__btrc_str_track(__btrc_trim(Interactive_ask(self, ">")))) - 1);
        if ((selected >= 0) && (selected < candidates->len)) {
            __auto_type __btrc_ret_378 = btrc_Vector_string_get(candidates, selected);
            return __btrc_ret_378;
        }
        NixosLog_error("Invalid choice.");
    }
    __auto_type __btrc_ret_379 = btrc_Vector_string_get(candidates, 0);
    return __btrc_ret_379;
}

void Interactive_askToReboot(Interactive* self) {
    if (Interactive_confirm(self, "Restart now?")) {
        Command* reboot = Command_capture(Command_arg(Command_arg(Command_new("shutdown"), "-r"), "now"), false);
        UnixShell_runCommand(self->shell, reboot);
    }
}

void NixEvalCache_init(NixEvalCache* self) {
    self->__rc = 1;
    if (self->values != NULL) {
        if ((--self->values->__rc) <= 0) {
            btrc_Map_string_string_free(self->values);
        }
    }
    (self->values = btrc_Map_string_string_new());
    (btrc_Map_string_string_new()->__rc++);
}

NixEvalCache* NixEvalCache_new(void) {
    NixEvalCache* self = ((NixEvalCache*)malloc(sizeof(NixEvalCache)));
    memset(self, 0, sizeof(NixEvalCache));
    NixEvalCache_init(self);
    return self;
}

void NixEvalCache_destroy(NixEvalCache* self) {
    if (self->values != NULL) {
        if ((--self->values->__rc) <= 0) {
            btrc_Map_string_string_free(self->values);
        }
    }
    if (__btrc_tracking) {
        __btrc_mark_destroyed(self);
    }
    free(self);
}

bool NixEvalCache_has(NixEvalCache* self, char* key) {
    __auto_type __btrc_ret_380 = btrc_Map_string_string_has(self->values, key);
    return __btrc_ret_380;
}

char* NixEvalCache_get(NixEvalCache* self, char* key) {
    __auto_type __btrc_ret_381 = btrc_Map_string_string_get(self->values, key);
    return __btrc_ret_381;
}

void NixEvalCache_put(NixEvalCache* self, char* key, char* value) {
    btrc_Map_string_string_put(self->values, key, value);
}

void NixosConfig_init(NixosConfig* self, char* root) {
    self->__rc = 1;
    if (self->paths != NULL) {
        if ((--self->paths->__rc) <= 0) {
            NixosPaths_destroy(self->paths);
        }
    }
    (self->paths = NixosPaths_new(root));
    (NixosPaths_new(root)->__rc++);
    if (self->local != NULL) {
        if ((--self->local->__rc) <= 0) {
            LocalConfigFile_destroy(self->local);
        }
    }
    (self->local = LocalConfigFile_new(NixosPaths_configPath(self->paths)));
    (LocalConfigFile_new(NixosPaths_configPath(self->paths))->__rc++);
    if (self->shell != NULL) {
        if ((--self->shell->__rc) <= 0) {
            UnixShell_destroy(self->shell);
        }
    }
    (self->shell = UnixShell_new());
    (UnixShell_new()->__rc++);
    if (self->cache != NULL) {
        if ((--self->cache->__rc) <= 0) {
            NixEvalCache_destroy(self->cache);
        }
    }
    (self->cache = NixEvalCache_new());
    (NixEvalCache_new()->__rc++);
}

NixosConfig* NixosConfig_new(char* root) {
    NixosConfig* self = ((NixosConfig*)malloc(sizeof(NixosConfig)));
    memset(self, 0, sizeof(NixosConfig));
    NixosConfig_init(self, root);
    return self;
}

void NixosConfig_destroy(NixosConfig* self) {
    if (self->paths != NULL) {
        if ((--self->paths->__rc) <= 0) {
            NixosPaths_destroy(self->paths);
        }
    }
    if (self->local != NULL) {
        if ((--self->local->__rc) <= 0) {
            LocalConfigFile_destroy(self->local);
        }
    }
    if (self->shell != NULL) {
        if ((--self->shell->__rc) <= 0) {
            UnixShell_destroy(self->shell);
        }
    }
    if (self->cache != NULL) {
        if ((--self->cache->__rc) <= 0) {
            NixEvalCache_destroy(self->cache);
        }
    }
    if (__btrc_tracking) {
        __btrc_mark_destroyed(self);
    }
    free(self);
}

bool NixosConfig_exists(NixosConfig* self) {
    __auto_type __btrc_ret_382 = LocalConfigFile_exists(self->local);
    return __btrc_ret_382;
}

char* NixosConfig_hostPath(NixosConfig* self) {
    __auto_type __btrc_ret_383 = LocalConfigFile_getString(self->local, "host_path", "");
    return __btrc_ret_383;
}

char* NixosConfig_target(NixosConfig* self) {
    __auto_type __btrc_ret_384 = LocalConfigFile_getString(self->local, "target", "Standard-Boot");
    return __btrc_ret_384;
}

void NixosConfig_reset(NixosConfig* self, char* hostPath, char* target) {
    JsonObject* data = JsonObject_new();
    JsonObject_setString(data, "host_path", hostPath);
    JsonObject_setString(data, "target", target);
    LocalConfigFile_overwrite(self->local, data);
    if (data != NULL) {
        if ((--data->__rc) <= 0) {
            JsonObject_destroy(data);
        }
    }
}

char* NixosConfig_host(NixosConfig* self) {
    char* base = PathTools_basename(NixosConfig_hostPath(self));
    __auto_type __btrc_ret_385 = Strings_replace(base, ".nix", "");
    return __btrc_ret_385;
}

char* NixosConfig_flakeRef(NixosConfig* self) {
    __auto_type __btrc_ret_387 = __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(self->paths->root, "#")), NixosConfig_host(self))), "-")), NixosConfig_target(self)));
    return __btrc_ret_387;
}

char* NixosConfig_inputLockedRev(NixosConfig* self, char* inputName) {
    Command* cmd = Command_new("nix");
    Command_arg(cmd, "--extra-experimental-features");
    Command_arg(cmd, "nix-command");
    Command_arg(cmd, "--extra-experimental-features");
    Command_arg(cmd, "flakes");
    Command_arg(cmd, "flake");
    Command_arg(cmd, "metadata");
    Command_arg(cmd, inputName);
    Command_arg(cmd, "--json");
    Command_arg(cmd, "-I");
    Command_arg(cmd, self->paths->root);
    ExecResult* result = UnixShell_runCommand(self->shell, cmd);
    if (!ExecResult_ok(result)) {
        __auto_type __btrc_ret_388 = "master";
        if (cmd != NULL) {
            if ((--cmd->__rc) <= 0) {
                Command_destroy(cmd);
            }
        }
        return __btrc_ret_388;
    }
    char* json = ExecResult_stdout(result);
    char* marker = "\"rev\":\"";
    int start = Strings_find(json, marker, 0);
    if (start < 0) {
        __auto_type __btrc_ret_389 = "master";
        if (cmd != NULL) {
            if ((--cmd->__rc) <= 0) {
                Command_destroy(cmd);
            }
        }
        return __btrc_ret_389;
    }
    (start = (start + ((int)strlen(marker))));
    int end = start;
    while ((json[end] != '\0') && (json[end] != '"')) {
        (end++);
    }
    __auto_type __btrc_ret_390 = JsonObject_slice(json, start, end);
    if (cmd != NULL) {
        if ((--cmd->__rc) <= 0) {
            Command_destroy(cmd);
        }
    }
    return __btrc_ret_390;
    if (cmd != NULL) {
        if ((--cmd->__rc) <= 0) {
            Command_destroy(cmd);
        }
    }
}

char* NixosConfig_evalRaw(NixosConfig* self, char* attribute) {
    char* key = __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(NixosConfig_flakeRef(self), ":")), attribute));
    if (NixEvalCache_has(self->cache, key)) {
        __auto_type __btrc_ret_391 = NixEvalCache_get(self->cache, key);
        return __btrc_ret_391;
    }
    Command* cmd = Command_new("nix");
    Command_arg(cmd, "--extra-experimental-features");
    Command_arg(cmd, "nix-command");
    Command_arg(cmd, "--extra-experimental-features");
    Command_arg(cmd, "flakes");
    Command_arg(cmd, "eval");
    Command_arg(cmd, __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(self->paths->root, "#nixosConfigurations.")), NixosConfig_host(self))), "-")), NixosConfig_target(self))), ".")), attribute)));
    ExecResult* result = UnixShell_runCommand(self->shell, cmd);
    if (!ExecResult_ok(result)) {
        NixosLog_fatal(__btrc_str_track(__btrc_strcat("nix eval failed for ", attribute)));
    }
    char* value = __btrc_str_track(__btrc_trim(ExecResult_stdout(result)));
    (value = Strings_replace(value, "\"", ""));
    NixEvalCache_put(self->cache, key, value);
    if (cmd != NULL) {
        if ((--cmd->__rc) <= 0) {
            Command_destroy(cmd);
        }
    }
    return value;
    if (cmd != NULL) {
        if ((--cmd->__rc) <= 0) {
            Command_destroy(cmd);
        }
    }
}

bool NixosConfig_evalBool(NixosConfig* self, char* attribute) {
    char* value = NixosConfig_evalRaw(self, attribute);
    __auto_type __btrc_ret_392 = (strcmp(value, "true") == 0);
    return __btrc_ret_392;
}

char* NixosConfig_standardTarget(NixosConfig* self) {
    __auto_type __btrc_ret_393 = "Standard-Boot";
    return __btrc_ret_393;
}

char* NixosConfig_secureBootTarget(NixosConfig* self) {
    __auto_type __btrc_ret_394 = "Secure-Boot";
    return __btrc_ret_394;
}

char* NixosConfig_diskOperationTarget(NixosConfig* self) {
    __auto_type __btrc_ret_395 = "Disk-Operation";
    return __btrc_ret_395;
}

char* NixosConfig_username(NixosConfig* self) {
    __auto_type __btrc_ret_396 = NixosConfig_evalRaw(self, "config.settings.user.admin.username");
    return __btrc_ret_396;
}

char* NixosConfig_secretsPath(NixosConfig* self) {
    __auto_type __btrc_ret_397 = NixosConfig_evalRaw(self, "config.settings.secrets.path");
    return __btrc_ret_397;
}

char* NixosConfig_hashedPasswordPath(NixosConfig* self) {
    char* name = NixosConfig_evalRaw(self, "config.settings.secrets.hashedPasswordFile");
    __auto_type __btrc_ret_398 = PathTools_join(NixosConfig_secretsPath(self), name);
    return __btrc_ret_398;
}

char* NixosConfig_diskDevice(NixosConfig* self) {
    __auto_type __btrc_ret_399 = NixosConfig_evalRaw(self, "config.settings.disk.device");
    return __btrc_ret_399;
}

char* NixosConfig_rootPartLabelPath(NixosConfig* self) {
    __auto_type __btrc_ret_400 = NixosConfig_evalRaw(self, "config.settings.disk.by.partlabel.root");
    return __btrc_ret_400;
}

char* NixosConfig_tpmDevice(NixosConfig* self) {
    __auto_type __btrc_ret_401 = NixosConfig_evalRaw(self, "config.settings.tpm.device");
    return __btrc_ret_401;
}

char* NixosConfig_tpmVersionPath(NixosConfig* self) {
    __auto_type __btrc_ret_402 = NixosConfig_evalRaw(self, "config.settings.tpm.versionPath");
    return __btrc_ret_402;
}

void SecretsManager_init(SecretsManager* self, NixosConfig* config) {
    self->__rc = 1;
    if (self->config != NULL) {
        if ((--self->config->__rc) <= 0) {
            NixosConfig_destroy(self->config);
        }
    }
    (self->config = config);
    (config->__rc++);
    if (self->shell != NULL) {
        if ((--self->shell->__rc) <= 0) {
            UnixShell_destroy(self->shell);
        }
    }
    (self->shell = UnixShell_new());
    (UnixShell_new()->__rc++);
    if (self->interactive != NULL) {
        if ((--self->interactive->__rc) <= 0) {
            Interactive_destroy(self->interactive);
        }
    }
    (self->interactive = Interactive_new());
    (Interactive_new()->__rc++);
}

SecretsManager* SecretsManager_new(NixosConfig* config) {
    SecretsManager* self = ((SecretsManager*)malloc(sizeof(SecretsManager)));
    memset(self, 0, sizeof(SecretsManager));
    SecretsManager_init(self, config);
    return self;
}

void SecretsManager_destroy(SecretsManager* self) {
    if (self->config != NULL) {
        if ((--self->config->__rc) <= 0) {
            NixosConfig_destroy(self->config);
        }
    }
    if (self->shell != NULL) {
        if ((--self->shell->__rc) <= 0) {
            UnixShell_destroy(self->shell);
        }
    }
    if (self->interactive != NULL) {
        if ((--self->interactive->__rc) <= 0) {
            Interactive_destroy(self->interactive);
        }
    }
    if (__btrc_tracking) {
        __btrc_mark_destroyed(self);
    }
    free(self);
}

bool SecretsManager_hasHashedPassword(SecretsManager* self) {
    __auto_type __btrc_ret_403 = FileSystem_exists(NixosConfig_hashedPasswordPath(self->config));
    return __btrc_ret_403;
}

void SecretsManager_writeHashedPassword(SecretsManager* self, char* hashed) {
    FileSystem_mkdirp(PathTools_dirname(NixosConfig_hashedPasswordPath(self->config)));
    Path_writeAll(NixosConfig_hashedPasswordPath(self->config), hashed);
}

bool SecretsManager_needsPassword(SecretsManager* self, char* plainTextPasswordPath) {
    if (!SecretsManager_hasHashedPassword(self)) {
        __auto_type __btrc_ret_404 = true;
        return __btrc_ret_404;
    }
    if ((!__btrc_isEmpty(plainTextPasswordPath)) && (!FileSystem_exists(plainTextPasswordPath))) {
        __auto_type __btrc_ret_405 = true;
        return __btrc_ret_405;
    }
    __auto_type __btrc_ret_406 = false;
    return __btrc_ret_406;
}

char* SecretsManager_hashPassword(SecretsManager* self, char* password) {
    Command* cmd = Command_new("mkpasswd");
    Command_arg(cmd, "-m");
    Command_arg(cmd, "sha-512");
    Command_arg(cmd, password);
    Command_redact(cmd, password);
    ExecResult* result = UnixShell_runCommand(self->shell, cmd);
    if (!ExecResult_ok(result)) {
        NixosLog_fatal("mkpasswd failed");
    }
    __auto_type __btrc_ret_407 = __btrc_str_track(__btrc_trim(ExecResult_stdout(result)));
    if (cmd != NULL) {
        if ((--cmd->__rc) <= 0) {
            Command_destroy(cmd);
        }
    }
    return __btrc_ret_407;
    if (cmd != NULL) {
        if ((--cmd->__rc) <= 0) {
            Command_destroy(cmd);
        }
    }
}

void SecretsManager_createIfMissing(SecretsManager* self, char* plainTextPasswordPath) {
    FileSystem_mkdirp(NixosConfig_secretsPath(self->config));
    if (!SecretsManager_needsPassword(self, plainTextPasswordPath)) {
        return;
    }
    char* password = Interactive_askPasswordConfirmed(self->interactive, "Set your password");
    if (!__btrc_isEmpty(plainTextPasswordPath)) {
        FileSystem_mkdirp(PathTools_dirname(plainTextPasswordPath));
        Path_writeAll(plainTextPasswordPath, password);
    }
    SecretsManager_writeHashedPassword(self, SecretsManager_hashPassword(self, password));
}

void SecretsManager_secure(SecretsManager* self) {
    if (!FileSystem_exists(NixosConfig_secretsPath(self->config))) {
        return;
    }
    char* root = UnixShell_quote(NixosConfig_secretsPath(self->config));
    UnixShell_runRaw(self->shell, __btrc_str_track(__btrc_strcat("chown -R root:root ", root)), false, false, "");
    UnixShell_runRaw(self->shell, __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("find ", root)), " -type d -exec chmod 700 {} +")), false, false, "");
    UnixShell_runRaw(self->shell, __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("find ", root)), " -type f -exec chmod 600 {} +")), false, false, "");
}

void PermissionsManager_init(PermissionsManager* self, NixosConfig* config) {
    self->__rc = 1;
    if (self->config != NULL) {
        if ((--self->config->__rc) <= 0) {
            NixosConfig_destroy(self->config);
        }
    }
    (self->config = config);
    (config->__rc++);
    if (self->shell != NULL) {
        if ((--self->shell->__rc) <= 0) {
            UnixShell_destroy(self->shell);
        }
    }
    (self->shell = UnixShell_new());
    (UnixShell_new()->__rc++);
}

PermissionsManager* PermissionsManager_new(NixosConfig* config) {
    PermissionsManager* self = ((PermissionsManager*)malloc(sizeof(PermissionsManager)));
    memset(self, 0, sizeof(PermissionsManager));
    PermissionsManager_init(self, config);
    return self;
}

void PermissionsManager_destroy(PermissionsManager* self) {
    if (self->config != NULL) {
        if ((--self->config->__rc) <= 0) {
            NixosConfig_destroy(self->config);
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

char* PermissionsManager_ignorePredicate(PermissionsManager* self, char* quotedRoot) {
    __auto_type __btrc_ret_408 = __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("\\( -path ", UnixShell_quote("*/secrets*"))), " -o -path ")), UnixShell_quote("*/.venv*"))), " -o -path ")), UnixShell_quote("*/.direnv*"))), " \\)"));
    return __btrc_ret_408;
}

void PermissionsManager_secureSecretsPath(PermissionsManager* self, char* path) {
    if (!FileSystem_exists(path)) {
        return;
    }
    char* quoted = UnixShell_quote(path);
    if (Platform_isRoot()) {
        UnixShell_runRaw(self->shell, __btrc_str_track(__btrc_strcat("chown -R root:root ", quoted)), false, false, "");
    }
    UnixShell_runRaw(self->shell, __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("find ", quoted)), " -type d -exec chmod 700 {} +")), false, false, "");
    UnixShell_runRaw(self->shell, __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("find ", quoted)), " -type f -exec chmod 600 {} +")), false, false, "");
}

void PermissionsManager_secureTree(PermissionsManager* self, char* username) {
    char* root = self->config->paths->root;
    if (Platform_isRoot()) {
        Command* chownRoot = Command_new("chown");
        Command_arg(chownRoot, "-R");
        Command_arg(chownRoot, username);
        Command_arg(chownRoot, root);
        Command_capture(chownRoot, false);
        UnixShell_runCommand(self->shell, chownRoot);
        if (chownRoot != NULL) {
            if ((--chownRoot->__rc) <= 0) {
                Command_destroy(chownRoot);
            }
        }
    }
    char* quotedRoot = UnixShell_quote(root);
    char* ignores = PermissionsManager_ignorePredicate(self, quotedRoot);
    UnixShell_runRaw(self->shell, __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("find ", quotedRoot)), " ")), ignores)), " -prune -o -type d -exec chmod 755 {} +")), false, false, "");
    UnixShell_runRaw(self->shell, __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("find ", quotedRoot)), " ")), ignores)), " -prune -o -type f -exec chmod 644 {} +")), false, false, "");
    UnixShell_runRaw(self->shell, __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("find ", quotedRoot)), " ")), ignores)), " -prune -o \\( -path '*/scripts/*' -o -path '*/bin/*' \\) -exec chmod 755 {} +")), false, false, "");
    char* gitObjects = PathTools_join(root, ".git/objects");
    if (FileSystem_exists(gitObjects)) {
        UnixShell_runRaw(self->shell, __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("find ", UnixShell_quote(gitObjects))), " -type f -exec chmod 444 {} +")), false, false, "");
    }
    Command* safe = Command_new("git");
    Command_arg(safe, "config");
    Command_arg(safe, "--global");
    Command_arg(safe, "--add");
    Command_arg(safe, "safe.directory");
    Command_arg(safe, root);
    Command_capture(safe, false);
    Command_check(safe, false);
    UnixShell_runCommand(self->shell, safe);
    if (NixosConfig_exists(self->config)) {
        SecretsManager_secure(SecretsManager_new(self->config));
    } else {
        PermissionsManager_secureSecretsPath(self, NixosPaths_secretsPathFallback(self->config->paths));
    }
    if (safe != NULL) {
        if ((--safe->__rc) <= 0) {
            Command_destroy(safe);
        }
    }
}

void ResetSubvolume_init(ResetSubvolume* self, char* name, char* mountPoint) {
    self->__rc = 1;
    (self->name = name);
    (self->mountPoint = mountPoint);
}

ResetSubvolume* ResetSubvolume_new(char* name, char* mountPoint) {
    ResetSubvolume* self = ((ResetSubvolume*)malloc(sizeof(ResetSubvolume)));
    memset(self, 0, sizeof(ResetSubvolume));
    ResetSubvolume_init(self, name, mountPoint);
    return self;
}

void ResetSubvolume_destroy(ResetSubvolume* self) {
    free(self);
}

void SnapshotManager_init(SnapshotManager* self, NixosConfig* config) {
    self->__rc = 1;
    if (self->config != NULL) {
        if ((--self->config->__rc) <= 0) {
            NixosConfig_destroy(self->config);
        }
    }
    (self->config = config);
    (config->__rc++);
    if (self->shell != NULL) {
        if ((--self->shell->__rc) <= 0) {
            UnixShell_destroy(self->shell);
        }
    }
    (self->shell = UnixShell_new());
    (UnixShell_new()->__rc++);
    (self->rootPrefix = "");
}

SnapshotManager* SnapshotManager_new(NixosConfig* config) {
    SnapshotManager* self = ((SnapshotManager*)malloc(sizeof(SnapshotManager)));
    memset(self, 0, sizeof(SnapshotManager));
    SnapshotManager_init(self, config);
    return self;
}

void SnapshotManager_destroy(SnapshotManager* self) {
    if (self->config != NULL) {
        if ((--self->config->__rc) <= 0) {
            NixosConfig_destroy(self->config);
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

char* SnapshotManager_hostPath(SnapshotManager* self, char* path) {
    if (__btrc_isEmpty(self->rootPrefix)) {
        return path;
    }
    if (strcmp(path, "/") == 0) {
        __auto_type __btrc_ret_409 = self->rootPrefix;
        return __btrc_ret_409;
    }
    if (__btrc_startsWith(path, "/")) {
        __auto_type __btrc_ret_410 = __btrc_str_track(__btrc_strcat(self->rootPrefix, path));
        return __btrc_ret_410;
    }
    __auto_type __btrc_ret_411 = PathTools_join(self->rootPrefix, path);
    return __btrc_ret_411;
}

char* SnapshotManager_snapshotsPath(SnapshotManager* self) {
    __auto_type __btrc_ret_412 = NixosConfig_evalRaw(self->config, "config.settings.disk.subvolumes.snapshots.mountPoint");
    return __btrc_ret_412;
}

char* SnapshotManager_cleanName(SnapshotManager* self) {
    __auto_type __btrc_ret_413 = NixosConfig_evalRaw(self->config, "config.settings.disk.immutability.persist.snapshots.cleanName");
    return __btrc_ret_413;
}

btrc_Vector_ResetSubvolume* SnapshotManager_resetSubvolumes(SnapshotManager* self) {
    btrc_Vector_ResetSubvolume* result = btrc_Vector_ResetSubvolume_new();
    char* raw = NixosConfig_evalRaw(self->config, "config.settings.disk.subvolumes.nameMountPointPairs.resetOnBoot");
    btrc_Vector_string* pairs = Strings_split(raw, " ");
    int __n_415 = btrc_Vector_string_iterLen(pairs);
    for (int __i_414 = 0; (__i_414 < __n_415); (__i_414++)) {
        char* pair = btrc_Vector_string_iterGet(pairs, __i_414);
        if (__btrc_isEmpty(pair)) {
            continue;
        }
        btrc_Vector_string* parts = Strings_split(pair, "=");
        if (parts->len >= 2) {
            btrc_Vector_ResetSubvolume_push(result, ResetSubvolume_new(btrc_Vector_string_get(parts, 0), btrc_Vector_string_get(parts, 1)));
        }
    }
    return result;
}

char* SnapshotManager_cleanSnapshotPath(SnapshotManager* self, char* subvolumeName) {
    __auto_type __btrc_ret_416 = PathTools_join(PathTools_join(SnapshotManager_snapshotsPath(self), subvolumeName), SnapshotManager_cleanName(self));
    return __btrc_ret_416;
}

bool SnapshotManager_isSubvolume(SnapshotManager* self, char* path) {
    if (!FileSystem_exists(path)) {
        __auto_type __btrc_ret_417 = false;
        return __btrc_ret_417;
    }
    Command* cmd = Command_check(Command_arg(Command_arg(Command_arg(Command_new("btrfs"), "subvolume"), "show"), path), false);
    __auto_type __btrc_ret_418 = ExecResult_ok(UnixShell_runCommand(self->shell, cmd));
    return __btrc_ret_418;
}

bool SnapshotManager_isReadonly(SnapshotManager* self, char* path) {
    Command* cmd = Command_check(Command_arg(Command_arg(Command_arg(Command_arg(Command_arg(Command_new("btrfs"), "property"), "get"), "-ts"), path), "ro"), false);
    ExecResult* result = UnixShell_runCommand(self->shell, cmd);
    __auto_type __btrc_ret_419 = (ExecResult_ok(result) && __btrc_strContains(ExecResult_stdout(result), "ro=true"));
    return __btrc_ret_419;
}

btrc_Vector_string* SnapshotManager_childSubvolumes(SnapshotManager* self, char* path) {
    btrc_Vector_string* result = btrc_Vector_string_new();
    Command* cmd = Command_check(Command_arg(Command_arg(Command_arg(Command_arg(Command_new("btrfs"), "subvolume"), "list"), "-o"), path), false);
    ExecResult* listed = UnixShell_runCommand(self->shell, cmd);
    if (!ExecResult_ok(listed)) {
        NixosLog_fatal(__btrc_str_track(__btrc_strcat("Failed to list child subvolumes under ", path)));
    }
    char* snapshotsName = NixosConfig_evalRaw(self->config, "config.settings.disk.subvolumes.snapshots.name");
    char* snapshotsMount = SnapshotManager_snapshotsPath(self);
    int __n_421 = btrc_Vector_string_iterLen(Strings_split(ExecResult_stdout(listed), "\n"));
    for (int __i_420 = 0; (__i_420 < __n_421); (__i_420++)) {
        char* line = btrc_Vector_string_iterGet(Strings_split(ExecResult_stdout(listed), "\n"), __i_420);
        int marker = Strings_find(line, " path ", 0);
        if (marker < 0) {
            continue;
        }
        char* child = __btrc_str_track(__btrc_trim(__btrc_str_track(__btrc_substring(line, (marker + 6), (((int)strlen(line)) - (marker + 6))))));
        if (strcmp(child, snapshotsName) == 0) {
            btrc_Vector_string_push(result, SnapshotManager_hostPath(self, snapshotsMount));
        } else if (__btrc_startsWith(child, __btrc_str_track(__btrc_strcat(snapshotsName, "/")))) {
            btrc_Vector_string_push(result, SnapshotManager_hostPath(self, PathTools_join(snapshotsMount, __btrc_str_track(__btrc_substring(child, (((int)strlen(snapshotsName)) + 1), ((((int)strlen(child)) - ((int)strlen(snapshotsName))) - 1))))));
        } else {
            btrc_Vector_string_push(result, SnapshotManager_hostPath(self, __btrc_str_track(__btrc_strcat("/", child))));
        }
    }
    return result;
}

void SnapshotManager_deleteSubvolume(SnapshotManager* self, char* path) {
    if (!FileSystem_exists(path)) {
        return;
    }
    if (!SnapshotManager_isSubvolume(self, path)) {
        NixosLog_fatal(__btrc_str_track(__btrc_strcat("Refusing to delete non-subvolume: ", path)));
    }
    int __n_423 = btrc_Vector_string_iterLen(SnapshotManager_childSubvolumes(self, path));
    for (int __i_422 = 0; (__i_422 < __n_423); (__i_422++)) {
        char* child = btrc_Vector_string_iterGet(SnapshotManager_childSubvolumes(self, path), __i_422);
        SnapshotManager_deleteSubvolume(self, child);
    }
    Command* cmd = Command_capture(Command_arg(Command_arg(Command_arg(Command_arg(Command_new("btrfs"), "subvolume"), "delete"), "-C"), path), false);
    UnixShell_runCommand(self->shell, cmd);
}

void SnapshotManager_createReadonlySnapshot(SnapshotManager* self, char* source, char* destination) {
    SnapshotManager_deleteSubvolume(self, destination);
    FileSystem_mkdirp(PathTools_dirname(destination));
    Command* cmd = Command_capture(Command_arg(Command_arg(Command_arg(Command_arg(Command_arg(Command_new("btrfs"), "subvolume"), "snapshot"), "-r"), source), destination), false);
    ExecResult* result = UnixShell_runCommand(self->shell, cmd);
    if (!ExecResult_ok(result)) {
        NixosLog_fatal(__btrc_str_track(__btrc_strcat("Failed to create readonly snapshot ", destination)));
    }
}

void SnapshotManager_createCleanSnapshot(SnapshotManager* self, ResetSubvolume* volume) {
    char* clean = SnapshotManager_hostPath(self, SnapshotManager_cleanSnapshotPath(self, volume->name));
    char* source = SnapshotManager_hostPath(self, volume->mountPoint);
    char* temporary = __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(clean, ".tmp.")), Strings_fromInt(Platform_pid())));
    SnapshotManager_deleteSubvolume(self, temporary);
    SnapshotManager_createReadonlySnapshot(self, source, temporary);
    if (!SnapshotManager_isSubvolume(self, temporary)) {
        NixosLog_fatal(__btrc_str_track(__btrc_strcat("CLEAN temporary is not a subvolume: ", temporary)));
    }
    if (!SnapshotManager_isReadonly(self, temporary)) {
        NixosLog_fatal(__btrc_str_track(__btrc_strcat("CLEAN temporary is not readonly: ", temporary)));
    }
    char* parent = PathTools_dirname(clean);
    SnapshotManager_deleteSubvolume(self, PathTools_join(parent, "READY"));
    SnapshotManager_deleteSubvolume(self, PathTools_join(parent, "NEXT"));
    if (FileSystem_exists(clean)) {
        if (!SnapshotManager_isSubvolume(self, clean)) {
            NixosLog_fatal(__btrc_str_track(__btrc_strcat("Refusing to replace non-subvolume CLEAN: ", clean)));
        }
        UnixShell_runRaw(self->shell, __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("mv -T --exchange --no-copy ", UnixShell_quote(temporary))), " ")), UnixShell_quote(clean))), false, true, "");
        SnapshotManager_deleteSubvolume(self, temporary);
    } else {
        UnixShell_runRaw(self->shell, __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("mv --no-copy ", UnixShell_quote(temporary))), " ")), UnixShell_quote(clean))), false, true, "");
    }
    if (!SnapshotManager_isSubvolume(self, clean)) {
        NixosLog_fatal(__btrc_str_track(__btrc_strcat("CLEAN is not a subvolume: ", clean)));
    }
    if (!SnapshotManager_isReadonly(self, clean)) {
        NixosLog_fatal(__btrc_str_track(__btrc_strcat("CLEAN is not readonly: ", clean)));
    }
}

void SnapshotManager_createInitialSnapshots(SnapshotManager* self) {
    btrc_Vector_ResetSubvolume* volumes = SnapshotManager_resetSubvolumes(self);
    int __n_425 = btrc_Vector_ResetSubvolume_iterLen(volumes);
    for (int __i_424 = 0; (__i_424 < __n_425); (__i_424++)) {
        ResetSubvolume* volume = btrc_Vector_ResetSubvolume_iterGet(volumes, __i_424);
        SnapshotManager_createCleanSnapshot(self, volume);
    }
}

void RebuildOptions_init(RebuildOptions* self) {
    self->__rc = 1;
    (self->rebuildFileSystem = false);
    (self->reboot = false);
    (self->clean = false);
    (self->upgrade = false);
}

RebuildOptions* RebuildOptions_new(void) {
    RebuildOptions* self = ((RebuildOptions*)malloc(sizeof(RebuildOptions)));
    memset(self, 0, sizeof(RebuildOptions));
    RebuildOptions_init(self);
    return self;
}

void RebuildOptions_destroy(RebuildOptions* self) {
    free(self);
}

void NixosRebuilder_init(NixosRebuilder* self, NixosConfig* config) {
    self->__rc = 1;
    if (self->config != NULL) {
        if ((--self->config->__rc) <= 0) {
            NixosConfig_destroy(self->config);
        }
    }
    (self->config = config);
    (config->__rc++);
    if (self->shell != NULL) {
        if ((--self->shell->__rc) <= 0) {
            UnixShell_destroy(self->shell);
        }
    }
    (self->shell = UnixShell_new());
    (UnixShell_new()->__rc++);
    if (self->interactive != NULL) {
        if ((--self->interactive->__rc) <= 0) {
            Interactive_destroy(self->interactive);
        }
    }
    (self->interactive = Interactive_new());
    (Interactive_new()->__rc++);
}

NixosRebuilder* NixosRebuilder_new(NixosConfig* config) {
    NixosRebuilder* self = ((NixosRebuilder*)malloc(sizeof(NixosRebuilder)));
    memset(self, 0, sizeof(NixosRebuilder));
    NixosRebuilder_init(self, config);
    return self;
}

void NixosRebuilder_destroy(NixosRebuilder* self) {
    if (self->config != NULL) {
        if ((--self->config->__rc) <= 0) {
            NixosConfig_destroy(self->config);
        }
    }
    if (self->shell != NULL) {
        if ((--self->shell->__rc) <= 0) {
            UnixShell_destroy(self->shell);
        }
    }
    if (self->interactive != NULL) {
        if ((--self->interactive->__rc) <= 0) {
            Interactive_destroy(self->interactive);
        }
    }
    if (__btrc_tracking) {
        __btrc_mark_destroyed(self);
    }
    free(self);
}

ExecResult* NixosRebuilder_runNixCollectGarbage(NixosRebuilder* self) {
    Command* cmd = Command_new("nix-collect-garbage");
    Command_arg(cmd, "-d");
    Command_capture(cmd, false);
    __auto_type __btrc_ret_426 = UnixShell_runCommand(self->shell, cmd);
    if (cmd != NULL) {
        if ((--cmd->__rc) <= 0) {
            Command_destroy(cmd);
        }
    }
    return __btrc_ret_426;
    if (cmd != NULL) {
        if ((--cmd->__rc) <= 0) {
            Command_destroy(cmd);
        }
    }
}

ExecResult* NixosRebuilder_verifyStore(NixosRebuilder* self) {
    Command* cmd = Command_new("nix-store");
    Command_arg(cmd, "--verify");
    Command_arg(cmd, "--repair");
    Command_capture(cmd, false);
    __auto_type __btrc_ret_427 = UnixShell_runCommand(self->shell, cmd);
    if (cmd != NULL) {
        if ((--cmd->__rc) <= 0) {
            Command_destroy(cmd);
        }
    }
    return __btrc_ret_427;
    if (cmd != NULL) {
        if ((--cmd->__rc) <= 0) {
            Command_destroy(cmd);
        }
    }
}

ExecResult* NixosRebuilder_updateFlake(NixosRebuilder* self) {
    Command* cmd = Command_new("nix");
    Command_arg(cmd, "flake");
    Command_arg(cmd, "update");
    Command_flag(cmd, "--flake", self->config->paths->root);
    Command_capture(cmd, false);
    __auto_type __btrc_ret_428 = UnixShell_runCommand(self->shell, cmd);
    if (cmd != NULL) {
        if ((--cmd->__rc) <= 0) {
            Command_destroy(cmd);
        }
    }
    return __btrc_ret_428;
    if (cmd != NULL) {
        if ((--cmd->__rc) <= 0) {
            Command_destroy(cmd);
        }
    }
}

ExecResult* NixosRebuilder_switchSystem(NixosRebuilder* self, RebuildOptions* options) {
    Command* cmd = Command_new("nixos-rebuild");
    Command_arg(cmd, "switch");
    Command_flag(cmd, "--flake", NixosConfig_flakeRef(self->config));
    if (options->rebuildFileSystem) {
        Command_envVar(cmd, "NIXOS_INSTALL_BOOTLOADER", "1");
    }
    Command_capture(cmd, false);
    __auto_type __btrc_ret_429 = UnixShell_runCommand(self->shell, cmd);
    if (cmd != NULL) {
        if ((--cmd->__rc) <= 0) {
            Command_destroy(cmd);
        }
    }
    return __btrc_ret_429;
    if (cmd != NULL) {
        if ((--cmd->__rc) <= 0) {
            Command_destroy(cmd);
        }
    }
}

char* NixosRebuilder_immutabilityDevice(NixosRebuilder* self) {
    if (NixosConfig_evalBool(self->config, "config.settings.disk.encryption.enable")) {
        __auto_type __btrc_ret_430 = NixosConfig_evalRaw(self->config, "config.settings.disk.by.mapper.root");
        return __btrc_ret_430;
    }
    __auto_type __btrc_ret_431 = NixosConfig_rootPartLabelPath(self->config);
    return __btrc_ret_431;
}

void NixosRebuilder_bootstrapConfigIfMissing(NixosRebuilder* self, RebuildOptions* options) {
    if (NixosConfig_exists(self->config)) {
        return;
    }
    NixosLog_error(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("'", NixosPaths_configPath(self->config->paths))), "' is missing.")));
    char* hostPath = Interactive_askHostPath(self->interactive, NixosPaths_hostsPath(self->config->paths));
    NixosConfig_reset(self->config, hostPath, NixosConfig_standardTarget(self->config));
    (options->rebuildFileSystem = true);
}

char* NixosRebuilder_plainTextPasswordPath(NixosRebuilder* self) {
    if (NixosConfig_evalBool(self->config, "config.settings.disk.encryption.enable")) {
        __auto_type __btrc_ret_432 = NixosConfig_evalRaw(self->config, "config.settings.disk.encryption.plainTextPasswordFile");
        return __btrc_ret_432;
    }
    __auto_type __btrc_ret_433 = "";
    return __btrc_ret_433;
}

char* NixosRebuilder_homeManagerLogs(NixosRebuilder* self) {
    ExecResult* result = UnixShell_runUnchecked(self->shell, "journalctl -u 'home-manager-*.service' --no-pager -o cat -q -r");
    btrc_Vector_string* lines = btrc_Vector_string_new();
    int __n_435 = btrc_Vector_string_iterLen(Strings_split(ExecResult_stdout(result), "\n"));
    for (int __i_434 = 0; (__i_434 < __n_435); (__i_434++)) {
        char* line = btrc_Vector_string_iterGet(Strings_split(ExecResult_stdout(result), "\n"), __i_434);
        if (strcmp(line, "Starting Home Manager activation") == 0) {
            break;
        }
        if ((((__btrc_startsWith(line, "Starting ") || __btrc_startsWith(line, "Stopping ")) || __btrc_startsWith(line, "Stopped ")) || __btrc_startsWith(line, "Finished ")) || __btrc_startsWith(line, "Activating ")) {
            continue;
        }
        if (!__btrc_isEmpty(__btrc_str_track(__btrc_trim(line)))) {
            btrc_Vector_string_push(lines, line);
        }
    }
    btrc_Vector_string_reverse(lines);
    __auto_type __btrc_ret_436 = __btrc_str_track(__btrc_trim(btrc_Vector_string_join(lines, "\n")));
    return __btrc_ret_436;
}

void NixosRebuilder_refreshImmutableSnapshots(NixosRebuilder* self) {
    if (!NixosConfig_evalBool(self->config, "config.settings.disk.immutability.enable")) {
        return;
    }
    if (!NixosConfig_evalBool(self->config, "config.settings.disk.immutability.enforce.onUpdate")) {
        return;
    }
    SnapshotManager* snapshots = SnapshotManager_new(self->config);
    char* implementation = NixosConfig_evalRaw(self->config, "config.settings.disk.immutability.implementation");
    if (!(strcmp(implementation, "semipermeable_membrane") == 0)) {
        NixosLog_info("Refreshing immutable CLEAN snapshots after update");
        SnapshotManager_createInitialSnapshots(snapshots);
        if (snapshots != NULL) {
            if ((--snapshots->__rc) <= 0) {
                SnapshotManager_destroy(snapshots);
            }
        }
        return;
    }
    if (!NixosConfig_evalBool(self->config, "config.settings.disk.immutability.semipermeable_membrane.enable")) {
        NixosLog_info("Semipermeable membrane is disabled; skipping update enforcement");
        if (snapshots != NULL) {
            if ((--snapshots->__rc) <= 0) {
                SnapshotManager_destroy(snapshots);
            }
        }
        return;
    }
    char* mode = NixosConfig_evalRaw(self->config, "config.settings.disk.immutability.semipermeable_membrane.mode");
    if (strcmp(mode, "disabled") == 0) {
        NixosLog_info("Semipermeable membrane mode is disabled; skipping update enforcement");
        if (snapshots != NULL) {
            if ((--snapshots->__rc) <= 0) {
                SnapshotManager_destroy(snapshots);
            }
        }
        return;
    }
    bool dryRun = NixosConfig_evalBool(self->config, "config.settings.disk.immutability.semipermeable_membrane.dryRun");
    if (dryRun) {
        NixosLog_info("DRY would refresh immutable CLEAN snapshots after update");
    } else {
        NixosLog_info("Refreshing immutable CLEAN snapshots after update");
        SnapshotManager_createInitialSnapshots(snapshots);
    }
    NixosLog_info("Converging semipermeable membrane after update");
    btrc_Vector_string* __list_437 = btrc_Vector_string_new();
    btrc_Vector_string_push(__list_437, "/run/current-system/sw/bin/semipermeable_membrane");
    btrc_Vector_string_push(__list_437, NixosRebuilder_immutabilityDevice(self));
    btrc_Vector_string_push(__list_437, NixosConfig_evalRaw(self->config, "config.settings.disk.subvolumes.snapshots.name"));
    btrc_Vector_string_push(__list_437, NixosConfig_evalRaw(self->config, "config.settings.disk.immutability.persist.snapshots.cleanName"));
    btrc_Vector_string_push(__list_437, mode);
    btrc_Vector_string_push(__list_437, NixosConfig_evalRaw(self->config, "config.settings.disk.immutability.semipermeable_membrane.persist.subvolumeRoot"));
    btrc_Vector_string_push(__list_437, "/etc/semipermeable_membrane/spec.tsv");
    btrc_Vector_string* args = __list_437;
    if (dryRun) {
        btrc_Vector_string_insert(args, 1, "--dry-run");
    }
    char* pairs = NixosConfig_evalRaw(self->config, "config.settings.disk.subvolumes.nameMountPointPairs.resetOnBoot");
    int __n_439 = btrc_Vector_string_iterLen(Strings_split(pairs, " "));
    for (int __i_438 = 0; (__i_438 < __n_439); (__i_438++)) {
        char* pair = btrc_Vector_string_iterGet(Strings_split(pairs, " "), __i_438);
        if (!__btrc_isEmpty(__btrc_str_track(__btrc_trim(pair)))) {
            btrc_Vector_string_push(args, __btrc_str_track(__btrc_trim(pair)));
        }
    }
    Command* membrane = Command_new(btrc_Vector_string_get(args, 0));
    for (int i = 1; (i < args->len); (i++)) {
        Command_arg(membrane, btrc_Vector_string_get(args, i));
    }
    Command_capture(membrane, false);
    Command_check(membrane, false);
    UnixShell_runCommand(self->shell, membrane);
    if (dryRun) {
        NixosLog_info("DRY would start semipermeable-membrane-mounts.service");
        if (membrane != NULL) {
            if ((--membrane->__rc) <= 0) {
                Command_destroy(membrane);
            }
        }
        if (snapshots != NULL) {
            if ((--snapshots->__rc) <= 0) {
                SnapshotManager_destroy(snapshots);
            }
        }
        return;
    }
    UnixShell_runRaw(self->shell, "systemctl start semipermeable-membrane-mounts.service", false, false, "");
    if (membrane != NULL) {
        if ((--membrane->__rc) <= 0) {
            Command_destroy(membrane);
        }
    }
    if (snapshots != NULL) {
        if ((--snapshots->__rc) <= 0) {
            SnapshotManager_destroy(snapshots);
        }
    }
}

void NixosRebuilder_update(NixosRebuilder* self, RebuildOptions* options) {
    NixosRebuilder_bootstrapConfigIfMissing(self, options);
    char* username = NixosConfig_username(self->config);
    if (options->clean || options->upgrade) {
        NixosRebuilder_runNixCollectGarbage(self);
        FileSystem_removeRecursive("/root/.cache");
        NixosRebuilder_verifyStore(self);
    }
    if (options->upgrade) {
        FileSystem_removeRecursive("/root/.cache");
        NixosRebuilder_updateFlake(self);
    }
    SecretsManager_createIfMissing(SecretsManager_new(self->config), NixosRebuilder_plainTextPasswordPath(self));
    PermissionsManager_secureTree(PermissionsManager_new(self->config), username);
    ExecResult* result = NixosRebuilder_switchSystem(self, options);
    if (!ExecResult_ok(result)) {
        NixosLog_fatal("nixos-rebuild switch failed");
    }
    PermissionsManager_secureTree(PermissionsManager_new(self->config), username);
    NixosRebuilder_refreshImmutableSnapshots(self);
    char* logs = NixosRebuilder_homeManagerLogs(self);
    if (!__btrc_isEmpty(logs)) {
        Console_log(logs);
    }
    if (options->reboot) {
        Command* reboot = Command_new("shutdown");
        Command_arg(reboot, "-r");
        Command_arg(reboot, "now");
        Command_capture(reboot, false);
        UnixShell_runCommand(self->shell, reboot);
        if (reboot != NULL) {
            if ((--reboot->__rc) <= 0) {
                Command_destroy(reboot);
            }
        }
    } else {
        Interactive_askToReboot(self->interactive);
    }
}

void DiffOptions_init(DiffOptions* self) {
    self->__rc = 1;
    (self->recent = false);
    (self->showSymlinks = false);
    (self->showPersistPaths = false);
    (self->showChildren = "");
    (self->depth = 0);
    (self->pattern = "");
    (self->diffignore = "");
}

DiffOptions* DiffOptions_new(void) {
    DiffOptions* self = ((DiffOptions*)malloc(sizeof(DiffOptions)));
    memset(self, 0, sizeof(DiffOptions));
    DiffOptions_init(self);
    return self;
}

void DiffOptions_destroy(DiffOptions* self) {
    free(self);
}

void DiffScanner_init(DiffScanner* self, NixosConfig* config) {
    self->__rc = 1;
    if (self->config != NULL) {
        if ((--self->config->__rc) <= 0) {
            NixosConfig_destroy(self->config);
        }
    }
    (self->config = config);
    (config->__rc++);
    if (self->snapshots != NULL) {
        if ((--self->snapshots->__rc) <= 0) {
            SnapshotManager_destroy(self->snapshots);
        }
    }
    (self->snapshots = SnapshotManager_new(config));
    (SnapshotManager_new(config)->__rc++);
    if (self->shell != NULL) {
        if ((--self->shell->__rc) <= 0) {
            UnixShell_destroy(self->shell);
        }
    }
    (self->shell = UnixShell_new());
    (UnixShell_new()->__rc++);
}

DiffScanner* DiffScanner_new(NixosConfig* config) {
    DiffScanner* self = ((DiffScanner*)malloc(sizeof(DiffScanner)));
    memset(self, 0, sizeof(DiffScanner));
    DiffScanner_init(self, config);
    return self;
}

void DiffScanner_destroy(DiffScanner* self) {
    if (self->config != NULL) {
        if ((--self->config->__rc) <= 0) {
            NixosConfig_destroy(self->config);
        }
    }
    if (self->snapshots != NULL) {
        if ((--self->snapshots->__rc) <= 0) {
            SnapshotManager_destroy(self->snapshots);
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

btrc_Vector_string* DiffScanner_keepPaths(DiffScanner* self) {
    char* raw = NixosConfig_evalRaw(self->config, "config.settings.disk.immutability.persist.paths");
    (raw = Strings_replace(raw, "[", ""));
    (raw = Strings_replace(raw, "]", ""));
    btrc_Vector_string* result = btrc_Vector_string_new();
    int __n_441 = btrc_Vector_string_iterLen(Strings_split(__btrc_str_track(__btrc_trim(raw)), " "));
    for (int __i_440 = 0; (__i_440 < __n_441); (__i_440++)) {
        char* item = btrc_Vector_string_iterGet(Strings_split(__btrc_str_track(__btrc_trim(raw)), " "), __i_440);
        if (!__btrc_isEmpty(__btrc_str_track(__btrc_trim(item)))) {
            btrc_Vector_string_push(result, __btrc_str_track(__btrc_trim(item)));
        }
    }
    return result;
}

btrc_Vector_string* DiffScanner_ignorePatterns(DiffScanner* self, char* path) {
    btrc_Vector_string* patterns = btrc_Vector_string_new();
    if (!FileSystem_exists(path)) {
        return patterns;
    }
    int __n_443 = btrc_Vector_string_iterLen(Strings_split(Path_readAll(path), "\n"));
    for (int __i_442 = 0; (__i_442 < __n_443); (__i_442++)) {
        char* line = btrc_Vector_string_iterGet(Strings_split(Path_readAll(path), "\n"), __i_442);
        char* trimmed = __btrc_str_track(__btrc_trim(line));
        if ((!__btrc_isEmpty(trimmed)) && (!__btrc_startsWith(trimmed, "#"))) {
            btrc_Vector_string_push(patterns, trimmed);
        }
    }
    return patterns;
}

btrc_Vector_string* DiffScanner_mountPoints(DiffScanner* self) {
    btrc_Vector_string* result = btrc_Vector_string_new();
    btrc_Vector_ResetSubvolume* volumes = SnapshotManager_resetSubvolumes(self->snapshots);
    int __n_445 = btrc_Vector_ResetSubvolume_iterLen(volumes);
    for (int __i_444 = 0; (__i_444 < __n_445); (__i_444++)) {
        ResetSubvolume* volume = btrc_Vector_ResetSubvolume_iterGet(volumes, __i_444);
        btrc_Vector_string_push(result, volume->mountPoint);
    }
    return result;
}

btrc_Vector_string* DiffScanner_changedFiles(DiffScanner* self) {
    btrc_Vector_string* result = btrc_Vector_string_new();
    btrc_Vector_ResetSubvolume* volumes = SnapshotManager_resetSubvolumes(self->snapshots);
    int __n_447 = btrc_Vector_ResetSubvolume_iterLen(volumes);
    for (int __i_446 = 0; (__i_446 < __n_447); (__i_446++)) {
        ResetSubvolume* volume = btrc_Vector_ResetSubvolume_iterGet(volumes, __i_446);
        char* tmp = PathTools_join(PathTools_join(SnapshotManager_snapshotsPath(self->snapshots), volume->name), "tmp");
        char* clean = SnapshotManager_cleanSnapshotPath(self->snapshots, volume->name);
        SnapshotManager_deleteSubvolume(self->snapshots, tmp);
        SnapshotManager_createReadonlySnapshot(self->snapshots, volume->mountPoint, tmp);
        ExecResult* tx = UnixShell_runUnchecked(self->shell, __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("echo \"$(sudo btrfs subvolume find-new ", UnixShell_quote(clean))), " 9999999)\" | cut -d' ' -f4")));
        char* transaction = __btrc_str_track(__btrc_trim(ExecResult_stdout(tx)));
        ExecResult* changes = UnixShell_runUnchecked(self->shell, __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("btrfs subvolume find-new ", UnixShell_quote(tmp))), " ")), UnixShell_quote(transaction))), " | sed '$d' | cut -f17- -d' ' | sort | uniq")));
        if (ExecResult_ok(changes)) {
            int __n_449 = btrc_Vector_string_iterLen(Strings_split(ExecResult_stdout(changes), "\n"));
            for (int __i_448 = 0; (__i_448 < __n_449); (__i_448++)) {
                char* line = btrc_Vector_string_iterGet(Strings_split(ExecResult_stdout(changes), "\n"), __i_448);
                if (!__btrc_isEmpty(__btrc_str_track(__btrc_trim(line)))) {
                    char* full = __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(volume->mountPoint, "/")), __btrc_str_track(__btrc_trim(line))));
                    btrc_Vector_string_push(result, Strings_replace(full, "//", "/"));
                }
            }
        }
        SnapshotManager_deleteSubvolume(self->snapshots, tmp);
    }
    return result;
}

bool DiffScanner_isPersisted(DiffScanner* self, char* path, btrc_Vector_string* keepPaths) {
    int __n_451 = btrc_Vector_string_iterLen(keepPaths);
    for (int __i_450 = 0; (__i_450 < __n_451); (__i_450++)) {
        char* keepPath = btrc_Vector_string_iterGet(keepPaths, __i_450);
        if ((strcmp(path, keepPath) == 0) || __btrc_startsWith(path, __btrc_str_track(__btrc_strcat(keepPath, "/")))) {
            __auto_type __btrc_ret_452 = true;
            return __btrc_ret_452;
        }
    }
    __auto_type __btrc_ret_453 = false;
    return __btrc_ret_453;
}

bool DiffScanner_matchesPattern(DiffScanner* self, char* path, char* pattern) {
    if (__btrc_isEmpty(pattern)) {
        __auto_type __btrc_ret_454 = true;
        return __btrc_ret_454;
    }
    __auto_type __btrc_ret_455 = Pattern_matches(__btrc_str_track(__btrc_toLower(pattern)), __btrc_str_track(__btrc_toLower(path)));
    return __btrc_ret_455;
}

bool DiffScanner_ignored(DiffScanner* self, char* path, btrc_Vector_string* patterns) {
    int __n_457 = btrc_Vector_string_iterLen(patterns);
    for (int __i_456 = 0; (__i_456 < __n_457); (__i_456++)) {
        char* pattern = btrc_Vector_string_iterGet(patterns, __i_456);
        if (Pattern_matches(pattern, path)) {
            __auto_type __btrc_ret_458 = true;
            return __btrc_ret_458;
        }
    }
    __auto_type __btrc_ret_459 = false;
    return __btrc_ret_459;
}

char* DiffScanner_topAncestor(DiffScanner* self, char* path, btrc_Vector_string* keepList, btrc_Vector_string* mounts) {
    btrc_Vector_string* segments = Strings_split(path, "/");
    char* ancestor = "";
    for (int i = 1; (i < segments->len); (i++)) {
        if (__btrc_isEmpty(btrc_Vector_string_get(segments, i))) {
            continue;
        }
        (ancestor = __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(ancestor, "/")), btrc_Vector_string_get(segments, i))));
        if (btrc_Vector_string_contains(mounts, ancestor)) {
            continue;
        }
        bool coversKeep = false;
        int __n_461 = btrc_Vector_string_iterLen(keepList);
        for (int __i_460 = 0; (__i_460 < __n_461); (__i_460++)) {
            char* keepPath = btrc_Vector_string_iterGet(keepList, __i_460);
            if (__btrc_startsWith(keepPath, __btrc_str_track(__btrc_strcat(ancestor, "/")))) {
                (coversKeep = true);
            }
        }
        if (coversKeep) {
            continue;
        }
        return ancestor;
    }
    return path;
}

btrc_Vector_string* DiffScanner_collapse(DiffScanner* self, btrc_Vector_string* paths, btrc_Vector_string* keepList, btrc_Vector_string* mounts) {
    btrc_Vector_string* result = btrc_Vector_string_new();
    int __n_463 = btrc_Vector_string_iterLen(paths);
    for (int __i_462 = 0; (__i_462 < __n_463); (__i_462++)) {
        char* path = btrc_Vector_string_iterGet(paths, __i_462);
        btrc_Vector_string_push(result, DiffScanner_topAncestor(self, path, keepList, mounts));
    }
    btrc_Vector_string* distinct = btrc_Vector_string_distinct(result);
    btrc_Vector_string_sort(distinct);
    return distinct;
}

btrc_Vector_string* DiffScanner_collapseToPersist(DiffScanner* self, btrc_Vector_string* persisted, btrc_Vector_string* keepList) {
    btrc_Vector_string* result = btrc_Vector_string_new();
    int __n_465 = btrc_Vector_string_iterLen(persisted);
    for (int __i_464 = 0; (__i_464 < __n_465); (__i_464++)) {
        char* path = btrc_Vector_string_iterGet(persisted, __i_464);
        int __n_467 = btrc_Vector_string_iterLen(keepList);
        for (int __i_466 = 0; (__i_466 < __n_467); (__i_466++)) {
            char* keepPath = btrc_Vector_string_iterGet(keepList, __i_466);
            if ((strcmp(path, keepPath) == 0) || __btrc_startsWith(path, __btrc_str_track(__btrc_strcat(keepPath, "/")))) {
                btrc_Vector_string_push(result, keepPath);
                break;
            }
        }
    }
    btrc_Vector_string* distinct = btrc_Vector_string_distinct(result);
    btrc_Vector_string_sort(distinct);
    return distinct;
}

btrc_Vector_string* DiffScanner_atDepth(DiffScanner* self, btrc_Vector_string* bases, btrc_Vector_string* source, int depth) {
    if (depth == 0) {
        return bases;
    }
    btrc_Vector_string* result = btrc_Vector_string_new();
    int __n_469 = btrc_Vector_string_iterLen(bases);
    for (int __i_468 = 0; (__i_468 < __n_469); (__i_468++)) {
        char* base = btrc_Vector_string_iterGet(bases, __i_468);
        char* prefix = __btrc_str_track(__btrc_strcat(base, "/"));
        int __n_471 = btrc_Vector_string_iterLen(source);
        for (int __i_470 = 0; (__i_470 < __n_471); (__i_470++)) {
            char* path = btrc_Vector_string_iterGet(source, __i_470);
            if (!__btrc_startsWith(path, prefix)) {
                continue;
            }
            if (depth < 0) {
                btrc_Vector_string_push(result, path);
            } else {
                btrc_Vector_string* parts = Strings_split(path, "/");
                int wanted = ((Strings_count(base, "/") + depth) + 1);
                char* collapsed = "";
                for (int i = 1; ((i < parts->len) && (i <= wanted)); (i++)) {
                    if (!__btrc_isEmpty(btrc_Vector_string_get(parts, i))) {
                        (collapsed = __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(collapsed, "/")), btrc_Vector_string_get(parts, i))));
                    }
                }
                if (!__btrc_isEmpty(collapsed)) {
                    btrc_Vector_string_push(result, collapsed);
                }
            }
        }
    }
    btrc_Vector_string* distinct = btrc_Vector_string_distinct(result);
    btrc_Vector_string_sort(distinct);
    return distinct;
}

btrc_Vector_string* DiffScanner_filterPattern(DiffScanner* self, btrc_Vector_string* input, char* pattern) {
    if (__btrc_isEmpty(pattern)) {
        return input;
    }
    btrc_Vector_string* output = btrc_Vector_string_new();
    int __n_473 = btrc_Vector_string_iterLen(input);
    for (int __i_472 = 0; (__i_472 < __n_473); (__i_472++)) {
        char* path = btrc_Vector_string_iterGet(input, __i_472);
        if (DiffScanner_matchesPattern(self, path, pattern)) {
            btrc_Vector_string_push(output, path);
        }
    }
    return output;
}

btrc_Vector_string* DiffScanner_previousCache(DiffScanner* self, char* cachePath) {
    btrc_Vector_string* result = btrc_Vector_string_new();
    if (!FileSystem_exists(cachePath)) {
        return result;
    }
    JsonObject* data = JsonObject_readFile(cachePath);
    int __n_475 = btrc_Vector_string_iterLen(btrc_Map_string_string_keys(data->values));
    for (int __i_474 = 0; (__i_474 < __n_475); (__i_474++)) {
        char* key = btrc_Vector_string_iterGet(btrc_Map_string_string_keys(data->values), __i_474);
        btrc_Vector_string_push(result, key);
    }
    return result;
}

void DiffScanner_writeCache(DiffScanner* self, char* cachePath, btrc_Vector_string* paths) {
    JsonObject* data = JsonObject_new();
    int __n_477 = btrc_Vector_string_iterLen(paths);
    for (int __i_476 = 0; (__i_476 < __n_477); (__i_476++)) {
        char* path = btrc_Vector_string_iterGet(paths, __i_476);
        JsonObject_setBool(data, path, true);
    }
    FileSystem_mkdirp(PathTools_dirname(cachePath));
    JsonObject_writeFile(data, cachePath);
    if (data != NULL) {
        if ((--data->__rc) <= 0) {
            JsonObject_destroy(data);
        }
    }
}

void DiffScanner_print(DiffScanner* self, DiffOptions* options) {
    btrc_Vector_string* keepList = DiffScanner_keepPaths(self);
    btrc_Vector_string* mounts = DiffScanner_mountPoints(self);
    char* ignorePath = (__btrc_isEmpty(options->diffignore) ? PathTools_join(self->config->paths->root, "lib/btrfs/.diffignore") : options->diffignore);
    btrc_Vector_string* ignores = DiffScanner_ignorePatterns(self, ignorePath);
    btrc_Vector_string* ephemeral = btrc_Vector_string_new();
    btrc_Vector_string* persistedPaths = btrc_Vector_string_new();
    btrc_Vector_string* changed = DiffScanner_changedFiles(self);
    int __n_479 = btrc_Vector_string_iterLen(changed);
    for (int __i_478 = 0; (__i_478 < __n_479); (__i_478++)) {
        char* path = btrc_Vector_string_iterGet(changed, __i_478);
        if (DiffScanner_isPersisted(self, path, keepList)) {
            btrc_Vector_string_push(persistedPaths, path);
            continue;
        }
        if (!DiffScanner_ignored(self, path, ignores)) {
            btrc_Vector_string_push(ephemeral, path);
        }
    }
    btrc_Vector_string* output = btrc_Vector_string_new();
    if (options->showPersistPaths) {
        btrc_Vector_string* bases = DiffScanner_collapseToPersist(self, persistedPaths, keepList);
        (output = DiffScanner_atDepth(self, bases, persistedPaths, options->depth));
    } else {
        btrc_Vector_string* top = DiffScanner_collapse(self, ephemeral, keepList, mounts);
        char* cachePath = "/tmp/etc/nixos/scripts/bin/diff/cache.json";
        btrc_Vector_string* previous = DiffScanner_previousCache(self, cachePath);
        DiffScanner_writeCache(self, cachePath, top);
        if (options->recent) {
            btrc_Vector_string* recent = btrc_Vector_string_new();
            int __n_481 = btrc_Vector_string_iterLen(top);
            for (int __i_480 = 0; (__i_480 < __n_481); (__i_480++)) {
                char* path = btrc_Vector_string_iterGet(top, __i_480);
                if (!btrc_Vector_string_contains(previous, path)) {
                    btrc_Vector_string_push(recent, path);
                }
            }
            (top = recent);
        }
        if (!options->showSymlinks) {
            btrc_Vector_string* filtered = btrc_Vector_string_new();
            int __n_483 = btrc_Vector_string_iterLen(top);
            for (int __i_482 = 0; (__i_482 < __n_483); (__i_482++)) {
                char* path = btrc_Vector_string_iterGet(top, __i_482);
                if (!FileSystem_isSymlink(path)) {
                    btrc_Vector_string_push(filtered, path);
                }
            }
            (top = filtered);
        }
        if (!__btrc_isEmpty(options->showChildren)) {
            bool covers = false;
            int __n_485 = btrc_Vector_string_iterLen(top);
            for (int __i_484 = 0; (__i_484 < __n_485); (__i_484++)) {
                char* path = btrc_Vector_string_iterGet(top, __i_484);
                if (((strcmp(path, options->showChildren) == 0) || __btrc_startsWith(options->showChildren, __btrc_str_track(__btrc_strcat(path, "/")))) || __btrc_startsWith(path, __btrc_str_track(__btrc_strcat(options->showChildren, "/")))) {
                    (covers = true);
                }
            }
            btrc_Vector_string* bases = btrc_Vector_string_new();
            if (covers) {
                btrc_Vector_string_push(bases, options->showChildren);
            }
            (output = DiffScanner_atDepth(self, bases, ephemeral, options->depth));
        } else {
            (output = DiffScanner_atDepth(self, top, ephemeral, options->depth));
        }
    }
    (output = DiffScanner_filterPattern(self, output, options->pattern));
    if (output->len == 0) {
        return;
    }
    if (options->showPersistPaths) {
        NixosLog_error("\nPERSISTED CHANGES:");
    } else {
        NixosLog_error("\nCHANGES TO DELETE:");
    }
    int __n_487 = btrc_Vector_string_iterLen(output);
    for (int __i_486 = 0; (__i_486 < __n_487); (__i_486++)) {
        char* path = btrc_Vector_string_iterGet(output, __i_486);
        Console_error(path);
    }
}

void Installer_init(Installer* self, NixosConfig* config) {
    self->__rc = 1;
    if (self->config != NULL) {
        if ((--self->config->__rc) <= 0) {
            NixosConfig_destroy(self->config);
        }
    }
    (self->config = config);
    (config->__rc++);
    if (self->shell != NULL) {
        if ((--self->shell->__rc) <= 0) {
            UnixShell_destroy(self->shell);
        }
    }
    (self->shell = UnixShell_new());
    (UnixShell_new()->__rc++);
    if (self->interactive != NULL) {
        if ((--self->interactive->__rc) <= 0) {
            Interactive_destroy(self->interactive);
        }
    }
    (self->interactive = Interactive_new());
    (Interactive_new()->__rc++);
    (self->mountPoint = "/mnt");
}

Installer* Installer_new(NixosConfig* config) {
    Installer* self = ((Installer*)malloc(sizeof(Installer)));
    memset(self, 0, sizeof(Installer));
    Installer_init(self, config);
    return self;
}

void Installer_destroy(Installer* self) {
    if (self->config != NULL) {
        if ((--self->config->__rc) <= 0) {
            NixosConfig_destroy(self->config);
        }
    }
    if (self->shell != NULL) {
        if ((--self->shell->__rc) <= 0) {
            UnixShell_destroy(self->shell);
        }
    }
    if (self->interactive != NULL) {
        if ((--self->interactive->__rc) <= 0) {
            Interactive_destroy(self->interactive);
        }
    }
    if (__btrc_tracking) {
        __btrc_mark_destroyed(self);
    }
    free(self);
}

ExecResult* Installer_runDisko(Installer* self, char* mode, char* extraArgs) {
    char* rev = NixosConfig_inputLockedRev(self->config, "disko");
    if (__btrc_isEmpty(rev)) {
        (rev = "master");
    }
    char* command = __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("nix --extra-experimental-features nix-command --extra-experimental-features flakes run ", "github:nix-community/disko/")), rev)), " --verbose -- --show-trace --flake ")), UnixShell_quote(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(self->config->paths->root, "#")), NixosConfig_host(self->config))), "-")), NixosConfig_diskOperationTarget(self->config)))))), " --mode ")), UnixShell_quote(mode))), " --root-mountpoint ")), UnixShell_quote(self->mountPoint))), " ")), extraArgs));
    __auto_type __btrc_ret_488 = UnixShell_runRaw(self->shell, command, false, true, "");
    return __btrc_ret_488;
}

ExecResult* Installer_mountDisk(Installer* self) {
    __auto_type __btrc_ret_489 = Installer_runDisko(self, "mount", "");
    return __btrc_ret_489;
}

ExecResult* Installer_eraseAndMountDisk(Installer* self) {
    __auto_type __btrc_ret_490 = Installer_runDisko(self, "destroy,format,mount", "--yes-wipe-all-disks");
    return __btrc_ret_490;
}

ExecResult* Installer_installNixos(Installer* self) {
    FileSystem_mkdirp(PathTools_join(self->mountPoint, "/nix/tmp"));
    FileSystem_removeRecursive(PathTools_join(self->mountPoint, self->config->paths->root));
    Command* cp = Command_new("cp");
    Command_arg(cp, "-r");
    Command_arg(cp, self->config->paths->root);
    Command_arg(cp, PathTools_join(self->mountPoint, self->config->paths->root));
    Command_capture(cp, false);
    UnixShell_runCommand(self->shell, cp);
    Command* install = Command_new("nixos-install");
    Command_flag(install, "--flake", __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(PathTools_join(self->mountPoint, self->config->paths->root), "#")), NixosConfig_host(self->config))), "-")), NixosConfig_target(self->config))));
    Command_flag(install, "--root", self->mountPoint);
    Command_arg(install, "--no-channel-copy");
    Command_arg(install, "--show-trace");
    Command_arg(install, "--no-root-password");
    Command_arg(install, "--cores");
    Command_arg(install, "0");
    Command_envVar(install, "TMPDIR", PathTools_join(self->mountPoint, "/nix/tmp"));
    Command_capture(install, false);
    ExecResult* result = UnixShell_runCommand(self->shell, install);
    FileSystem_removeRecursive(PathTools_join(self->mountPoint, "/nix/tmp"));
    if (ExecResult_ok(result)) {
        Installer_permissionNixos(self);
    }
    if (install != NULL) {
        if ((--install->__rc) <= 0) {
            Command_destroy(install);
        }
    }
    if (cp != NULL) {
        if ((--cp->__rc) <= 0) {
            Command_destroy(cp);
        }
    }
    return result;
    if (install != NULL) {
        if ((--install->__rc) <= 0) {
            Command_destroy(install);
        }
    }
    if (cp != NULL) {
        if ((--cp->__rc) <= 0) {
            Command_destroy(cp);
        }
    }
}

void Installer_permissionNixos(Installer* self) {
    PermissionsManager* permissions = PermissionsManager_new(self->config);
    UnixShell_chroot(permissions->shell, self->mountPoint);
    PermissionsManager_secureTree(permissions, NixosConfig_username(self->config));
    UnixShell_clearChroot(permissions->shell);
    SnapshotManager* snapshots = SnapshotManager_new(self->config);
    (snapshots->rootPrefix = self->mountPoint);
    SnapshotManager_createInitialSnapshots(snapshots);
    if (snapshots != NULL) {
        if ((--snapshots->__rc) <= 0) {
            SnapshotManager_destroy(snapshots);
        }
    }
    if (permissions != NULL) {
        if ((--permissions->__rc) <= 0) {
            PermissionsManager_destroy(permissions);
        }
    }
}

char* Installer_plainTextPasswordPath(Installer* self) {
    if (NixosConfig_evalBool(self->config, "config.settings.disk.encryption.enable")) {
        __auto_type __btrc_ret_491 = NixosConfig_evalRaw(self->config, "config.settings.disk.encryption.plainTextPasswordFile");
        return __btrc_ret_491;
    }
    __auto_type __btrc_ret_492 = "";
    return __btrc_ret_492;
}

void Installer_bootstrapConfigIfMissing(Installer* self) {
    if (NixosConfig_exists(self->config)) {
        return;
    }
    char* hostPath = Interactive_askHostPath(self->interactive, NixosPaths_hostsPath(self->config->paths));
    NixosConfig_reset(self->config, hostPath, NixosConfig_standardTarget(self->config));
}

void Installer_collectGarbage(Installer* self) {
    Command* cmd = Command_capture(Command_arg(Command_new("nix-collect-garbage"), "-d"), false);
    UnixShell_runCommand(self->shell, cmd);
}

void Installer_debugShell(Installer* self) {
    char* base = "nix --extra-experimental-features nix-command --extra-experimental-features flakes run nixpkgs#vscodium -- --no-sandbox --user-data-dir /tmp/vscodium-data";
    UnixShell_runRaw(self->shell, __btrc_str_track(__btrc_strcat(base, " --install-extension ms-python.python")), false, false, "");
    UnixShell_runRaw(self->shell, __btrc_str_track(__btrc_strcat(base, " --install-extension rogalmic.bash-debug")), false, false, "");
    UnixShell_runRaw(self->shell, __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(base, " ")), UnixShell_quote(self->config->paths->root))), false, false, "");
    NixosLog_fatal("Please continue in VSCodium");
}

void SecureBootManager_init(SecureBootManager* self, NixosConfig* config) {
    self->__rc = 1;
    if (self->config != NULL) {
        if ((--self->config->__rc) <= 0) {
            NixosConfig_destroy(self->config);
        }
    }
    (self->config = config);
    (config->__rc++);
    if (self->shell != NULL) {
        if ((--self->shell->__rc) <= 0) {
            UnixShell_destroy(self->shell);
        }
    }
    (self->shell = UnixShell_new());
    (UnixShell_new()->__rc++);
}

SecureBootManager* SecureBootManager_new(NixosConfig* config) {
    SecureBootManager* self = ((SecureBootManager*)malloc(sizeof(SecureBootManager)));
    memset(self, 0, sizeof(SecureBootManager));
    SecureBootManager_init(self, config);
    return self;
}

void SecureBootManager_destroy(SecureBootManager* self) {
    if (self->config != NULL) {
        if ((--self->config->__rc) <= 0) {
            NixosConfig_destroy(self->config);
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

void SecureBootManager_removeOldEntries(SecureBootManager* self) {
    FileSystem_mkdirp("/boot/EFI/Linux");
    FileSystem_mkdirp("/var/lib/sbctl");
    UnixShell_runRaw(self->shell, "find /boot/EFI/Linux -type f -name 'linux-*.efi' -delete", false, false, "");
    FileSystem_removeRecursive("/etc/secureboot");
}

void SecureBootManager_createKeys(SecureBootManager* self) {
    Command* create = Command_new("sbctl");
    Command_arg(create, "create-keys");
    Command_capture(create, false);
    Command_check(create, false);
    UnixShell_runCommand(self->shell, create);
    if (create != NULL) {
        if ((--create->__rc) <= 0) {
            Command_destroy(create);
        }
    }
}

void SecureBootManager_enrollKeys(SecureBootManager* self, bool microsoft) {
    Command* enroll = Command_new("sbctl");
    Command_arg(enroll, "enroll-keys");
    if (microsoft) {
        Command_arg(enroll, "--microsoft");
    } else {
        Command_arg(enroll, "--yes-this-might-brick-my-machine");
    }
    Command_capture(enroll, false);
    UnixShell_runCommand(self->shell, enroll);
    if (enroll != NULL) {
        if ((--enroll->__rc) <= 0) {
            Command_destroy(enroll);
        }
    }
}

char* SecureBootManager_compactJson(SecureBootManager* self, char* text) {
    char* compact = Strings_replace(text, " ", "");
    (compact = Strings_replace(compact, "\n", ""));
    (compact = Strings_replace(compact, "\t", ""));
    (compact = Strings_replace(compact, "\r", ""));
    return compact;
}

btrc_Vector_string* SecureBootManager_unsignedFromJson(SecureBootManager* self, char* json) {
    btrc_Vector_string* result = btrc_Vector_string_new();
    int len = ((int)strlen(json));
    int i = 0;
    while (i < len) {
        while ((i < len) && (json[i] != ((char)34))) {
            (i++);
        }
        if (i >= len) {
            break;
        }
        int keyStart = (i + 1);
        (i = keyStart);
        while ((i < len) && (json[i] != ((char)34))) {
            (i++);
        }
        if (i >= len) {
            break;
        }
        char* key = JsonObject_slice(json, keyStart, i);
        (i++);
        while ((i < len) && (json[i] != '{')) {
            (i++);
        }
        if (i >= len) {
            break;
        }
        int bodyStart = i;
        int depth = 0;
        bool inString = false;
        bool escaped = false;
        for (; (i < len); (i++)) {
            char c = json[i];
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
            if (c == '{') {
                (depth++);
            }
            if (c == '}') {
                (depth--);
                if (depth == 0) {
                    char* body = JsonObject_slice(json, bodyStart, (i + 1));
                    if (__btrc_strContains(SecureBootManager_compactJson(self, body), "\"is_signed\":false")) {
                        btrc_Vector_string_push(result, key);
                    }
                    (i++);
                    break;
                }
            }
        }
    }
    return result;
}

void SecureBootManager_verify(SecureBootManager* self) {
    Command* verify = Command_check(Command_arg(Command_arg(Command_new("sbctl"), "verify"), "--json"), false);
    ExecResult* result = UnixShell_runCommand(self->shell, verify);
    char* output = __btrc_str_track(__btrc_trim(ExecResult_stdout(result)));
    if (__btrc_isEmpty(output) || (strcmp(output, "null") == 0)) {
        NixosLog_error("No EFI binaries registered");
        return;
    }
    btrc_Vector_string* unsignedPaths = SecureBootManager_unsignedFromJson(self, output);
    if (btrc_Vector_string_isEmpty(unsignedPaths)) {
        NixosLog_info("All EFI binaries are signed");
        return;
    }
    int __n_496 = btrc_Vector_string_iterLen(unsignedPaths);
    for (int __i_495 = 0; (__i_495 < __n_496); (__i_495++)) {
        char* path = btrc_Vector_string_iterGet(unsignedPaths, __i_495);
        NixosLog_error(__btrc_str_track(__btrc_strcat("NOT signed: ", path)));
    }
}

void SecureBootManager_enable(SecureBootManager* self, bool microsoft) {
    SecureBootManager_removeOldEntries(self);
    SecureBootManager_createKeys(self);
    SecureBootManager_enrollKeys(self, microsoft);
    LocalConfigFile_setString(self->config->local, "target", NixosConfig_secureBootTarget(self->config));
    RebuildOptions* options = RebuildOptions_new();
    (options->rebuildFileSystem = true);
    (options->clean = true);
    NixosRebuilder_update(NixosRebuilder_new(self->config), options);
    SecureBootManager_verify(self);
    if (options != NULL) {
        if ((--options->__rc) <= 0) {
            RebuildOptions_destroy(options);
        }
    }
}

void SecureBootManager_disable(SecureBootManager* self) {
    SecureBootManager_removeOldEntries(self);
    LocalConfigFile_setString(self->config->local, "target", NixosConfig_standardTarget(self->config));
    RebuildOptions* options = RebuildOptions_new();
    (options->rebuildFileSystem = true);
    (options->clean = true);
    NixosRebuilder_update(NixosRebuilder_new(self->config), options);
    if (options != NULL) {
        if ((--options->__rc) <= 0) {
            RebuildOptions_destroy(options);
        }
    }
}

void SecureBootManager_status(SecureBootManager* self) {
    Command* status = Command_check(Command_capture(Command_arg(Command_new("sbctl"), "status"), false), false);
    UnixShell_runCommand(self->shell, status);
    SecureBootManager_verify(self);
}

void Tpm2Manager_init(Tpm2Manager* self, NixosConfig* config) {
    self->__rc = 1;
    if (self->config != NULL) {
        if ((--self->config->__rc) <= 0) {
            NixosConfig_destroy(self->config);
        }
    }
    (self->config = config);
    (config->__rc++);
    if (self->shell != NULL) {
        if ((--self->shell->__rc) <= 0) {
            UnixShell_destroy(self->shell);
        }
    }
    (self->shell = UnixShell_new());
    (UnixShell_new()->__rc++);
}

Tpm2Manager* Tpm2Manager_new(NixosConfig* config) {
    Tpm2Manager* self = ((Tpm2Manager*)malloc(sizeof(Tpm2Manager)));
    memset(self, 0, sizeof(Tpm2Manager));
    Tpm2Manager_init(self, config);
    return self;
}

void Tpm2Manager_destroy(Tpm2Manager* self) {
    if (self->config != NULL) {
        if ((--self->config->__rc) <= 0) {
            NixosConfig_destroy(self->config);
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

bool Tpm2Manager_exists(Tpm2Manager* self) {
    if (!FileSystem_exists(NixosConfig_tpmDevice(self->config))) {
        __auto_type __btrc_ret_497 = false;
        return __btrc_ret_497;
    }
    char* version = __btrc_str_track(__btrc_trim(Path_readAll(NixosConfig_tpmVersionPath(self->config))));
    __auto_type __btrc_ret_498 = (strcmp(version, "2") == 0);
    return __btrc_ret_498;
}

bool Tpm2Manager_diskEncrypted(Tpm2Manager* self) {
    Command* cmd = Command_new("cryptsetup");
    Command_arg(cmd, "isLuks");
    Command_arg(cmd, NixosConfig_rootPartLabelPath(self->config));
    Command_check(cmd, false);
    ExecResult* result = UnixShell_runCommand(self->shell, cmd);
    __auto_type __btrc_ret_499 = ExecResult_ok(result);
    if (cmd != NULL) {
        if ((--cmd->__rc) <= 0) {
            Command_destroy(cmd);
        }
    }
    return __btrc_ret_499;
    if (cmd != NULL) {
        if ((--cmd->__rc) <= 0) {
            Command_destroy(cmd);
        }
    }
}

bool Tpm2Manager_enroll(Tpm2Manager* self) {
    Command* cmd = Command_new("systemd-cryptenroll");
    Command_arg(cmd, NixosConfig_rootPartLabelPath(self->config));
    Command_arg(cmd, "--wipe-slot=tpm2");
    Command_arg(cmd, __btrc_str_track(__btrc_strcat("--tpm2-device=", NixosConfig_tpmDevice(self->config))));
    Command_arg(cmd, "--tpm2-pcrs=7+12");
    Command_capture(cmd, false);
    Command_check(cmd, false);
    __auto_type __btrc_ret_500 = ExecResult_ok(UnixShell_runCommand(self->shell, cmd));
    if (cmd != NULL) {
        if ((--cmd->__rc) <= 0) {
            Command_destroy(cmd);
        }
    }
    return __btrc_ret_500;
    if (cmd != NULL) {
        if ((--cmd->__rc) <= 0) {
            Command_destroy(cmd);
        }
    }
}

bool Tpm2Manager_wipe(Tpm2Manager* self) {
    Command* cmd = Command_new("systemd-cryptenroll");
    Command_arg(cmd, NixosConfig_rootPartLabelPath(self->config));
    Command_arg(cmd, "--wipe-slot=tpm2");
    Command_capture(cmd, false);
    Command_check(cmd, false);
    __auto_type __btrc_ret_501 = ExecResult_ok(UnixShell_runCommand(self->shell, cmd));
    if (cmd != NULL) {
        if ((--cmd->__rc) <= 0) {
            Command_destroy(cmd);
        }
    }
    return __btrc_ret_501;
    if (cmd != NULL) {
        if ((--cmd->__rc) <= 0) {
            Command_destroy(cmd);
        }
    }
}

void Tpm2Manager_status(Tpm2Manager* self) {
    Console_log(__btrc_str_track(__btrc_strcat("TPM2 device: ", NixosConfig_tpmDevice(self->config))));
    Command* list = Command_check(Command_capture(Command_arg(Command_arg(Command_new("systemd-cryptenroll"), "--tpm2-device=list"), NixosConfig_rootPartLabelPath(self->config)), false), false);
    UnixShell_runCommand(self->shell, list);
    Command* dump = Command_check(Command_capture(Command_arg(Command_arg(Command_new("cryptsetup"), "luksDump"), NixosConfig_rootPartLabelPath(self->config)), false), false);
    UnixShell_runCommand(self->shell, dump);
}

void PasswordManager_init(PasswordManager* self, NixosConfig* config) {
    self->__rc = 1;
    if (self->config != NULL) {
        if ((--self->config->__rc) <= 0) {
            NixosConfig_destroy(self->config);
        }
    }
    (self->config = config);
    (config->__rc++);
    if (self->shell != NULL) {
        if ((--self->shell->__rc) <= 0) {
            UnixShell_destroy(self->shell);
        }
    }
    (self->shell = UnixShell_new());
    (UnixShell_new()->__rc++);
    if (self->secrets != NULL) {
        if ((--self->secrets->__rc) <= 0) {
            SecretsManager_destroy(self->secrets);
        }
    }
    (self->secrets = SecretsManager_new(config));
    (SecretsManager_new(config)->__rc++);
}

PasswordManager* PasswordManager_new(NixosConfig* config) {
    PasswordManager* self = ((PasswordManager*)malloc(sizeof(PasswordManager)));
    memset(self, 0, sizeof(PasswordManager));
    PasswordManager_init(self, config);
    return self;
}

void PasswordManager_destroy(PasswordManager* self) {
    if (self->config != NULL) {
        if ((--self->config->__rc) <= 0) {
            NixosConfig_destroy(self->config);
        }
    }
    if (self->shell != NULL) {
        if ((--self->shell->__rc) <= 0) {
            UnixShell_destroy(self->shell);
        }
    }
    if (self->secrets != NULL) {
        if ((--self->secrets->__rc) <= 0) {
            SecretsManager_destroy(self->secrets);
        }
    }
    if (__btrc_tracking) {
        __btrc_mark_destroyed(self);
    }
    free(self);
}

bool PasswordManager_changeLuksPassword(PasswordManager* self, char* oldPassword, char* newPassword) {
    char* command = __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("printf '%s\\n%s' ", UnixShell_quote(oldPassword))), " ")), UnixShell_quote(newPassword))), " | cryptsetup luksChangeKey ")), UnixShell_quote(NixosConfig_rootPartLabelPath(self->config))));
    ExecResult* result = UnixShell_runRaw(self->shell, command, false, false, oldPassword);
    __auto_type __btrc_ret_502 = ExecResult_ok(result);
    return __btrc_ret_502;
}

void PasswordManager_fallbackChangeUserPassword(PasswordManager* self, char* user, char* password) {
    char* command = __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("printf '%s:%s\\n' ", UnixShell_quote(user))), " ")), UnixShell_quote(password))), " | chpasswd"));
    ExecResult* changed = UnixShell_runRaw(self->shell, command, false, false, password);
    if (!ExecResult_ok(changed)) {
        NixosLog_error("Fallback account password update failed.");
    }
}

void PasswordManager_changeUserPassword(PasswordManager* self, char* oldPassword, char* password) {
    char* user = NixosConfig_username(self->config);
    if (!UnixPamPassword_change(user, oldPassword, password)) {
        NixosLog_error("PAM password update failed; falling back to chpasswd. KDE Wallet may need manual repair.");
        PasswordManager_fallbackChangeUserPassword(self, user, password);
    }
    char* hashed = SecretsManager_hashPassword(self->secrets, password);
    SecretsManager_writeHashedPassword(self->secrets, hashed);
    RebuildOptions* options = RebuildOptions_new();
    NixosRebuilder_update(NixosRebuilder_new(self->config), options);
    if (options != NULL) {
        if ((--options->__rc) <= 0) {
            RebuildOptions_destroy(options);
        }
    }
}

void PasswordManager_updateTpm2(PasswordManager* self) {
    Tpm2Manager* tpm = Tpm2Manager_new(self->config);
    if (!Tpm2Manager_exists(tpm)) {
        NixosLog_error("TPM2 does not exist");
        if (tpm != NULL) {
            if ((--tpm->__rc) <= 0) {
                Tpm2Manager_destroy(tpm);
            }
        }
        return;
    }
    if (!Tpm2Manager_enroll(tpm)) {
        NixosLog_error("TPM2 enrollment failed");
    }
    if (tpm != NULL) {
        if ((--tpm->__rc) <= 0) {
            Tpm2Manager_destroy(tpm);
        }
    }
}

void PasswordManager_change(PasswordManager* self, char* oldPassword, char* newPassword, bool changeFde, bool changeUser, bool updateTpm) {
    if (changeFde) {
        if (!PasswordManager_changeLuksPassword(self, oldPassword, newPassword)) {
            NixosLog_fatal("Failed to change LUKS password");
        }
        if (updateTpm) {
            PasswordManager_updateTpm2(self);
        }
    }
    if (changeUser) {
        PasswordManager_changeUserPassword(self, oldPassword, newPassword);
    }
}

void DisplayLayoutRule_init(DisplayLayoutRule* self) {
    self->__rc = 1;
    (self->display = "");
    (self->position = "");
    (self->relativeTo = "");
}

DisplayLayoutRule* DisplayLayoutRule_new(void) {
    DisplayLayoutRule* self = ((DisplayLayoutRule*)malloc(sizeof(DisplayLayoutRule)));
    memset(self, 0, sizeof(DisplayLayoutRule));
    DisplayLayoutRule_init(self);
    return self;
}

void DisplayLayoutRule_destroy(DisplayLayoutRule* self) {
    free(self);
}

void AudioPreset_init(AudioPreset* self) {
    self->__rc = 1;
    (self->label = "");
    (self->card = "");
    (self->profile = "");
    (self->sink = "");
    (self->volume = "");
}

AudioPreset* AudioPreset_new(void) {
    AudioPreset* self = ((AudioPreset*)malloc(sizeof(AudioPreset)));
    memset(self, 0, sizeof(AudioPreset));
    AudioPreset_init(self);
    return self;
}

void AudioPreset_destroy(AudioPreset* self) {
    free(self);
}

void LabelsConfig_init(LabelsConfig* self) {
    self->__rc = 1;
    (self->path = Environment_get("NIXOS_CONFIG", ""));
    (self->content = (__btrc_isEmpty(self->path) ? "" : Path_readAll(self->path)));
}

LabelsConfig* LabelsConfig_new(void) {
    LabelsConfig* self = ((LabelsConfig*)malloc(sizeof(LabelsConfig)));
    memset(self, 0, sizeof(LabelsConfig));
    LabelsConfig_init(self);
    return self;
}

void LabelsConfig_destroy(LabelsConfig* self) {
    free(self);
}

char* LabelsConfig_displayLabel(LabelsConfig* self, char* name) {
    bool inDisplays = false;
    int __n_504 = btrc_Vector_string_iterLen(Strings_split(self->content, "\n"));
    for (int __i_503 = 0; (__i_503 < __n_504); (__i_503++)) {
        char* line = btrc_Vector_string_iterGet(Strings_split(self->content, "\n"), __i_503);
        char* trimmed = __btrc_str_track(__btrc_trim(line));
        char* section = Toml_sectionName(trimmed);
        if (strcmp(section, "displays") == 0) {
            (inDisplays = true);
            continue;
        }
        if ((!__btrc_isEmpty(section)) || (!__btrc_isEmpty(Toml_tableArrayName(trimmed)))) {
            (inDisplays = false);
        }
        if (inDisplays && (strcmp(Toml_key(trimmed), name) == 0)) {
            __auto_type __btrc_ret_505 = Toml_value(trimmed);
            return __btrc_ret_505;
        }
    }
    return name;
}

btrc_Vector_DisplayLayoutRule* LabelsConfig_layoutRules(LabelsConfig* self) {
    btrc_Vector_DisplayLayoutRule* result = btrc_Vector_DisplayLayoutRule_new();
    DisplayLayoutRule* current = DisplayLayoutRule_new();
    bool inLayout = false;
    int __n_507 = btrc_Vector_string_iterLen(Strings_split(self->content, "\n"));
    for (int __i_506 = 0; (__i_506 < __n_507); (__i_506++)) {
        char* line = btrc_Vector_string_iterGet(Strings_split(self->content, "\n"), __i_506);
        char* trimmed = __btrc_str_track(__btrc_trim(line));
        char* table = Toml_tableArrayName(trimmed);
        if (strcmp(table, "layout") == 0) {
            if (inLayout && (!__btrc_isEmpty(current->display))) {
                btrc_Vector_DisplayLayoutRule_push(result, current);
            }
            (current = DisplayLayoutRule_new());
            (inLayout = true);
            continue;
        }
        if (((!__btrc_isEmpty(Toml_sectionName(trimmed))) || (!__btrc_isEmpty(table))) && (!(strcmp(table, "layout") == 0))) {
            if (inLayout && (!__btrc_isEmpty(current->display))) {
                btrc_Vector_DisplayLayoutRule_push(result, current);
            }
            (inLayout = false);
        }
        if ((!inLayout) || (Strings_find(trimmed, "=", 0) < 0)) {
            continue;
        }
        char* key = Toml_key(trimmed);
        char* value = Toml_value(trimmed);
        if (strcmp(key, "display") == 0) {
            (current->display = value);
        }
        if (strcmp(key, "position") == 0) {
            (current->position = value);
        }
        if (strcmp(key, "relative_to") == 0) {
            (current->relativeTo = value);
        }
    }
    if (inLayout && (!__btrc_isEmpty(current->display))) {
        btrc_Vector_DisplayLayoutRule_push(result, current);
    }
    if (current != NULL) {
        if ((--current->__rc) <= 0) {
            DisplayLayoutRule_destroy(current);
        }
    }
    return result;
    if (current != NULL) {
        if ((--current->__rc) <= 0) {
            DisplayLayoutRule_destroy(current);
        }
    }
}

btrc_Vector_AudioPreset* LabelsConfig_audioPresets(LabelsConfig* self) {
    btrc_Vector_AudioPreset* result = btrc_Vector_AudioPreset_new();
    AudioPreset* current = AudioPreset_new();
    bool inAudio = false;
    int __n_509 = btrc_Vector_string_iterLen(Strings_split(self->content, "\n"));
    for (int __i_508 = 0; (__i_508 < __n_509); (__i_508++)) {
        char* line = btrc_Vector_string_iterGet(Strings_split(self->content, "\n"), __i_508);
        char* trimmed = __btrc_str_track(__btrc_trim(line));
        char* table = Toml_tableArrayName(trimmed);
        if (strcmp(table, "audio") == 0) {
            if (inAudio && ((!__btrc_isEmpty(current->sink)) || (!__btrc_isEmpty(current->label)))) {
                btrc_Vector_AudioPreset_push(result, current);
            }
            (current = AudioPreset_new());
            (inAudio = true);
            continue;
        }
        if (((!__btrc_isEmpty(Toml_sectionName(trimmed))) || (!__btrc_isEmpty(table))) && (!(strcmp(table, "audio") == 0))) {
            if (inAudio && ((!__btrc_isEmpty(current->sink)) || (!__btrc_isEmpty(current->label)))) {
                btrc_Vector_AudioPreset_push(result, current);
            }
            (inAudio = false);
        }
        if ((!inAudio) || (Strings_find(trimmed, "=", 0) < 0)) {
            continue;
        }
        char* key = Toml_key(trimmed);
        char* value = Toml_value(trimmed);
        if (strcmp(key, "label") == 0) {
            (current->label = value);
        }
        if (strcmp(key, "card") == 0) {
            (current->card = value);
        }
        if (strcmp(key, "profile") == 0) {
            (current->profile = value);
        }
        if (strcmp(key, "sink") == 0) {
            (current->sink = value);
        }
        if (strcmp(key, "volume") == 0) {
            (current->volume = value);
        }
    }
    if (inAudio && ((!__btrc_isEmpty(current->sink)) || (!__btrc_isEmpty(current->label)))) {
        btrc_Vector_AudioPreset_push(result, current);
    }
    if (current != NULL) {
        if ((--current->__rc) <= 0) {
            AudioPreset_destroy(current);
        }
    }
    return result;
    if (current != NULL) {
        if ((--current->__rc) <= 0) {
            AudioPreset_destroy(current);
        }
    }
}

void AudioSink_init(AudioSink* self) {
    self->__rc = 1;
    (self->name = "");
    (self->description = "");
}

AudioSink* AudioSink_new(void) {
    AudioSink* self = ((AudioSink*)malloc(sizeof(AudioSink)));
    memset(self, 0, sizeof(AudioSink));
    AudioSink_init(self);
    return self;
}

void AudioSink_destroy(AudioSink* self) {
    free(self);
}

void AudioManager_init(AudioManager* self) {
    self->__rc = 1;
    if (self->shell != NULL) {
        if ((--self->shell->__rc) <= 0) {
            UnixShell_destroy(self->shell);
        }
    }
    (self->shell = UnixShell_new());
    (UnixShell_new()->__rc++);
    if (self->labels != NULL) {
        if ((--self->labels->__rc) <= 0) {
            LabelsConfig_destroy(self->labels);
        }
    }
    (self->labels = LabelsConfig_new());
    (LabelsConfig_new()->__rc++);
}

AudioManager* AudioManager_new(void) {
    AudioManager* self = ((AudioManager*)malloc(sizeof(AudioManager)));
    memset(self, 0, sizeof(AudioManager));
    AudioManager_init(self);
    return self;
}

void AudioManager_destroy(AudioManager* self) {
    if (self->shell != NULL) {
        if ((--self->shell->__rc) <= 0) {
            UnixShell_destroy(self->shell);
        }
    }
    if (self->labels != NULL) {
        if ((--self->labels->__rc) <= 0) {
            LabelsConfig_destroy(self->labels);
        }
    }
    if (__btrc_tracking) {
        __btrc_mark_destroyed(self);
    }
    free(self);
}

char* AudioManager_current(AudioManager* self) {
    ExecResult* result = UnixShell_runUnchecked(self->shell, "pactl get-default-sink");
    __auto_type __btrc_ret_510 = __btrc_str_track(__btrc_trim(ExecResult_stdout(result)));
    return __btrc_ret_510;
}

char* AudioManager_jsonStringValue(AudioManager* self, char* line, char* key) {
    char* marker = __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("\"", key)), "\""));
    int pos = Strings_find(line, marker, 0);
    if (pos < 0) {
        __auto_type __btrc_ret_511 = "";
        return __btrc_ret_511;
    }
    int colon = Strings_find(line, ":", pos);
    if (colon < 0) {
        __auto_type __btrc_ret_512 = "";
        return __btrc_ret_512;
    }
    int quote = Strings_find(line, "\"", (colon + 1));
    if (quote < 0) {
        __auto_type __btrc_ret_513 = "";
        return __btrc_ret_513;
    }
    int end = (quote + 1);
    bool escaped = false;
    while (end < ((int)strlen(line))) {
        char c = line[end];
        if ((!escaped) && (c == ((char)34))) {
            __auto_type __btrc_ret_514 = JsonObject_slice(line, (quote + 1), end);
            return __btrc_ret_514;
        }
        (escaped = ((!escaped) && (c == '\\')));
        if (c != '\\') {
            (escaped = false);
        }
        (end++);
    }
    __auto_type __btrc_ret_515 = "";
    return __btrc_ret_515;
}

btrc_Vector_AudioSink* AudioManager_sinks(AudioManager* self) {
    btrc_Vector_AudioSink* result = btrc_Vector_AudioSink_new();
    ExecResult* listed = UnixShell_runUnchecked(self->shell, "pactl -f json list sinks");
    AudioSink* current = AudioSink_new();
    int __n_517 = btrc_Vector_string_iterLen(Strings_split(ExecResult_stdout(listed), "\n"));
    for (int __i_516 = 0; (__i_516 < __n_517); (__i_516++)) {
        char* line = btrc_Vector_string_iterGet(Strings_split(ExecResult_stdout(listed), "\n"), __i_516);
        char* name = AudioManager_jsonStringValue(self, line, "name");
        if (!__btrc_isEmpty(name)) {
            if (!__btrc_isEmpty(current->name)) {
                btrc_Vector_AudioSink_push(result, current);
            }
            (current = AudioSink_new());
            (current->name = name);
            continue;
        }
        char* description = AudioManager_jsonStringValue(self, line, "description");
        if (!__btrc_isEmpty(description)) {
            (current->description = description);
        }
    }
    if (!__btrc_isEmpty(current->name)) {
        btrc_Vector_AudioSink_push(result, current);
    }
    if (current != NULL) {
        if ((--current->__rc) <= 0) {
            AudioSink_destroy(current);
        }
    }
    return result;
    if (current != NULL) {
        if ((--current->__rc) <= 0) {
            AudioSink_destroy(current);
        }
    }
}

char* AudioManager_entryJson(AudioManager* self, char* name, char* label, char* description, bool isDefault) {
    JsonObject* obj = JsonObject_new();
    JsonObject_setString(obj, "name", name);
    JsonObject_setString(obj, "label", label);
    if (!__btrc_isEmpty(description)) {
        JsonObject_setString(obj, "description", description);
    }
    JsonObject_setBool(obj, "default", isDefault);
    char* encoded = JsonObject_stringify(obj);
    if (obj != NULL) {
        if ((--obj->__rc) <= 0) {
            JsonObject_destroy(obj);
        }
    }
    return encoded;
    if (obj != NULL) {
        if ((--obj->__rc) <= 0) {
            JsonObject_destroy(obj);
        }
    }
}

void AudioManager_list(AudioManager* self) {
    char* currentSink = AudioManager_current(self);
    btrc_Vector_string* entries = btrc_Vector_string_new();
    btrc_Vector_string* covered = btrc_Vector_string_new();
    btrc_Vector_AudioPreset* presets = LabelsConfig_audioPresets(self->labels);
    int __n_519 = btrc_Vector_AudioPreset_iterLen(presets);
    for (int __i_518 = 0; (__i_518 < __n_519); (__i_518++)) {
        AudioPreset* preset = btrc_Vector_AudioPreset_iterGet(presets, __i_518);
        char* name = (__btrc_isEmpty(preset->sink) ? preset->label : preset->sink);
        char* label = (__btrc_isEmpty(preset->label) ? name : preset->label);
        btrc_Vector_string_push(entries, AudioManager_entryJson(self, name, label, "", ((!__btrc_isEmpty(preset->sink)) && (strcmp(preset->sink, currentSink) == 0))));
        if (!__btrc_isEmpty(preset->sink)) {
            btrc_Vector_string_push(covered, preset->sink);
        }
    }
    btrc_Vector_AudioSink* allSinks = AudioManager_sinks(self);
    int __n_521 = btrc_Vector_AudioSink_iterLen(allSinks);
    for (int __i_520 = 0; (__i_520 < __n_521); (__i_520++)) {
        AudioSink* sink = btrc_Vector_AudioSink_iterGet(allSinks, __i_520);
        if (!btrc_Vector_string_contains(covered, sink->name)) {
            char* label = (__btrc_isEmpty(sink->description) ? sink->name : sink->description);
            btrc_Vector_string_push(entries, AudioManager_entryJson(self, sink->name, label, sink->description, (strcmp(sink->name, currentSink) == 0)));
        }
    }
    Console_log(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("[", btrc_Vector_string_join(entries, ","))), "]")));
}

AudioPreset* AudioManager_findPreset(AudioManager* self, char* selector) {
    btrc_Vector_AudioPreset* presets = LabelsConfig_audioPresets(self->labels);
    int __n_523 = btrc_Vector_AudioPreset_iterLen(presets);
    for (int __i_522 = 0; (__i_522 < __n_523); (__i_522++)) {
        AudioPreset* preset = btrc_Vector_AudioPreset_iterGet(presets, __i_522);
        if ((strcmp(selector, preset->label) == 0) || (strcmp(selector, preset->sink) == 0)) {
            return preset;
        }
    }
    __auto_type __btrc_ret_524 = AudioPreset_new();
    return __btrc_ret_524;
}

void AudioManager_applyPreset(AudioManager* self, AudioPreset* preset) {
    if ((!__btrc_isEmpty(preset->card)) && (!__btrc_isEmpty(preset->profile))) {
        Command* profile = Command_check(Command_capture(Command_arg(Command_arg(Command_arg(Command_new("pactl"), "set-card-profile"), preset->card), preset->profile), false), false);
        UnixShell_runCommand(self->shell, profile);
    }
    if (!__btrc_isEmpty(preset->sink)) {
        Command* sink = Command_check(Command_capture(Command_arg(Command_arg(Command_new("pactl"), "set-default-sink"), preset->sink), false), false);
        UnixShell_runCommand(self->shell, sink);
    }
    if ((!__btrc_isEmpty(preset->sink)) && (!__btrc_isEmpty(preset->volume))) {
        Command* volume = Command_check(Command_capture(Command_arg(Command_arg(Command_arg(Command_new("pactl"), "set-sink-volume"), preset->sink), preset->volume), false), false);
        UnixShell_runCommand(self->shell, volume);
    }
}

void AudioManager_set(AudioManager* self, char* selector) {
    AudioPreset* preset = AudioManager_findPreset(self, selector);
    if ((!__btrc_isEmpty(preset->sink)) || (!__btrc_isEmpty(preset->label))) {
        AudioManager_applyPreset(self, preset);
        return;
    }
    Command* cmd = Command_check(Command_capture(Command_arg(Command_arg(Command_new("pactl"), "set-default-sink"), selector), false), false);
    UnixShell_runCommand(self->shell, cmd);
}

void CaffeineManager_init(CaffeineManager* self) {
    self->__rc = 1;
    if (self->shell != NULL) {
        if ((--self->shell->__rc) <= 0) {
            UnixShell_destroy(self->shell);
        }
    }
    (self->shell = UnixShell_new());
    (UnixShell_new()->__rc++);
    char* runtime = Environment_get("XDG_RUNTIME_DIR", "/tmp");
    (self->pidFile = PathTools_join(runtime, "nixos-caffeine.pid"));
}

CaffeineManager* CaffeineManager_new(void) {
    CaffeineManager* self = ((CaffeineManager*)malloc(sizeof(CaffeineManager)));
    memset(self, 0, sizeof(CaffeineManager));
    CaffeineManager_init(self);
    return self;
}

void CaffeineManager_destroy(CaffeineManager* self) {
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

int CaffeineManager_pid(CaffeineManager* self) {
    if (!FileSystem_exists(self->pidFile)) {
        __auto_type __btrc_ret_525 = 0;
        return __btrc_ret_525;
    }
    int existing = Strings_toInt(__btrc_str_track(__btrc_trim(Path_readAll(self->pidFile))));
    if (existing <= 0) {
        FileSystem_removeRecursive(self->pidFile);
        __auto_type __btrc_ret_526 = 0;
        return __btrc_ret_526;
    }
    ExecResult* alive = UnixShell_runUnchecked(self->shell, __btrc_str_track(__btrc_strcat("kill -0 ", Strings_fromInt(existing))));
    if (!ExecResult_ok(alive)) {
        FileSystem_removeRecursive(self->pidFile);
        __auto_type __btrc_ret_527 = 0;
        return __btrc_ret_527;
    }
    return existing;
}

bool CaffeineManager_enabled(CaffeineManager* self) {
    __auto_type __btrc_ret_528 = (CaffeineManager_pid(self) > 0);
    return __btrc_ret_528;
}

void CaffeineManager_enable(CaffeineManager* self) {
    if (CaffeineManager_enabled(self)) {
        return;
    }
    char* command = __btrc_str_track(__btrc_strcat("systemd-inhibit --what=idle --who=nixos --why='User disabled sleep' sleep infinity >/dev/null 2>&1 & echo $! > ", UnixShell_quote(self->pidFile)));
    UnixShell_runRaw(self->shell, command, false, true, "");
}

void CaffeineManager_disable(CaffeineManager* self) {
    int existing = CaffeineManager_pid(self);
    if (existing > 0) {
        UnixShell_runUnchecked(self->shell, __btrc_str_track(__btrc_strcat("kill ", Strings_fromInt(existing))));
    }
    FileSystem_removeRecursive(self->pidFile);
}

void CaffeineManager_toggle(CaffeineManager* self) {
    if (CaffeineManager_enabled(self)) {
        CaffeineManager_disable(self);
    } else {
        CaffeineManager_enable(self);
    }
}

void DisplayOutput_init(DisplayOutput* self) {
    self->__rc = 1;
    (self->name = "");
    (self->label = "");
    (self->kind = "");
    (self->priority = 999);
    (self->x = 0);
    (self->y = 0);
    (self->width = 0);
    (self->height = 0);
    (self->enabled = false);
    (self->connected = false);
}

DisplayOutput* DisplayOutput_new(void) {
    DisplayOutput* self = ((DisplayOutput*)malloc(sizeof(DisplayOutput)));
    memset(self, 0, sizeof(DisplayOutput));
    DisplayOutput_init(self);
    return self;
}

void DisplayOutput_destroy(DisplayOutput* self) {
    free(self);
}

char* DisplayOutput_json(DisplayOutput* self) {
    JsonObject* obj = JsonObject_new();
    JsonObject_setString(obj, "name", self->name);
    JsonObject_setString(obj, "label", self->label);
    JsonObject_setString(obj, "type", self->kind);
    JsonObject_setBool(obj, "enabled", self->enabled);
    JsonObject_setBool(obj, "connected", self->connected);
    JsonObject_setInt(obj, "priority", self->priority);
    JsonObject_setRaw(obj, "geometry", __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("{\"x\":", Strings_fromInt(self->x))), ",\"y\":")), Strings_fromInt(self->y))), ",\"w\":")), Strings_fromInt(self->width))), ",\"h\":")), Strings_fromInt(self->height))), "}")));
    char* encoded = JsonObject_stringify(obj);
    if (obj != NULL) {
        if ((--obj->__rc) <= 0) {
            JsonObject_destroy(obj);
        }
    }
    return encoded;
    if (obj != NULL) {
        if ((--obj->__rc) <= 0) {
            JsonObject_destroy(obj);
        }
    }
}

void DisplayManager_init(DisplayManager* self) {
    self->__rc = 1;
    if (self->shell != NULL) {
        if ((--self->shell->__rc) <= 0) {
            UnixShell_destroy(self->shell);
        }
    }
    (self->shell = UnixShell_new());
    (UnixShell_new()->__rc++);
    if (self->labels != NULL) {
        if ((--self->labels->__rc) <= 0) {
            LabelsConfig_destroy(self->labels);
        }
    }
    (self->labels = LabelsConfig_new());
    (LabelsConfig_new()->__rc++);
}

DisplayManager* DisplayManager_new(void) {
    DisplayManager* self = ((DisplayManager*)malloc(sizeof(DisplayManager)));
    memset(self, 0, sizeof(DisplayManager));
    DisplayManager_init(self);
    return self;
}

void DisplayManager_destroy(DisplayManager* self) {
    if (self->shell != NULL) {
        if ((--self->shell->__rc) <= 0) {
            UnixShell_destroy(self->shell);
        }
    }
    if (self->labels != NULL) {
        if ((--self->labels->__rc) <= 0) {
            LabelsConfig_destroy(self->labels);
        }
    }
    if (__btrc_tracking) {
        __btrc_mark_destroyed(self);
    }
    free(self);
}

char* DisplayManager_kscreen(DisplayManager* self) {
    ExecResult* result = UnixShell_runUnchecked(self->shell, "kscreen-doctor -o | sed -E 's/\\x1b\\[[0-9;]*m//g'");
    __auto_type __btrc_ret_529 = ExecResult_stdout(result);
    return __btrc_ret_529;
}

bool DisplayManager_drmConnected(DisplayManager* self, char* name) {
    char* status = PathTools_join(PathTools_join("/sys/class/drm", __btrc_str_track(__btrc_strcat("card*-", name))), "status");
    ExecResult* result = UnixShell_runUnchecked(self->shell, __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("cat ", status)), " 2>/dev/null | head -n 1")));
    __auto_type __btrc_ret_530 = (strcmp(__btrc_str_track(__btrc_trim(ExecResult_stdout(result))), "connected") == 0);
    return __btrc_ret_530;
}

char* DisplayManager_lineValue(DisplayManager* self, char* block, char* prefix) {
    int __n_532 = btrc_Vector_string_iterLen(Strings_split(block, "\n"));
    for (int __i_531 = 0; (__i_531 < __n_532); (__i_531++)) {
        char* line = btrc_Vector_string_iterGet(Strings_split(block, "\n"), __i_531);
        char* trimmed = __btrc_str_track(__btrc_trim(line));
        if (__btrc_startsWith(trimmed, prefix)) {
            __auto_type __btrc_ret_533 = __btrc_str_track(__btrc_trim(__btrc_str_track(__btrc_substring(trimmed, ((int)strlen(prefix)), (((int)strlen(trimmed)) - ((int)strlen(prefix)))))));
            return __btrc_ret_533;
        }
    }
    __auto_type __btrc_ret_534 = "";
    return __btrc_ret_534;
}

DisplayOutput* DisplayManager_parseBlock(DisplayManager* self, char* block) {
    DisplayOutput* output = DisplayOutput_new();
    btrc_Vector_string* lines = Strings_split(block, "\n");
    if (lines->len == 0) {
        return output;
    }
    char* header = __btrc_str_track(__btrc_trim(btrc_Vector_string_get(lines, 0)));
    btrc_Vector_string* headerParts = Strings_split(header, " ");
    if (headerParts->len < 2) {
        return output;
    }
    (output->name = btrc_Vector_string_get(headerParts, 1));
    (output->label = LabelsConfig_displayLabel(self->labels, output->name));
    (output->enabled = __btrc_strContains(block, "\tenabled"));
    (output->connected = (__btrc_strContains(block, "\tconnected") || DisplayManager_drmConnected(self, output->name)));
    char* priority = DisplayManager_lineValue(self, block, "priority ");
    if (!__btrc_isEmpty(priority)) {
        (output->priority = Strings_toInt(priority));
    }
    btrc_Vector_string* __list_535 = btrc_Vector_string_new();
    btrc_Vector_string_push(__list_535, "HDMI");
    btrc_Vector_string_push(__list_535, "DisplayPort");
    btrc_Vector_string_push(__list_535, "VGA");
    btrc_Vector_string_push(__list_535, "DVI");
    btrc_Vector_string_push(__list_535, "Panel");
    btrc_Vector_string_push(__list_535, "TV");
    btrc_Vector_string_push(__list_535, "Unknown");
    int __n_537 = btrc_Vector_string_iterLen(__list_535);
    for (int __i_536 = 0; (__i_536 < __n_537); (__i_536++)) {
        btrc_Vector_string* __list_535 = btrc_Vector_string_new();
        btrc_Vector_string_push(__list_535, "HDMI");
        btrc_Vector_string_push(__list_535, "DisplayPort");
        btrc_Vector_string_push(__list_535, "VGA");
        btrc_Vector_string_push(__list_535, "DVI");
        btrc_Vector_string_push(__list_535, "Panel");
        btrc_Vector_string_push(__list_535, "TV");
        btrc_Vector_string_push(__list_535, "Unknown");
        char* kind = btrc_Vector_string_iterGet(__list_535, __i_536);
        if (__btrc_strContains(block, __btrc_str_track(__btrc_strcat("\t", kind)))) {
            (output->kind = kind);
        }
    }
    char* geometry = DisplayManager_lineValue(self, block, "Geometry: ");
    if (!__btrc_isEmpty(geometry)) {
        btrc_Vector_string* parts = Strings_split(geometry, " ");
        if (parts->len >= 2) {
            btrc_Vector_string* xy = Strings_split(btrc_Vector_string_get(parts, 0), ",");
            btrc_Vector_string* wh = Strings_split(btrc_Vector_string_get(parts, 1), "x");
            if (xy->len >= 2) {
                (output->x = Strings_toInt(btrc_Vector_string_get(xy, 0)));
                (output->y = Strings_toInt(btrc_Vector_string_get(xy, 1)));
            }
            if (wh->len >= 2) {
                (output->width = Strings_toInt(btrc_Vector_string_get(wh, 0)));
                (output->height = Strings_toInt(btrc_Vector_string_get(wh, 1)));
            }
        }
    }
    return output;
    if (output != NULL) {
        if ((--output->__rc) <= 0) {
            DisplayOutput_destroy(output);
        }
    }
}

btrc_Vector_DisplayOutput* DisplayManager_outputs(DisplayManager* self) {
    btrc_Vector_DisplayOutput* result = btrc_Vector_DisplayOutput_new();
    int __n_539 = btrc_Vector_string_iterLen(Strings_split(DisplayManager_kscreen(self), "Output: "));
    for (int __i_538 = 0; (__i_538 < __n_539); (__i_538++)) {
        char* block = btrc_Vector_string_iterGet(Strings_split(DisplayManager_kscreen(self), "Output: "), __i_538);
        char* trimmed = __btrc_str_track(__btrc_trim(block));
        if (__btrc_isEmpty(trimmed)) {
            continue;
        }
        DisplayOutput* output = DisplayManager_parseBlock(self, trimmed);
        if ((!__btrc_isEmpty(output->name)) && (output->connected || output->enabled)) {
            btrc_Vector_DisplayOutput_push(result, output);
        }
    }
    return result;
}

DisplayOutput* DisplayManager_findOutput(DisplayManager* self, char* name, btrc_Vector_DisplayOutput* outputs) {
    int __n_541 = btrc_Vector_DisplayOutput_iterLen(outputs);
    for (int __i_540 = 0; (__i_540 < __n_541); (__i_540++)) {
        DisplayOutput* output = btrc_Vector_DisplayOutput_iterGet(outputs, __i_540);
        if (strcmp(output->name, name) == 0) {
            return output;
        }
    }
    __auto_type __btrc_ret_542 = DisplayOutput_new();
    return __btrc_ret_542;
}

void DisplayManager_run(DisplayManager* self, char* arg) {
    Command* cmd = Command_check(Command_capture(Command_arg(Command_new("kscreen-doctor"), arg), false), false);
    UnixShell_runCommand(self->shell, cmd);
}

void DisplayManager_enable(DisplayManager* self, char* name) {
    DisplayManager_run(self, __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("output.", name)), ".enable")));
    DisplayManager_applyLayout(self, name);
}

void DisplayManager_disable(DisplayManager* self, char* name) {
    DisplayManager_run(self, __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("output.", name)), ".disable")));
}

void DisplayManager_primary(DisplayManager* self, char* name) {
    DisplayManager_run(self, __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("output.", name)), ".priority.1")));
}

void DisplayManager_dpms(DisplayManager* self, char* state) {
    Command* cmd = Command_check(Command_capture(Command_arg(Command_arg(Command_new("kscreen-doctor"), "--dpms"), state), false), false);
    UnixShell_runCommand(self->shell, cmd);
}

void DisplayManager_layout(DisplayManager* self) {
    btrc_Vector_DisplayLayoutRule* rules = LabelsConfig_layoutRules(self->labels);
    int __n_544 = btrc_Vector_DisplayLayoutRule_iterLen(rules);
    for (int __i_543 = 0; (__i_543 < __n_544); (__i_543++)) {
        DisplayLayoutRule* rule = btrc_Vector_DisplayLayoutRule_iterGet(rules, __i_543);
        DisplayManager_applyLayout(self, rule->display);
    }
}

void DisplayManager_applyLayout(DisplayManager* self, char* name) {
    DisplayLayoutRule* selected = DisplayLayoutRule_new();
    btrc_Vector_DisplayLayoutRule* rules = LabelsConfig_layoutRules(self->labels);
    int __n_546 = btrc_Vector_DisplayLayoutRule_iterLen(rules);
    for (int __i_545 = 0; (__i_545 < __n_546); (__i_545++)) {
        DisplayLayoutRule* rule = btrc_Vector_DisplayLayoutRule_iterGet(rules, __i_545);
        if (strcmp(rule->display, name) == 0) {
            (selected = rule);
        }
    }
    if (__btrc_isEmpty(selected->display)) {
        if (selected != NULL) {
            if ((--selected->__rc) <= 0) {
                DisplayLayoutRule_destroy(selected);
            }
        }
        return;
    }
    btrc_Vector_DisplayOutput* all = DisplayManager_outputs(self);
    DisplayOutput* display = DisplayManager_findOutput(self, selected->display, all);
    DisplayOutput* anchor = DisplayManager_findOutput(self, selected->relativeTo, all);
    if ((__btrc_isEmpty(display->name) || __btrc_isEmpty(anchor->name)) || (!display->enabled)) {
        if (selected != NULL) {
            if ((--selected->__rc) <= 0) {
                DisplayLayoutRule_destroy(selected);
            }
        }
        return;
    }
    int px = anchor->x;
    int py = anchor->y;
    if (strcmp(selected->position, "left-of") == 0) {
        (px = (anchor->x - display->width));
        (py = anchor->y);
    }
    if (strcmp(selected->position, "right-of") == 0) {
        (px = (anchor->x + anchor->width));
        (py = anchor->y);
    }
    if (strcmp(selected->position, "above") == 0) {
        (px = anchor->x);
        (py = (anchor->y - display->height));
    }
    if (strcmp(selected->position, "below") == 0) {
        (px = anchor->x);
        (py = (anchor->y + anchor->height));
    }
    DisplayManager_run(self, __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("output.", display->name)), ".position.")), Strings_fromInt(px))), ",")), Strings_fromInt(py))));
    if (selected != NULL) {
        if ((--selected->__rc) <= 0) {
            DisplayLayoutRule_destroy(selected);
        }
    }
}

void DisplayManager_list(DisplayManager* self) {
    btrc_Vector_string* rows = btrc_Vector_string_new();
    btrc_Vector_DisplayOutput* all = DisplayManager_outputs(self);
    int __n_548 = btrc_Vector_DisplayOutput_iterLen(all);
    for (int __i_547 = 0; (__i_547 < __n_548); (__i_547++)) {
        DisplayOutput* output = btrc_Vector_DisplayOutput_iterGet(all, __i_547);
        btrc_Vector_string_push(rows, DisplayOutput_json(output));
    }
    Console_log(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("[", btrc_Vector_string_join(rows, ","))), "]")));
}

void SystemUi_init(SystemUi* self) {
    self->__rc = 1;
    if (self->shell != NULL) {
        if ((--self->shell->__rc) <= 0) {
            UnixShell_destroy(self->shell);
        }
    }
    (self->shell = UnixShell_new());
    (UnixShell_new()->__rc++);
}

SystemUi* SystemUi_new(void) {
    SystemUi* self = ((SystemUi*)malloc(sizeof(SystemUi)));
    memset(self, 0, sizeof(SystemUi));
    SystemUi_init(self);
    return self;
}

void SystemUi_destroy(SystemUi* self) {
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

void SystemUi_terminal(SystemUi* self, char* command) {
    Command* cmd = Command_arg(Command_arg(Command_arg(Command_arg(Command_new("konsole"), "-e"), "bash"), "-c"), __btrc_str_track(__btrc_strcat(command, "; echo; read -rp 'Press enter to close...'")));
    Command_capture(cmd, false);
    Command_check(cmd, false);
    UnixShell_runCommand(self->shell, cmd);
}

void SystemUi_update(SystemUi* self) {
    SystemUi_terminal(self, "sudo nixosctl update");
}

void SystemUi_upgrade(SystemUi* self) {
    SystemUi_terminal(self, "sudo nixosctl upgrade");
}

void VmOperation_init(VmOperation* self) {
    self->__rc = 1;
    (self->kind = "");
    (self->command = "");
    (self->expect = "");
    (self->name = "");
    (self->localPath = "");
    (self->remotePath = "");
    (self->timeout = 120);
}

VmOperation* VmOperation_new(void) {
    VmOperation* self = ((VmOperation*)malloc(sizeof(VmOperation)));
    memset(self, 0, sizeof(VmOperation));
    VmOperation_init(self);
    return self;
}

void VmOperation_destroy(VmOperation* self) {
    free(self);
}

void VmOperation_expandArgs(VmOperation* self, btrc_Map_string_string* args) {
    (self->kind = VmSpecParser_expandArgs(self->kind, args));
    (self->command = VmSpecParser_expandArgs(self->command, args));
    (self->expect = VmSpecParser_expandArgs(self->expect, args));
    (self->name = VmSpecParser_expandArgs(self->name, args));
    (self->localPath = VmSpecParser_expandArgs(self->localPath, args));
    (self->remotePath = VmSpecParser_expandArgs(self->remotePath, args));
}

void VmTestSpec_init(VmTestSpec* self) {
    self->__rc = 1;
    (self->name = "vm-test");
    (self->workDir = ".vm/e2e/vm-test");
    (self->arch = VmSpecParser_hostArch());
    (self->iso = "");
    (self->isoUrl = "");
    (self->diskSize = "30G");
    (self->memory = "4G");
    (self->cpus = 4);
    (self->sshPort = 2222);
    (self->state = "vm-test");
    (self->parentState = "root");
    (self->stateRoot = ".vm/e2e/chain");
    (self->stateMaterial = "");
    (self->parentHash = "root");
    (self->stateHash = "");
    (self->stateHashShort = "");
    if (self->args != NULL) {
        if ((--self->args->__rc) <= 0) {
            btrc_Map_string_string_free(self->args);
        }
    }
    (self->args = btrc_Map_string_string_new());
    (btrc_Map_string_string_new()->__rc++);
    if (self->operations != NULL) {
        if ((--self->operations->__rc) <= 0) {
            btrc_Vector_VmOperation_free(self->operations);
        }
    }
    btrc_Vector_VmOperation* __list_550 = btrc_Vector_VmOperation_new();
    (self->operations = __list_550);
    btrc_Vector_VmOperation* __list_549 = btrc_Vector_VmOperation_new();
    (__list_549->__rc++);
}

VmTestSpec* VmTestSpec_new(void) {
    VmTestSpec* self = ((VmTestSpec*)malloc(sizeof(VmTestSpec)));
    memset(self, 0, sizeof(VmTestSpec));
    VmTestSpec_init(self);
    return self;
}

void VmTestSpec_destroy(VmTestSpec* self) {
    if (self->args != NULL) {
        if ((--self->args->__rc) <= 0) {
            btrc_Map_string_string_free(self->args);
        }
    }
    if (self->operations != NULL) {
        if ((--self->operations->__rc) <= 0) {
            btrc_Vector_VmOperation_free(self->operations);
        }
    }
    if (__btrc_tracking) {
        __btrc_mark_destroyed(self);
    }
    free(self);
}

void VmTestSpec_setArg(VmTestSpec* self, char* key, char* value) {
    btrc_Map_string_string_put(self->args, key, value);
}

void VmTestSpec_setArgPair(VmTestSpec* self, char* pair) {
    int pos = Strings_find(pair, "=", 0);
    if (pos <= 0) {
        NixosLog_fatal(__btrc_str_track(__btrc_strcat("Expected --arg key=value, got ", pair)));
    }
    char* key = JsonObject_slice(pair, 0, pos);
    char* value = JsonObject_slice(pair, (pos + 1), ((int)strlen(pair)));
    VmTestSpec_setArg(self, key, value);
}

void VmTestSpec_setDerivedArgs(VmTestSpec* self) {
    btrc_Map_string_string_putIfAbsent(self->args, "name", self->name);
    btrc_Map_string_string_putIfAbsent(self->args, "workDir", self->workDir);
    btrc_Map_string_string_putIfAbsent(self->args, "arch", self->arch);
    btrc_Map_string_string_putIfAbsent(self->args, "iso", self->iso);
    btrc_Map_string_string_putIfAbsent(self->args, "isoUrl", self->isoUrl);
    btrc_Map_string_string_putIfAbsent(self->args, "diskSize", self->diskSize);
    btrc_Map_string_string_putIfAbsent(self->args, "memory", self->memory);
    btrc_Map_string_string_putIfAbsent(self->args, "cpus", Strings_fromInt(self->cpus));
    btrc_Map_string_string_putIfAbsent(self->args, "sshPort", Strings_fromInt(self->sshPort));
    btrc_Map_string_string_putIfAbsent(self->args, "state", self->state);
    btrc_Map_string_string_putIfAbsent(self->args, "parentState", self->parentState);
    btrc_Map_string_string_putIfAbsent(self->args, "stateRoot", self->stateRoot);
    btrc_Map_string_string_putIfAbsent(self->args, "stateMaterial", self->stateMaterial);
    btrc_Map_string_string_putIfAbsent(self->args, "parentHash", self->parentHash);
    btrc_Map_string_string_putIfAbsent(self->args, "stateHash", self->stateHash);
    btrc_Map_string_string_putIfAbsent(self->args, "stateHashShort", self->stateHashShort);
}

void VmTestSpec_refreshDerivedArgs(VmTestSpec* self) {
    btrc_Map_string_string_put(self->args, "name", self->name);
    btrc_Map_string_string_put(self->args, "workDir", self->workDir);
    btrc_Map_string_string_put(self->args, "arch", self->arch);
    btrc_Map_string_string_put(self->args, "iso", self->iso);
    btrc_Map_string_string_put(self->args, "isoUrl", self->isoUrl);
    btrc_Map_string_string_put(self->args, "diskSize", self->diskSize);
    btrc_Map_string_string_put(self->args, "memory", self->memory);
    btrc_Map_string_string_put(self->args, "cpus", Strings_fromInt(self->cpus));
    btrc_Map_string_string_put(self->args, "sshPort", Strings_fromInt(self->sshPort));
    btrc_Map_string_string_put(self->args, "state", self->state);
    btrc_Map_string_string_put(self->args, "parentState", self->parentState);
    btrc_Map_string_string_put(self->args, "stateRoot", self->stateRoot);
    btrc_Map_string_string_put(self->args, "stateMaterial", self->stateMaterial);
    btrc_Map_string_string_put(self->args, "parentHash", self->parentHash);
    btrc_Map_string_string_put(self->args, "stateHash", self->stateHash);
    btrc_Map_string_string_put(self->args, "stateHashShort", self->stateHashShort);
}

char* VmTestSpec_stateDir(VmTestSpec* self) {
    __auto_type __btrc_ret_551 = PathTools_join(self->stateRoot, self->state);
    return __btrc_ret_551;
}

char* VmTestSpec_stateHashFile(VmTestSpec* self) {
    __auto_type __btrc_ret_552 = PathTools_join(VmTestSpec_stateDir(self), "hash");
    return __btrc_ret_552;
}

char* VmTestSpec_parentHashFile(VmTestSpec* self) {
    __auto_type __btrc_ret_553 = PathTools_join(PathTools_join(self->stateRoot, self->parentState), "hash");
    return __btrc_ret_553;
}

char* VmTestSpec_resolveParentHash(VmTestSpec* self) {
    if ((strcmp(self->parentState, "root") == 0) || __btrc_isEmpty(self->parentState)) {
        __auto_type __btrc_ret_554 = "root";
        return __btrc_ret_554;
    }
    char* path = VmTestSpec_parentHashFile(self);
    if (FileSystem_exists(path)) {
        __auto_type __btrc_ret_555 = __btrc_str_track(__btrc_trim(Path_readAll(path)));
        return __btrc_ret_555;
    }
    __auto_type __btrc_ret_556 = __btrc_str_track(__btrc_strcat("missing:", self->parentState));
    return __btrc_ret_556;
}

char* VmTestSpec_operationsMaterial(VmTestSpec* self) {
    btrc_Vector_string* lines = btrc_Vector_string_new();
    int __n_558 = btrc_Vector_VmOperation_iterLen(self->operations);
    for (int __i_557 = 0; (__i_557 < __n_558); (__i_557++)) {
        VmOperation* op = btrc_Vector_VmOperation_iterGet(self->operations, __i_557);
        btrc_Vector_string_push(lines, __btrc_str_track(__btrc_strcat("op=", op->kind)));
        btrc_Vector_string_push(lines, __btrc_str_track(__btrc_strcat("name=", op->name)));
        btrc_Vector_string_push(lines, __btrc_str_track(__btrc_strcat("command=", op->command)));
        btrc_Vector_string_push(lines, __btrc_str_track(__btrc_strcat("expect=", op->expect)));
        btrc_Vector_string_push(lines, __btrc_str_track(__btrc_strcat("local=", op->localPath)));
        btrc_Vector_string_push(lines, __btrc_str_track(__btrc_strcat("remote=", op->remotePath)));
        btrc_Vector_string_push(lines, __btrc_str_track(__btrc_strcat("timeout=", Strings_fromInt(op->timeout))));
    }
    __auto_type __btrc_ret_559 = btrc_Vector_string_join(lines, "\n");
    return __btrc_ret_559;
}

char* VmTestSpec_hashMaterial(VmTestSpec* self) {
    char* material = self->stateMaterial;
    if (__btrc_isEmpty(material)) {
        int __fstr_560_len = snprintf(NULL, 0, "%s:%s:%s:%s:%s:%d:%d", self->name, self->arch, self->isoUrl, self->diskSize, self->memory, self->cpus, self->sshPort);
        char* __fstr_560_buf = __btrc_str_track(((char*)malloc((__fstr_560_len + 1))));
        snprintf(__fstr_560_buf, (__fstr_560_len + 1), "%s:%s:%s:%s:%s:%d:%d", self->name, self->arch, self->isoUrl, self->diskSize, self->memory, self->cpus, self->sshPort);
        (material = __fstr_560_buf);
    }
    int __fstr_561_len = snprintf(NULL, 0, "parent=%s\nstate=%s\nmaterial=%s\noperations=\n%s\n", self->parentHash, self->state, material, VmTestSpec_operationsMaterial(self));
    char* __fstr_561_buf = __btrc_str_track(((char*)malloc((__fstr_561_len + 1))));
    snprintf(__fstr_561_buf, (__fstr_561_len + 1), "parent=%s\nstate=%s\nmaterial=%s\noperations=\n%s\n", self->parentHash, self->state, material, VmTestSpec_operationsMaterial(self));
    __auto_type __btrc_ret_562 = __fstr_561_buf;
    return __btrc_ret_562;
}

void VmTestSpec_computeStateHash(VmTestSpec* self) {
    (self->parentHash = VmTestSpec_resolveParentHash(self));
    char* command = __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("printf %s ", UnixShell_quote(VmTestSpec_hashMaterial(self)))), " | (if command -v sha256sum >/dev/null 2>&1; then sha256sum; else shasum -a 256; fi) | awk '{print $1}'"));
    ExecResult* result = UnixShell_run(UnixShell_new(), command);
    if (!ExecResult_ok(result)) {
        NixosLog_fatal(__btrc_str_track(__btrc_strcat("Failed to compute state hash for ", self->state)));
    }
    (self->stateHash = __btrc_str_track(__btrc_trim(ExecResult_stdout(result))));
    (self->stateHashShort = __btrc_str_track(__btrc_substring(self->stateHash, 0, 12)));
}

void VmTestSpec_expandArgs(VmTestSpec* self) {
    char* rawWorkDir = self->workDir;
    VmTestSpec_setDerivedArgs(self);
    (self->name = VmSpecParser_expandArgs(self->name, self->args));
    (self->arch = VmSpecParser_expandArgs(self->arch, self->args));
    (self->iso = VmSpecParser_expandArgs(self->iso, self->args));
    (self->isoUrl = VmSpecParser_expandArgs(self->isoUrl, self->args));
    (self->diskSize = VmSpecParser_expandArgs(self->diskSize, self->args));
    (self->memory = VmSpecParser_expandArgs(self->memory, self->args));
    (self->state = VmSpecParser_expandArgs(self->state, self->args));
    (self->parentState = VmSpecParser_expandArgs(self->parentState, self->args));
    (self->stateRoot = VmSpecParser_expandArgs(self->stateRoot, self->args));
    (self->stateMaterial = VmSpecParser_expandArgs(self->stateMaterial, self->args));
    VmTestSpec_refreshDerivedArgs(self);
    VmTestSpec_computeStateHash(self);
    VmTestSpec_refreshDerivedArgs(self);
    (self->workDir = VmSpecParser_expandArgs(rawWorkDir, self->args));
    VmTestSpec_refreshDerivedArgs(self);
    int __n_564 = btrc_Vector_VmOperation_iterLen(self->operations);
    for (int __i_563 = 0; (__i_563 < __n_564); (__i_563++)) {
        VmOperation* op = btrc_Vector_VmOperation_iterGet(self->operations, __i_563);
        VmOperation_expandArgs(op, self->args);
    }
}

void VmSpecParser_init(VmSpecParser* self) {
    self->__rc = 1;
}

void VmSpecParser_destroy(VmSpecParser* self) {
    free(self);
}

char* VmSpecParser_hostArch(void) {
    UnixShell* shell = UnixShell_new();
    ExecResult* result = UnixShell_run(shell, "uname -m");
    char* machine = __btrc_str_track(__btrc_trim(ExecResult_stdout(result)));
    if (__btrc_strContains(machine, "arm64") || __btrc_strContains(machine, "aarch64")) {
        __auto_type __btrc_ret_565 = "aarch64";
        if (shell != NULL) {
            if ((--shell->__rc) <= 0) {
                UnixShell_destroy(shell);
            }
        }
        return __btrc_ret_565;
    }
    __auto_type __btrc_ret_566 = "x86_64";
    if (shell != NULL) {
        if ((--shell->__rc) <= 0) {
            UnixShell_destroy(shell);
        }
    }
    return __btrc_ret_566;
    if (shell != NULL) {
        if ((--shell->__rc) <= 0) {
            UnixShell_destroy(shell);
        }
    }
}

char* VmSpecParser_defaultIsoUrl(char* arch) {
    __auto_type __btrc_ret_567 = __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("https://channels.nixos.org/nixos-unstable/latest-nixos-minimal-", arch)), "-linux.iso"));
    return __btrc_ret_567;
}

char* VmSpecParser_expandArgs(char* text, btrc_Map_string_string* args) {
    char* result = Strings_copy(text);
    int __n_569 = btrc_Map_string_string_iterLen(args);
    for (int __i_568 = 0; (__i_568 < __n_569); (__i_568++)) {
        char* key = btrc_Map_string_string_iterGet(args, __i_568);
        char* value = btrc_Map_string_string_iterValueAt(args, __i_568);
        (result = Strings_replace(result, __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("{{", key)), "}}")), value));
    }
    int __n_571 = btrc_Map_string_string_iterLen(args);
    for (int __i_570 = 0; (__i_570 < __n_571); (__i_570++)) {
        char* key = btrc_Map_string_string_iterGet(args, __i_570);
        char* value = btrc_Map_string_string_iterValueAt(args, __i_570);
        (result = Strings_replace(result, __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("${", key)), "}")), value));
    }
    return result;
}

int VmSpecParser_skipSpaces(char* text, int i) {
    int len = ((int)strlen(text));
    while ((i < len) && ((((text[i] == ' ') || (text[i] == '\n')) || (text[i] == '\t')) || (text[i] == '\r'))) {
        (i++);
    }
    return i;
}

int VmSpecParser_keyPosition(char* text, char* key) {
    __auto_type __btrc_ret_572 = Strings_find(text, __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("\"", key)), "\"")), 0);
    return __btrc_ret_572;
}

char* VmSpecParser_objectField(char* text, char* key) {
    int pos = VmSpecParser_keyPosition(text, key);
    if (pos < 0) {
        __auto_type __btrc_ret_573 = "";
        return __btrc_ret_573;
    }
    int colon = Strings_find(text, ":", pos);
    if (colon < 0) {
        __auto_type __btrc_ret_574 = "";
        return __btrc_ret_574;
    }
    int i = VmSpecParser_skipSpaces(text, (colon + 1));
    int len = ((int)strlen(text));
    if ((i >= len) || (text[i] != '{')) {
        __auto_type __btrc_ret_575 = "";
        return __btrc_ret_575;
    }
    int depth = 0;
    bool inString = false;
    bool escaped = false;
    for (int j = i; (j < len); (j++)) {
        char c = text[j];
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
        if (c == '{') {
            (depth++);
            continue;
        }
        if (c == '}') {
            (depth--);
            if (depth == 0) {
                __auto_type __btrc_ret_576 = JsonObject_slice(text, i, (j + 1));
                return __btrc_ret_576;
            }
        }
    }
    __auto_type __btrc_ret_577 = "";
    return __btrc_ret_577;
}

btrc_Map_string_string* VmSpecParser_argsObject(char* text) {
    btrc_Map_string_string* result = btrc_Map_string_string_new();
    char* objectText = VmSpecParser_objectField(text, "args");
    if (__btrc_isEmpty(objectText)) {
        return result;
    }
    JsonObject* parsed = JsonObject_parse(objectText);
    int __n_579 = btrc_Map_string_string_iterLen(parsed->values);
    for (int __i_578 = 0; (__i_578 < __n_579); (__i_578++)) {
        char* key = btrc_Map_string_string_iterGet(parsed->values, __i_578);
        char* value = btrc_Map_string_string_iterValueAt(parsed->values, __i_578);
        btrc_Map_string_string_put(result, key, value);
    }
    return result;
}

char* VmSpecParser_parseStringValue(char* text, int i, char* fallback) {
    int len = ((int)strlen(text));
    (i = VmSpecParser_skipSpaces(text, i));
    if ((i >= len) || (text[i] != ((char)34))) {
        return fallback;
    }
    (i++);
    int start = i;
    bool escaped = false;
    while (i < len) {
        if ((!escaped) && (text[i] == ((char)34))) {
            __auto_type __btrc_ret_580 = JsonObject_unescape(JsonObject_slice(text, start, i));
            return __btrc_ret_580;
        }
        (escaped = ((!escaped) && (text[i] == '\\')));
        if (text[i] != '\\') {
            (escaped = false);
        }
        (i++);
    }
    return fallback;
}

char* VmSpecParser_field(char* text, char* key, char* fallback) {
    int pos = VmSpecParser_keyPosition(text, key);
    if (pos < 0) {
        return fallback;
    }
    int colon = Strings_find(text, ":", pos);
    if (colon < 0) {
        return fallback;
    }
    int i = VmSpecParser_skipSpaces(text, (colon + 1));
    int len = ((int)strlen(text));
    if ((i < len) && (text[i] == ((char)34))) {
        __auto_type __btrc_ret_581 = VmSpecParser_parseStringValue(text, i, fallback);
        return __btrc_ret_581;
    }
    int start = i;
    while ((((i < len) && (text[i] != ',')) && (text[i] != '}')) && (text[i] != ']')) {
        (i++);
    }
    char* raw = __btrc_str_track(__btrc_trim(JsonObject_slice(text, start, i)));
    if (__btrc_isEmpty(raw)) {
        return fallback;
    }
    return raw;
}

int VmSpecParser_intField(char* text, char* key, int fallback) {
    char* raw = VmSpecParser_field(text, key, "");
    if (__btrc_isEmpty(raw)) {
        return fallback;
    }
    __auto_type __btrc_ret_582 = Strings_toInt(raw);
    return __btrc_ret_582;
}

VmOperation* VmSpecParser_operation(char* objectText) {
    VmOperation* op = VmOperation_new();
    (op->kind = VmSpecParser_field(objectText, "op", ""));
    (op->command = VmSpecParser_field(objectText, "command", ""));
    (op->expect = VmSpecParser_field(objectText, "expect", ""));
    (op->name = VmSpecParser_field(objectText, "name", ""));
    (op->localPath = VmSpecParser_field(objectText, "local", ""));
    (op->remotePath = VmSpecParser_field(objectText, "remote", ""));
    (op->timeout = VmSpecParser_intField(objectText, "timeout", 120));
    return op;
    if (op != NULL) {
        if ((--op->__rc) <= 0) {
            VmOperation_destroy(op);
        }
    }
}

btrc_Vector_VmOperation* VmSpecParser_operations(char* text) {
    btrc_Vector_VmOperation* result = btrc_Vector_VmOperation_new();
    int pos = VmSpecParser_keyPosition(text, "operations");
    if (pos < 0) {
        return result;
    }
    int arrayStart = Strings_find(text, "[", pos);
    if (arrayStart < 0) {
        return result;
    }
    int len = ((int)strlen(text));
    int depth = 0;
    int start = (-1);
    bool inString = false;
    bool escaped = false;
    for (int i = (arrayStart + 1); (i < len); (i++)) {
        char c = text[i];
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
        if (c == '{') {
            if (depth == 0) {
                (start = i);
            }
            (depth++);
            continue;
        }
        if (c == '}') {
            (depth--);
            if ((depth == 0) && (start >= 0)) {
                char* objectText = JsonObject_slice(text, start, (i + 1));
                btrc_Vector_VmOperation_push(result, VmSpecParser_operation(objectText));
                (start = (-1));
            }
            continue;
        }
        if ((c == ']') && (depth == 0)) {
            break;
        }
    }
    return result;
}

VmTestSpec* VmSpecParser_parse(char* text) {
    VmTestSpec* spec = VmTestSpec_new();
    (spec->name = VmSpecParser_field(text, "name", spec->name));
    (spec->state = VmSpecParser_field(text, "state", spec->name));
    (spec->parentState = VmSpecParser_field(text, "parentState", spec->parentState));
    (spec->stateRoot = VmSpecParser_field(text, "stateRoot", spec->stateRoot));
    (spec->stateMaterial = VmSpecParser_field(text, "stateMaterial", spec->stateMaterial));
    (spec->workDir = VmSpecParser_field(text, "workDir", __btrc_str_track(__btrc_strcat(".vm/e2e/", spec->name))));
    (spec->arch = VmSpecParser_field(text, "arch", spec->arch));
    (spec->iso = VmSpecParser_field(text, "iso", PathTools_join(PathTools_join(".vm", "iso"), __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("nixos-minimal-", spec->arch)), ".iso")))));
    (spec->isoUrl = VmSpecParser_field(text, "isoUrl", VmSpecParser_defaultIsoUrl(spec->arch)));
    (spec->diskSize = VmSpecParser_field(text, "diskSize", spec->diskSize));
    (spec->memory = VmSpecParser_field(text, "memory", spec->memory));
    (spec->cpus = VmSpecParser_intField(text, "cpus", spec->cpus));
    (spec->sshPort = VmSpecParser_intField(text, "sshPort", spec->sshPort));
    if (spec->args != NULL) {
        if ((--spec->args->__rc) <= 0) {
            btrc_Map_string_string_free(spec->args);
        }
    }
    (spec->args = VmSpecParser_argsObject(text));
    (VmSpecParser_argsObject(text)->__rc++);
    if (spec->operations != NULL) {
        if ((--spec->operations->__rc) <= 0) {
            btrc_Vector_VmOperation_free(spec->operations);
        }
    }
    (spec->operations = VmSpecParser_operations(text));
    (VmSpecParser_operations(text)->__rc++);
    return spec;
    if (spec != NULL) {
        if ((--spec->__rc) <= 0) {
            VmTestSpec_destroy(spec);
        }
    }
}

void VmSpecParser_applySpecField(VmTestSpec* spec, char* key, char* value) {
    if (strcmp(key, "name") == 0) {
        (spec->name = value);
        return;
    }
    if (strcmp(key, "state") == 0) {
        (spec->state = value);
        return;
    }
    if (strcmp(key, "parentState") == 0) {
        (spec->parentState = value);
        return;
    }
    if (strcmp(key, "stateRoot") == 0) {
        (spec->stateRoot = value);
        return;
    }
    if (strcmp(key, "stateMaterial") == 0) {
        (spec->stateMaterial = value);
        return;
    }
    if (strcmp(key, "workDir") == 0) {
        (spec->workDir = value);
        return;
    }
    if (strcmp(key, "arch") == 0) {
        (spec->arch = value);
        return;
    }
    if (strcmp(key, "iso") == 0) {
        (spec->iso = value);
        return;
    }
    if (strcmp(key, "isoUrl") == 0) {
        (spec->isoUrl = value);
        return;
    }
    if (strcmp(key, "diskSize") == 0) {
        (spec->diskSize = value);
        return;
    }
    if (strcmp(key, "memory") == 0) {
        (spec->memory = value);
        return;
    }
    if (strcmp(key, "cpus") == 0) {
        (spec->cpus = Strings_toInt(value));
        return;
    }
    if (strcmp(key, "sshPort") == 0) {
        (spec->sshPort = Strings_toInt(value));
        return;
    }
}

void VmSpecParser_applyOperationField(VmOperation* op, char* key, char* value) {
    if (strcmp(key, "op") == 0) {
        (op->kind = value);
        return;
    }
    if (strcmp(key, "kind") == 0) {
        (op->kind = value);
        return;
    }
    if (strcmp(key, "command") == 0) {
        (op->command = value);
        return;
    }
    if (strcmp(key, "expect") == 0) {
        (op->expect = value);
        return;
    }
    if (strcmp(key, "name") == 0) {
        (op->name = value);
        return;
    }
    if (strcmp(key, "local") == 0) {
        (op->localPath = value);
        return;
    }
    if (strcmp(key, "remote") == 0) {
        (op->remotePath = value);
        return;
    }
    if (strcmp(key, "timeout") == 0) {
        (op->timeout = Strings_toInt(value));
        return;
    }
}

bool VmSpecParser_hasOperation(VmOperation* op) {
    __auto_type __btrc_ret_583 = (((!__btrc_isEmpty(op->kind)) || (!__btrc_isEmpty(op->command))) || (!__btrc_isEmpty(op->name)));
    return __btrc_ret_583;
}

char* VmSpecParser_yamlKey(char* line) {
    int pos = Strings_find(line, ":", 0);
    if (pos < 0) {
        __auto_type __btrc_ret_584 = "";
        return __btrc_ret_584;
    }
    __auto_type __btrc_ret_585 = Toml_unquote(__btrc_str_track(__btrc_trim(__btrc_str_track(__btrc_substring(line, 0, pos)))));
    return __btrc_ret_585;
}

char* VmSpecParser_yamlValue(char* line) {
    int pos = Strings_find(line, ":", 0);
    if (pos < 0) {
        __auto_type __btrc_ret_586 = "";
        return __btrc_ret_586;
    }
    __auto_type __btrc_ret_587 = Toml_unquote(__btrc_str_track(__btrc_trim(Toml_stripInlineComment(__btrc_str_track(__btrc_substring(line, (pos + 1), ((((int)strlen(line)) - pos) - 1)))))));
    return __btrc_ret_587;
}

VmTestSpec* VmSpecParser_parseToml(char* text) {
    VmTestSpec* spec = VmTestSpec_new();
    char* section = "root";
    VmOperation* op = VmOperation_new();
    btrc_Vector_string* lines = Strings_split(text, "\n");
    int __n_589 = btrc_Vector_string_iterLen(lines);
    for (int __i_588 = 0; (__i_588 < __n_589); (__i_588++)) {
        char* line = btrc_Vector_string_iterGet(lines, __i_588);
        char* cleaned = Toml_stripInlineComment(line);
        if (__btrc_isEmpty(cleaned)) {
            continue;
        }
        char* arrayName = Toml_tableArrayName(cleaned);
        if (strcmp(arrayName, "operations") == 0) {
            if (VmSpecParser_hasOperation(op)) {
                btrc_Vector_VmOperation_push(spec->operations, op);
            }
            (op = VmOperation_new());
            (section = "operations");
            continue;
        }
        char* sectionName = Toml_sectionName(cleaned);
        if (!__btrc_isEmpty(sectionName)) {
            if (VmSpecParser_hasOperation(op)) {
                btrc_Vector_VmOperation_push(spec->operations, op);
                (op = VmOperation_new());
            }
            (section = sectionName);
            continue;
        }
        char* key = Toml_key(cleaned);
        if (__btrc_isEmpty(key)) {
            continue;
        }
        char* value = Toml_value(cleaned);
        if (strcmp(section, "args") == 0) {
            btrc_Map_string_string_put(spec->args, key, value);
            continue;
        }
        if (strcmp(section, "operations") == 0) {
            VmSpecParser_applyOperationField(op, key, value);
            continue;
        }
        VmSpecParser_applySpecField(spec, key, value);
    }
    if (VmSpecParser_hasOperation(op)) {
        btrc_Vector_VmOperation_push(spec->operations, op);
    }
    if (op != NULL) {
        if ((--op->__rc) <= 0) {
            VmOperation_destroy(op);
        }
    }
    return spec;
    if (op != NULL) {
        if ((--op->__rc) <= 0) {
            VmOperation_destroy(op);
        }
    }
    if (spec != NULL) {
        if ((--spec->__rc) <= 0) {
            VmTestSpec_destroy(spec);
        }
    }
}

VmTestSpec* VmSpecParser_parseYaml(char* text) {
    VmTestSpec* spec = VmTestSpec_new();
    char* section = "root";
    VmOperation* op = VmOperation_new();
    btrc_Vector_string* lines = Strings_split(text, "\n");
    int __n_591 = btrc_Vector_string_iterLen(lines);
    for (int __i_590 = 0; (__i_590 < __n_591); (__i_590++)) {
        char* raw = btrc_Vector_string_iterGet(lines, __i_590);
        char* line = Toml_stripInlineComment(raw);
        if (__btrc_isEmpty(__btrc_str_track(__btrc_trim(line)))) {
            continue;
        }
        char* trimmed = __btrc_str_track(__btrc_trim(line));
        if (strcmp(trimmed, "args:") == 0) {
            if (VmSpecParser_hasOperation(op)) {
                btrc_Vector_VmOperation_push(spec->operations, op);
                (op = VmOperation_new());
            }
            (section = "args");
            continue;
        }
        if (strcmp(trimmed, "operations:") == 0) {
            (section = "operations");
            continue;
        }
        if (__btrc_startsWith(trimmed, "- ")) {
            if (VmSpecParser_hasOperation(op)) {
                btrc_Vector_VmOperation_push(spec->operations, op);
            }
            (op = VmOperation_new());
            (section = "operations");
            char* rest = __btrc_str_track(__btrc_trim(__btrc_str_track(__btrc_substring(trimmed, 2, (((int)strlen(trimmed)) - 2)))));
            if (!__btrc_isEmpty(rest)) {
                VmSpecParser_applyOperationField(op, VmSpecParser_yamlKey(rest), VmSpecParser_yamlValue(rest));
            }
            continue;
        }
        char* key = VmSpecParser_yamlKey(trimmed);
        if (__btrc_isEmpty(key)) {
            continue;
        }
        char* value = VmSpecParser_yamlValue(trimmed);
        if (strcmp(section, "args") == 0) {
            btrc_Map_string_string_put(spec->args, key, value);
            continue;
        }
        if (strcmp(section, "operations") == 0) {
            VmSpecParser_applyOperationField(op, key, value);
            continue;
        }
        VmSpecParser_applySpecField(spec, key, value);
    }
    if (VmSpecParser_hasOperation(op)) {
        btrc_Vector_VmOperation_push(spec->operations, op);
    }
    if (op != NULL) {
        if ((--op->__rc) <= 0) {
            VmOperation_destroy(op);
        }
    }
    return spec;
    if (op != NULL) {
        if ((--op->__rc) <= 0) {
            VmOperation_destroy(op);
        }
    }
    if (spec != NULL) {
        if ((--spec->__rc) <= 0) {
            VmTestSpec_destroy(spec);
        }
    }
}

VmTestSpec* VmSpecParser_readFile(char* path) {
    char* text = Path_readAll(path);
    if (__btrc_endsWith(path, ".toml")) {
        __auto_type __btrc_ret_592 = VmSpecParser_parseToml(text);
        return __btrc_ret_592;
    }
    if (__btrc_endsWith(path, ".yaml") || __btrc_endsWith(path, ".yml")) {
        __auto_type __btrc_ret_593 = VmSpecParser_parseYaml(text);
        return __btrc_ret_593;
    }
    __auto_type __btrc_ret_594 = VmSpecParser_parse(text);
    return __btrc_ret_594;
}

void QemuE2eHarness_init(QemuE2eHarness* self, VmTestSpec* spec) {
    self->__rc = 1;
    if (self->spec != NULL) {
        if ((--self->spec->__rc) <= 0) {
            VmTestSpec_destroy(self->spec);
        }
    }
    (self->spec = spec);
    (spec->__rc++);
    if (self->shell != NULL) {
        if ((--self->shell->__rc) <= 0) {
            UnixShell_destroy(self->shell);
        }
    }
    (self->shell = UnixShell_new());
    (UnixShell_new()->__rc++);
}

QemuE2eHarness* QemuE2eHarness_new(VmTestSpec* spec) {
    QemuE2eHarness* self = ((QemuE2eHarness*)malloc(sizeof(QemuE2eHarness)));
    memset(self, 0, sizeof(QemuE2eHarness));
    QemuE2eHarness_init(self, spec);
    return self;
}

void QemuE2eHarness_destroy(QemuE2eHarness* self) {
    if (self->spec != NULL) {
        if ((--self->spec->__rc) <= 0) {
            VmTestSpec_destroy(self->spec);
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

char* QemuE2eHarness_diskPath(QemuE2eHarness* self) {
    __auto_type __btrc_ret_595 = PathTools_join(self->spec->workDir, "disk.qcow2");
    return __btrc_ret_595;
}

char* QemuE2eHarness_pidPath(QemuE2eHarness* self) {
    __auto_type __btrc_ret_596 = PathTools_join(self->spec->workDir, "qemu.pid");
    return __btrc_ret_596;
}

char* QemuE2eHarness_monitorPath(QemuE2eHarness* self) {
    __auto_type __btrc_ret_597 = PathTools_join(self->spec->workDir, "monitor.sock");
    return __btrc_ret_597;
}

char* QemuE2eHarness_qmpPath(QemuE2eHarness* self) {
    __auto_type __btrc_ret_598 = PathTools_join(self->spec->workDir, "qmp.sock");
    return __btrc_ret_598;
}

char* QemuE2eHarness_serialBasePath(QemuE2eHarness* self) {
    __auto_type __btrc_ret_599 = PathTools_join(self->spec->workDir, "serial");
    return __btrc_ret_599;
}

char* QemuE2eHarness_serialInPath(QemuE2eHarness* self) {
    __auto_type __btrc_ret_600 = __btrc_str_track(__btrc_strcat(QemuE2eHarness_serialBasePath(self), ".in"));
    return __btrc_ret_600;
}

char* QemuE2eHarness_serialOutPath(QemuE2eHarness* self) {
    __auto_type __btrc_ret_601 = __btrc_str_track(__btrc_strcat(QemuE2eHarness_serialBasePath(self), ".out"));
    return __btrc_ret_601;
}

char* QemuE2eHarness_serialLogPath(QemuE2eHarness* self) {
    __auto_type __btrc_ret_602 = __btrc_str_track(__btrc_strcat(QemuE2eHarness_serialBasePath(self), ".log"));
    return __btrc_ret_602;
}

char* QemuE2eHarness_serialReaderPidPath(QemuE2eHarness* self) {
    __auto_type __btrc_ret_603 = PathTools_join(self->spec->workDir, "serial-reader.pid");
    return __btrc_ret_603;
}

char* QemuE2eHarness_tpmDir(QemuE2eHarness* self) {
    __auto_type __btrc_ret_604 = PathTools_join(self->spec->workDir, "tpm");
    return __btrc_ret_604;
}

char* QemuE2eHarness_tpmStateDir(QemuE2eHarness* self) {
    __auto_type __btrc_ret_605 = PathTools_join(QemuE2eHarness_tpmDir(self), "state");
    return __btrc_ret_605;
}

char* QemuE2eHarness_tpmSocketPath(QemuE2eHarness* self) {
    __auto_type __btrc_ret_606 = PathTools_join(QemuE2eHarness_tpmDir(self), "swtpm.sock");
    return __btrc_ret_606;
}

char* QemuE2eHarness_tpmPidPath(QemuE2eHarness* self) {
    __auto_type __btrc_ret_607 = PathTools_join(QemuE2eHarness_tpmDir(self), "swtpm.pid");
    return __btrc_ret_607;
}

char* QemuE2eHarness_tpmLogPath(QemuE2eHarness* self) {
    __auto_type __btrc_ret_608 = PathTools_join(QemuE2eHarness_tpmDir(self), "swtpm.log");
    return __btrc_ret_608;
}

char* QemuE2eHarness_sshKeyPath(QemuE2eHarness* self) {
    __auto_type __btrc_ret_609 = PathTools_join(self->spec->workDir, "id_ed25519");
    return __btrc_ret_609;
}

char* QemuE2eHarness_sshPubKeyPath(QemuE2eHarness* self) {
    __auto_type __btrc_ret_610 = __btrc_str_track(__btrc_strcat(QemuE2eHarness_sshKeyPath(self), ".pub"));
    return __btrc_ret_610;
}

char* QemuE2eHarness_bootDir(QemuE2eHarness* self) {
    __auto_type __btrc_ret_611 = PathTools_join(self->spec->workDir, "boot");
    return __btrc_ret_611;
}

char* QemuE2eHarness_bootKernelFile(QemuE2eHarness* self) {
    __auto_type __btrc_ret_612 = PathTools_join(QemuE2eHarness_bootDir(self), "kernel.path");
    return __btrc_ret_612;
}

char* QemuE2eHarness_bootInitrdFile(QemuE2eHarness* self) {
    __auto_type __btrc_ret_613 = PathTools_join(QemuE2eHarness_bootDir(self), "initrd.path");
    return __btrc_ret_613;
}

char* QemuE2eHarness_bootAppendFile(QemuE2eHarness* self) {
    __auto_type __btrc_ret_614 = PathTools_join(QemuE2eHarness_bootDir(self), "append.txt");
    return __btrc_ret_614;
}

char* QemuE2eHarness_firmwareVarsPath(QemuE2eHarness* self) {
    __auto_type __btrc_ret_615 = PathTools_join(self->spec->workDir, "edk2-vars.fd");
    return __btrc_ret_615;
}

char* QemuE2eHarness_firmwareVarsSnapshotPath(QemuE2eHarness* self, char* name) {
    __auto_type __btrc_ret_616 = PathTools_join(self->spec->workDir, __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("edk2-vars.", name)), ".fd")));
    return __btrc_ret_616;
}

char* QemuE2eHarness_parentStateDir(QemuE2eHarness* self) {
    __auto_type __btrc_ret_617 = PathTools_join(self->spec->stateRoot, self->spec->parentState);
    return __btrc_ret_617;
}

char* QemuE2eHarness_parentWorkDirFile(QemuE2eHarness* self) {
    __auto_type __btrc_ret_618 = PathTools_join(QemuE2eHarness_parentStateDir(self), "workDir");
    return __btrc_ret_618;
}

char* QemuE2eHarness_backingDiskFile(QemuE2eHarness* self) {
    __auto_type __btrc_ret_619 = PathTools_join(self->spec->workDir, "backing-disk");
    return __btrc_ret_619;
}

char* QemuE2eHarness_absolutePath(QemuE2eHarness* self, char* path) {
    Command* command = Command_arg(Command_arg(Command_new("sh"), "-c"), __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("cd ", UnixShell_quote(PathTools_dirname(path)))), " && printf '%s/%s' \"$PWD\" ")), UnixShell_quote(PathTools_basename(path)))));
    ExecResult* result = UnixShell_runCommand(self->shell, command);
    if (!ExecResult_ok(result)) {
        NixosLog_fatal(__btrc_str_track(__btrc_strcat("Failed to resolve path ", path)));
    }
    __auto_type __btrc_ret_620 = __btrc_str_track(__btrc_trim(ExecResult_stdout(result)));
    return __btrc_ret_620;
}

void QemuE2eHarness_ensureWorkDir(QemuE2eHarness* self) {
    FileSystem_mkdirp(self->spec->workDir);
    FileSystem_mkdirp(PathTools_dirname(self->spec->iso));
}

char* QemuE2eHarness_qemuBinary(QemuE2eHarness* self) {
    int __fstr_621_len = snprintf(NULL, 0, "qemu-system-%s", self->spec->arch);
    char* __fstr_621_buf = __btrc_str_track(((char*)malloc((__fstr_621_len + 1))));
    snprintf(__fstr_621_buf, (__fstr_621_len + 1), "qemu-system-%s", self->spec->arch);
    __auto_type __btrc_ret_622 = __fstr_621_buf;
    return __btrc_ret_622;
}

bool QemuE2eHarness_argEnabled(QemuE2eHarness* self, char* key) {
    __auto_type __btrc_ret_623 = (strcmp(btrc_Map_string_string_getOrDefault(self->spec->args, key, "false"), "true") == 0);
    return __btrc_ret_623;
}

bool QemuE2eHarness_tpm2Enabled(QemuE2eHarness* self) {
    __auto_type __btrc_ret_624 = QemuE2eHarness_argEnabled(self, "tpm2");
    return __btrc_ret_624;
}

bool QemuE2eHarness_secureBootEnabled(QemuE2eHarness* self) {
    __auto_type __btrc_ret_625 = QemuE2eHarness_argEnabled(self, "secureBoot");
    return __btrc_ret_625;
}

bool QemuE2eHarness_uefiEnabled(QemuE2eHarness* self) {
    __auto_type __btrc_ret_626 = ((QemuE2eHarness_argEnabled(self, "uefi") || QemuE2eHarness_argEnabled(self, "uefiDisk")) || QemuE2eHarness_secureBootEnabled(self));
    return __btrc_ret_626;
}

bool QemuE2eHarness_shouldUseUefi(QemuE2eHarness* self, bool fromIso) {
    if (strcmp(self->spec->arch, "aarch64") == 0) {
        __auto_type __btrc_ret_627 = true;
        return __btrc_ret_627;
    }
    if (QemuE2eHarness_argEnabled(self, "uefi")) {
        __auto_type __btrc_ret_628 = true;
        return __btrc_ret_628;
    }
    if ((!fromIso) && QemuE2eHarness_uefiEnabled(self)) {
        __auto_type __btrc_ret_629 = true;
        return __btrc_ret_629;
    }
    if (fromIso && QemuE2eHarness_argEnabled(self, "uefiIso")) {
        __auto_type __btrc_ret_630 = true;
        return __btrc_ret_630;
    }
    __auto_type __btrc_ret_631 = false;
    return __btrc_ret_631;
}

bool QemuE2eHarness_hostArchMatchesGuest(QemuE2eHarness* self) {
    __auto_type __btrc_ret_632 = (strcmp(VmSpecParser_hostArch(), self->spec->arch) == 0);
    return __btrc_ret_632;
}

char* QemuE2eHarness_qemuSharePath(QemuE2eHarness* self, char* fileName) {
    char* script = __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("bin=$(command -v ", UnixShell_quote(QemuE2eHarness_qemuBinary(self)))), ") || exit 1; ")), "path=$(dirname $(dirname \"$bin\"))/share/qemu/")), UnixShell_quote(fileName))), "; ")), "test -f \"$path\" || exit 1; printf %s \"$path\""));
    ExecResult* result = UnixShell_runUnchecked(self->shell, script);
    if (!ExecResult_ok(result)) {
        __auto_type __btrc_ret_633 = "";
        return __btrc_ret_633;
    }
    __auto_type __btrc_ret_634 = Strings_copy(__btrc_str_track(__btrc_trim(ExecResult_stdout(result))));
    return __btrc_ret_634;
}

char* QemuE2eHarness_firmwareCodePath(QemuE2eHarness* self) {
    if (QemuE2eHarness_secureBootEnabled(self)) {
        __auto_type __btrc_ret_635 = QemuE2eHarness_secureFirmwareCodePath(self);
        return __btrc_ret_635;
    }
    if (strcmp(self->spec->arch, "aarch64") == 0) {
        char* packaged = QemuE2eHarness_qemuSharePath(self, "edk2-aarch64-code.fd");
        if (!__btrc_isEmpty(packaged)) {
            return packaged;
        }
        if (FileSystem_exists("/opt/homebrew/share/qemu/edk2-aarch64-code.fd")) {
            __auto_type __btrc_ret_636 = "/opt/homebrew/share/qemu/edk2-aarch64-code.fd";
            return __btrc_ret_636;
        }
    }
    if (strcmp(self->spec->arch, "x86_64") == 0) {
        char* packaged = QemuE2eHarness_qemuSharePath(self, "edk2-x86_64-code.fd");
        if (!__btrc_isEmpty(packaged)) {
            return packaged;
        }
    }
    ExecResult* result = UnixShell_runUnchecked(self->shell, "find /nix/store -maxdepth 4 -name OVMF_CODE.fd -print -quit 2>/dev/null");
    char* found = __btrc_str_track(__btrc_trim(ExecResult_stdout(result)));
    if (!__btrc_isEmpty(found)) {
        return found;
    }
    __auto_type __btrc_ret_637 = "";
    return __btrc_ret_637;
}

char* QemuE2eHarness_secureFirmwareCodePath(QemuE2eHarness* self) {
    if (strcmp(self->spec->arch, "aarch64") == 0) {
        ExecResult* store = UnixShell_runUnchecked(self->shell, "find /nix/store -path '*aavmf-secboot*/AAVMF_CODE.secboot.fd' -print -quit 2>/dev/null");
        char* ssb = __btrc_str_track(__btrc_trim(ExecResult_stdout(store)));
        if (!__btrc_isEmpty(ssb)) {
            __auto_type __btrc_ret_638 = Strings_copy(ssb);
            return __btrc_ret_638;
        }
        ExecResult* deb = UnixShell_runUnchecked(self->shell, "find \"$(pwd)/.vm/firmware\" -name AAVMF_CODE.secboot.fd -print -quit 2>/dev/null");
        __auto_type __btrc_ret_639 = Strings_copy(__btrc_str_track(__btrc_trim(ExecResult_stdout(deb))));
        return __btrc_ret_639;
    }
    if (!(strcmp(self->spec->arch, "x86_64") == 0)) {
        __auto_type __btrc_ret_640 = "";
        return __btrc_ret_640;
    }
    char* packaged = QemuE2eHarness_qemuSharePath(self, "edk2-x86_64-secure-code.fd");
    if (!__btrc_isEmpty(packaged)) {
        return packaged;
    }
    ExecResult* result = UnixShell_runUnchecked(self->shell, "find /nix/store -maxdepth 5 \\( -name OVMF_CODE.secboot.fd -o -name OVMF_CODE_4M.secboot.fd -o -name '*secure*CODE*.fd' \\) -print -quit 2>/dev/null");
    char* found = __btrc_str_track(__btrc_trim(ExecResult_stdout(result)));
    if (!__btrc_isEmpty(found)) {
        return found;
    }
    __auto_type __btrc_ret_641 = "";
    return __btrc_ret_641;
}

char* QemuE2eHarness_firmwareVarsTemplatePath(QemuE2eHarness* self) {
    if (QemuE2eHarness_secureBootEnabled(self) && (strcmp(self->spec->arch, "aarch64") == 0)) {
        ExecResult* store = UnixShell_runUnchecked(self->shell, "find /nix/store -path '*aavmf-secboot*/AAVMF_VARS.fd' -print -quit 2>/dev/null");
        char* ssv = __btrc_str_track(__btrc_trim(ExecResult_stdout(store)));
        if (!__btrc_isEmpty(ssv)) {
            __auto_type __btrc_ret_642 = Strings_copy(ssv);
            return __btrc_ret_642;
        }
        ExecResult* deb = UnixShell_runUnchecked(self->shell, "find \"$(pwd)/.vm/firmware\" -name AAVMF_VARS.fd -print -quit 2>/dev/null");
        char* dvp = __btrc_str_track(__btrc_trim(ExecResult_stdout(deb)));
        if (!__btrc_isEmpty(dvp)) {
            __auto_type __btrc_ret_643 = Strings_copy(dvp);
            return __btrc_ret_643;
        }
    }
    if (strcmp(self->spec->arch, "aarch64") == 0) {
        char* packaged = QemuE2eHarness_qemuSharePath(self, "edk2-arm-vars.fd");
        if (!__btrc_isEmpty(packaged)) {
            return packaged;
        }
    }
    if (strcmp(self->spec->arch, "x86_64") == 0) {
        char* packaged = QemuE2eHarness_qemuSharePath(self, "edk2-i386-vars.fd");
        if (!__btrc_isEmpty(packaged)) {
            return packaged;
        }
    }
    ExecResult* result = UnixShell_runUnchecked(self->shell, "find /nix/store -maxdepth 4 -name OVMF_VARS.fd -print -quit 2>/dev/null");
    char* found = __btrc_str_track(__btrc_trim(ExecResult_stdout(result)));
    if (!__btrc_isEmpty(found)) {
        return found;
    }
    __auto_type __btrc_ret_644 = "";
    return __btrc_ret_644;
}

void QemuE2eHarness_makeFirmwareVarsWritable(QemuE2eHarness* self) {
    Command* chmod = Command_check(Command_capture(Command_arg(Command_arg(Command_new("chmod"), "600"), QemuE2eHarness_firmwareVarsPath(self)), false), false);
    UnixShell_runCommand(self->shell, chmod);
}

void QemuE2eHarness_setupFirmwareVars(QemuE2eHarness* self) {
    if (FileSystem_exists(QemuE2eHarness_firmwareVarsPath(self))) {
        QemuE2eHarness_makeFirmwareVarsWritable(self);
        return;
    }
    char* template = QemuE2eHarness_firmwareVarsTemplatePath(self);
    if (!__btrc_isEmpty(template)) {
        Command* cp = Command_capture(Command_arg(Command_arg(Command_new("cp"), template), QemuE2eHarness_firmwareVarsPath(self)), false);
        ExecResult* result = UnixShell_runCommand(self->shell, cp);
        if (!ExecResult_ok(result)) {
            NixosLog_fatal("Failed to copy EDK2 vars file");
        }
        QemuE2eHarness_makeFirmwareVarsWritable(self);
        return;
    }
    int __fstr_646_len = snprintf(NULL, 0, "dd if=/dev/zero of=%s bs=1M count=64", UnixShell_quote(QemuE2eHarness_firmwareVarsPath(self)));
    char* __fstr_646_buf = __btrc_str_track(((char*)malloc((__fstr_646_len + 1))));
    snprintf(__fstr_646_buf, (__fstr_646_len + 1), "dd if=/dev/zero of=%s bs=1M count=64", UnixShell_quote(QemuE2eHarness_firmwareVarsPath(self)));
    UnixShell_runRaw(self->shell, __fstr_646_buf, false, true, "");
    QemuE2eHarness_makeFirmwareVarsWritable(self);
}

bool QemuE2eHarness_commandExists(QemuE2eHarness* self, char* name) {
    ExecResult* result = UnixShell_runRaw(self->shell, __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("command -v ", UnixShell_quote(name))), " >/dev/null 2>&1")), false, false, "");
    __auto_type __btrc_ret_647 = ExecResult_ok(result);
    return __btrc_ret_647;
}

void QemuE2eHarness_requireCommand(QemuE2eHarness* self, char* name) {
    if (!QemuE2eHarness_commandExists(self, name)) {
        NixosLog_fatal(__btrc_str_track(__btrc_strcat("Missing required command: ", name)));
    }
}

bool QemuE2eHarness_qemuDeviceAvailable(QemuE2eHarness* self, char* deviceName) {
    char* command = __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(QemuE2eHarness_qemuBinary(self), " -device help 2>/dev/null | grep -q ")), UnixShell_quote(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("name \"", deviceName)), "\"")))));
    ExecResult* result = UnixShell_runRaw(self->shell, command, false, false, "");
    __auto_type __btrc_ret_648 = ExecResult_ok(result);
    return __btrc_ret_648;
}

char* QemuE2eHarness_tpmQemuDevice(QemuE2eHarness* self) {
    if (strcmp(self->spec->arch, "aarch64") == 0) {
        __auto_type __btrc_ret_649 = "tpm-tis-device";
        return __btrc_ret_649;
    }
    __auto_type __btrc_ret_650 = "tpm-tis";
    return __btrc_ret_650;
}

void QemuE2eHarness_requireTpm2Capability(QemuE2eHarness* self) {
    QemuE2eHarness_requireCommand(self, "swtpm");
    char* deviceName = QemuE2eHarness_tpmQemuDevice(self);
    if (!QemuE2eHarness_qemuDeviceAvailable(self, deviceName)) {
        NixosLog_fatal(__btrc_str_track(__btrc_strcat("QEMU TPM2 device is unavailable: ", deviceName)));
    }
}

void QemuE2eHarness_requireUefiCapability(QemuE2eHarness* self) {
    char* codePath = QemuE2eHarness_firmwareCodePath(self);
    char* varsPath = QemuE2eHarness_firmwareVarsTemplatePath(self);
    if (__btrc_isEmpty(codePath) || __btrc_isEmpty(varsPath)) {
        NixosLog_fatal(__btrc_str_track(__btrc_strcat("QEMU UEFI firmware is unavailable for ", self->spec->arch)));
    }
}

void QemuE2eHarness_requireSecureBootCapability(QemuE2eHarness* self) {
    char* codePath = QemuE2eHarness_secureFirmwareCodePath(self);
    char* varsPath = QemuE2eHarness_firmwareVarsTemplatePath(self);
    if (__btrc_isEmpty(codePath) || __btrc_isEmpty(varsPath)) {
        NixosLog_fatal(__btrc_str_track(__btrc_strcat("QEMU Secure Boot firmware is unavailable for ", self->spec->arch)));
    }
    QemuE2eHarness_requireTpm2Capability(self);
}

char* QemuE2eHarness_secureBootCapabilityReport(QemuE2eHarness* self) {
    bool qemu = QemuE2eHarness_commandExists(self, QemuE2eHarness_qemuBinary(self));
    bool swtpm = QemuE2eHarness_commandExists(self, "swtpm");
    char* firmware = QemuE2eHarness_secureFirmwareCodePath(self);
    char* vars = QemuE2eHarness_firmwareVarsTemplatePath(self);
    char* device = QemuE2eHarness_tpmQemuDevice(self);
    bool tpmDevice = (qemu && QemuE2eHarness_qemuDeviceAvailable(self, device));
    bool available = ((((((strcmp(self->spec->arch, "x86_64") == 0) && qemu) && swtpm) && tpmDevice) && (!__btrc_isEmpty(firmware))) && (!__btrc_isEmpty(vars)));
    __auto_type __btrc_ret_651 = __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("secureBootQemu=", (available ? "available" : "unavailable"))), "\narch=")), self->spec->arch)), "\nqemu=")), (qemu ? "yes" : "no"))), "\nswtpm=")), (swtpm ? "yes" : "no"))), "\ntpmDevice=")), (tpmDevice ? device : "missing"))), "\nfirmware=")), (__btrc_isEmpty(firmware) ? "missing" : firmware))), "\nvars=")), (__btrc_isEmpty(vars) ? "missing" : vars)));
    return __btrc_ret_651;
}

bool QemuE2eHarness_isDarwin(QemuE2eHarness* self) {
    ExecResult* result = UnixShell_run(self->shell, "uname -s");
    __auto_type __btrc_ret_652 = (strcmp(__btrc_str_track(__btrc_trim(ExecResult_stdout(result))), "Darwin") == 0);
    return __btrc_ret_652;
}

char* QemuE2eHarness_stripLeadingSlash(QemuE2eHarness* self, char* path) {
    if (__btrc_startsWith(path, "/")) {
        __auto_type __btrc_ret_653 = __btrc_str_track(__btrc_substring(path, 1, (((int)strlen(path)) - 1)));
        return __btrc_ret_653;
    }
    return path;
}

char* QemuE2eHarness_valueAfterLinePrefix(QemuE2eHarness* self, char* text, char* prefix, int start) {
    int pos = Strings_find(text, prefix, start);
    if (pos < 0) {
        __auto_type __btrc_ret_654 = "";
        return __btrc_ret_654;
    }
    (pos = (pos + ((int)strlen(prefix))));
    int end = pos;
    while ((text[end] != '\0') && (text[end] != '\n')) {
        (end++);
    }
    __auto_type __btrc_ret_655 = __btrc_str_track(__btrc_trim(JsonObject_slice(text, pos, end)));
    return __btrc_ret_655;
}

void QemuE2eHarness_extractBootSerial(QemuE2eHarness* self) {
    if (!(strcmp(self->spec->arch, "x86_64") == 0)) {
        return;
    }
    if (FileSystem_exists(QemuE2eHarness_bootAppendFile(self))) {
        return;
    }
    FileSystem_mkdirp(QemuE2eHarness_bootDir(self));
    Command* cfgExtract = Command_capture(Command_arg(Command_arg(Command_arg(Command_arg(Command_arg(Command_new("bsdtar"), "-xf"), self->spec->iso), "-C"), QemuE2eHarness_bootDir(self)), "isolinux/isolinux.cfg"), false);
    ExecResult* cfgResult = UnixShell_runCommand(self->shell, cfgExtract);
    if (!ExecResult_ok(cfgResult)) {
        NixosLog_fatal("Failed to extract isolinux.cfg from ISO");
    }
    char* cfgPath = PathTools_join(PathTools_join(QemuE2eHarness_bootDir(self), "isolinux"), "isolinux.cfg");
    char* cfg = Path_readAll(cfgPath);
    int label = Strings_find(cfg, "LABEL boot-serial", 0);
    if (label < 0) {
        NixosLog_fatal("Could not find boot-serial entry in installer ISO");
    }
    char* kernelIso = QemuE2eHarness_valueAfterLinePrefix(self, cfg, "LINUX ", label);
    char* append = QemuE2eHarness_valueAfterLinePrefix(self, cfg, "APPEND ", label);
    char* initrdIso = QemuE2eHarness_valueAfterLinePrefix(self, cfg, "INITRD ", label);
    if ((__btrc_isEmpty(kernelIso) || __btrc_isEmpty(initrdIso)) || __btrc_isEmpty(append)) {
        NixosLog_fatal("Incomplete boot-serial entry in installer ISO");
    }
    char* kernelRel = QemuE2eHarness_stripLeadingSlash(self, kernelIso);
    char* initrdRel = QemuE2eHarness_stripLeadingSlash(self, initrdIso);
    Command* bootExtract = Command_capture(Command_arg(Command_arg(Command_arg(Command_arg(Command_arg(Command_arg(Command_new("bsdtar"), "-xf"), self->spec->iso), "-C"), QemuE2eHarness_bootDir(self)), kernelRel), initrdRel), false);
    ExecResult* bootResult = UnixShell_runCommand(self->shell, bootExtract);
    if (!ExecResult_ok(bootResult)) {
        NixosLog_fatal("Failed to extract serial boot kernel/initrd from ISO");
    }
    Path_writeAll(QemuE2eHarness_bootKernelFile(self), PathTools_join(QemuE2eHarness_bootDir(self), kernelRel));
    Path_writeAll(QemuE2eHarness_bootInitrdFile(self), PathTools_join(QemuE2eHarness_bootDir(self), initrdRel));
    Path_writeAll(QemuE2eHarness_bootAppendFile(self), append);
}

void QemuE2eHarness_downloadIso(QemuE2eHarness* self) {
    QemuE2eHarness_ensureWorkDir(self);
    if (FileSystem_exists(self->spec->iso)) {
        return;
    }
    if (__btrc_isEmpty(self->spec->isoUrl)) {
        NixosLog_fatal("No iso or isoUrl in VM spec");
    }
    int __fstr_656_len = snprintf(NULL, 0, "%s.tmp", self->spec->iso);
    char* __fstr_656_buf = __btrc_str_track(((char*)malloc((__fstr_656_len + 1))));
    snprintf(__fstr_656_buf, (__fstr_656_len + 1), "%s.tmp", self->spec->iso);
    char* tmp = __fstr_656_buf;
    Command* curl = Command_capture(Command_arg(Command_arg(Command_arg(Command_arg(Command_new("curl"), "-L"), "-o"), tmp), self->spec->isoUrl), false);
    ExecResult* result = UnixShell_runCommand(self->shell, curl);
    if (!ExecResult_ok(result)) {
        NixosLog_fatal("Failed to download installer ISO");
    }
    Command* mv = Command_capture(Command_arg(Command_arg(Command_new("mv"), tmp), self->spec->iso), false);
    UnixShell_runCommand(self->shell, mv);
}

void QemuE2eHarness_createSshKey(QemuE2eHarness* self) {
    QemuE2eHarness_ensureWorkDir(self);
    if (FileSystem_exists(QemuE2eHarness_sshKeyPath(self))) {
        return;
    }
    Command* cmd = Command_capture(Command_arg(Command_arg(Command_arg(Command_arg(Command_arg(Command_arg(Command_arg(Command_new("ssh-keygen"), "-t"), "ed25519"), "-N"), ""), "-f"), QemuE2eHarness_sshKeyPath(self)), "-q"), false);
    ExecResult* result = UnixShell_runCommand(self->shell, cmd);
    if (!ExecResult_ok(result)) {
        NixosLog_fatal("Failed to create VM SSH key");
    }
}

char* QemuE2eHarness_sshPubKey(QemuE2eHarness* self) {
    __auto_type __btrc_ret_657 = __btrc_str_track(__btrc_trim(Path_readAll(QemuE2eHarness_sshPubKeyPath(self))));
    return __btrc_ret_657;
}

void QemuE2eHarness_createDisk(QemuE2eHarness* self) {
    QemuE2eHarness_ensureWorkDir(self);
    if (FileSystem_exists(QemuE2eHarness_diskPath(self))) {
        return;
    }
    Command* cmd = Command_capture(Command_arg(Command_arg(Command_arg(Command_arg(Command_arg(Command_new("qemu-img"), "create"), "-f"), "qcow2"), QemuE2eHarness_diskPath(self)), self->spec->diskSize), false);
    ExecResult* result = UnixShell_runCommand(self->shell, cmd);
    if (!ExecResult_ok(result)) {
        NixosLog_fatal("Failed to create VM disk");
    }
}

void QemuE2eHarness_setup(QemuE2eHarness* self) {
    QemuE2eHarness_downloadIso(self);
    QemuE2eHarness_createSshKey(self);
    QemuE2eHarness_createDisk(self);
    QemuE2eHarness_extractBootSerial(self);
}

void QemuE2eHarness_resetState(QemuE2eHarness* self) {
    QemuE2eHarness_stop(self);
    FileSystem_removeRecursive(self->spec->workDir);
    QemuE2eHarness_ensureWorkDir(self);
}

void QemuE2eHarness_cleanStateRecord(QemuE2eHarness* self) {
    FileSystem_removeRecursive(VmTestSpec_stateDir(self->spec));
}

void QemuE2eHarness_requireParentState(QemuE2eHarness* self) {
    if ((strcmp(self->spec->parentState, "root") == 0) || __btrc_isEmpty(self->spec->parentState)) {
        return;
    }
    if (!FileSystem_exists(VmTestSpec_parentHashFile(self->spec))) {
        int __fstr_659_len = snprintf(NULL, 0, "Missing parent state %s; run its test first", self->spec->parentState);
        char* __fstr_659_buf = __btrc_str_track(((char*)malloc((__fstr_659_len + 1))));
        snprintf(__fstr_659_buf, (__fstr_659_len + 1), "Missing parent state %s; run its test first", self->spec->parentState);
        NixosLog_fatal(__fstr_659_buf);
    }
}

void QemuE2eHarness_copyIfExists(QemuE2eHarness* self, char* source, char* target) {
    if (!FileSystem_exists(source)) {
        return;
    }
    Command* cp = Command_capture(Command_arg(Command_arg(Command_new("cp"), source), target), false);
    ExecResult* result = UnixShell_runCommand(self->shell, cp);
    if (!ExecResult_ok(result)) {
        int __fstr_661_len = snprintf(NULL, 0, "Failed to copy state artifact %s", source);
        char* __fstr_661_buf = __btrc_str_track(((char*)malloc((__fstr_661_len + 1))));
        snprintf(__fstr_661_buf, (__fstr_661_len + 1), "Failed to copy state artifact %s", source);
        NixosLog_fatal(__fstr_661_buf);
    }
}

void QemuE2eHarness_copyTreeIfExists(QemuE2eHarness* self, char* source, char* target) {
    if (!FileSystem_exists(source)) {
        return;
    }
    FileSystem_removeRecursive(target);
    Command* cp = Command_capture(Command_arg(Command_arg(Command_arg(Command_new("cp"), "-R"), source), target), false);
    ExecResult* result = UnixShell_runCommand(self->shell, cp);
    if (!ExecResult_ok(result)) {
        int __fstr_663_len = snprintf(NULL, 0, "Failed to copy state tree %s", source);
        char* __fstr_663_buf = __btrc_str_track(((char*)malloc((__fstr_663_len + 1))));
        snprintf(__fstr_663_buf, (__fstr_663_len + 1), "Failed to copy state tree %s", source);
        NixosLog_fatal(__fstr_663_buf);
    }
}

void QemuE2eHarness_inheritState(QemuE2eHarness* self) {
    QemuE2eHarness_requireParentState(self);
    if ((strcmp(self->spec->parentState, "root") == 0) || __btrc_isEmpty(self->spec->parentState)) {
        return;
    }
    if (!FileSystem_exists(QemuE2eHarness_parentWorkDirFile(self))) {
        int __fstr_665_len = snprintf(NULL, 0, "Parent state %s has no workDir record", self->spec->parentState);
        char* __fstr_665_buf = __btrc_str_track(((char*)malloc((__fstr_665_len + 1))));
        snprintf(__fstr_665_buf, (__fstr_665_len + 1), "Parent state %s has no workDir record", self->spec->parentState);
        NixosLog_fatal(__fstr_665_buf);
    }
    char* parentWorkDir = __btrc_str_track(__btrc_trim(Path_readAll(QemuE2eHarness_parentWorkDirFile(self))));
    QemuE2eHarness_ensureWorkDir(self);
    char* parentDisk = PathTools_join(parentWorkDir, "disk.qcow2");
    if ((!FileSystem_exists(QemuE2eHarness_diskPath(self))) && FileSystem_exists(parentDisk)) {
        char* backing = QemuE2eHarness_absolutePath(self, parentDisk);
        Command* create = Command_capture(Command_arg(Command_arg(Command_arg(Command_arg(Command_arg(Command_arg(Command_arg(Command_arg(Command_new("qemu-img"), "create"), "-f"), "qcow2"), "-F"), "qcow2"), "-b"), backing), QemuE2eHarness_diskPath(self)), false);
        ExecResult* result = UnixShell_runCommand(self->shell, create);
        if (!ExecResult_ok(result)) {
            NixosLog_fatal(__btrc_str_track(__btrc_strcat("Failed to create qcow2 state delta for ", self->spec->state)));
        }
        Path_writeAll(QemuE2eHarness_backingDiskFile(self), __btrc_str_track(__btrc_strcat(backing, "\n")));
    }
    QemuE2eHarness_copyIfExists(self, PathTools_join(parentWorkDir, "id_ed25519"), QemuE2eHarness_sshKeyPath(self));
    QemuE2eHarness_copyIfExists(self, PathTools_join(parentWorkDir, "id_ed25519.pub"), QemuE2eHarness_sshPubKeyPath(self));
    QemuE2eHarness_copyIfExists(self, PathTools_join(parentWorkDir, "edk2-vars.fd"), QemuE2eHarness_firmwareVarsPath(self));
    QemuE2eHarness_copyTreeIfExists(self, PathTools_join(parentWorkDir, "tpm"), QemuE2eHarness_tpmDir(self));
}

void QemuE2eHarness_recordState(QemuE2eHarness* self) {
    FileSystem_mkdirp(VmTestSpec_stateDir(self->spec));
    Path_writeAll(VmTestSpec_stateHashFile(self->spec), __btrc_str_track(__btrc_strcat(self->spec->stateHash, "\n")));
    Path_writeAll(PathTools_join(VmTestSpec_stateDir(self->spec), "workDir"), __btrc_str_track(__btrc_strcat(self->spec->workDir, "\n")));
    JsonObject* metadata = JsonObject_new();
    JsonObject_setString(metadata, "state", self->spec->state);
    JsonObject_setString(metadata, "parentState", self->spec->parentState);
    JsonObject_setString(metadata, "parentHash", self->spec->parentHash);
    JsonObject_setString(metadata, "hash", self->spec->stateHash);
    JsonObject_setString(metadata, "hashShort", self->spec->stateHashShort);
    JsonObject_setString(metadata, "workDir", self->spec->workDir);
    JsonObject_setString(metadata, "iso", self->spec->iso);
    JsonObject_setString(metadata, "isoUrl", self->spec->isoUrl);
    JsonObject_setString(metadata, "disk", QemuE2eHarness_diskPath(self));
    JsonObject_setString(metadata, "uefi", (QemuE2eHarness_uefiEnabled(self) ? "true" : "false"));
    JsonObject_setString(metadata, "secureBoot", (QemuE2eHarness_secureBootEnabled(self) ? "true" : "false"));
    JsonObject_setString(metadata, "tpm2", (QemuE2eHarness_tpm2Enabled(self) ? "true" : "false"));
    if (FileSystem_exists(QemuE2eHarness_backingDiskFile(self))) {
        JsonObject_setString(metadata, "delta", "qcow2-backing");
        JsonObject_setString(metadata, "backingDisk", __btrc_str_track(__btrc_trim(Path_readAll(QemuE2eHarness_backingDiskFile(self)))));
    } else {
        JsonObject_setString(metadata, "delta", "base");
    }
    JsonObject_writeFile(metadata, PathTools_join(VmTestSpec_stateDir(self->spec), "metadata.json"));
    if (metadata != NULL) {
        if ((--metadata->__rc) <= 0) {
            JsonObject_destroy(metadata);
        }
    }
}

bool QemuE2eHarness_isRunning(QemuE2eHarness* self) {
    if (!FileSystem_exists(QemuE2eHarness_pidPath(self))) {
        __auto_type __btrc_ret_666 = false;
        return __btrc_ret_666;
    }
    ExecResult* result = UnixShell_runRaw(self->shell, __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("kill -0 $(cat ", UnixShell_quote(QemuE2eHarness_pidPath(self)))), ") 2>/dev/null")), false, false, "");
    __auto_type __btrc_ret_667 = ExecResult_ok(result);
    return __btrc_ret_667;
}

bool QemuE2eHarness_hasSnapshot(QemuE2eHarness* self, char* name) {
    if (!FileSystem_exists(QemuE2eHarness_diskPath(self))) {
        __auto_type __btrc_ret_668 = false;
        return __btrc_ret_668;
    }
    ExecResult* result = UnixShell_runUnchecked(self->shell, __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("qemu-img snapshot -l ", UnixShell_quote(QemuE2eHarness_diskPath(self)))), " | awk '{print $2}'")));
    if (!ExecResult_ok(result)) {
        __auto_type __btrc_ret_669 = false;
        return __btrc_ret_669;
    }
    btrc_Vector_string* lines = Strings_split(ExecResult_stdout(result), "\n");
    int __n_671 = btrc_Vector_string_iterLen(lines);
    for (int __i_670 = 0; (__i_670 < __n_671); (__i_670++)) {
        char* line = btrc_Vector_string_iterGet(lines, __i_670);
        if (strcmp(__btrc_str_track(__btrc_trim(line)), name) == 0) {
            __auto_type __btrc_ret_672 = true;
            return __btrc_ret_672;
        }
    }
    __auto_type __btrc_ret_673 = false;
    return __btrc_ret_673;
}

bool QemuE2eHarness_hasBackingDisk(QemuE2eHarness* self) {
    __auto_type __btrc_ret_674 = FileSystem_exists(QemuE2eHarness_backingDiskFile(self));
    return __btrc_ret_674;
}

char* QemuE2eHarness_sshOptionsForShell(QemuE2eHarness* self) {
    btrc_Vector_string* opts = btrc_Vector_string_new();
    btrc_Vector_string_push(opts, "-o");
    btrc_Vector_string_push(opts, "StrictHostKeyChecking=no");
    btrc_Vector_string_push(opts, "-o");
    btrc_Vector_string_push(opts, "UserKnownHostsFile=/dev/null");
    btrc_Vector_string_push(opts, "-o");
    btrc_Vector_string_push(opts, "LogLevel=ERROR");
    btrc_Vector_string_push(opts, "-o");
    btrc_Vector_string_push(opts, "BatchMode=yes");
    btrc_Vector_string_push(opts, "-o");
    btrc_Vector_string_push(opts, "IdentitiesOnly=yes");
    btrc_Vector_string_push(opts, "-o");
    btrc_Vector_string_push(opts, "ConnectTimeout=3");
    btrc_Vector_string_push(opts, "-o");
    btrc_Vector_string_push(opts, "ConnectionAttempts=1");
    btrc_Vector_string_push(opts, "-i");
    btrc_Vector_string_push(opts, QemuE2eHarness_sshKeyPath(self));
    btrc_Vector_string* quoted = btrc_Vector_string_new();
    int __n_676 = btrc_Vector_string_iterLen(opts);
    for (int __i_675 = 0; (__i_675 < __n_676); (__i_675++)) {
        char* opt = btrc_Vector_string_iterGet(opts, __i_675);
        btrc_Vector_string_push(quoted, UnixShell_quote(opt));
    }
    __auto_type __btrc_ret_677 = btrc_Vector_string_join(quoted, " ");
    return __btrc_ret_677;
}

void QemuE2eHarness_printStatus(QemuE2eHarness* self) {
    char* recorded = "no";
    if (FileSystem_exists(VmTestSpec_stateHashFile(self->spec))) {
        char* savedHash = __btrc_str_track(__btrc_trim(Path_readAll(VmTestSpec_stateHashFile(self->spec))));
        (recorded = ((strcmp(savedHash, self->spec->stateHash) == 0) ? "yes" : "stale"));
    }
    Console_log(__btrc_str_track(__btrc_strcat("State:     ", self->spec->state)));
    Console_log(__btrc_str_track(__btrc_strcat("Hash:      ", self->spec->stateHash)));
    Console_log(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("Parent:    ", self->spec->parentState)), " (")), self->spec->parentHash)), ")")));
    Console_log(__btrc_str_track(__btrc_strcat("Work dir:  ", self->spec->workDir)));
    Console_log(__btrc_str_track(__btrc_strcat("Arch:      ", self->spec->arch)));
    Console_log(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("ISO:       ", (FileSystem_exists(self->spec->iso) ? "yes" : "no"))), " ")), self->spec->iso)));
    Console_log(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("Disk:      ", (FileSystem_exists(QemuE2eHarness_diskPath(self)) ? "yes" : "no"))), " ")), QemuE2eHarness_diskPath(self))));
    Console_log(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("SSH key:   ", (FileSystem_exists(QemuE2eHarness_sshKeyPath(self)) ? "yes" : "no"))), " ")), QemuE2eHarness_sshKeyPath(self))));
    Console_log(__btrc_str_track(__btrc_strcat("Running:   ", (QemuE2eHarness_isRunning(self) ? "yes" : "no"))));
    Console_log(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("Recorded:  ", recorded)), " ")), VmTestSpec_stateDir(self->spec))));
}

void QemuE2eHarness_prepareSerialPipe(QemuE2eHarness* self) {
    FileSystem_removeRecursive(QemuE2eHarness_serialInPath(self));
    FileSystem_removeRecursive(QemuE2eHarness_serialOutPath(self));
    Command* inPipe = Command_new("mkfifo");
    Command_arg(inPipe, QemuE2eHarness_serialInPath(self));
    Command_capture(inPipe, false);
    ExecResult* inResult = UnixShell_runCommand(self->shell, inPipe);
    if (!ExecResult_ok(inResult)) {
        NixosLog_fatal("Failed to create VM serial input pipe");
    }
    Command* outPipe = Command_new("mkfifo");
    Command_arg(outPipe, QemuE2eHarness_serialOutPath(self));
    Command_capture(outPipe, false);
    ExecResult* outResult = UnixShell_runCommand(self->shell, outPipe);
    if (!ExecResult_ok(outResult)) {
        NixosLog_fatal("Failed to create VM serial output pipe");
    }
    if (outPipe != NULL) {
        if ((--outPipe->__rc) <= 0) {
            Command_destroy(outPipe);
        }
    }
    if (inPipe != NULL) {
        if ((--inPipe->__rc) <= 0) {
            Command_destroy(inPipe);
        }
    }
}

void QemuE2eHarness_startSerialReader(QemuE2eHarness* self) {
    FileSystem_removeRecursive(QemuE2eHarness_serialReaderPidPath(self));
    FileSystem_removeRecursive(QemuE2eHarness_serialLogPath(self));
    char* outPath = UnixShell_quote(QemuE2eHarness_serialOutPath(self));
    char* logPath = UnixShell_quote(QemuE2eHarness_serialLogPath(self));
    char* pidPath = UnixShell_quote(QemuE2eHarness_serialReaderPidPath(self));
    UnixShell_runRaw(self->shell, __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("(cat ", outPath)), " >> ")), logPath)), ") & echo $! > ")), pidPath)), false, false, "");
}

void QemuE2eHarness_stopSerialReader(QemuE2eHarness* self) {
    if (!FileSystem_exists(QemuE2eHarness_serialReaderPidPath(self))) {
        return;
    }
    char* pidPath = UnixShell_quote(QemuE2eHarness_serialReaderPidPath(self));
    UnixShell_runRaw(self->shell, __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("pid=$(cat ", pidPath)), "); kill $pid 2>/dev/null || true")), false, false, "");
    FileSystem_removeRecursive(QemuE2eHarness_serialReaderPidPath(self));
}

void QemuE2eHarness_startSwtpm(QemuE2eHarness* self) {
    if (!QemuE2eHarness_tpm2Enabled(self)) {
        return;
    }
    QemuE2eHarness_requireTpm2Capability(self);
    FileSystem_mkdirp(QemuE2eHarness_tpmStateDir(self));
    FileSystem_removeRecursive(QemuE2eHarness_tpmSocketPath(self));
    if (FileSystem_exists(QemuE2eHarness_tpmPidPath(self))) {
        ExecResult* running = UnixShell_runRaw(self->shell, __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("kill -0 $(cat ", UnixShell_quote(QemuE2eHarness_tpmPidPath(self)))), ") 2>/dev/null")), false, false, "");
        if (ExecResult_ok(running)) {
            return;
        }
        FileSystem_removeRecursive(QemuE2eHarness_tpmPidPath(self));
    }
    Command* cmd = Command_capture(Command_arg(Command_arg(Command_arg(Command_arg(Command_arg(Command_arg(Command_arg(Command_arg(Command_arg(Command_arg(Command_arg(Command_new("swtpm"), "socket"), "--tpm2"), "--tpmstate"), __btrc_str_track(__btrc_strcat("dir=", QemuE2eHarness_tpmStateDir(self)))), "--ctrl"), __btrc_str_track(__btrc_strcat("type=unixio,path=", QemuE2eHarness_tpmSocketPath(self)))), "--daemon"), "--pid"), __btrc_str_track(__btrc_strcat("file=", QemuE2eHarness_tpmPidPath(self)))), "--log"), __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("file=", QemuE2eHarness_tpmLogPath(self))), ",level=20"))), false);
    ExecResult* result = UnixShell_runCommand(self->shell, cmd);
    if (!ExecResult_ok(result)) {
        NixosLog_fatal("Failed to start swtpm");
    }
    char* waitCommand = __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("for i in 1 2 3 4 5 6 7 8 9 10; do test -S ", UnixShell_quote(QemuE2eHarness_tpmSocketPath(self)))), " && exit 0; sleep 1; done; exit 1"));
    ExecResult* ready = UnixShell_runRaw(self->shell, waitCommand, false, false, "");
    if (!ExecResult_ok(ready)) {
        NixosLog_fatal("swtpm socket did not become ready");
    }
}

void QemuE2eHarness_stopSwtpm(QemuE2eHarness* self) {
    if (FileSystem_exists(QemuE2eHarness_tpmPidPath(self))) {
        char* quotedPid = UnixShell_quote(QemuE2eHarness_tpmPidPath(self));
        UnixShell_runRaw(self->shell, __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("pid=$(cat ", quotedPid)), "); kill $pid 2>/dev/null || true")), false, false, "");
        FileSystem_removeRecursive(QemuE2eHarness_tpmPidPath(self));
    }
    FileSystem_removeRecursive(QemuE2eHarness_tpmSocketPath(self));
}

void QemuE2eHarness_addFirmware(QemuE2eHarness* self, Command* cmd, bool fromIso) {
    if (!QemuE2eHarness_shouldUseUefi(self, fromIso)) {
        return;
    }
    QemuE2eHarness_requireUefiCapability(self);
    if (QemuE2eHarness_secureBootEnabled(self)) {
        QemuE2eHarness_requireSecureBootCapability(self);
    }
    if (strcmp(self->spec->arch, "x86_64") == 0) {
        Command_arg(cmd, "-machine");
        Command_arg(cmd, "q35");
    }
    QemuE2eHarness_setupFirmwareVars(self);
    char* codePath = QemuE2eHarness_firmwareCodePath(self);
    if (__btrc_isEmpty(codePath)) {
        NixosLog_fatal(__btrc_str_track(__btrc_strcat("Could not find EDK2 firmware for ", self->spec->arch)));
    }
    Command_arg(cmd, "-drive");
    Command_arg(cmd, __btrc_str_track(__btrc_strcat("if=pflash,format=raw,readonly=on,file=", codePath)));
    Command_arg(cmd, "-drive");
    Command_arg(cmd, __btrc_str_track(__btrc_strcat("if=pflash,format=raw,file=", QemuE2eHarness_firmwareVarsPath(self))));
}

void QemuE2eHarness_addTpm2(QemuE2eHarness* self, Command* cmd) {
    if (!QemuE2eHarness_tpm2Enabled(self)) {
        return;
    }
    QemuE2eHarness_startSwtpm(self);
    Command_arg(cmd, "-chardev");
    Command_arg(cmd, __btrc_str_track(__btrc_strcat("socket,id=chrtpm,path=", QemuE2eHarness_tpmSocketPath(self))));
    Command_arg(cmd, "-tpmdev");
    Command_arg(cmd, "emulator,id=tpm0,chardev=chrtpm");
    Command_arg(cmd, "-device");
    Command_arg(cmd, __btrc_str_track(__btrc_strcat(QemuE2eHarness_tpmQemuDevice(self), ",tpmdev=tpm0")));
}

void QemuE2eHarness_start(QemuE2eHarness* self, bool fromIso) {
    QemuE2eHarness_ensureWorkDir(self);
    if (FileSystem_exists(QemuE2eHarness_pidPath(self))) {
        NixosLog_info("QEMU pidfile exists; assuming VM is already running");
        return;
    }
    Command* cmd = Command_new(QemuE2eHarness_qemuBinary(self));
    if (QemuE2eHarness_isDarwin(self)) {
        Command_arg(cmd, "-accel");
        Command_arg(cmd, (QemuE2eHarness_hostArchMatchesGuest(self) ? "hvf" : "tcg"));
    } else if (QemuE2eHarness_hostArchMatchesGuest(self)) {
        Command_arg(cmd, "-enable-kvm");
    } else {
        Command_arg(cmd, "-accel");
        Command_arg(cmd, "tcg");
    }
    if (strcmp(self->spec->arch, "aarch64") == 0) {
        Command_arg(cmd, "-machine");
        Command_arg(cmd, "virt");
        Command_arg(cmd, "-cpu");
        Command_arg(cmd, (QemuE2eHarness_hostArchMatchesGuest(self) ? "host" : "max"));
        Command_arg(cmd, "-device");
        Command_arg(cmd, "virtio-rng-pci");
    }
    QemuE2eHarness_addFirmware(self, cmd, fromIso);
    Command_arg(cmd, "-m");
    Command_arg(cmd, self->spec->memory);
    Command_arg(cmd, "-smp");
    Command_arg(cmd, Strings_fromInt(self->spec->cpus));
    Command_arg(cmd, "-drive");
    Command_arg(cmd, __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("file=", QemuE2eHarness_diskPath(self))), ",format=qcow2,if=virtio,cache=none")));
    Command_arg(cmd, "-netdev");
    Command_arg(cmd, __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("user,id=net0,hostfwd=tcp::", Strings_fromInt(self->spec->sshPort))), "-:22")));
    Command_arg(cmd, "-device");
    Command_arg(cmd, "virtio-net-pci,netdev=net0");
    Command_arg(cmd, "-display");
    Command_arg(cmd, "none");
    Command_arg(cmd, "-serial");
    Command_arg(cmd, __btrc_str_track(__btrc_strcat("pipe:", QemuE2eHarness_serialBasePath(self))));
    Command_arg(cmd, "-monitor");
    Command_arg(cmd, __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("unix:", QemuE2eHarness_monitorPath(self))), ",server,nowait")));
    Command_arg(cmd, "-qmp");
    Command_arg(cmd, __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("unix:", QemuE2eHarness_qmpPath(self))), ",server,nowait")));
    QemuE2eHarness_addTpm2(self, cmd);
    if (fromIso) {
        if ((strcmp(self->spec->arch, "x86_64") == 0) && (!QemuE2eHarness_shouldUseUefi(self, fromIso))) {
            QemuE2eHarness_extractBootSerial(self);
            Command_arg(cmd, "-kernel");
            Command_arg(cmd, __btrc_str_track(__btrc_trim(Path_readAll(QemuE2eHarness_bootKernelFile(self)))));
            Command_arg(cmd, "-initrd");
            Command_arg(cmd, __btrc_str_track(__btrc_trim(Path_readAll(QemuE2eHarness_bootInitrdFile(self)))));
            Command_arg(cmd, "-append");
            Command_arg(cmd, __btrc_str_track(__btrc_trim(Path_readAll(QemuE2eHarness_bootAppendFile(self)))));
            Command_arg(cmd, "-cdrom");
            Command_arg(cmd, self->spec->iso);
        } else {
            Command_arg(cmd, "-cdrom");
            Command_arg(cmd, self->spec->iso);
            Command_arg(cmd, "-boot");
            Command_arg(cmd, "d");
        }
    }
    QemuE2eHarness_prepareSerialPipe(self);
    QemuE2eHarness_startSerialReader(self);
    Command_arg(cmd, "-daemonize");
    Command_arg(cmd, "-pidfile");
    Command_arg(cmd, QemuE2eHarness_pidPath(self));
    Command_capture(cmd, false);
    ExecResult* result = UnixShell_runCommand(self->shell, cmd);
    if (!ExecResult_ok(result)) {
        QemuE2eHarness_stopSerialReader(self);
        QemuE2eHarness_stopSwtpm(self);
        NixosLog_fatal("Failed to start QEMU");
    }
    if (cmd != NULL) {
        if ((--cmd->__rc) <= 0) {
            Command_destroy(cmd);
        }
    }
}

void QemuE2eHarness_stop(QemuE2eHarness* self) {
    QemuE2eHarness_stopSerialReader(self);
    if (FileSystem_exists(QemuE2eHarness_pidPath(self))) {
        char* quotedPid = UnixShell_quote(QemuE2eHarness_pidPath(self));
        UnixShell_runRaw(self->shell, __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("pid=$(cat ", quotedPid)), "); kill $pid 2>/dev/null || true; sleep 2; kill -0 $pid 2>/dev/null && kill -9 $pid 2>/dev/null || true")), false, false, "");
        FileSystem_removeRecursive(QemuE2eHarness_pidPath(self));
    }
    FileSystem_removeRecursive(QemuE2eHarness_monitorPath(self));
    FileSystem_removeRecursive(QemuE2eHarness_qmpPath(self));
    FileSystem_removeRecursive(QemuE2eHarness_serialInPath(self));
    FileSystem_removeRecursive(QemuE2eHarness_serialOutPath(self));
    QemuE2eHarness_stopSwtpm(self);
}

void QemuE2eHarness_sleepSeconds(QemuE2eHarness* self, int seconds) {
    UnixShell_runRaw(self->shell, __btrc_str_track(__btrc_strcat("sleep ", Strings_fromInt(seconds))), false, false, "");
}

void QemuE2eHarness_serialSend(QemuE2eHarness* self, char* command) {
    char* payload = __btrc_str_track(__btrc_strcat(command, "\n"));
    UnixShell_runRaw(self->shell, __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("printf %s ", UnixShell_quote(payload))), " > ")), UnixShell_quote(QemuE2eHarness_serialInPath(self)))), false, false, "");
}

void QemuE2eHarness_bootstrapSsh(QemuE2eHarness* self) {
    QemuE2eHarness_createSshKey(self);
    char* key = QemuE2eHarness_sshPubKey(self);
    QemuE2eHarness_serialSend(self, "");
    QemuE2eHarness_serialSend(self, __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("sudo install -d -m 700 /root/.ssh; echo ", UnixShell_quote(key))), " | sudo tee /root/.ssh/authorized_keys > /dev/null; sudo chmod 600 /root/.ssh/authorized_keys; sudo systemctl restart sshd")));
    QemuE2eHarness_sleepSeconds(self, 5);
}

void QemuE2eHarness_upFromIso(QemuE2eHarness* self) {
    QemuE2eHarness_setup(self);
    QemuE2eHarness_start(self, true);
    QemuE2eHarness_sleepSeconds(self, 20);
    QemuE2eHarness_bootstrapSsh(self);
    if (!QemuE2eHarness_waitForSsh(self, 180)) {
        NixosLog_fatal("SSH was not ready after booting installer ISO");
    }
}

void QemuE2eHarness_rebootDisk(QemuE2eHarness* self) {
    QemuE2eHarness_ssh(self, "reboot", false);
    QemuE2eHarness_sleepSeconds(self, 5);
    QemuE2eHarness_stop(self);
    QemuE2eHarness_start(self, false);
    if (!QemuE2eHarness_waitForSsh(self, 180)) {
        NixosLog_fatal("SSH was not ready after disk reboot");
    }
}

Command* QemuE2eHarness_addSshOptions(QemuE2eHarness* self, Command* cmd) {
    __auto_type __btrc_ret_678 = Command_arg(Command_arg(Command_arg(Command_arg(Command_arg(Command_arg(Command_arg(Command_arg(Command_arg(Command_arg(Command_arg(Command_arg(Command_arg(Command_arg(Command_arg(Command_arg(cmd, "-o"), "StrictHostKeyChecking=no"), "-o"), "UserKnownHostsFile=/dev/null"), "-o"), "LogLevel=ERROR"), "-o"), "BatchMode=yes"), "-o"), "IdentitiesOnly=yes"), "-o"), "ConnectTimeout=3"), "-o"), "ConnectionAttempts=1"), "-i"), QemuE2eHarness_sshKeyPath(self));
    return __btrc_ret_678;
}

ExecResult* QemuE2eHarness_sshWithTimeout(QemuE2eHarness* self, char* command, bool checkStatus, int timeoutSeconds) {
    Command* cmd = Command_check(Command_arg(Command_arg(QemuE2eHarness_addSshOptions(self, Command_arg(Command_arg(Command_arg(Command_arg(Command_new("timeout"), Strings_fromInt(timeoutSeconds)), "ssh"), "-p"), Strings_fromInt(self->spec->sshPort))), "root@localhost"), command), checkStatus);
    ExecResult* result = UnixShell_runCommand(self->shell, cmd);
    return result;
}

ExecResult* QemuE2eHarness_ssh(QemuE2eHarness* self, char* command, bool checkStatus) {
    __auto_type __btrc_ret_679 = QemuE2eHarness_sshWithTimeout(self, command, checkStatus, 5);
    return __btrc_ret_679;
}

bool QemuE2eHarness_waitForSsh(QemuE2eHarness* self, int timeout) {
    int elapsed = 0;
    while (elapsed < timeout) {
        ExecResult* result = QemuE2eHarness_ssh(self, "true", false);
        if (ExecResult_ok(result)) {
            __auto_type __btrc_ret_680 = true;
            return __btrc_ret_680;
        }
        QemuE2eHarness_sleepSeconds(self, 3);
        (elapsed = (elapsed + 8));
        if ((elapsed > 0) && (__btrc_mod_int(elapsed, 24) == 0)) {
            QemuE2eHarness_bootstrapSsh(self);
        }
    }
    __auto_type __btrc_ret_681 = false;
    return __btrc_ret_681;
}

ExecResult* QemuE2eHarness_host(QemuE2eHarness* self, char* command, bool checkStatus) {
    __auto_type __btrc_ret_682 = UnixShell_runRaw(self->shell, command, true, checkStatus, "");
    return __btrc_ret_682;
}

char* QemuE2eHarness_workspaceRoot(QemuE2eHarness* self) {
    __auto_type __btrc_ret_683 = btrc_Map_string_string_getOrDefault(self->spec->args, "workspaceRoot", ".");
    return __btrc_ret_683;
}

ExecResult* QemuE2eHarness_workspaceFileExists(QemuE2eHarness* self, char* relativePath) {
    char* command = __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("cd ", UnixShell_quote(QemuE2eHarness_workspaceRoot(self)))), " && test -f ")), UnixShell_quote(relativePath))), " && printf exists"));
    __auto_type __btrc_ret_684 = UnixShell_runRaw(self->shell, command, true, false, "");
    return __btrc_ret_684;
}

ExecResult* QemuE2eHarness_nixEval(QemuE2eHarness* self, char* attribute, int timeoutSeconds) {
    char* command = __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("cd ", UnixShell_quote(QemuE2eHarness_workspaceRoot(self)))), " && timeout ")), Strings_fromInt(timeoutSeconds))), " nix eval ")), UnixShell_quote(__btrc_str_track(__btrc_strcat(".#", attribute))))), " --show-trace"));
    __auto_type __btrc_ret_685 = UnixShell_runRaw(self->shell, command, true, false, "");
    return __btrc_ret_685;
}

ExecResult* QemuE2eHarness_qmp(QemuE2eHarness* self, char* command, int timeoutSeconds) {
    if (!FileSystem_exists(QemuE2eHarness_qmpPath(self))) {
        __auto_type __btrc_ret_686 = ExecResult_new(1, "", __btrc_str_track(__btrc_strcat("QMP socket does not exist: ", QemuE2eHarness_qmpPath(self))), "");
        return __btrc_ret_686;
    }
    char* payload = __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("{\"execute\":\"qmp_capabilities\"}\n", command)), "\n"));
    char* rendered = __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("printf %s ", UnixShell_quote(payload))), " | timeout ")), Strings_fromInt(timeoutSeconds))), " socat - ")), UnixShell_quote(__btrc_str_track(__btrc_strcat("UNIX-CONNECT:", QemuE2eHarness_qmpPath(self))))));
    __auto_type __btrc_ret_687 = UnixShell_runRaw(self->shell, rendered, true, false, "");
    return __btrc_ret_687;
}

void QemuE2eHarness_copyWorkspace(QemuE2eHarness* self, char* localPath, char* remotePath) {
    char* source = (__btrc_isEmpty(localPath) ? ".." : localPath);
    char* target = (__btrc_isEmpty(remotePath) ? "/etc/nixos" : remotePath);
    char* remote = __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("mkdir -p ", UnixShell_quote(target))), " && tar xzf - -C ")), UnixShell_quote(target)));
    char* tar = __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("tar czf - --exclude=.vm --exclude=build --exclude=secrets --exclude=.git --exclude=result --exclude='._*' -C ", UnixShell_quote(source))), " ."));
    char* ssh = __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("ssh -p ", Strings_fromInt(self->spec->sshPort))), " ")), QemuE2eHarness_sshOptionsForShell(self))), " root@localhost ")), UnixShell_quote(remote)));
    ExecResult* result = UnixShell_runRaw(self->shell, __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(tar, " | ")), ssh)), false, true, "");
    if (!ExecResult_ok(result)) {
        NixosLog_fatal("Failed to copy workspace into VM");
    }
}

void QemuE2eHarness_configureVmHost(QemuE2eHarness* self) {
    char* mode = btrc_Map_string_string_getOrDefault(self->spec->args, "immutabilityMode", "reset");
    char* enabled = btrc_Map_string_string_getOrDefault(self->spec->args, "immutabilityEnabled", "true");
    char* bootTargetName = btrc_Map_string_string_getOrDefault(self->spec->args, "bootTarget", "Standard-Boot");
    char* desktop = btrc_Map_string_string_getOrDefault(self->spec->args, "desktop", "none");
    char* encryption = btrc_Map_string_string_getOrDefault(self->spec->args, "encryption", "false");
    char* luksPass = "testpass123";
    bool secureBootTarget = (strcmp(bootTargetName, "Secure-Boot") == 0);
    char* otherArch = ((strcmp(self->spec->arch, "aarch64") == 0) ? "x86_64" : "aarch64");
    char* serialConsole = ((strcmp(self->spec->arch, "aarch64") == 0) ? "ttyAMA0,115200n8" : "ttyS0,115200n8");
    char* initrdModules = ((strcmp(self->spec->arch, "aarch64") == 0) ? "[ \"virtio_pci\" \"virtio_blk\" \"virtio_net\" \"virtio_mmio\" \"virtio_rng\" \"tpm_tis\" \"tpm_tis_core\" ]" : "[ \"virtio_pci\" \"virtio_blk\" \"virtio_net\" \"tpm_tis\" ]");
    char* hostDir = __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("/etc/nixos/modules/hosts/", self->spec->arch)), "/VM-TEST"));
    char* hostPath = __btrc_str_track(__btrc_strcat(hostDir, "/VM-TEST.nix"));
    char* pubkey = QemuE2eHarness_sshPubKey(self);
    char* passwordHash = "$6$vmtest$zfOOVFBtEpiYVE486Cybmppi9zXd0QKYfRZ5FRLsMV26K7vK6QHIpZpUo1sxVt7liGoR9C/W1I5ih4VCt34n3.";
    char* secureBootSettings = (secureBootTarget ? "  settings.boot.pkiBundle = \"/var/lib/sbctl\";\n" : "");
    char* secureBootModulePath = "/etc/nixos/modules/system/vm-test-secure-boot.nix";
    char* secureBootModule = __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("{ config, lib, pkgs, ... }: {\n", "  config = lib.mkIf (config.networking.hostName == \"VM-TEST\" && config.settings.boot.method == \"Secure-Boot\") {\n")), "    boot.loader.timeout = lib.mkForce 0;\n")), "    boot.lanzaboote.autoGenerateKeys.enable = true;\n")), "    boot.lanzaboote.autoEnrollKeys.enable = true;\n")), "    boot.lanzaboote.allowUnsigned = true;\n")), "    boot.lanzaboote.autoEnrollKeys.includeMicrosoftKeys = true;\n")), "    boot.lanzaboote.autoEnrollKeys.autoReboot = false;\n")), "    environment.systemPackages = [ pkgs.sbctl ];\n")), "  };\n")), "}\n"));
    char* secureBootModuleCommand = (secureBootTarget ? __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("printf %s ", UnixShell_quote(secureBootModule))), " > ")), UnixShell_quote(secureBootModulePath))), "; ")) : "");
    char* hostNix = __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("{ lib, ... }: {\n", "  networking.hostName = \"VM-TEST\";\n")), "  networking.firewall.enable = lib.mkForce false;\n")), "  users.users.root.openssh.authorizedKeys.keys = [ \"")), pubkey)), "\" ];\n")), "  boot.kernelParams = [ \"console=")), serialConsole)), "\" ];\n")), "  boot.initrd.availableKernelModules = ")), initrdModules)), ";\n")), "  boot.plymouth.enable = lib.mkForce false;\n")), "  services.getty.autologinUser = lib.mkForce \"root\";\n")), "  services.openssh.enable = lib.mkForce true;\n")), "  services.openssh.settings.PermitRootLogin = lib.mkForce \"yes\";\n")), "  services.openssh.settings.UseDns = lib.mkForce false;\n")), "  settings.user.admin.autoUnlockWallet.enabled = false;\n")), "  settings.user.admin.homeManager.enable = false;\n")), "  settings.networking.identityAgent = \"none\";\n")), "  settings.networking.lanSubnet = \"10.0.2.0/24\";\n")), "  settings.disk.device = \"/dev/vda\";\n")), "  settings.disk.encryption.enable = ")), encryption)), ";\n")), "  settings.disk.immutability.enable = ")), enabled)), ";\n")), "  settings.disk.immutability.mode = \"")), mode)), "\";\n")), "  settings.disk.immutability.persist.paths = lib.mkForce [ \"/etc/machine-id\" \"/etc/nixos\" \"/var/lib/nixos\" \"/var/lib/sbctl\" \"/var/log\" ];\n")), "  settings.disk.immutability.semipermeable_membrane.mode = \"")), mode)), "\";\n")), "  settings.desktop.enable = ")), ((strcmp(desktop, "none") == 0) ? "false" : "true"))), ";\n")), "  settings.disk.swap.size = \"2G\";\n")), secureBootSettings)), "}\n"));
    char* localConfig = __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("{\"host_path\":\"modules/hosts/", self->spec->arch)), "/VM-TEST/VM-TEST.nix\",\"target\":\"")), bootTargetName)), "\"}\n"));
    char* luksCommand = ((strcmp(encryption, "true") == 0) ? __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("printf %s ", UnixShell_quote(luksPass))), " > /tmp/plain_text_password.txt; chmod 600 /tmp/plain_text_password.txt; ")) : "");
    char* command = __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("set -e; ", luksCommand)), "rm -rf /etc/nixos/modules/hosts/")), otherArch)), "; ")), "rm -f ")), UnixShell_quote(secureBootModulePath))), "; ")), secureBootModuleCommand)), "find /etc/nixos/modules/apps -maxdepth 1 -type f -name '*.nix' ! -name 'nixosctl.nix' -delete; ")), "find /etc/nixos/modules/apps/home-manager -maxdepth 1 -type f -name '*.nix' -delete 2>/dev/null || true; ")), "mkdir -p /etc/nixos/modules/apps/home-manager; ")), "touch /etc/nixos/modules/apps/home-manager/.keep; ")), "mkdir -p /etc/nixos/secrets; ")), "printf %s ")), UnixShell_quote(passwordHash))), " > /etc/nixos/secrets/hashed_password.txt; ")), "chmod 600 /etc/nixos/secrets/hashed_password.txt; ")), "mkdir -p ")), UnixShell_quote(hostDir))), "; ")), "printf %s ")), UnixShell_quote(hostNix))), " > ")), UnixShell_quote(hostPath))), "; ")), "printf %s ")), UnixShell_quote(localConfig))), " > /etc/nixos/config.json; ")), "git -C /etc/nixos init; ")), "git config --global --add safe.directory /etc/nixos; ")), "git -C /etc/nixos add -A; ")), "git -C /etc/nixos -c user.name=test -c user.email=test@test commit -m init --allow-empty"));
    ExecResult* result = QemuE2eHarness_sshWithTimeout(self, command, false, 120);
    if (!ExecResult_ok(result)) {
        Console_error(ExecResult_stdout(result));
        NixosLog_fatal("Failed to configure VM host");
    }
}

void QemuE2eHarness_installNixosGuest(QemuE2eHarness* self) {
    char* diskTarget = "VM-TEST-Disk-Operation";
    char* bootTargetName = btrc_Map_string_string_getOrDefault(self->spec->args, "bootTarget", "Standard-Boot");
    bool secureBootTarget = (strcmp(bootTargetName, "Secure-Boot") == 0);
    char* bootTarget = __btrc_str_track(__btrc_strcat("VM-TEST-", bootTargetName));
    char* nix = "nix --extra-experimental-features nix-command --extra-experimental-features flakes";
    char* disko = __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(nix, " run --inputs-from /etc/nixos github:nix-community/disko -- --show-trace --flake /etc/nixos#")), diskTarget)), " --mode destroy,format,mount --root-mountpoint /mnt --yes-wipe-all-disks"));
    char* install = __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("mkdir -p /mnt/etc; rm -rf /mnt/etc/nixos; cp -r /etc/nixos /mnt/etc/nixos; mkdir -p /mnt/nix/tmp; TMPDIR=/mnt/nix/tmp nixos-install --flake /mnt/etc/nixos#", bootTarget)), " --root /mnt --no-channel-copy --show-trace --no-root-password --cores 0"));
    char* lanzabooteInstall = (secureBootTarget ? __btrc_str_track(__btrc_strcat("nixos-enter --root /mnt --command ", UnixShell_quote("set -e; cmd=$(nix --extra-experimental-features nix-command --extra-experimental-features flakes eval --raw /etc/nixos#nixosConfigurations.VM-TEST-Secure-Boot.config.boot.lanzaboote.installCommand); tmp=$(mktemp -d); ln -s /nix/var/nix/profiles/system \"$tmp/system-1-link\"; sh -c \"$cmd --public-key /var/lib/sbctl/keys/db/db.pem --private-key /var/lib/sbctl/keys/db/db.key /boot $tmp/system-1-link\"; rm -rf \"$tmp\""))) : "true");
    char* snapshots = __btrc_str_track(__btrc_strcat("nixos-enter --root /mnt --command ", UnixShell_quote("set -e; mkdir -p /.snapshots/@root /.snapshots/@home; rm -rf /.snapshots/@root/CLEAN /.snapshots/@home/CLEAN; btrfs subvolume snapshot -r / /.snapshots/@root/CLEAN; btrfs subvolume snapshot -r /home /.snapshots/@home/CLEAN; git config --global --add safe.directory /etc/nixos || true")));
    char* command = __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("set -e; ", disko)), "; ")), install)), "; ")), lanzabooteInstall)), "; ")), snapshots)), "; echo INSTALL_OK"));
    int timeoutSeconds = (secureBootTarget ? 7200 : 3600);
    ExecResult* result = QemuE2eHarness_sshWithTimeout(self, command, false, timeoutSeconds);
    if ((!ExecResult_ok(result)) || (!__btrc_strContains(ExecResult_stdout(result), "INSTALL_OK"))) {
        Console_error(ExecResult_stdout(result));
        QemuE2eHarness_stop(self);
        NixosLog_fatal("NixOS installation failed");
    }
}

void QemuE2eHarness_snapshot(QemuE2eHarness* self, char* name) {
    QemuE2eHarness_stop(self);
    Command* cmd = Command_capture(Command_arg(Command_arg(Command_arg(Command_arg(Command_new("qemu-img"), "snapshot"), "-c"), name), QemuE2eHarness_diskPath(self)), false);
    ExecResult* result = UnixShell_runCommand(self->shell, cmd);
    if (!ExecResult_ok(result)) {
        NixosLog_fatal(__btrc_str_track(__btrc_strcat("Failed to create VM snapshot ", name)));
    }
    if (FileSystem_exists(QemuE2eHarness_firmwareVarsPath(self))) {
        Command* cp = Command_capture(Command_arg(Command_arg(Command_new("cp"), QemuE2eHarness_firmwareVarsPath(self)), QemuE2eHarness_firmwareVarsSnapshotPath(self, name)), false);
        UnixShell_runCommand(self->shell, cp);
    }
}

void QemuE2eHarness_restore(QemuE2eHarness* self, char* name) {
    QemuE2eHarness_stop(self);
    if ((!QemuE2eHarness_hasSnapshot(self, name)) && QemuE2eHarness_hasBackingDisk(self)) {
        NixosLog_info(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("Snapshot ", name)), " is inherited through qcow2 backing; restore is a no-op")));
        return;
    }
    Command* cmd = Command_capture(Command_arg(Command_arg(Command_arg(Command_arg(Command_new("qemu-img"), "snapshot"), "-a"), name), QemuE2eHarness_diskPath(self)), false);
    ExecResult* result = UnixShell_runCommand(self->shell, cmd);
    if (!ExecResult_ok(result)) {
        NixosLog_fatal(__btrc_str_track(__btrc_strcat("Failed to restore VM snapshot ", name)));
    }
    if (FileSystem_exists(QemuE2eHarness_firmwareVarsSnapshotPath(self, name))) {
        Command* cp = Command_capture(Command_arg(Command_arg(Command_new("cp"), QemuE2eHarness_firmwareVarsSnapshotPath(self, name)), QemuE2eHarness_firmwareVarsPath(self)), false);
        UnixShell_runCommand(self->shell, cp);
        QemuE2eHarness_makeFirmwareVarsWritable(self);
    }
}

void QemuE2eHarness_copyTo(QemuE2eHarness* self, char* localPath, char* remotePath) {
    int __fstr_688_len = snprintf(NULL, 0, "root@localhost:%s", remotePath);
    char* __fstr_688_buf = __btrc_str_track(((char*)malloc((__fstr_688_len + 1))));
    snprintf(__fstr_688_buf, (__fstr_688_len + 1), "root@localhost:%s", remotePath);
    Command* cmd = Command_capture(Command_arg(Command_arg(QemuE2eHarness_addSshOptions(self, Command_arg(Command_arg(Command_arg(Command_new("scp"), "-r"), "-P"), Strings_fromInt(self->spec->sshPort))), localPath), __fstr_688_buf), false);
    ExecResult* result = UnixShell_runCommand(self->shell, cmd);
    if (!ExecResult_ok(result)) {
        NixosLog_fatal("Failed to copy file to VM");
    }
}

void QemuE2eHarness_copyFrom(QemuE2eHarness* self, char* remotePath, char* localPath) {
    int __fstr_689_len = snprintf(NULL, 0, "root@localhost:%s", remotePath);
    char* __fstr_689_buf = __btrc_str_track(((char*)malloc((__fstr_689_len + 1))));
    snprintf(__fstr_689_buf, (__fstr_689_len + 1), "root@localhost:%s", remotePath);
    Command* cmd = Command_capture(Command_arg(Command_arg(QemuE2eHarness_addSshOptions(self, Command_arg(Command_arg(Command_arg(Command_new("scp"), "-r"), "-P"), Strings_fromInt(self->spec->sshPort))), __fstr_689_buf), localPath), false);
    ExecResult* result = UnixShell_runCommand(self->shell, cmd);
    if (!ExecResult_ok(result)) {
        NixosLog_fatal("Failed to copy file from VM");
    }
}

void VmOperationCatalog_init(VmOperationCatalog* self) {
    self->__rc = 1;
}

void VmOperationCatalog_destroy(VmOperationCatalog* self) {
    free(self);
}

btrc_Vector_string* VmOperationCatalog_all(void) {
    btrc_Vector_string* kinds = btrc_Vector_string_new();
    btrc_Vector_string_push(kinds, "download-iso");
    btrc_Vector_string_push(kinds, "require-command");
    btrc_Vector_string_push(kinds, "require-tpm2");
    btrc_Vector_string_push(kinds, "require-uefi");
    btrc_Vector_string_push(kinds, "require-secure-boot");
    btrc_Vector_string_push(kinds, "probe-secure-boot");
    btrc_Vector_string_push(kinds, "require-state");
    btrc_Vector_string_push(kinds, "inherit-state");
    btrc_Vector_string_push(kinds, "record-state");
    btrc_Vector_string_push(kinds, "validate-immutability-v2");
    btrc_Vector_string_push(kinds, "reset-state");
    btrc_Vector_string_push(kinds, "create-key");
    btrc_Vector_string_push(kinds, "create-disk");
    btrc_Vector_string_push(kinds, "setup");
    btrc_Vector_string_push(kinds, "boot-iso");
    btrc_Vector_string_push(kinds, "boot-disk");
    btrc_Vector_string_push(kinds, "up-iso");
    btrc_Vector_string_push(kinds, "bootstrap-ssh");
    btrc_Vector_string_push(kinds, "wait-ssh");
    btrc_Vector_string_push(kinds, "serial-send");
    btrc_Vector_string_push(kinds, "sleep");
    btrc_Vector_string_push(kinds, "guest");
    btrc_Vector_string_push(kinds, "reboot-disk");
    btrc_Vector_string_push(kinds, "host");
    btrc_Vector_string_push(kinds, "workspace-file");
    btrc_Vector_string_push(kinds, "nix-eval");
    btrc_Vector_string_push(kinds, "qmp");
    btrc_Vector_string_push(kinds, "copy-workspace");
    btrc_Vector_string_push(kinds, "configure-vm-host");
    btrc_Vector_string_push(kinds, "install-nixos");
    btrc_Vector_string_push(kinds, "copy-to");
    btrc_Vector_string_push(kinds, "copy-from");
    btrc_Vector_string_push(kinds, "snapshot");
    btrc_Vector_string_push(kinds, "restore");
    btrc_Vector_string_push(kinds, "stop");
    return kinds;
}

void VmTestRunner_init(VmTestRunner* self, VmTestSpec* spec) {
    self->__rc = 1;
    if (self->spec != NULL) {
        if ((--self->spec->__rc) <= 0) {
            VmTestSpec_destroy(self->spec);
        }
    }
    (self->spec = spec);
    (spec->__rc++);
    if (self->vm != NULL) {
        if ((--self->vm->__rc) <= 0) {
            QemuE2eHarness_destroy(self->vm);
        }
    }
    (self->vm = QemuE2eHarness_new(spec));
    (QemuE2eHarness_new(spec)->__rc++);
    (self->failures = 0);
}

VmTestRunner* VmTestRunner_new(VmTestSpec* spec) {
    VmTestRunner* self = ((VmTestRunner*)malloc(sizeof(VmTestRunner)));
    memset(self, 0, sizeof(VmTestRunner));
    VmTestRunner_init(self, spec);
    return self;
}

void VmTestRunner_destroy(VmTestRunner* self) {
    if (self->spec != NULL) {
        if ((--self->spec->__rc) <= 0) {
            VmTestSpec_destroy(self->spec);
        }
    }
    if (self->vm != NULL) {
        if ((--self->vm->__rc) <= 0) {
            QemuE2eHarness_destroy(self->vm);
        }
    }
    if (__btrc_tracking) {
        __btrc_mark_destroyed(self);
    }
    free(self);
}

void VmTestRunner_fail(VmTestRunner* self, char* message) {
    (self->failures++);
    NixosLog_error(message);
}

bool VmTestRunner_outputMatches(VmTestRunner* self, ExecResult* result, char* expect) {
    if (__btrc_isEmpty(expect)) {
        __auto_type __btrc_ret_691 = true;
        return __btrc_ret_691;
    }
    __auto_type __btrc_ret_692 = __btrc_strContains(ExecResult_stdout(result), expect);
    return __btrc_ret_692;
}

void VmTestRunner_assertResult(VmTestRunner* self, char* label, ExecResult* result, char* expect) {
    if (!ExecResult_ok(result)) {
        Console_error(__btrc_str_track(__btrc_trim(ExecResult_stdout(result))));
        VmTestRunner_fail(self, __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(label, " failed with exit code ")), Strings_fromInt(result->code))));
        return;
    }
    if (!VmTestRunner_outputMatches(self, result, expect)) {
        Console_error(__btrc_str_track(__btrc_trim(ExecResult_stdout(result))));
        VmTestRunner_fail(self, __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(label, " output did not contain expected text: ")), expect)));
    }
}

void VmTestRunner_runOperation(VmTestRunner* self, VmOperation* op) {
    NixosLog_info(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("e2e ", self->spec->name)), ": ")), op->kind)));
    if (strcmp(op->kind, "download-iso") == 0) {
        QemuE2eHarness_downloadIso(self->vm);
        return;
    }
    if (strcmp(op->kind, "require-command") == 0) {
        QemuE2eHarness_requireCommand(self->vm, op->command);
        return;
    }
    if (strcmp(op->kind, "require-tpm2") == 0) {
        QemuE2eHarness_requireTpm2Capability(self->vm);
        return;
    }
    if (strcmp(op->kind, "require-uefi") == 0) {
        QemuE2eHarness_requireUefiCapability(self->vm);
        return;
    }
    if (strcmp(op->kind, "require-secure-boot") == 0) {
        QemuE2eHarness_requireSecureBootCapability(self->vm);
        return;
    }
    if (strcmp(op->kind, "probe-secure-boot") == 0) {
        Console_log(QemuE2eHarness_secureBootCapabilityReport(self->vm));
        return;
    }
    if (strcmp(op->kind, "require-state") == 0) {
        QemuE2eHarness_requireParentState(self->vm);
        return;
    }
    if (strcmp(op->kind, "inherit-state") == 0) {
        QemuE2eHarness_inheritState(self->vm);
        return;
    }
    if (strcmp(op->kind, "record-state") == 0) {
        QemuE2eHarness_recordState(self->vm);
        return;
    }
    if (strcmp(op->kind, "validate-immutability-v2") == 0) {
        char* version = btrc_Map_string_string_getOrDefault(self->spec->args, "immutabilityVersion", "");
        if (!(strcmp(version, "v2") == 0)) {
            VmTestRunner_fail(self, "btrc E2E specs must use immutabilityVersion=v2; v1 belongs only to the original Rust implementation");
        }
        return;
    }
    if (strcmp(op->kind, "reset-state") == 0) {
        QemuE2eHarness_resetState(self->vm);
        return;
    }
    if (strcmp(op->kind, "create-key") == 0) {
        QemuE2eHarness_createSshKey(self->vm);
        return;
    }
    if (strcmp(op->kind, "create-disk") == 0) {
        QemuE2eHarness_createDisk(self->vm);
        return;
    }
    if (strcmp(op->kind, "setup") == 0) {
        QemuE2eHarness_setup(self->vm);
        return;
    }
    if (strcmp(op->kind, "boot-iso") == 0) {
        QemuE2eHarness_start(self->vm, true);
        return;
    }
    if (strcmp(op->kind, "boot-disk") == 0) {
        QemuE2eHarness_start(self->vm, false);
        return;
    }
    if (strcmp(op->kind, "up-iso") == 0) {
        QemuE2eHarness_upFromIso(self->vm);
        return;
    }
    if (strcmp(op->kind, "bootstrap-ssh") == 0) {
        QemuE2eHarness_bootstrapSsh(self->vm);
        return;
    }
    if (strcmp(op->kind, "wait-ssh") == 0) {
        if (!QemuE2eHarness_waitForSsh(self->vm, op->timeout)) {
            VmTestRunner_fail(self, __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("SSH was not ready after ", Strings_fromInt(op->timeout))), " seconds; serial log: ")), QemuE2eHarness_serialLogPath(self->vm))));
        }
        return;
    }
    if (strcmp(op->kind, "serial-send") == 0) {
        QemuE2eHarness_serialSend(self->vm, op->command);
        return;
    }
    if (strcmp(op->kind, "sleep") == 0) {
        QemuE2eHarness_sleepSeconds(self->vm, op->timeout);
        return;
    }
    if (strcmp(op->kind, "guest") == 0) {
        ExecResult* result = QemuE2eHarness_sshWithTimeout(self->vm, op->command, false, op->timeout);
        VmTestRunner_assertResult(self, "guest command", result, op->expect);
        return;
    }
    if (strcmp(op->kind, "reboot-disk") == 0) {
        QemuE2eHarness_rebootDisk(self->vm);
        return;
    }
    if (strcmp(op->kind, "host") == 0) {
        ExecResult* result = QemuE2eHarness_host(self->vm, op->command, false);
        VmTestRunner_assertResult(self, "host command", result, op->expect);
        return;
    }
    if (strcmp(op->kind, "workspace-file") == 0) {
        ExecResult* result = QemuE2eHarness_workspaceFileExists(self->vm, op->command);
        VmTestRunner_assertResult(self, __btrc_str_track(__btrc_strcat("workspace file ", op->command)), result, op->expect);
        return;
    }
    if (strcmp(op->kind, "nix-eval") == 0) {
        ExecResult* result = QemuE2eHarness_nixEval(self->vm, op->command, op->timeout);
        VmTestRunner_assertResult(self, __btrc_str_track(__btrc_strcat("nix eval ", op->command)), result, op->expect);
        return;
    }
    if (strcmp(op->kind, "qmp") == 0) {
        ExecResult* result = QemuE2eHarness_qmp(self->vm, op->command, op->timeout);
        VmTestRunner_assertResult(self, "qmp command", result, op->expect);
        return;
    }
    if (strcmp(op->kind, "copy-workspace") == 0) {
        QemuE2eHarness_copyWorkspace(self->vm, op->localPath, op->remotePath);
        return;
    }
    if (strcmp(op->kind, "configure-vm-host") == 0) {
        QemuE2eHarness_configureVmHost(self->vm);
        return;
    }
    if (strcmp(op->kind, "install-nixos") == 0) {
        QemuE2eHarness_installNixosGuest(self->vm);
        return;
    }
    if (strcmp(op->kind, "copy-to") == 0) {
        QemuE2eHarness_copyTo(self->vm, op->localPath, op->remotePath);
        return;
    }
    if (strcmp(op->kind, "copy-from") == 0) {
        QemuE2eHarness_copyFrom(self->vm, op->remotePath, op->localPath);
        return;
    }
    if (strcmp(op->kind, "snapshot") == 0) {
        QemuE2eHarness_snapshot(self->vm, op->name);
        return;
    }
    if (strcmp(op->kind, "restore") == 0) {
        QemuE2eHarness_restore(self->vm, op->name);
        return;
    }
    if (strcmp(op->kind, "stop") == 0) {
        QemuE2eHarness_stop(self->vm);
        return;
    }
    VmTestRunner_fail(self, __btrc_str_track(__btrc_strcat("Unknown VM operation: ", op->kind)));
}

int VmTestRunner_run(VmTestRunner* self) {
    if (self->spec->operations->len == 0) {
        NixosLog_fatal("VM spec has no operations");
    }
    int __n_694 = btrc_Vector_VmOperation_iterLen(self->spec->operations);
    for (int __i_693 = 0; (__i_693 < __n_694); (__i_693++)) {
        VmOperation* op = btrc_Vector_VmOperation_iterGet(self->spec->operations, __i_693);
        if (self->failures > 0) {
            break;
        }
        VmTestRunner_runOperation(self, op);
    }
    if (self->failures > 0) {
        QemuE2eHarness_stop(self->vm);
        __auto_type __btrc_ret_695 = 1;
        return __btrc_ret_695;
    }
    NixosLog_info(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("e2e ", self->spec->name)), ": pass")));
    __auto_type __btrc_ret_696 = 0;
    return __btrc_ret_696;
}

void VmGraphNode_init(VmGraphNode* self) {
    self->__rc = 1;
    (self->id = "");
    (self->specPath = "");
    if (self->after != NULL) {
        if ((--self->after->__rc) <= 0) {
            btrc_Vector_string_free(self->after);
        }
    }
    btrc_Vector_string* __list_698 = btrc_Vector_string_new();
    (self->after = __list_698);
    btrc_Vector_string* __list_697 = btrc_Vector_string_new();
    (__list_697->__rc++);
    if (self->args != NULL) {
        if ((--self->args->__rc) <= 0) {
            btrc_Map_string_string_free(self->args);
        }
    }
    (self->args = btrc_Map_string_string_new());
    (btrc_Map_string_string_new()->__rc++);
}

VmGraphNode* VmGraphNode_new(void) {
    VmGraphNode* self = ((VmGraphNode*)malloc(sizeof(VmGraphNode)));
    memset(self, 0, sizeof(VmGraphNode));
    VmGraphNode_init(self);
    return self;
}

void VmGraphNode_destroy(VmGraphNode* self) {
    if (self->after != NULL) {
        if ((--self->after->__rc) <= 0) {
            btrc_Vector_string_free(self->after);
        }
    }
    if (self->args != NULL) {
        if ((--self->args->__rc) <= 0) {
            btrc_Map_string_string_free(self->args);
        }
    }
    if (__btrc_tracking) {
        __btrc_mark_destroyed(self);
    }
    free(self);
}

void VmTestGraph_init(VmTestGraph* self) {
    self->__rc = 1;
    (self->name = "e2e");
    (self->path = "");
    (self->baseDir = ".");
    (self->workspaceRoot = "..");
    if (self->defaults != NULL) {
        if ((--self->defaults->__rc) <= 0) {
            btrc_Vector_string_free(self->defaults);
        }
    }
    btrc_Vector_string* __list_700 = btrc_Vector_string_new();
    (self->defaults = __list_700);
    btrc_Vector_string* __list_699 = btrc_Vector_string_new();
    (__list_699->__rc++);
    if (self->nodes != NULL) {
        if ((--self->nodes->__rc) <= 0) {
            btrc_Vector_VmGraphNode_free(self->nodes);
        }
    }
    btrc_Vector_VmGraphNode* __list_702 = btrc_Vector_VmGraphNode_new();
    (self->nodes = __list_702);
    btrc_Vector_VmGraphNode* __list_701 = btrc_Vector_VmGraphNode_new();
    (__list_701->__rc++);
}

VmTestGraph* VmTestGraph_new(void) {
    VmTestGraph* self = ((VmTestGraph*)malloc(sizeof(VmTestGraph)));
    memset(self, 0, sizeof(VmTestGraph));
    VmTestGraph_init(self);
    return self;
}

void VmTestGraph_destroy(VmTestGraph* self) {
    if (self->defaults != NULL) {
        if ((--self->defaults->__rc) <= 0) {
            btrc_Vector_string_free(self->defaults);
        }
    }
    if (self->nodes != NULL) {
        if ((--self->nodes->__rc) <= 0) {
            btrc_Vector_VmGraphNode_free(self->nodes);
        }
    }
    if (__btrc_tracking) {
        __btrc_mark_destroyed(self);
    }
    free(self);
}

VmGraphNode* VmTestGraph_node(VmTestGraph* self, char* id) {
    int __n_704 = btrc_Vector_VmGraphNode_iterLen(self->nodes);
    for (int __i_703 = 0; (__i_703 < __n_704); (__i_703++)) {
        VmGraphNode* node = btrc_Vector_VmGraphNode_iterGet(self->nodes, __i_703);
        if (strcmp(node->id, id) == 0) {
            return node;
        }
    }
    NixosLog_fatal(__btrc_str_track(__btrc_strcat("Unknown graph node: ", id)));
    __auto_type __btrc_ret_705 = VmGraphNode_new();
    return __btrc_ret_705;
}

char* VmTestGraph_resolvedSpecPath(VmTestGraph* self, VmGraphNode* node) {
    if (__btrc_startsWith(node->specPath, "/")) {
        __auto_type __btrc_ret_706 = node->specPath;
        return __btrc_ret_706;
    }
    if (FileSystem_exists(node->specPath)) {
        __auto_type __btrc_ret_707 = node->specPath;
        return __btrc_ret_707;
    }
    __auto_type __btrc_ret_708 = PathTools_join(self->baseDir, node->specPath);
    return __btrc_ret_708;
}

char* VmTestGraph_resolvedWorkspaceRoot(VmTestGraph* self) {
    if (__btrc_startsWith(self->workspaceRoot, "/")) {
        __auto_type __btrc_ret_709 = self->workspaceRoot;
        return __btrc_ret_709;
    }
    __auto_type __btrc_ret_710 = PathTools_join(self->baseDir, self->workspaceRoot);
    return __btrc_ret_710;
}

btrc_Vector_string* VmTestGraph_defaultTargets(VmTestGraph* self) {
    if (!btrc_Vector_string_isEmpty(self->defaults)) {
        __auto_type __btrc_ret_711 = self->defaults;
        return __btrc_ret_711;
    }
    btrc_Vector_string* result = btrc_Vector_string_new();
    int __n_713 = btrc_Vector_VmGraphNode_iterLen(self->nodes);
    for (int __i_712 = 0; (__i_712 < __n_713); (__i_712++)) {
        VmGraphNode* node = btrc_Vector_VmGraphNode_iterGet(self->nodes, __i_712);
        btrc_Vector_string_push(result, node->id);
    }
    return result;
}

void VmGraphParser_init(VmGraphParser* self) {
    self->__rc = 1;
}

void VmGraphParser_destroy(VmGraphParser* self) {
    free(self);
}

btrc_Vector_string* VmGraphParser_stringArray(char* text, char* key) {
    btrc_Vector_string* result = btrc_Vector_string_new();
    int pos = VmSpecParser_keyPosition(text, key);
    if (pos < 0) {
        return result;
    }
    int arrayStart = Strings_find(text, "[", pos);
    if (arrayStart < 0) {
        return result;
    }
    int len = ((int)strlen(text));
    bool inString = false;
    bool escaped = false;
    int start = (-1);
    for (int i = (arrayStart + 1); (i < len); (i++)) {
        char c = text[i];
        if (inString) {
            if ((!escaped) && (c == ((char)34))) {
                btrc_Vector_string_push(result, JsonObject_slice(text, start, i));
                (inString = false);
                (start = (-1));
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
            (start = (i + 1));
            continue;
        }
        if (c == ']') {
            break;
        }
    }
    return result;
}

btrc_Vector_string* VmGraphParser_objectArray(char* text, char* key) {
    btrc_Vector_string* result = btrc_Vector_string_new();
    int pos = VmSpecParser_keyPosition(text, key);
    if (pos < 0) {
        return result;
    }
    int arrayStart = Strings_find(text, "[", pos);
    if (arrayStart < 0) {
        return result;
    }
    int len = ((int)strlen(text));
    int depth = 0;
    int start = (-1);
    bool inString = false;
    bool escaped = false;
    for (int i = (arrayStart + 1); (i < len); (i++)) {
        char c = text[i];
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
        if (c == '{') {
            if (depth == 0) {
                (start = i);
            }
            (depth++);
            continue;
        }
        if (c == '}') {
            (depth--);
            if ((depth == 0) && (start >= 0)) {
                btrc_Vector_string_push(result, JsonObject_slice(text, start, (i + 1)));
                (start = (-1));
            }
            continue;
        }
        if ((c == ']') && (depth == 0)) {
            break;
        }
    }
    return result;
}

VmGraphNode* VmGraphParser_node(char* objectText) {
    VmGraphNode* node = VmGraphNode_new();
    (node->id = VmSpecParser_field(objectText, "id", ""));
    (node->specPath = VmSpecParser_field(objectText, "spec", ""));
    if (node->after != NULL) {
        if ((--node->after->__rc) <= 0) {
            btrc_Vector_string_free(node->after);
        }
    }
    (node->after = VmGraphParser_stringArray(objectText, "after"));
    (VmGraphParser_stringArray(objectText, "after")->__rc++);
    char* argsText = VmSpecParser_objectField(objectText, "args");
    if (!__btrc_isEmpty(argsText)) {
        JsonObject* parsed = JsonObject_parse(argsText);
        int __n_715 = btrc_Map_string_string_iterLen(parsed->values);
        for (int __i_714 = 0; (__i_714 < __n_715); (__i_714++)) {
            char* key = btrc_Map_string_string_iterGet(parsed->values, __i_714);
            char* value = btrc_Map_string_string_iterValueAt(parsed->values, __i_714);
            btrc_Map_string_string_put(node->args, key, value);
        }
    }
    if (__btrc_isEmpty(node->id)) {
        NixosLog_fatal("Graph node is missing id");
    }
    if (__btrc_isEmpty(node->specPath)) {
        NixosLog_fatal(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("Graph node ", node->id)), " is missing spec")));
    }
    return node;
    if (node != NULL) {
        if ((--node->__rc) <= 0) {
            VmGraphNode_destroy(node);
        }
    }
}

VmTestGraph* VmGraphParser_readFile(char* path) {
    char* text = Path_readAll(path);
    VmTestGraph* graph = VmTestGraph_new();
    (graph->path = path);
    (graph->baseDir = PathTools_dirname(path));
    (graph->name = VmSpecParser_field(text, "name", graph->name));
    (graph->workspaceRoot = VmSpecParser_field(text, "workspaceRoot", graph->workspaceRoot));
    if (graph->defaults != NULL) {
        if ((--graph->defaults->__rc) <= 0) {
            btrc_Vector_string_free(graph->defaults);
        }
    }
    (graph->defaults = VmGraphParser_stringArray(text, "default"));
    (VmGraphParser_stringArray(text, "default")->__rc++);
    int __n_717 = btrc_Vector_string_iterLen(VmGraphParser_objectArray(text, "nodes"));
    for (int __i_716 = 0; (__i_716 < __n_717); (__i_716++)) {
        char* objectText = btrc_Vector_string_iterGet(VmGraphParser_objectArray(text, "nodes"), __i_716);
        btrc_Vector_VmGraphNode_push(graph->nodes, VmGraphParser_node(objectText));
    }
    if (btrc_Vector_VmGraphNode_isEmpty(graph->nodes)) {
        NixosLog_fatal(__btrc_str_track(__btrc_strcat("Graph has no nodes: ", path)));
    }
    return graph;
    if (graph != NULL) {
        if ((--graph->__rc) <= 0) {
            VmTestGraph_destroy(graph);
        }
    }
}

void VmGraphRunner_init(VmGraphRunner* self, VmTestGraph* graph, btrc_Map_string_string* args) {
    self->__rc = 1;
    if (self->graph != NULL) {
        if ((--self->graph->__rc) <= 0) {
            VmTestGraph_destroy(self->graph);
        }
    }
    (self->graph = graph);
    (graph->__rc++);
    if (self->args != NULL) {
        if ((--self->args->__rc) <= 0) {
            btrc_Map_string_string_free(self->args);
        }
    }
    (self->args = args);
    (args->__rc++);
    if (self->done != NULL) {
        if ((--self->done->__rc) <= 0) {
            btrc_Vector_string_free(self->done);
        }
    }
    btrc_Vector_string* __list_719 = btrc_Vector_string_new();
    (self->done = __list_719);
    btrc_Vector_string* __list_718 = btrc_Vector_string_new();
    (__list_718->__rc++);
    if (self->visiting != NULL) {
        if ((--self->visiting->__rc) <= 0) {
            btrc_Vector_string_free(self->visiting);
        }
    }
    btrc_Vector_string* __list_721 = btrc_Vector_string_new();
    (self->visiting = __list_721);
    btrc_Vector_string* __list_720 = btrc_Vector_string_new();
    (__list_720->__rc++);
    (self->sourceHashValue = "");
}

VmGraphRunner* VmGraphRunner_new(VmTestGraph* graph, btrc_Map_string_string* args) {
    VmGraphRunner* self = ((VmGraphRunner*)malloc(sizeof(VmGraphRunner)));
    memset(self, 0, sizeof(VmGraphRunner));
    VmGraphRunner_init(self, graph, args);
    return self;
}

void VmGraphRunner_destroy(VmGraphRunner* self) {
    if (self->graph != NULL) {
        if ((--self->graph->__rc) <= 0) {
            VmTestGraph_destroy(self->graph);
        }
    }
    if (self->args != NULL) {
        if ((--self->args->__rc) <= 0) {
            btrc_Map_string_string_free(self->args);
        }
    }
    if (self->done != NULL) {
        if ((--self->done->__rc) <= 0) {
            btrc_Vector_string_free(self->done);
        }
    }
    if (self->visiting != NULL) {
        if ((--self->visiting->__rc) <= 0) {
            btrc_Vector_string_free(self->visiting);
        }
    }
    if (__btrc_tracking) {
        __btrc_mark_destroyed(self);
    }
    free(self);
}

char* VmGraphRunner_sourceHash(VmGraphRunner* self) {
    if (!__btrc_isEmpty(self->sourceHashValue)) {
        __auto_type __btrc_ret_722 = self->sourceHashValue;
        return __btrc_ret_722;
    }
    char* root = VmTestGraph_resolvedWorkspaceRoot(self->graph);
    char* command = __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("cd ", UnixShell_quote(root))), " && find . -type f")), " ! -path './.git/*'")), " ! -path '*/.vm/*'")), " ! -path '*/build/*'")), " ! -path '*/secrets/*'")), " ! -path './result/*'")), " ! -name '.DS_Store'")), " ! -name '._*'")), " \\( -name '*.btrc' -o -name '*.nix' -o -name '*.json' -o -name '*.toml' -o -name '*.tsv' -o -name '*.c' -o -name '*.rs' -o -name 'Makefile' -o -name 'flake.lock' \\)")), " -print0 | LC_ALL=C sort -z | xargs -0 shasum -a 256 | shasum -a 256 | awk '{print $1}'"));
    ExecResult* result = UnixShell_run(UnixShell_new(), command);
    if (!ExecResult_ok(result)) {
        NixosLog_fatal(__btrc_str_track(__btrc_strcat("Failed to compute workspace source hash for ", root)));
    }
    char* toolPath = PathTools_join(root, "build/nixosctl");
    char* toolHash = "missing";
    if (FileSystem_exists(toolPath)) {
        ExecResult* tool = UnixShell_run(UnixShell_new(), __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("shasum -a 256 ", UnixShell_quote(toolPath))), " | awk '{print $1}'")));
        if (!ExecResult_ok(tool)) {
            NixosLog_fatal(__btrc_str_track(__btrc_strcat("Failed to compute nixosctl hash for ", toolPath)));
        }
        (toolHash = __btrc_str_track(__btrc_trim(ExecResult_stdout(tool))));
    }
    (self->sourceHashValue = __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_trim(ExecResult_stdout(result))), ":nixosctl=")), toolHash)));
    __auto_type __btrc_ret_723 = self->sourceHashValue;
    return __btrc_ret_723;
}

VmTestSpec* VmGraphRunner_specFor(VmGraphRunner* self, VmGraphNode* node) {
    VmTestSpec* spec = VmSpecParser_readFile(VmTestGraph_resolvedSpecPath(self->graph, node));
    VmTestSpec_setArg(spec, "graphName", self->graph->name);
    VmTestSpec_setArg(spec, "nodeId", node->id);
    VmTestSpec_setArg(spec, "workspaceRoot", VmTestGraph_resolvedWorkspaceRoot(self->graph));
    VmTestSpec_setArg(spec, "sourceHash", VmGraphRunner_sourceHash(self));
    VmGraphRunner_applyStructuralOverrides(self, spec, node->args);
    VmGraphRunner_applyStructuralOverrides(self, spec, self->args);
    int __n_725 = btrc_Map_string_string_iterLen(node->args);
    for (int __i_724 = 0; (__i_724 < __n_725); (__i_724++)) {
        char* key = btrc_Map_string_string_iterGet(node->args, __i_724);
        char* value = btrc_Map_string_string_iterValueAt(node->args, __i_724);
        VmTestSpec_setArg(spec, key, value);
    }
    int __n_727 = btrc_Map_string_string_iterLen(self->args);
    for (int __i_726 = 0; (__i_726 < __n_727); (__i_726++)) {
        char* key = btrc_Map_string_string_iterGet(self->args, __i_726);
        char* value = btrc_Map_string_string_iterValueAt(self->args, __i_726);
        VmTestSpec_setArg(spec, key, value);
    }
    VmTestSpec_expandArgs(spec);
    return spec;
}

void VmGraphRunner_applyStructuralOverrides(VmGraphRunner* self, VmTestSpec* spec, btrc_Map_string_string* overrides) {
    bool archChanged = btrc_Map_string_string_has(overrides, "arch");
    if (btrc_Map_string_string_has(overrides, "name")) {
        (spec->name = btrc_Map_string_string_get(overrides, "name"));
    }
    if (btrc_Map_string_string_has(overrides, "workDir")) {
        (spec->workDir = btrc_Map_string_string_get(overrides, "workDir"));
    }
    if (btrc_Map_string_string_has(overrides, "arch")) {
        (spec->arch = btrc_Map_string_string_get(overrides, "arch"));
    }
    if (archChanged && (!btrc_Map_string_string_has(overrides, "iso"))) {
        (spec->iso = PathTools_join(PathTools_join(".vm", "iso"), __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("nixos-minimal-", spec->arch)), ".iso"))));
    }
    if (archChanged && (!btrc_Map_string_string_has(overrides, "isoUrl"))) {
        (spec->isoUrl = VmSpecParser_defaultIsoUrl(spec->arch));
    }
    if (btrc_Map_string_string_has(overrides, "iso")) {
        (spec->iso = btrc_Map_string_string_get(overrides, "iso"));
    }
    if (btrc_Map_string_string_has(overrides, "isoUrl")) {
        (spec->isoUrl = btrc_Map_string_string_get(overrides, "isoUrl"));
    }
    if (btrc_Map_string_string_has(overrides, "diskSize")) {
        (spec->diskSize = btrc_Map_string_string_get(overrides, "diskSize"));
    }
    if (btrc_Map_string_string_has(overrides, "memory")) {
        (spec->memory = btrc_Map_string_string_get(overrides, "memory"));
    }
    if (btrc_Map_string_string_has(overrides, "cpus")) {
        (spec->cpus = Strings_toInt(btrc_Map_string_string_get(overrides, "cpus")));
    }
    if (btrc_Map_string_string_has(overrides, "sshPort")) {
        (spec->sshPort = Strings_toInt(btrc_Map_string_string_get(overrides, "sshPort")));
    }
    if (btrc_Map_string_string_has(overrides, "state")) {
        (spec->state = btrc_Map_string_string_get(overrides, "state"));
    }
    if (btrc_Map_string_string_has(overrides, "parentState")) {
        (spec->parentState = btrc_Map_string_string_get(overrides, "parentState"));
    }
    if (btrc_Map_string_string_has(overrides, "stateRoot")) {
        (spec->stateRoot = btrc_Map_string_string_get(overrides, "stateRoot"));
    }
    if (btrc_Map_string_string_has(overrides, "stateMaterial")) {
        (spec->stateMaterial = btrc_Map_string_string_get(overrides, "stateMaterial"));
    }
}

void VmGraphRunner_list(VmGraphRunner* self) {
    int __n_729 = btrc_Vector_VmGraphNode_iterLen(self->graph->nodes);
    for (int __i_728 = 0; (__i_728 < __n_729); (__i_728++)) {
        VmGraphNode* node = btrc_Vector_VmGraphNode_iterGet(self->graph->nodes, __i_728);
        char* parents = (btrc_Vector_string_isEmpty(node->after) ? "root" : btrc_Vector_string_join(node->after, ","));
        Console_log(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(node->id, " <- ")), parents)), " :: ")), VmTestGraph_resolvedSpecPath(self->graph, node))));
    }
}

void VmGraphRunner_status(VmGraphRunner* self) {
    int __n_731 = btrc_Vector_VmGraphNode_iterLen(self->graph->nodes);
    for (int __i_730 = 0; (__i_730 < __n_731); (__i_730++)) {
        VmGraphNode* node = btrc_Vector_VmGraphNode_iterGet(self->graph->nodes, __i_730);
        VmTestSpec* spec = VmGraphRunner_specFor(self, node);
        char* recorded = "missing";
        if (FileSystem_exists(VmTestSpec_stateHashFile(spec))) {
            char* saved = __btrc_str_track(__btrc_trim(Path_readAll(VmTestSpec_stateHashFile(spec))));
            (recorded = ((strcmp(saved, spec->stateHash) == 0) ? "ready" : "stale"));
        }
        Console_log(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(node->id, " ")), recorded)), " ")), spec->stateHashShort)), " ")), VmTestSpec_stateDir(spec))));
    }
}

int VmGraphRunner_operationCoverage(VmGraphRunner* self) {
    btrc_Vector_string* covered = btrc_Vector_string_new();
    int __n_733 = btrc_Vector_VmGraphNode_iterLen(self->graph->nodes);
    for (int __i_732 = 0; (__i_732 < __n_733); (__i_732++)) {
        VmGraphNode* node = btrc_Vector_VmGraphNode_iterGet(self->graph->nodes, __i_732);
        VmTestSpec* spec = VmSpecParser_readFile(VmTestGraph_resolvedSpecPath(self->graph, node));
        int __n_735 = btrc_Vector_VmOperation_iterLen(spec->operations);
        for (int __i_734 = 0; (__i_734 < __n_735); (__i_734++)) {
            VmOperation* op = btrc_Vector_VmOperation_iterGet(spec->operations, __i_734);
            if ((!__btrc_isEmpty(op->kind)) && (!btrc_Vector_string_contains(covered, op->kind))) {
                btrc_Vector_string_push(covered, op->kind);
            }
        }
    }
    btrc_Vector_string* missing = btrc_Vector_string_new();
    int __n_737 = btrc_Vector_string_iterLen(VmOperationCatalog_all());
    for (int __i_736 = 0; (__i_736 < __n_737); (__i_736++)) {
        char* kind = btrc_Vector_string_iterGet(VmOperationCatalog_all(), __i_736);
        if (!btrc_Vector_string_contains(covered, kind)) {
            btrc_Vector_string_push(missing, kind);
        }
    }
    if (!btrc_Vector_string_isEmpty(missing)) {
        Console_error(__btrc_str_track(__btrc_strcat("Missing e2e operation coverage: ", btrc_Vector_string_join(missing, ", "))));
        __auto_type __btrc_ret_738 = 1;
        return __btrc_ret_738;
    }
    Console_log(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("E2E operation coverage: ", Strings_fromInt(covered->len))), "/")), Strings_fromInt(VmOperationCatalog_all()->len))));
    __auto_type __btrc_ret_739 = 0;
    return __btrc_ret_739;
}

bool VmGraphRunner_force(VmGraphRunner* self) {
    __auto_type __btrc_ret_740 = (strcmp(btrc_Map_string_string_getOrDefault(self->args, "force", "false"), "true") == 0);
    return __btrc_ret_740;
}

bool VmGraphRunner_ready(VmGraphRunner* self, VmTestSpec* spec) {
    if (!FileSystem_exists(VmTestSpec_stateHashFile(spec))) {
        __auto_type __btrc_ret_741 = false;
        return __btrc_ret_741;
    }
    char* saved = __btrc_str_track(__btrc_trim(Path_readAll(VmTestSpec_stateHashFile(spec))));
    __auto_type __btrc_ret_742 = (strcmp(saved, spec->stateHash) == 0);
    return __btrc_ret_742;
}

int VmGraphRunner_runNode(VmGraphRunner* self, char* id) {
    if (btrc_Vector_string_contains(self->done, id)) {
        __auto_type __btrc_ret_743 = 0;
        return __btrc_ret_743;
    }
    if (btrc_Vector_string_contains(self->visiting, id)) {
        NixosLog_fatal(__btrc_str_track(__btrc_strcat("Cycle in graph at ", id)));
    }
    btrc_Vector_string_push(self->visiting, id);
    VmGraphNode* node = VmTestGraph_node(self->graph, id);
    int __n_745 = btrc_Vector_string_iterLen(node->after);
    for (int __i_744 = 0; (__i_744 < __n_745); (__i_744++)) {
        char* parent = btrc_Vector_string_iterGet(node->after, __i_744);
        int parentResult = VmGraphRunner_runNode(self, parent);
        if (parentResult != 0) {
            btrc_Vector_string_removeAll(self->visiting, id);
            return parentResult;
        }
    }
    VmTestSpec* spec = VmGraphRunner_specFor(self, node);
    if ((!VmGraphRunner_force(self)) && VmGraphRunner_ready(self, spec)) {
        NixosLog_info(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("graph ", self->graph->name)), ": skip ")), node->id)), " -> ")), spec->state)), "@")), spec->stateHashShort)));
        btrc_Vector_string_push(self->done, id);
        btrc_Vector_string_removeAll(self->visiting, id);
        __auto_type __btrc_ret_746 = 0;
        return __btrc_ret_746;
    }
    NixosLog_info(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("graph ", self->graph->name)), ": run ")), node->id)), " -> ")), spec->state)), "@")), spec->stateHashShort)));
    VmTestRunner* runner = VmTestRunner_new(spec);
    int result = VmTestRunner_run(runner);
    if (result != 0) {
        btrc_Vector_string_removeAll(self->visiting, id);
        if (runner != NULL) {
            if ((--runner->__rc) <= 0) {
                VmTestRunner_destroy(runner);
            }
        }
        return result;
    }
    btrc_Vector_string_push(self->done, id);
    btrc_Vector_string_removeAll(self->visiting, id);
    __auto_type __btrc_ret_747 = 0;
    if (runner != NULL) {
        if ((--runner->__rc) <= 0) {
            VmTestRunner_destroy(runner);
        }
    }
    return __btrc_ret_747;
    if (runner != NULL) {
        if ((--runner->__rc) <= 0) {
            VmTestRunner_destroy(runner);
        }
    }
}

int VmGraphRunner_run(VmGraphRunner* self, btrc_Vector_string* targets) {
    btrc_Vector_string* selected = (btrc_Vector_string_isEmpty(targets) ? VmTestGraph_defaultTargets(self->graph) : targets);
    int __n_749 = btrc_Vector_string_iterLen(selected);
    for (int __i_748 = 0; (__i_748 < __n_749); (__i_748++)) {
        char* id = btrc_Vector_string_iterGet(selected, __i_748);
        int result = VmGraphRunner_runNode(self, id);
        if (result != 0) {
            return result;
        }
    }
    __auto_type __btrc_ret_750 = 0;
    return __btrc_ret_750;
}

void NixosCtl_init(NixosCtl* self) {
    self->__rc = 1;
    char* root = Environment_get("NIXOS_CONFIG_ROOT", "/etc/nixos");
    if (self->config != NULL) {
        if ((--self->config->__rc) <= 0) {
            NixosConfig_destroy(self->config);
        }
    }
    (self->config = NixosConfig_new(root));
    (NixosConfig_new(root)->__rc++);
}

NixosCtl* NixosCtl_new(void) {
    NixosCtl* self = ((NixosCtl*)malloc(sizeof(NixosCtl)));
    memset(self, 0, sizeof(NixosCtl));
    NixosCtl_init(self);
    return self;
}

void NixosCtl_destroy(NixosCtl* self) {
    if (self->config != NULL) {
        if ((--self->config->__rc) <= 0) {
            NixosConfig_destroy(self->config);
        }
    }
    if (__btrc_tracking) {
        __btrc_mark_destroyed(self);
    }
    free(self);
}

char* NixosCtl_env(char* name, char* fallback) {
    __auto_type __btrc_ret_751 = Environment_get(name, fallback);
    return __btrc_ret_751;
}

void NixosCtl_usage(NixosCtl* self) {
    Console_log("Usage: nixosctl <eval|update|upgrade|install|snapshot|diff|fix-permissions|change-password|secure-boot|tpm2|displays|audio|caffeine|system|vm|e2e|graph> [args]");
}

void NixosCtl_applySpecArgs(NixosCtl* self, VmTestSpec* spec, CliArgs* args, int startIndex) {
    for (int i = startIndex; (i < CliArgs_count(args)); (i++)) {
        char* value = CliArgs_get(args, i);
        if (strcmp(value, "--arg") == 0) {
            if ((i + 1) >= CliArgs_count(args)) {
                NixosLog_fatal("Expected --arg key=value");
            }
            VmTestSpec_setArgPair(spec, CliArgs_get(args, (i + 1)));
            (i++);
            continue;
        }
        if (__btrc_startsWith(value, "--arg=")) {
            VmTestSpec_setArgPair(spec, Strings_removePrefix(value, "--arg="));
            continue;
        }
    }
    VmTestSpec_expandArgs(spec);
}

char* NixosCtl_tail(NixosCtl* self, CliArgs* args, int startIndex) {
    btrc_Vector_string* parts = btrc_Vector_string_new();
    for (int i = startIndex; (i < CliArgs_count(args)); (i++)) {
        btrc_Vector_string_push(parts, CliArgs_get(args, i));
    }
    __auto_type __btrc_ret_752 = btrc_Vector_string_join(parts, " ");
    return __btrc_ret_752;
}

bool NixosCtl_needsRoot(NixosCtl* self, char* command) {
    if (strcmp(NixosCtl_env("NIXOSCTL_ASSUME_ROOT_FOR_TESTS", "false"), "true") == 0) {
        __auto_type __btrc_ret_753 = false;
        return __btrc_ret_753;
    }
    __auto_type __btrc_ret_754 = (((((((((strcmp(command, "update") == 0) || (strcmp(command, "upgrade") == 0)) || (strcmp(command, "install") == 0)) || (strcmp(command, "snapshot") == 0)) || (strcmp(command, "diff") == 0)) || (strcmp(command, "fix-permissions") == 0)) || (strcmp(command, "change-password") == 0)) || (strcmp(command, "secure-boot") == 0)) || (strcmp(command, "tpm2") == 0));
    return __btrc_ret_754;
}

int NixosCtl_sudoSelf(NixosCtl* self, CliArgs* args) {
    Command* sudo = Command_new("sudo");
    Command_arg(sudo, args->program);
    int __n_756 = btrc_Vector_string_iterLen(args->values);
    for (int __i_755 = 0; (__i_755 < __n_756); (__i_755++)) {
        char* value = btrc_Vector_string_iterGet(args->values, __i_755);
        Command_arg(sudo, value);
    }
    Command_capture(sudo, false);
    ExecResult* result = UnixShell_runCommand(UnixShell_new(), sudo);
    __auto_type __btrc_ret_757 = result->code;
    if (sudo != NULL) {
        if ((--sudo->__rc) <= 0) {
            Command_destroy(sudo);
        }
    }
    return __btrc_ret_757;
    if (sudo != NULL) {
        if ((--sudo->__rc) <= 0) {
            Command_destroy(sudo);
        }
    }
}

int NixosCtl_runVm(NixosCtl* self, CliArgs* args) {
    if (CliArgs_count(args) < 3) {
        NixosLog_fatal("Usage: nixosctl vm <spec.json> <status|hash|setup|up|boot-iso|boot-disk|bootstrap-ssh|wait-ssh|ssh|snapshot|restore|stop|reset-state|clean-state>");
    }
    VmTestSpec* spec = VmSpecParser_readFile(CliArgs_get(args, 1));
    NixosCtl_applySpecArgs(self, spec, args, 3);
    QemuE2eHarness* vm = QemuE2eHarness_new(spec);
    char* action = CliArgs_get(args, 2);
    if (strcmp(action, "status") == 0) {
        QemuE2eHarness_printStatus(vm);
        __auto_type __btrc_ret_758 = 0;
        if (vm != NULL) {
            if ((--vm->__rc) <= 0) {
                QemuE2eHarness_destroy(vm);
            }
        }
        return __btrc_ret_758;
    }
    if (strcmp(action, "hash") == 0) {
        Console_log(spec->stateHash);
        __auto_type __btrc_ret_759 = 0;
        if (vm != NULL) {
            if ((--vm->__rc) <= 0) {
                QemuE2eHarness_destroy(vm);
            }
        }
        return __btrc_ret_759;
    }
    if (strcmp(action, "setup") == 0) {
        QemuE2eHarness_setup(vm);
        __auto_type __btrc_ret_760 = 0;
        if (vm != NULL) {
            if ((--vm->__rc) <= 0) {
                QemuE2eHarness_destroy(vm);
            }
        }
        return __btrc_ret_760;
    }
    if (strcmp(action, "download-iso") == 0) {
        QemuE2eHarness_downloadIso(vm);
        __auto_type __btrc_ret_761 = 0;
        if (vm != NULL) {
            if ((--vm->__rc) <= 0) {
                QemuE2eHarness_destroy(vm);
            }
        }
        return __btrc_ret_761;
    }
    if (strcmp(action, "create-key") == 0) {
        QemuE2eHarness_createSshKey(vm);
        __auto_type __btrc_ret_762 = 0;
        if (vm != NULL) {
            if ((--vm->__rc) <= 0) {
                QemuE2eHarness_destroy(vm);
            }
        }
        return __btrc_ret_762;
    }
    if (strcmp(action, "create-disk") == 0) {
        QemuE2eHarness_createDisk(vm);
        __auto_type __btrc_ret_763 = 0;
        if (vm != NULL) {
            if ((--vm->__rc) <= 0) {
                QemuE2eHarness_destroy(vm);
            }
        }
        return __btrc_ret_763;
    }
    if (strcmp(action, "up") == 0) {
        QemuE2eHarness_upFromIso(vm);
        __auto_type __btrc_ret_764 = 0;
        if (vm != NULL) {
            if ((--vm->__rc) <= 0) {
                QemuE2eHarness_destroy(vm);
            }
        }
        return __btrc_ret_764;
    }
    if (strcmp(action, "boot-iso") == 0) {
        QemuE2eHarness_start(vm, true);
        __auto_type __btrc_ret_765 = 0;
        if (vm != NULL) {
            if ((--vm->__rc) <= 0) {
                QemuE2eHarness_destroy(vm);
            }
        }
        return __btrc_ret_765;
    }
    if (strcmp(action, "boot-disk") == 0) {
        QemuE2eHarness_start(vm, false);
        __auto_type __btrc_ret_766 = 0;
        if (vm != NULL) {
            if ((--vm->__rc) <= 0) {
                QemuE2eHarness_destroy(vm);
            }
        }
        return __btrc_ret_766;
    }
    if (strcmp(action, "bootstrap-ssh") == 0) {
        QemuE2eHarness_bootstrapSsh(vm);
        __auto_type __btrc_ret_767 = 0;
        if (vm != NULL) {
            if ((--vm->__rc) <= 0) {
                QemuE2eHarness_destroy(vm);
            }
        }
        return __btrc_ret_767;
    }
    if (strcmp(action, "wait-ssh") == 0) {
        int timeout = Strings_toInt(CliArgs_valueAfter(args, "--timeout", "180"));
        if (!QemuE2eHarness_waitForSsh(vm, timeout)) {
            __auto_type __btrc_ret_768 = 1;
            if (vm != NULL) {
                if ((--vm->__rc) <= 0) {
                    QemuE2eHarness_destroy(vm);
                }
            }
            return __btrc_ret_768;
        }
        __auto_type __btrc_ret_769 = 0;
        if (vm != NULL) {
            if ((--vm->__rc) <= 0) {
                QemuE2eHarness_destroy(vm);
            }
        }
        return __btrc_ret_769;
    }
    if (strcmp(action, "ssh") == 0) {
        char* command = NixosCtl_tail(self, args, 3);
        if (__btrc_isEmpty(command)) {
            NixosLog_fatal("Usage: nixosctl vm <spec.json> ssh <command>");
        }
        ExecResult* result = QemuE2eHarness_ssh(vm, command, false);
        Console_log(__btrc_str_track(__btrc_trim(ExecResult_stdout(result))));
        __auto_type __btrc_ret_770 = result->code;
        if (vm != NULL) {
            if ((--vm->__rc) <= 0) {
                QemuE2eHarness_destroy(vm);
            }
        }
        return __btrc_ret_770;
    }
    if (strcmp(action, "host") == 0) {
        char* command = NixosCtl_tail(self, args, 3);
        if (__btrc_isEmpty(command)) {
            NixosLog_fatal("Usage: nixosctl vm <spec.json> host <command>");
        }
        ExecResult* result = QemuE2eHarness_host(vm, command, false);
        Console_log(__btrc_str_track(__btrc_trim(ExecResult_stdout(result))));
        __auto_type __btrc_ret_771 = result->code;
        if (vm != NULL) {
            if ((--vm->__rc) <= 0) {
                QemuE2eHarness_destroy(vm);
            }
        }
        return __btrc_ret_771;
    }
    if (strcmp(action, "copy-workspace") == 0) {
        QemuE2eHarness_copyWorkspace(vm, CliArgs_valueAfter(args, "--local", ".."), CliArgs_valueAfter(args, "--remote", "/etc/nixos"));
        __auto_type __btrc_ret_772 = 0;
        if (vm != NULL) {
            if ((--vm->__rc) <= 0) {
                QemuE2eHarness_destroy(vm);
            }
        }
        return __btrc_ret_772;
    }
    if (strcmp(action, "configure-vm-host") == 0) {
        QemuE2eHarness_configureVmHost(vm);
        __auto_type __btrc_ret_773 = 0;
        if (vm != NULL) {
            if ((--vm->__rc) <= 0) {
                QemuE2eHarness_destroy(vm);
            }
        }
        return __btrc_ret_773;
    }
    if (strcmp(action, "install-nixos") == 0) {
        QemuE2eHarness_installNixosGuest(vm);
        __auto_type __btrc_ret_774 = 0;
        if (vm != NULL) {
            if ((--vm->__rc) <= 0) {
                QemuE2eHarness_destroy(vm);
            }
        }
        return __btrc_ret_774;
    }
    if (strcmp(action, "reboot-disk") == 0) {
        QemuE2eHarness_rebootDisk(vm);
        __auto_type __btrc_ret_775 = 0;
        if (vm != NULL) {
            if ((--vm->__rc) <= 0) {
                QemuE2eHarness_destroy(vm);
            }
        }
        return __btrc_ret_775;
    }
    if (strcmp(action, "snapshot") == 0) {
        QemuE2eHarness_snapshot(vm, CliArgs_valueAfter(args, "--name", "manual"));
        __auto_type __btrc_ret_776 = 0;
        if (vm != NULL) {
            if ((--vm->__rc) <= 0) {
                QemuE2eHarness_destroy(vm);
            }
        }
        return __btrc_ret_776;
    }
    if (strcmp(action, "restore") == 0) {
        QemuE2eHarness_restore(vm, CliArgs_valueAfter(args, "--name", "manual"));
        __auto_type __btrc_ret_777 = 0;
        if (vm != NULL) {
            if ((--vm->__rc) <= 0) {
                QemuE2eHarness_destroy(vm);
            }
        }
        return __btrc_ret_777;
    }
    if (strcmp(action, "record-state") == 0) {
        QemuE2eHarness_recordState(vm);
        __auto_type __btrc_ret_778 = 0;
        if (vm != NULL) {
            if ((--vm->__rc) <= 0) {
                QemuE2eHarness_destroy(vm);
            }
        }
        return __btrc_ret_778;
    }
    if (strcmp(action, "stop") == 0) {
        QemuE2eHarness_stop(vm);
        __auto_type __btrc_ret_779 = 0;
        if (vm != NULL) {
            if ((--vm->__rc) <= 0) {
                QemuE2eHarness_destroy(vm);
            }
        }
        return __btrc_ret_779;
    }
    if (strcmp(action, "reset-state") == 0) {
        QemuE2eHarness_resetState(vm);
        __auto_type __btrc_ret_780 = 0;
        if (vm != NULL) {
            if ((--vm->__rc) <= 0) {
                QemuE2eHarness_destroy(vm);
            }
        }
        return __btrc_ret_780;
    }
    if (strcmp(action, "clean-state") == 0) {
        QemuE2eHarness_resetState(vm);
        QemuE2eHarness_cleanStateRecord(vm);
        __auto_type __btrc_ret_781 = 0;
        if (vm != NULL) {
            if ((--vm->__rc) <= 0) {
                QemuE2eHarness_destroy(vm);
            }
        }
        return __btrc_ret_781;
    }
    NixosLog_fatal(__btrc_str_track(__btrc_strcat("Unknown vm action: ", action)));
    __auto_type __btrc_ret_782 = 1;
    if (vm != NULL) {
        if ((--vm->__rc) <= 0) {
            QemuE2eHarness_destroy(vm);
        }
    }
    return __btrc_ret_782;
    if (vm != NULL) {
        if ((--vm->__rc) <= 0) {
            QemuE2eHarness_destroy(vm);
        }
    }
}

btrc_Map_string_string* NixosCtl_graphArgs(NixosCtl* self, CliArgs* args, int startIndex) {
    btrc_Map_string_string* result = btrc_Map_string_string_new();
    for (int i = startIndex; (i < CliArgs_count(args)); (i++)) {
        char* value = CliArgs_get(args, i);
        if (strcmp(value, "--arg") == 0) {
            if ((i + 1) >= CliArgs_count(args)) {
                NixosLog_fatal("Expected --arg key=value");
            }
            int pos = Strings_find(CliArgs_get(args, (i + 1)), "=", 0);
            if (pos <= 0) {
                NixosLog_fatal("Expected --arg key=value");
            }
            btrc_Map_string_string_put(result, JsonObject_slice(CliArgs_get(args, (i + 1)), 0, pos), JsonObject_slice(CliArgs_get(args, (i + 1)), (pos + 1), ((int)strlen(CliArgs_get(args, (i + 1))))));
            (i++);
            continue;
        }
        if (__btrc_startsWith(value, "--arg=")) {
            char* pair = Strings_removePrefix(value, "--arg=");
            int pos = Strings_find(pair, "=", 0);
            if (pos <= 0) {
                NixosLog_fatal("Expected --arg=key=value");
            }
            btrc_Map_string_string_put(result, JsonObject_slice(pair, 0, pos), JsonObject_slice(pair, (pos + 1), ((int)strlen(pair))));
        }
    }
    return result;
}

btrc_Vector_string* NixosCtl_graphTargets(NixosCtl* self, CliArgs* args, int startIndex) {
    btrc_Vector_string* result = btrc_Vector_string_new();
    for (int i = startIndex; (i < CliArgs_count(args)); (i++)) {
        char* value = CliArgs_get(args, i);
        if (strcmp(value, "--arg") == 0) {
            (i++);
            continue;
        }
        if (__btrc_startsWith(value, "--arg=")) {
            continue;
        }
        btrc_Vector_string_push(result, value);
    }
    return result;
}

int NixosCtl_runGraph(NixosCtl* self, CliArgs* args) {
    if (CliArgs_count(args) < 3) {
        NixosLog_fatal("Usage: nixosctl graph <graph.json> <list|status|coverage|run> [node ...] [--arg key=value]");
    }
    VmTestGraph* graph = VmGraphParser_readFile(CliArgs_get(args, 1));
    char* action = CliArgs_get(args, 2);
    btrc_Map_string_string* overrides = NixosCtl_graphArgs(self, args, 3);
    VmGraphRunner* runner = VmGraphRunner_new(graph, overrides);
    if (strcmp(action, "list") == 0) {
        VmGraphRunner_list(runner);
        __auto_type __btrc_ret_783 = 0;
        if (runner != NULL) {
            if ((--runner->__rc) <= 0) {
                VmGraphRunner_destroy(runner);
            }
        }
        return __btrc_ret_783;
    }
    if (strcmp(action, "status") == 0) {
        VmGraphRunner_status(runner);
        __auto_type __btrc_ret_784 = 0;
        if (runner != NULL) {
            if ((--runner->__rc) <= 0) {
                VmGraphRunner_destroy(runner);
            }
        }
        return __btrc_ret_784;
    }
    if (strcmp(action, "coverage") == 0) {
        int code = VmGraphRunner_operationCoverage(runner);
        if (runner != NULL) {
            if ((--runner->__rc) <= 0) {
                VmGraphRunner_destroy(runner);
            }
        }
        return code;
    }
    if (strcmp(action, "run") == 0) {
        int code = VmGraphRunner_run(runner, NixosCtl_graphTargets(self, args, 3));
        if (runner != NULL) {
            if ((--runner->__rc) <= 0) {
                VmGraphRunner_destroy(runner);
            }
        }
        return code;
    }
    NixosLog_fatal("Usage: nixosctl graph <graph.json> <list|status|coverage|run> [node ...] [--arg key=value]");
    __auto_type __btrc_ret_785 = 1;
    if (runner != NULL) {
        if ((--runner->__rc) <= 0) {
            VmGraphRunner_destroy(runner);
        }
    }
    return __btrc_ret_785;
    if (runner != NULL) {
        if ((--runner->__rc) <= 0) {
            VmGraphRunner_destroy(runner);
        }
    }
}

int NixosCtl_run(NixosCtl* self, CliArgs* args) {
    if (CliArgs_count(args) == 0) {
        NixosCtl_usage(self);
        __auto_type __btrc_ret_786 = 1;
        return __btrc_ret_786;
    }
    char* cmd = CliArgs_command(args);
    if (NixosCtl_needsRoot(self, cmd) && (!Platform_isRoot())) {
        __auto_type __btrc_ret_787 = NixosCtl_sudoSelf(self, args);
        return __btrc_ret_787;
    }
    if (strcmp(cmd, "eval") == 0) {
        if (CliArgs_count(args) < 2) {
            NixosLog_fatal("Usage: nixosctl eval <attribute>");
        }
        Console_log(NixosConfig_evalRaw(self->config, CliArgs_get(args, 1)));
        __auto_type __btrc_ret_788 = 0;
        return __btrc_ret_788;
    }
    if ((strcmp(cmd, "update") == 0) || (strcmp(cmd, "upgrade") == 0)) {
        RebuildOptions* options = RebuildOptions_new();
        (options->rebuildFileSystem = CliArgs_has(args, "--rebuild-filesystem"));
        (options->reboot = CliArgs_has(args, "--reboot"));
        (options->clean = ((CliArgs_has(args, "--clean") || CliArgs_has(args, "--upgrade")) || (strcmp(cmd, "upgrade") == 0)));
        (options->upgrade = (CliArgs_has(args, "--upgrade") || (strcmp(cmd, "upgrade") == 0)));
        NixosRebuilder_update(NixosRebuilder_new(self->config), options);
        __auto_type __btrc_ret_789 = 0;
        if (options != NULL) {
            if ((--options->__rc) <= 0) {
                RebuildOptions_destroy(options);
            }
        }
        return __btrc_ret_789;
        if (options != NULL) {
            if ((--options->__rc) <= 0) {
                RebuildOptions_destroy(options);
            }
        }
    }
    if (strcmp(cmd, "install") == 0) {
        Installer* installer = Installer_new(self->config);
        if (CliArgs_has(args, "--collect-garbage")) {
            Installer_collectGarbage(installer);
        }
        if (CliArgs_has(args, "--debug")) {
            Installer_debugShell(installer);
        }
        Installer_bootstrapConfigIfMissing(installer);
        SecretsManager_createIfMissing(SecretsManager_new(self->config), Installer_plainTextPasswordPath(installer));
        bool confirmed = CliArgs_has(args, "--yes");
        if (CliArgs_has(args, "--format") || ((!confirmed) && Interactive_confirm(installer->interactive, __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("Format ", NixosConfig_diskDevice(self->config))), "?"))))) {
            Installer_eraseAndMountDisk(installer);
        } else {
            Installer_mountDisk(installer);
        }
        if (confirmed || Interactive_confirm(installer->interactive, "Install NixOS?")) {
            ExecResult* installed = Installer_installNixos(installer);
            if (!ExecResult_ok(installed)) {
                __auto_type __btrc_ret_790 = installed->code;
                if (installer != NULL) {
                    if ((--installer->__rc) <= 0) {
                        Installer_destroy(installer);
                    }
                }
                return __btrc_ret_790;
            }
        } else if (Interactive_confirm(installer->interactive, "Permission NixOS?")) {
            Installer_permissionNixos(installer);
        }
        Interactive_askToReboot(installer->interactive);
        __auto_type __btrc_ret_791 = 0;
        if (installer != NULL) {
            if ((--installer->__rc) <= 0) {
                Installer_destroy(installer);
            }
        }
        return __btrc_ret_791;
        if (installer != NULL) {
            if ((--installer->__rc) <= 0) {
                Installer_destroy(installer);
            }
        }
    }
    if (strcmp(cmd, "snapshot") == 0) {
        SnapshotManager_createInitialSnapshots(SnapshotManager_new(self->config));
        __auto_type __btrc_ret_792 = 0;
        return __btrc_ret_792;
    }
    if (strcmp(cmd, "diff") == 0) {
        DiffOptions* options = DiffOptions_new();
        (options->recent = CliArgs_has(args, "--recent"));
        (options->showSymlinks = CliArgs_has(args, "--show-symlinks"));
        (options->showPersistPaths = CliArgs_has(args, "--show-persist-paths"));
        (options->showChildren = CliArgs_valueAfter(args, "--show-children", ""));
        char* depth = CliArgs_valueAfter(args, "--depth", "");
        if (!__btrc_isEmpty(depth)) {
            (options->depth = Strings_toInt(depth));
        }
        (options->pattern = CliArgs_valueAfter(args, "--pattern", ""));
        (options->diffignore = CliArgs_valueAfter(args, "--diffignore", ""));
        DiffScanner_print(DiffScanner_new(self->config), options);
        __auto_type __btrc_ret_793 = 0;
        if (options != NULL) {
            if ((--options->__rc) <= 0) {
                DiffOptions_destroy(options);
            }
        }
        return __btrc_ret_793;
        if (options != NULL) {
            if ((--options->__rc) <= 0) {
                DiffOptions_destroy(options);
            }
        }
    }
    if (strcmp(cmd, "fix-permissions") == 0) {
        char* username = NixosCtl_env("SUDO_USER", "");
        if (__btrc_isEmpty(username)) {
            (username = NixosCtl_env("USER", "root"));
        }
        PermissionsManager_secureTree(PermissionsManager_new(self->config), username);
        __auto_type __btrc_ret_794 = 0;
        return __btrc_ret_794;
    }
    if (strcmp(cmd, "change-password") == 0) {
        if (CliArgs_has(args, "--full-disk-encryption-only") && CliArgs_has(args, "--user-account-only")) {
            NixosLog_fatal("Cannot use both --full-disk-encryption-only and --user-account-only");
        }
        bool changeFde = (!CliArgs_has(args, "--user-account-only"));
        bool changeUser = (!CliArgs_has(args, "--full-disk-encryption-only"));
        char* oldPassword = CliArgs_valueAfter(args, "--old", NixosCtl_env("NIXOS_OLD_PASSWORD", ""));
        char* newPassword = CliArgs_valueAfter(args, "--new", NixosCtl_env("NIXOS_NEW_PASSWORD", ""));
        Interactive* interactive = Interactive_new();
        if ((changeFde || changeUser) && __btrc_isEmpty(oldPassword)) {
            (oldPassword = Interactive_askPassword(interactive, "Enter current password (LUKS + account)"));
        }
        if (__btrc_isEmpty(newPassword)) {
            (newPassword = Interactive_askPasswordConfirmed(interactive, "Enter new password"));
        }
        PasswordManager_change(PasswordManager_new(self->config), oldPassword, newPassword, changeFde, changeUser, CliArgs_has(args, "--update-tpm2"));
        __auto_type __btrc_ret_795 = 0;
        if (interactive != NULL) {
            if ((--interactive->__rc) <= 0) {
                Interactive_destroy(interactive);
            }
        }
        return __btrc_ret_795;
        if (interactive != NULL) {
            if ((--interactive->__rc) <= 0) {
                Interactive_destroy(interactive);
            }
        }
    }
    if (strcmp(cmd, "secure-boot") == 0) {
        if (CliArgs_count(args) < 2) {
            NixosLog_fatal("Usage: nixosctl secure-boot <enable|disable|status>");
        }
        SecureBootManager* manager = SecureBootManager_new(self->config);
        char* action = CliArgs_get(args, 1);
        if (strcmp(action, "enable") == 0) {
            SecureBootManager_enable(manager, CliArgs_has(args, "--microsoft"));
            __auto_type __btrc_ret_796 = 0;
            if (manager != NULL) {
                if ((--manager->__rc) <= 0) {
                    SecureBootManager_destroy(manager);
                }
            }
            return __btrc_ret_796;
        }
        if (strcmp(action, "disable") == 0) {
            SecureBootManager_disable(manager);
            __auto_type __btrc_ret_797 = 0;
            if (manager != NULL) {
                if ((--manager->__rc) <= 0) {
                    SecureBootManager_destroy(manager);
                }
            }
            return __btrc_ret_797;
        }
        if (strcmp(action, "status") == 0) {
            SecureBootManager_status(manager);
            __auto_type __btrc_ret_798 = 0;
            if (manager != NULL) {
                if ((--manager->__rc) <= 0) {
                    SecureBootManager_destroy(manager);
                }
            }
            return __btrc_ret_798;
        }
        NixosLog_fatal("Usage: nixosctl secure-boot <enable|disable|status>");
        if (manager != NULL) {
            if ((--manager->__rc) <= 0) {
                SecureBootManager_destroy(manager);
            }
        }
    }
    if (strcmp(cmd, "tpm2") == 0) {
        if (CliArgs_count(args) < 2) {
            NixosLog_fatal("Usage: nixosctl tpm2 <enable|disable|status>");
        }
        Tpm2Manager* tpm = Tpm2Manager_new(self->config);
        char* action = CliArgs_get(args, 1);
        if (strcmp(action, "status") == 0) {
            Tpm2Manager_status(tpm);
            __auto_type __btrc_ret_799 = 0;
            if (tpm != NULL) {
                if ((--tpm->__rc) <= 0) {
                    Tpm2Manager_destroy(tpm);
                }
            }
            return __btrc_ret_799;
        }
        if (strcmp(action, "enable") == 0) {
            if (!Tpm2Manager_exists(tpm)) {
                NixosLog_fatal("TPM2 does not exist");
            }
            if (!Tpm2Manager_diskEncrypted(tpm)) {
                NixosLog_fatal("Root disk is not LUKS encrypted");
            }
            if (!Tpm2Manager_enroll(tpm)) {
                NixosLog_fatal("TPM2 enrollment failed");
            }
            __auto_type __btrc_ret_800 = 0;
            if (tpm != NULL) {
                if ((--tpm->__rc) <= 0) {
                    Tpm2Manager_destroy(tpm);
                }
            }
            return __btrc_ret_800;
        }
        if (strcmp(action, "disable") == 0) {
            if (!Tpm2Manager_wipe(tpm)) {
                NixosLog_fatal("TPM2 wipe failed");
            }
            __auto_type __btrc_ret_801 = 0;
            if (tpm != NULL) {
                if ((--tpm->__rc) <= 0) {
                    Tpm2Manager_destroy(tpm);
                }
            }
            return __btrc_ret_801;
        }
        if (tpm != NULL) {
            if ((--tpm->__rc) <= 0) {
                Tpm2Manager_destroy(tpm);
            }
        }
    }
    if (strcmp(cmd, "displays") == 0) {
        if (CliArgs_count(args) < 2) {
            NixosLog_fatal("Usage: nixosctl displays <list|layout|enable|disable|primary|dpms> [args]");
        }
        DisplayManager* displays = DisplayManager_new();
        char* action = CliArgs_get(args, 1);
        if (strcmp(action, "list") == 0) {
            DisplayManager_list(displays);
            __auto_type __btrc_ret_802 = 0;
            if (displays != NULL) {
                if ((--displays->__rc) <= 0) {
                    DisplayManager_destroy(displays);
                }
            }
            return __btrc_ret_802;
        }
        if (strcmp(action, "layout") == 0) {
            DisplayManager_layout(displays);
            __auto_type __btrc_ret_803 = 0;
            if (displays != NULL) {
                if ((--displays->__rc) <= 0) {
                    DisplayManager_destroy(displays);
                }
            }
            return __btrc_ret_803;
        }
        if (CliArgs_count(args) < 3) {
            NixosLog_fatal("Missing display argument");
        }
        if (strcmp(action, "enable") == 0) {
            DisplayManager_enable(displays, CliArgs_get(args, 2));
            __auto_type __btrc_ret_804 = 0;
            if (displays != NULL) {
                if ((--displays->__rc) <= 0) {
                    DisplayManager_destroy(displays);
                }
            }
            return __btrc_ret_804;
        }
        if (strcmp(action, "disable") == 0) {
            DisplayManager_disable(displays, CliArgs_get(args, 2));
            __auto_type __btrc_ret_805 = 0;
            if (displays != NULL) {
                if ((--displays->__rc) <= 0) {
                    DisplayManager_destroy(displays);
                }
            }
            return __btrc_ret_805;
        }
        if (strcmp(action, "primary") == 0) {
            DisplayManager_primary(displays, CliArgs_get(args, 2));
            __auto_type __btrc_ret_806 = 0;
            if (displays != NULL) {
                if ((--displays->__rc) <= 0) {
                    DisplayManager_destroy(displays);
                }
            }
            return __btrc_ret_806;
        }
        if (strcmp(action, "dpms") == 0) {
            DisplayManager_dpms(displays, CliArgs_get(args, 2));
            __auto_type __btrc_ret_807 = 0;
            if (displays != NULL) {
                if ((--displays->__rc) <= 0) {
                    DisplayManager_destroy(displays);
                }
            }
            return __btrc_ret_807;
        }
        if (displays != NULL) {
            if ((--displays->__rc) <= 0) {
                DisplayManager_destroy(displays);
            }
        }
    }
    if (strcmp(cmd, "audio") == 0) {
        if (CliArgs_count(args) < 2) {
            NixosLog_fatal("Usage: nixosctl audio <list|current|set> [sink]");
        }
        AudioManager* audio = AudioManager_new();
        char* action = CliArgs_get(args, 1);
        if (strcmp(action, "list") == 0) {
            AudioManager_list(audio);
            __auto_type __btrc_ret_808 = 0;
            if (audio != NULL) {
                if ((--audio->__rc) <= 0) {
                    AudioManager_destroy(audio);
                }
            }
            return __btrc_ret_808;
        }
        if (strcmp(action, "current") == 0) {
            Console_log(AudioManager_current(audio));
            __auto_type __btrc_ret_809 = 0;
            if (audio != NULL) {
                if ((--audio->__rc) <= 0) {
                    AudioManager_destroy(audio);
                }
            }
            return __btrc_ret_809;
        }
        if (strcmp(action, "set") == 0) {
            if (CliArgs_count(args) < 3) {
                NixosLog_fatal("Usage: nixosctl audio set <sink>");
            }
            AudioManager_set(audio, CliArgs_get(args, 2));
            __auto_type __btrc_ret_810 = 0;
            if (audio != NULL) {
                if ((--audio->__rc) <= 0) {
                    AudioManager_destroy(audio);
                }
            }
            return __btrc_ret_810;
        }
        if (audio != NULL) {
            if ((--audio->__rc) <= 0) {
                AudioManager_destroy(audio);
            }
        }
    }
    if (strcmp(cmd, "caffeine") == 0) {
        if (CliArgs_count(args) < 2) {
            NixosLog_fatal("Usage: nixosctl caffeine <status|enable|disable|toggle>");
        }
        CaffeineManager* caffeine = CaffeineManager_new();
        char* action = CliArgs_get(args, 1);
        if (strcmp(action, "status") == 0) {
            __auto_type __btrc_ret_811 = (CaffeineManager_enabled(caffeine) ? 0 : 1);
            if (caffeine != NULL) {
                if ((--caffeine->__rc) <= 0) {
                    CaffeineManager_destroy(caffeine);
                }
            }
            return __btrc_ret_811;
        }
        if (strcmp(action, "enable") == 0) {
            CaffeineManager_enable(caffeine);
            __auto_type __btrc_ret_812 = 0;
            if (caffeine != NULL) {
                if ((--caffeine->__rc) <= 0) {
                    CaffeineManager_destroy(caffeine);
                }
            }
            return __btrc_ret_812;
        }
        if (strcmp(action, "disable") == 0) {
            CaffeineManager_disable(caffeine);
            __auto_type __btrc_ret_813 = 0;
            if (caffeine != NULL) {
                if ((--caffeine->__rc) <= 0) {
                    CaffeineManager_destroy(caffeine);
                }
            }
            return __btrc_ret_813;
        }
        if (strcmp(action, "toggle") == 0) {
            CaffeineManager_toggle(caffeine);
            __auto_type __btrc_ret_814 = 0;
            if (caffeine != NULL) {
                if ((--caffeine->__rc) <= 0) {
                    CaffeineManager_destroy(caffeine);
                }
            }
            return __btrc_ret_814;
        }
        if (caffeine != NULL) {
            if ((--caffeine->__rc) <= 0) {
                CaffeineManager_destroy(caffeine);
            }
        }
    }
    if (strcmp(cmd, "system") == 0) {
        if (CliArgs_count(args) < 2) {
            NixosLog_fatal("Usage: nixosctl system <update|upgrade>");
        }
        SystemUi* system = SystemUi_new();
        char* action = CliArgs_get(args, 1);
        if (strcmp(action, "update") == 0) {
            SystemUi_update(system);
            __auto_type __btrc_ret_815 = 0;
            if (system != NULL) {
                if ((--system->__rc) <= 0) {
                    SystemUi_destroy(system);
                }
            }
            return __btrc_ret_815;
        }
        if (strcmp(action, "upgrade") == 0) {
            SystemUi_upgrade(system);
            __auto_type __btrc_ret_816 = 0;
            if (system != NULL) {
                if ((--system->__rc) <= 0) {
                    SystemUi_destroy(system);
                }
            }
            return __btrc_ret_816;
        }
        if (system != NULL) {
            if ((--system->__rc) <= 0) {
                SystemUi_destroy(system);
            }
        }
    }
    if (strcmp(cmd, "vm") == 0) {
        __auto_type __btrc_ret_817 = NixosCtl_runVm(self, args);
        return __btrc_ret_817;
    }
    if (strcmp(cmd, "e2e") == 0) {
        if (CliArgs_count(args) < 2) {
            NixosLog_fatal("Usage: nixosctl e2e <spec.json>");
        }
        VmTestSpec* spec = VmSpecParser_readFile(CliArgs_get(args, 1));
        NixosCtl_applySpecArgs(self, spec, args, 2);
        VmTestRunner* runner = VmTestRunner_new(spec);
        int result = VmTestRunner_run(runner);
        if (runner != NULL) {
            if ((--runner->__rc) <= 0) {
                VmTestRunner_destroy(runner);
            }
        }
        return result;
        if (runner != NULL) {
            if ((--runner->__rc) <= 0) {
                VmTestRunner_destroy(runner);
            }
        }
    }
    if (strcmp(cmd, "graph") == 0) {
        __auto_type __btrc_ret_818 = NixosCtl_runGraph(self, args);
        return __btrc_ret_818;
    }
    NixosCtl_usage(self);
    __auto_type __btrc_ret_819 = 1;
    return __btrc_ret_819;
}

int main(int argc, char** argv) {
    CliArgs* args = CliArgs_new(argc, argv);
    NixosCtl* app = NixosCtl_new();
    int code = NixosCtl_run(app, args);
    if (app != NULL) {
        if ((--app->__rc) <= 0) {
            NixosCtl_destroy(app);
        }
    }
    if (args != NULL) {
        if ((--args->__rc) <= 0) {
            CliArgs_destroy(args);
        }
    }
    return code;
    if (app != NULL) {
        if ((--app->__rc) <= 0) {
            NixosCtl_destroy(app);
        }
    }
    if (args != NULL) {
        if ((--args->__rc) <= 0) {
            CliArgs_destroy(args);
        }
    }
}

