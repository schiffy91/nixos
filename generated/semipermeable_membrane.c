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
typedef struct JsonObject JsonObject;
void JsonObject_destroy(JsonObject* self);
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
typedef struct MembraneConstants MembraneConstants;
void MembraneConstants_destroy(MembraneConstants* self);
typedef struct MembraneAbsPath MembraneAbsPath;
void MembraneAbsPath_destroy(MembraneAbsPath* self);
typedef struct MembraneSpec MembraneSpec;
void MembraneSpec_destroy(MembraneSpec* self);
typedef struct MembraneVolume MembraneVolume;
void MembraneVolume_destroy(MembraneVolume* self);
typedef struct MembranePlan MembranePlan;
void MembranePlan_destroy(MembranePlan* self);
typedef struct MembranePaths MembranePaths;
void MembranePaths_destroy(MembranePaths* self);
typedef struct MembraneRun MembraneRun;
void MembraneRun_destroy(MembraneRun* self);
typedef struct MembraneBtrfs MembraneBtrfs;
void MembraneBtrfs_destroy(MembraneBtrfs* self);
typedef struct MembraneFs MembraneFs;
void MembraneFs_destroy(MembraneFs* self);
typedef struct MembranePlanner MembranePlanner;
void MembranePlanner_destroy(MembranePlanner* self);
typedef struct MembraneMounts MembraneMounts;
void MembraneMounts_destroy(MembraneMounts* self);
typedef struct MembraneSlots MembraneSlots;
void MembraneSlots_destroy(MembraneSlots* self);
typedef struct MembraneConfig MembraneConfig;
void MembraneConfig_destroy(MembraneConfig* self);
typedef struct SemipermeableMembrane SemipermeableMembrane;
void SemipermeableMembrane_destroy(SemipermeableMembrane* self);
typedef struct btrc_Vector_string btrc_Vector_string;
typedef struct btrc_Vector_bool btrc_Vector_bool;
typedef struct btrc_Vector_int btrc_Vector_int;
typedef struct btrc_Vector_float btrc_Vector_float;
typedef struct btrc_Vector_MembraneSpec btrc_Vector_MembraneSpec;
typedef struct btrc_Vector_MembranePlan btrc_Vector_MembranePlan;
typedef struct btrc_Vector_MembraneVolume btrc_Vector_MembraneVolume;
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
char* Strings_fromInt(int n);
char* Strings_fromFloat(float f);
void Console_init(Console* self);
void Console_error(char* msg);
void File_init(File* self, char* path, char* mode);
File* File_new(char* path, char* mode);
bool File_ok(File* self);
char* File_read(File* self);
void File_write(File* self, char* text);
void File_close(File* self);
void Path_init(Path* self);
Path* Path_new(void);
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
bool ExecResult_ok(ExecResult* self);
char* ExecResult_stdout(ExecResult* self);
char* ExecResult_trimmed(ExecResult* self);
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
ExecResult* UnixShell_runCommand(UnixShell* self, Command* command);
ExecResult* UnixShell_runRaw(UnixShell* self, char* command, bool captureOutput, bool checkStatus, char* sensitive);
void PowerShell_init(PowerShell* self);
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
bool FileSystem_isDir(char* path);
bool FileSystem_isSymlink(char* path);
int FileSystem_chmod(char* path, int mode);
int FileSystem_mkdir(char* path, int mode);
int FileSystem_mkdirp(char* path);
int FileSystem_removeRecursive(char* path);
int FileSystem_symlink(char* target, char* linkPath);
char* FileSystem_readLink(char* path);
char* FileSystem_tempDir(char* prefix);
char* FileSystem_readText(char* path);
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
Timer* Timer_new(void);
void Timer_start(Timer* self);
float Timer_elapsed(Timer* self);
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
void CliCommand_init(CliCommand* self, char* name);
void NixosLog_init(NixosLog* self);
char* NixosLog_gray(void);
char* NixosLog_orange(void);
char* NixosLog_red(void);
char* NixosLog_reset(void);
void NixosLog_fatal(char* message);
void MembraneConstants_init(MembraneConstants* self);
char* MembraneConstants_topBase(void);
char* MembraneConstants_dirs(void);
char* MembraneConstants_meta(void);
char* MembraneConstants_files(void);
char* MembraneConstants_next(void);
char* MembraneConstants_a(void);
char* MembraneConstants_b(void);
char* MembraneConstants_c(void);
char* MembraneConstants_disabled(void);
char* MembraneConstants_converge(void);
char* MembraneConstants_reset(void);
char* MembraneConstants_prepareOnly(void);
char* MembraneConstants_snapshotOnly(void);
char* MembraneConstants_restoreA(void);
char* MembraneConstants_restoreB(void);
char* MembraneConstants_restoreC(void);
char* MembraneConstants_restorePrevious(void);
char* MembraneConstants_restorePenultimate(void);
void MembraneAbsPath_init(MembraneAbsPath* self, char* path, bool endedWithSlash);
MembraneAbsPath* MembraneAbsPath_new(char* path, bool endedWithSlash);
void MembraneSpec_init(MembraneSpec* self, char* volume, char* mountPoint, char* absPath, char* relPath, char* kind);
MembraneSpec* MembraneSpec_new(char* volume, char* mountPoint, char* absPath, char* relPath, char* kind);
void MembraneVolume_init(MembraneVolume* self, char* name, char* mountPoint);
MembraneVolume* MembraneVolume_new(char* name, char* mountPoint);
void MembranePlan_init(MembranePlan* self, MembraneSpec* spec, char* kind, char* store);
MembranePlan* MembranePlan_new(MembraneSpec* spec, char* kind, char* store);
void MembranePaths_init(MembranePaths* self);
char* MembranePaths_trimSlashes(char* raw);
char* MembranePaths_trimTrailingSlashes(char* raw);
bool MembranePaths_validRel(char* path);
char* MembranePaths_normSubvol(char* raw);
char* MembranePaths_normMount(char* raw);
MembraneAbsPath* MembranePaths_normAbs(char* raw);
char* MembranePaths_relToMount(char* absPath, char* mountPoint);
char* MembranePaths_kind(char* raw);
bool MembranePaths_ancestor(char* parent, char* child);
char* MembranePaths_key(char* absPath);
char* MembranePaths_dirStore(char* persistRoot, char* absPath);
char* MembranePaths_fileStore(char* persistRoot, char* absPath);
void MembraneRun_init(MembraneRun* self, bool dryRun);
MembraneRun* MembraneRun_new(bool dryRun);
char* MembraneRun_elapsed(MembraneRun* self);
char* MembraneRun_path(MembraneRun* self, char* rel);
void MembraneRun_log(MembraneRun* self, char* message);
void MembraneRun_fatal(MembraneRun* self, char* message);
ExecResult* MembraneRun_runCommand(MembraneRun* self, Command* command);
ExecResult* MembraneRun_runRaw(MembraneRun* self, char* command, bool captureOutput, bool checkStatus);
void MembraneRun_requireCommand(MembraneRun* self, Command* command);
void MembraneRun_requireRaw(MembraneRun* self, char* command);
void MembraneRun_mkdirp(MembraneRun* self, char* path);
void MembraneRun_removePath(MembraneRun* self, char* path);
void MembraneRun_renamePath(MembraneRun* self, char* source, char* destination);
void MembraneBtrfs_init(MembraneBtrfs* self);
Command* MembraneBtrfs_show(char* path);
Command* MembraneBtrfs_getReadonly(char* path);
Command* MembraneBtrfs_listChildren(char* path);
Command* MembraneBtrfs_deleteSubvol(char* path);
Command* MembraneBtrfs_createSubvol(char* path);
Command* MembraneBtrfs_snapshot(char* source, char* destination, bool readonly);
Command* MembraneBtrfs_clearReadonly(char* path);
Command* MembraneBtrfs_sync(char* top);
void MembraneFs_init(MembraneFs* self, MembraneRun* run);
MembraneFs* MembraneFs_new(MembraneRun* run);
bool MembraneFs_exists(MembraneFs* self, char* path);
bool MembraneFs_isSubvolume(MembraneFs* self, char* path);
bool MembraneFs_isReadonly(MembraneFs* self, char* path);
bool MembraneFs_mounted(MembraneFs* self, char* path);
int MembraneFs_compareDeepestFirst(MembraneFs* self, char* left, char* right);
void MembraneFs_pushChildSorted(MembraneFs* self, btrc_Vector_string* paths, char* path);
btrc_Vector_string* MembraneFs_childSubvolumes(MembraneFs* self, char* path);
void MembraneFs_deleteSubvolume(MembraneFs* self, char* path);
void MembraneFs_makeSubvolume(MembraneFs* self, char* path);
void MembraneFs_snapshot(MembraneFs* self, char* source, char* destination, bool readonly);
char* MembraneFs_sourcePath(MembraneFs* self, char* live, char* clean, char* relPath);
void MembraneFs_copyPath(MembraneFs* self, char* source, char* destination);
void MembraneFs_copyDirectoryContents(MembraneFs* self, char* source, char* destination);
void MembranePlanner_init(MembranePlanner* self, MembraneFs* fs);
MembranePlanner* MembranePlanner_new(MembraneFs* fs);
int MembranePlanner_compareSpecs(MembranePlanner* self, MembraneSpec* left, MembraneSpec* right);
void MembranePlanner_pushSpecSorted(MembranePlanner* self, btrc_Vector_MembraneSpec* specs, MembraneSpec* spec);
btrc_Vector_MembraneSpec* MembranePlanner_sortedSpecs(MembranePlanner* self, btrc_Vector_MembraneSpec* input);
char* MembranePlanner_resolveKind(MembranePlanner* self, char* live, char* clean, MembraneSpec* spec);
bool MembranePlanner_planAlreadyCovers(MembranePlanner* self, btrc_Vector_MembranePlan* plans, MembraneSpec* spec);
btrc_Vector_MembranePlan* MembranePlanner_makePlan(MembranePlanner* self, btrc_Vector_MembraneSpec* specs, char* live, char* clean, char* persistRoot);
void MembraneMounts_init(MembraneMounts* self, MembraneRun* run, MembraneFs* fs);
MembraneMounts* MembraneMounts_new(MembraneRun* run, MembraneFs* fs);
void MembraneMounts_mountTop(MembraneMounts* self, char* device, bool assumeMounted);
void MembraneMounts_unmountTop(MembraneMounts* self, bool assumeMounted);
bool MembraneMounts_coveredBy(MembraneMounts* self, btrc_Vector_string* selected, char* target);
void MembraneMounts_mountTarget(MembraneMounts* self, char* device, char* target, char* subvol);
void MembraneMounts_mountPersist(MembraneMounts* self, char* device, char* persistRoot, btrc_Vector_MembraneSpec* specs, bool assumeMounted);
void MembraneMounts_snapshotCleanVolume(MembraneMounts* self, MembraneVolume* volume, char* snapshotsSubvolume, char* cleanName);
void MembraneMounts_snapshotCleanAll(MembraneMounts* self, char* device, char* snapshotsSubvolume, char* cleanName, btrc_Vector_MembraneVolume* volumes, bool assumeMounted);
void MembraneSlots_init(MembraneSlots* self, MembraneFs* fs, MembraneRun* run);
MembraneSlots* MembraneSlots_new(MembraneFs* fs, MembraneRun* run);
void MembraneSlots_rotate(MembraneSlots* self, char* volume, char* live, char* snapshotsSubvolume);
void MembraneSlots_publish(MembraneSlots* self, MembraneVolume* volume, char* live, char* next);
void MembraneSlots_restoreSlot(MembraneSlots* self, MembraneVolume* volume, char* slot, char* snapshotsSubvolume);
void MembraneConfig_init(MembraneConfig* self);
btrc_Vector_MembraneSpec* MembraneConfig_readSpecs(char* specFile);
bool MembraneConfig_volumeSeen(btrc_Vector_MembraneVolume* volumes, char* name);
btrc_Vector_MembraneVolume* MembraneConfig_readVolumes(btrc_Vector_string* args, btrc_Vector_MembraneSpec* specs);
void SemipermeableMembrane_init(SemipermeableMembrane* self, bool dryRun, bool assumeMounted);
SemipermeableMembrane* SemipermeableMembrane_new(bool dryRun, bool assumeMounted);
bool SemipermeableMembrane_envDryRun(void);
void SemipermeableMembrane_ensureNamespaces(SemipermeableMembrane* self);
bool SemipermeableMembrane_keyInSpec(SemipermeableMembrane* self, btrc_Vector_string* validKeys, char* key);
void SemipermeableMembrane_pruneOrphans(SemipermeableMembrane* self);
btrc_Vector_MembraneSpec* SemipermeableMembrane_specsForVolume(SemipermeableMembrane* self, char* volume);
void SemipermeableMembrane_resetVolume(SemipermeableMembrane* self, MembraneVolume* volume);
void SemipermeableMembrane_restorePersistentDirs(SemipermeableMembrane* self, btrc_Vector_MembranePlan* plans, char* live, char* clean);
void SemipermeableMembrane_restorePersistentFiles(SemipermeableMembrane* self, btrc_Vector_MembranePlan* plans, char* live, char* clean);
void SemipermeableMembrane_populateNext(SemipermeableMembrane* self, btrc_Vector_MembranePlan* plans, char* next);
void SemipermeableMembrane_configure(SemipermeableMembrane* self, char* device, char* snapshotsSubvolume, char* cleanName, char* mode, char* persistRoot, char* specFile, btrc_Vector_string* volumeArgs);
void SemipermeableMembrane_runAll(SemipermeableMembrane* self);
void SemipermeableMembrane_configureMount(SemipermeableMembrane* self, char* device, char* persistRoot, char* specFile);
void SemipermeableMembrane_mountPersist(SemipermeableMembrane* self);
void SemipermeableMembrane_configureSnapshotClean(SemipermeableMembrane* self, char* device, char* snapshotsSubvolume, char* cleanName, btrc_Vector_string* volumeArgs);
void SemipermeableMembrane_snapshotCleanAll(SemipermeableMembrane* self);
int SemipermeableMembrane_cli(CliArgs* args);
typedef bool (*__btrc_fn_bool_string)(char*);
typedef void (*__btrc_fn_void_string)(char*);
typedef char* (*__btrc_fn_string_string)(char*);
typedef char* (*__btrc_fn_string_string_string)(char*, char*);
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
typedef bool (*__btrc_fn_bool_MembraneSpec)(MembraneSpec*);
typedef void (*__btrc_fn_void_MembraneSpec)(MembraneSpec*);
typedef MembraneSpec* (*__btrc_fn_MembraneSpec_MembraneSpec)(MembraneSpec*);
typedef MembraneSpec* (*__btrc_fn_MembraneSpec_MembraneSpec_MembraneSpec)(MembraneSpec*, MembraneSpec*);
typedef bool (*__btrc_fn_bool_MembranePlan)(MembranePlan*);
typedef void (*__btrc_fn_void_MembranePlan)(MembranePlan*);
typedef MembranePlan* (*__btrc_fn_MembranePlan_MembranePlan)(MembranePlan*);
typedef MembranePlan* (*__btrc_fn_MembranePlan_MembranePlan_MembranePlan)(MembranePlan*, MembranePlan*);
typedef bool (*__btrc_fn_bool_MembraneVolume)(MembraneVolume*);
typedef void (*__btrc_fn_void_MembraneVolume)(MembraneVolume*);
typedef MembraneVolume* (*__btrc_fn_MembraneVolume_MembraneVolume)(MembraneVolume*);
typedef MembraneVolume* (*__btrc_fn_MembraneVolume_MembraneVolume_MembraneVolume)(MembraneVolume*, MembraneVolume*);

struct btrc_Vector_string {
    int __rc;
    char** data;
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

struct btrc_Vector_MembraneSpec {
    int __rc;
    MembraneSpec** data;
    int len;
    int cap;
};

struct btrc_Vector_MembranePlan {
    int __rc;
    MembranePlan** data;
    int len;
    int cap;
};

struct btrc_Vector_MembraneVolume {
    int __rc;
    MembraneVolume** data;
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

struct JsonObject {
    int __rc;
    btrc_Map_string_string* values;
    btrc_Map_string_bool* quoted;
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

struct MembraneConstants {
    int __rc;
};

struct MembraneAbsPath {
    int __rc;
    char* path;
    bool endedWithSlash;
};

struct MembraneSpec {
    int __rc;
    char* volume;
    char* mountPoint;
    char* absPath;
    char* relPath;
    char* kind;
};

struct MembraneVolume {
    int __rc;
    char* name;
    char* mountPoint;
};

struct MembranePlan {
    int __rc;
    MembraneSpec* spec;
    char* kind;
    char* store;
};

struct MembranePaths {
    int __rc;
};

struct MembraneRun {
    int __rc;
    UnixShell* shell;
    Timer* timer;
    char* top;
    bool dryRun;
};

struct MembraneBtrfs {
    int __rc;
};

struct MembraneFs {
    int __rc;
    MembraneRun* run;
};

struct MembranePlanner {
    int __rc;
    MembraneFs* fs;
};

struct MembraneMounts {
    int __rc;
    MembraneRun* run;
    MembraneFs* fs;
};

struct MembraneSlots {
    int __rc;
    MembraneFs* fs;
    MembraneRun* run;
};

struct MembraneConfig {
    int __rc;
};

struct SemipermeableMembrane {
    int __rc;
    MembraneRun* run;
    MembraneFs* fs;
    MembranePlanner* planner;
    MembraneMounts* mounts;
    MembraneSlots* slots;
    char* device;
    char* snapshotsSubvolume;
    char* cleanName;
    char* mode;
    char* persistRoot;
    char* specFile;
    btrc_Vector_MembraneSpec* specs;
    btrc_Vector_MembraneVolume* volumes;
    bool assumeMounted;
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

static void btrc_Vector_MembraneSpec_init(btrc_Vector_MembraneSpec* self);
static btrc_Vector_MembraneSpec* btrc_Vector_MembraneSpec_new(void);
static void btrc_Vector_MembraneSpec_destroy(btrc_Vector_MembraneSpec* self);
static void btrc_Vector_MembraneSpec_push(btrc_Vector_MembraneSpec* self, MembraneSpec* val);
static MembraneSpec* btrc_Vector_MembraneSpec_pop(btrc_Vector_MembraneSpec* self);
static MembraneSpec* btrc_Vector_MembraneSpec_get(btrc_Vector_MembraneSpec* self, int i);
static void btrc_Vector_MembraneSpec_set(btrc_Vector_MembraneSpec* self, int i, MembraneSpec* val);
static void btrc_Vector_MembraneSpec_free(btrc_Vector_MembraneSpec* self);
static void btrc_Vector_MembraneSpec_remove(btrc_Vector_MembraneSpec* self, int idx);
static void btrc_Vector_MembraneSpec_reverse(btrc_Vector_MembraneSpec* self);
static btrc_Vector_MembraneSpec* btrc_Vector_MembraneSpec_reversed(btrc_Vector_MembraneSpec* self);
static void btrc_Vector_MembraneSpec_swap(btrc_Vector_MembraneSpec* self, int i, int j);
static void btrc_Vector_MembraneSpec_clear(btrc_Vector_MembraneSpec* self);
static void btrc_Vector_MembraneSpec_fill(btrc_Vector_MembraneSpec* self, MembraneSpec* val);
static int btrc_Vector_MembraneSpec_size(btrc_Vector_MembraneSpec* self);
static bool btrc_Vector_MembraneSpec_isEmpty(btrc_Vector_MembraneSpec* self);
static MembraneSpec* btrc_Vector_MembraneSpec_first(btrc_Vector_MembraneSpec* self);
static MembraneSpec* btrc_Vector_MembraneSpec_last(btrc_Vector_MembraneSpec* self);
static btrc_Vector_MembraneSpec* btrc_Vector_MembraneSpec_slice(btrc_Vector_MembraneSpec* self, int start, int end);
static btrc_Vector_MembraneSpec* btrc_Vector_MembraneSpec_take(btrc_Vector_MembraneSpec* self, int n);
static btrc_Vector_MembraneSpec* btrc_Vector_MembraneSpec_drop(btrc_Vector_MembraneSpec* self, int n);
static void btrc_Vector_MembraneSpec_extend(btrc_Vector_MembraneSpec* self, btrc_Vector_MembraneSpec* other);
static void btrc_Vector_MembraneSpec_insert(btrc_Vector_MembraneSpec* self, int idx, MembraneSpec* val);
static bool btrc_Vector_MembraneSpec_contains(btrc_Vector_MembraneSpec* self, MembraneSpec* val);
static int btrc_Vector_MembraneSpec_indexOf(btrc_Vector_MembraneSpec* self, MembraneSpec* val);
static int btrc_Vector_MembraneSpec_lastIndexOf(btrc_Vector_MembraneSpec* self, MembraneSpec* val);
static int btrc_Vector_MembraneSpec_count(btrc_Vector_MembraneSpec* self, MembraneSpec* val);
static void btrc_Vector_MembraneSpec_removeAll(btrc_Vector_MembraneSpec* self, MembraneSpec* val);
static btrc_Vector_MembraneSpec* btrc_Vector_MembraneSpec_distinct(btrc_Vector_MembraneSpec* self);
static void btrc_Vector_MembraneSpec_sort(btrc_Vector_MembraneSpec* self);
static btrc_Vector_MembraneSpec* btrc_Vector_MembraneSpec_sorted(btrc_Vector_MembraneSpec* self);
static MembraneSpec* btrc_Vector_MembraneSpec_min(btrc_Vector_MembraneSpec* self);
static MembraneSpec* btrc_Vector_MembraneSpec_max(btrc_Vector_MembraneSpec* self);
static MembraneSpec* btrc_Vector_MembraneSpec_sum(btrc_Vector_MembraneSpec* self);
static char* btrc_Vector_MembraneSpec_join(btrc_Vector_MembraneSpec* self, char* sep);
static char* btrc_Vector_MembraneSpec_joinToString(btrc_Vector_MembraneSpec* self, char* sep);
static btrc_Vector_MembraneSpec* btrc_Vector_MembraneSpec_filter(btrc_Vector_MembraneSpec* self, __btrc_fn_bool_MembraneSpec pred);
static int btrc_Vector_MembraneSpec_findIndex(btrc_Vector_MembraneSpec* self, __btrc_fn_bool_MembraneSpec pred);
static void btrc_Vector_MembraneSpec_forEach(btrc_Vector_MembraneSpec* self, __btrc_fn_void_MembraneSpec fn);
static btrc_Vector_MembraneSpec* btrc_Vector_MembraneSpec_map(btrc_Vector_MembraneSpec* self, __btrc_fn_MembraneSpec_MembraneSpec fn);
static bool btrc_Vector_MembraneSpec_any(btrc_Vector_MembraneSpec* self, __btrc_fn_bool_MembraneSpec pred);
static bool btrc_Vector_MembraneSpec_all(btrc_Vector_MembraneSpec* self, __btrc_fn_bool_MembraneSpec pred);
static MembraneSpec* btrc_Vector_MembraneSpec_reduce(btrc_Vector_MembraneSpec* self, MembraneSpec* init, __btrc_fn_MembraneSpec_MembraneSpec_MembraneSpec fn);
static btrc_Vector_MembraneSpec* btrc_Vector_MembraneSpec_copy(btrc_Vector_MembraneSpec* self);
static void btrc_Vector_MembraneSpec_removeAt(btrc_Vector_MembraneSpec* self, int idx);
static int btrc_Vector_MembraneSpec_iterLen(btrc_Vector_MembraneSpec* self);
static MembraneSpec* btrc_Vector_MembraneSpec_iterGet(btrc_Vector_MembraneSpec* self, int i);

static void btrc_Vector_MembranePlan_init(btrc_Vector_MembranePlan* self);
static btrc_Vector_MembranePlan* btrc_Vector_MembranePlan_new(void);
static void btrc_Vector_MembranePlan_destroy(btrc_Vector_MembranePlan* self);
static void btrc_Vector_MembranePlan_push(btrc_Vector_MembranePlan* self, MembranePlan* val);
static MembranePlan* btrc_Vector_MembranePlan_pop(btrc_Vector_MembranePlan* self);
static MembranePlan* btrc_Vector_MembranePlan_get(btrc_Vector_MembranePlan* self, int i);
static void btrc_Vector_MembranePlan_set(btrc_Vector_MembranePlan* self, int i, MembranePlan* val);
static void btrc_Vector_MembranePlan_free(btrc_Vector_MembranePlan* self);
static void btrc_Vector_MembranePlan_remove(btrc_Vector_MembranePlan* self, int idx);
static void btrc_Vector_MembranePlan_reverse(btrc_Vector_MembranePlan* self);
static btrc_Vector_MembranePlan* btrc_Vector_MembranePlan_reversed(btrc_Vector_MembranePlan* self);
static void btrc_Vector_MembranePlan_swap(btrc_Vector_MembranePlan* self, int i, int j);
static void btrc_Vector_MembranePlan_clear(btrc_Vector_MembranePlan* self);
static void btrc_Vector_MembranePlan_fill(btrc_Vector_MembranePlan* self, MembranePlan* val);
static int btrc_Vector_MembranePlan_size(btrc_Vector_MembranePlan* self);
static bool btrc_Vector_MembranePlan_isEmpty(btrc_Vector_MembranePlan* self);
static MembranePlan* btrc_Vector_MembranePlan_first(btrc_Vector_MembranePlan* self);
static MembranePlan* btrc_Vector_MembranePlan_last(btrc_Vector_MembranePlan* self);
static btrc_Vector_MembranePlan* btrc_Vector_MembranePlan_slice(btrc_Vector_MembranePlan* self, int start, int end);
static btrc_Vector_MembranePlan* btrc_Vector_MembranePlan_take(btrc_Vector_MembranePlan* self, int n);
static btrc_Vector_MembranePlan* btrc_Vector_MembranePlan_drop(btrc_Vector_MembranePlan* self, int n);
static void btrc_Vector_MembranePlan_extend(btrc_Vector_MembranePlan* self, btrc_Vector_MembranePlan* other);
static void btrc_Vector_MembranePlan_insert(btrc_Vector_MembranePlan* self, int idx, MembranePlan* val);
static bool btrc_Vector_MembranePlan_contains(btrc_Vector_MembranePlan* self, MembranePlan* val);
static int btrc_Vector_MembranePlan_indexOf(btrc_Vector_MembranePlan* self, MembranePlan* val);
static int btrc_Vector_MembranePlan_lastIndexOf(btrc_Vector_MembranePlan* self, MembranePlan* val);
static int btrc_Vector_MembranePlan_count(btrc_Vector_MembranePlan* self, MembranePlan* val);
static void btrc_Vector_MembranePlan_removeAll(btrc_Vector_MembranePlan* self, MembranePlan* val);
static btrc_Vector_MembranePlan* btrc_Vector_MembranePlan_distinct(btrc_Vector_MembranePlan* self);
static void btrc_Vector_MembranePlan_sort(btrc_Vector_MembranePlan* self);
static btrc_Vector_MembranePlan* btrc_Vector_MembranePlan_sorted(btrc_Vector_MembranePlan* self);
static MembranePlan* btrc_Vector_MembranePlan_min(btrc_Vector_MembranePlan* self);
static MembranePlan* btrc_Vector_MembranePlan_max(btrc_Vector_MembranePlan* self);
static MembranePlan* btrc_Vector_MembranePlan_sum(btrc_Vector_MembranePlan* self);
static char* btrc_Vector_MembranePlan_join(btrc_Vector_MembranePlan* self, char* sep);
static char* btrc_Vector_MembranePlan_joinToString(btrc_Vector_MembranePlan* self, char* sep);
static btrc_Vector_MembranePlan* btrc_Vector_MembranePlan_filter(btrc_Vector_MembranePlan* self, __btrc_fn_bool_MembranePlan pred);
static int btrc_Vector_MembranePlan_findIndex(btrc_Vector_MembranePlan* self, __btrc_fn_bool_MembranePlan pred);
static void btrc_Vector_MembranePlan_forEach(btrc_Vector_MembranePlan* self, __btrc_fn_void_MembranePlan fn);
static btrc_Vector_MembranePlan* btrc_Vector_MembranePlan_map(btrc_Vector_MembranePlan* self, __btrc_fn_MembranePlan_MembranePlan fn);
static bool btrc_Vector_MembranePlan_any(btrc_Vector_MembranePlan* self, __btrc_fn_bool_MembranePlan pred);
static bool btrc_Vector_MembranePlan_all(btrc_Vector_MembranePlan* self, __btrc_fn_bool_MembranePlan pred);
static MembranePlan* btrc_Vector_MembranePlan_reduce(btrc_Vector_MembranePlan* self, MembranePlan* init, __btrc_fn_MembranePlan_MembranePlan_MembranePlan fn);
static btrc_Vector_MembranePlan* btrc_Vector_MembranePlan_copy(btrc_Vector_MembranePlan* self);
static void btrc_Vector_MembranePlan_removeAt(btrc_Vector_MembranePlan* self, int idx);
static int btrc_Vector_MembranePlan_iterLen(btrc_Vector_MembranePlan* self);
static MembranePlan* btrc_Vector_MembranePlan_iterGet(btrc_Vector_MembranePlan* self, int i);

static void btrc_Vector_MembraneVolume_init(btrc_Vector_MembraneVolume* self);
static btrc_Vector_MembraneVolume* btrc_Vector_MembraneVolume_new(void);
static void btrc_Vector_MembraneVolume_destroy(btrc_Vector_MembraneVolume* self);
static void btrc_Vector_MembraneVolume_push(btrc_Vector_MembraneVolume* self, MembraneVolume* val);
static MembraneVolume* btrc_Vector_MembraneVolume_pop(btrc_Vector_MembraneVolume* self);
static MembraneVolume* btrc_Vector_MembraneVolume_get(btrc_Vector_MembraneVolume* self, int i);
static void btrc_Vector_MembraneVolume_set(btrc_Vector_MembraneVolume* self, int i, MembraneVolume* val);
static void btrc_Vector_MembraneVolume_free(btrc_Vector_MembraneVolume* self);
static void btrc_Vector_MembraneVolume_remove(btrc_Vector_MembraneVolume* self, int idx);
static void btrc_Vector_MembraneVolume_reverse(btrc_Vector_MembraneVolume* self);
static btrc_Vector_MembraneVolume* btrc_Vector_MembraneVolume_reversed(btrc_Vector_MembraneVolume* self);
static void btrc_Vector_MembraneVolume_swap(btrc_Vector_MembraneVolume* self, int i, int j);
static void btrc_Vector_MembraneVolume_clear(btrc_Vector_MembraneVolume* self);
static void btrc_Vector_MembraneVolume_fill(btrc_Vector_MembraneVolume* self, MembraneVolume* val);
static int btrc_Vector_MembraneVolume_size(btrc_Vector_MembraneVolume* self);
static bool btrc_Vector_MembraneVolume_isEmpty(btrc_Vector_MembraneVolume* self);
static MembraneVolume* btrc_Vector_MembraneVolume_first(btrc_Vector_MembraneVolume* self);
static MembraneVolume* btrc_Vector_MembraneVolume_last(btrc_Vector_MembraneVolume* self);
static btrc_Vector_MembraneVolume* btrc_Vector_MembraneVolume_slice(btrc_Vector_MembraneVolume* self, int start, int end);
static btrc_Vector_MembraneVolume* btrc_Vector_MembraneVolume_take(btrc_Vector_MembraneVolume* self, int n);
static btrc_Vector_MembraneVolume* btrc_Vector_MembraneVolume_drop(btrc_Vector_MembraneVolume* self, int n);
static void btrc_Vector_MembraneVolume_extend(btrc_Vector_MembraneVolume* self, btrc_Vector_MembraneVolume* other);
static void btrc_Vector_MembraneVolume_insert(btrc_Vector_MembraneVolume* self, int idx, MembraneVolume* val);
static bool btrc_Vector_MembraneVolume_contains(btrc_Vector_MembraneVolume* self, MembraneVolume* val);
static int btrc_Vector_MembraneVolume_indexOf(btrc_Vector_MembraneVolume* self, MembraneVolume* val);
static int btrc_Vector_MembraneVolume_lastIndexOf(btrc_Vector_MembraneVolume* self, MembraneVolume* val);
static int btrc_Vector_MembraneVolume_count(btrc_Vector_MembraneVolume* self, MembraneVolume* val);
static void btrc_Vector_MembraneVolume_removeAll(btrc_Vector_MembraneVolume* self, MembraneVolume* val);
static btrc_Vector_MembraneVolume* btrc_Vector_MembraneVolume_distinct(btrc_Vector_MembraneVolume* self);
static void btrc_Vector_MembraneVolume_sort(btrc_Vector_MembraneVolume* self);
static btrc_Vector_MembraneVolume* btrc_Vector_MembraneVolume_sorted(btrc_Vector_MembraneVolume* self);
static MembraneVolume* btrc_Vector_MembraneVolume_min(btrc_Vector_MembraneVolume* self);
static MembraneVolume* btrc_Vector_MembraneVolume_max(btrc_Vector_MembraneVolume* self);
static MembraneVolume* btrc_Vector_MembraneVolume_sum(btrc_Vector_MembraneVolume* self);
static char* btrc_Vector_MembraneVolume_join(btrc_Vector_MembraneVolume* self, char* sep);
static char* btrc_Vector_MembraneVolume_joinToString(btrc_Vector_MembraneVolume* self, char* sep);
static btrc_Vector_MembraneVolume* btrc_Vector_MembraneVolume_filter(btrc_Vector_MembraneVolume* self, __btrc_fn_bool_MembraneVolume pred);
static int btrc_Vector_MembraneVolume_findIndex(btrc_Vector_MembraneVolume* self, __btrc_fn_bool_MembraneVolume pred);
static void btrc_Vector_MembraneVolume_forEach(btrc_Vector_MembraneVolume* self, __btrc_fn_void_MembraneVolume fn);
static btrc_Vector_MembraneVolume* btrc_Vector_MembraneVolume_map(btrc_Vector_MembraneVolume* self, __btrc_fn_MembraneVolume_MembraneVolume fn);
static bool btrc_Vector_MembraneVolume_any(btrc_Vector_MembraneVolume* self, __btrc_fn_bool_MembraneVolume pred);
static bool btrc_Vector_MembraneVolume_all(btrc_Vector_MembraneVolume* self, __btrc_fn_bool_MembraneVolume pred);
static MembraneVolume* btrc_Vector_MembraneVolume_reduce(btrc_Vector_MembraneVolume* self, MembraneVolume* init, __btrc_fn_MembraneVolume_MembraneVolume_MembraneVolume fn);
static btrc_Vector_MembraneVolume* btrc_Vector_MembraneVolume_copy(btrc_Vector_MembraneVolume* self);
static void btrc_Vector_MembraneVolume_removeAt(btrc_Vector_MembraneVolume* self, int idx);
static int btrc_Vector_MembraneVolume_iterLen(btrc_Vector_MembraneVolume* self);
static MembraneVolume* btrc_Vector_MembraneVolume_iterGet(btrc_Vector_MembraneVolume* self, int i);

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

static void btrc_Vector_MembraneSpec_init(btrc_Vector_MembraneSpec* self) {
    self->__rc = 1;
    (self->data = NULL);
    (self->len = 0);
    (self->cap = 0);
}

static btrc_Vector_MembraneSpec* btrc_Vector_MembraneSpec_new(void) {
    btrc_Vector_MembraneSpec* self = ((btrc_Vector_MembraneSpec*)malloc(sizeof(btrc_Vector_MembraneSpec)));
    memset(self, 0, sizeof(btrc_Vector_MembraneSpec));
    btrc_Vector_MembraneSpec_init(self);
    return self;
}

static void btrc_Vector_MembraneSpec_destroy(btrc_Vector_MembraneSpec* self) {
    free(self);
}

static void btrc_Vector_MembraneSpec_push(btrc_Vector_MembraneSpec* self, MembraneSpec* val) {
    (val->__rc++);
    if (self->len >= self->cap) {
        (self->cap = ((self->cap == 0) ? 4 : (self->cap * 2)));
        (self->data = ((MembraneSpec**)__btrc_safe_realloc(self->data, (sizeof(MembraneSpec*) * self->cap))));
    }
    (self->data[self->len] = val);
    (self->len++);
}

static MembraneSpec* btrc_Vector_MembraneSpec_pop(btrc_Vector_MembraneSpec* self) {
    if (self->len <= 0) {
        fprintf(stderr, "Vector pop from empty list\n");
        exit(1);
    }
    (self->len--);
    return self->data[self->len];
}

static MembraneSpec* btrc_Vector_MembraneSpec_get(btrc_Vector_MembraneSpec* self, int i) {
    if ((i < 0) || (i >= self->len)) {
        fprintf(stderr, "Vector index out of bounds: %d (len=%d)\n", i, self->len);
        exit(1);
    }
    return self->data[i];
}

static void btrc_Vector_MembraneSpec_set(btrc_Vector_MembraneSpec* self, int i, MembraneSpec* val) {
    if ((i < 0) || (i >= self->len)) {
        fprintf(stderr, "Vector index out of bounds: %d (len=%d)\n", i, self->len);
        exit(1);
    }
    if (self->data[i]) {
        if ((--self->data[i]->__rc) <= 0) {
            MembraneSpec_destroy(self->data[i]);
        }
    }
    (val->__rc++);
    (self->data[i] = val);
}

static void btrc_Vector_MembraneSpec_free(btrc_Vector_MembraneSpec* self) {
    for (int i = 0; (i < self->len); (i++)) {
        if (self->data[i]) {
            if ((--self->data[i]->__rc) <= 0) {
                MembraneSpec_destroy(self->data[i]);
            }
        }
    }
    free(self->data);
    (self->data = NULL);
    (self->len = 0);
    (self->cap = 0);
}

static void btrc_Vector_MembraneSpec_remove(btrc_Vector_MembraneSpec* self, int idx) {
    if ((idx < 0) || (idx >= self->len)) {
        fprintf(stderr, "Vector remove index out of bounds: %d (len=%d)\n", idx, self->len);
        exit(1);
    }
    if (self->data[idx]) {
        if ((--self->data[idx]->__rc) <= 0) {
            MembraneSpec_destroy(self->data[idx]);
        }
    }
    for (int i = idx; (i < (self->len - 1)); (i++)) {
        (self->data[i] = self->data[(i + 1)]);
    }
    (self->len--);
}

static void btrc_Vector_MembraneSpec_reverse(btrc_Vector_MembraneSpec* self) {
    for (int i = 0; (i < (self->len / 2)); (i++)) {
        MembraneSpec* tmp = self->data[i];
        (self->data[i] = self->data[((self->len - 1) - i)]);
        (self->data[((self->len - 1) - i)] = tmp);
    }
}

static btrc_Vector_MembraneSpec* btrc_Vector_MembraneSpec_reversed(btrc_Vector_MembraneSpec* self) {
    btrc_Vector_MembraneSpec* result = btrc_Vector_MembraneSpec_new();
    for (int i = (self->len - 1); (i >= 0); (i--)) {
        btrc_Vector_MembraneSpec_push(result, self->data[i]);
    }
    return result;
}

static void btrc_Vector_MembraneSpec_swap(btrc_Vector_MembraneSpec* self, int i, int j) {
    if ((((i < 0) || (i >= self->len)) || (j < 0)) || (j >= self->len)) {
        fprintf(stderr, "Vector swap index out of bounds\n");
        exit(1);
    }
    MembraneSpec* tmp = self->data[i];
    (self->data[i] = self->data[j]);
    (self->data[j] = tmp);
}

static void btrc_Vector_MembraneSpec_clear(btrc_Vector_MembraneSpec* self) {
    for (int i = 0; (i < self->len); (i++)) {
        if (self->data[i]) {
            if ((--self->data[i]->__rc) <= 0) {
                MembraneSpec_destroy(self->data[i]);
            }
        }
    }
    (self->len = 0);
}

static void btrc_Vector_MembraneSpec_fill(btrc_Vector_MembraneSpec* self, MembraneSpec* val) {
    for (int i = 0; (i < self->len); (i++)) {
        if (self->data[i]) {
            if ((--self->data[i]->__rc) <= 0) {
                MembraneSpec_destroy(self->data[i]);
            }
        }
        (val->__rc++);
        (self->data[i] = val);
    }
}

static int btrc_Vector_MembraneSpec_size(btrc_Vector_MembraneSpec* self) {
    return self->len;
}

static bool btrc_Vector_MembraneSpec_isEmpty(btrc_Vector_MembraneSpec* self) {
    return (self->len == 0);
}

static MembraneSpec* btrc_Vector_MembraneSpec_first(btrc_Vector_MembraneSpec* self) {
    if (self->len == 0) {
        fprintf(stderr, "Vector.first() called on empty list\n");
        exit(1);
    }
    return self->data[0];
}

static MembraneSpec* btrc_Vector_MembraneSpec_last(btrc_Vector_MembraneSpec* self) {
    if (self->len == 0) {
        fprintf(stderr, "Vector.last() called on empty list\n");
        exit(1);
    }
    return self->data[(self->len - 1)];
}

static btrc_Vector_MembraneSpec* btrc_Vector_MembraneSpec_slice(btrc_Vector_MembraneSpec* self, int start, int end) {
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
    btrc_Vector_MembraneSpec* result = btrc_Vector_MembraneSpec_new();
    for (int i = start; (i < end); (i++)) {
        btrc_Vector_MembraneSpec_push(result, self->data[i]);
    }
    return result;
}

static btrc_Vector_MembraneSpec* btrc_Vector_MembraneSpec_take(btrc_Vector_MembraneSpec* self, int n) {
    if (n > self->len) {
        (n = self->len);
    }
    if (n < 0) {
        (n = 0);
    }
    return btrc_Vector_MembraneSpec_slice(self, 0, n);
}

static btrc_Vector_MembraneSpec* btrc_Vector_MembraneSpec_drop(btrc_Vector_MembraneSpec* self, int n) {
    if (n > self->len) {
        (n = self->len);
    }
    if (n < 0) {
        (n = 0);
    }
    return btrc_Vector_MembraneSpec_slice(self, n, self->len);
}

static void btrc_Vector_MembraneSpec_extend(btrc_Vector_MembraneSpec* self, btrc_Vector_MembraneSpec* other) {
    for (int i = 0; (i < other->len); (i++)) {
        btrc_Vector_MembraneSpec_push(self, other->data[i]);
    }
}

static void btrc_Vector_MembraneSpec_insert(btrc_Vector_MembraneSpec* self, int idx, MembraneSpec* val) {
    if ((idx < 0) || (idx > self->len)) {
        fprintf(stderr, "Vector insert index out of bounds: %d (size %d)\n", idx, self->len);
        exit(1);
    }
    (val->__rc++);
    if (self->len >= self->cap) {
        (self->cap = ((self->cap == 0) ? 4 : (self->cap * 2)));
        (self->data = ((MembraneSpec**)__btrc_safe_realloc(self->data, (sizeof(MembraneSpec*) * self->cap))));
    }
    for (int i = self->len; (i > idx); (i--)) {
        (self->data[i] = self->data[(i - 1)]);
    }
    (self->data[idx] = val);
    (self->len++);
}

static bool btrc_Vector_MembraneSpec_contains(btrc_Vector_MembraneSpec* self, MembraneSpec* val) {
    for (int i = 0; (i < self->len); (i++)) {
        if (__btrc_eq(self->data[i], val)) {
            return true;
        }
    }
    return false;
}

static int btrc_Vector_MembraneSpec_indexOf(btrc_Vector_MembraneSpec* self, MembraneSpec* val) {
    for (int i = 0; (i < self->len); (i++)) {
        if (__btrc_eq(self->data[i], val)) {
            return i;
        }
    }
    return (-1);
}

static int btrc_Vector_MembraneSpec_lastIndexOf(btrc_Vector_MembraneSpec* self, MembraneSpec* val) {
    for (int i = (self->len - 1); (i >= 0); (i--)) {
        if (__btrc_eq(self->data[i], val)) {
            return i;
        }
    }
    return (-1);
}

static int btrc_Vector_MembraneSpec_count(btrc_Vector_MembraneSpec* self, MembraneSpec* val) {
    int c = 0;
    for (int i = 0; (i < self->len); (i++)) {
        if (__btrc_eq(self->data[i], val)) {
            (c++);
        }
    }
    return c;
}

static void btrc_Vector_MembraneSpec_removeAll(btrc_Vector_MembraneSpec* self, MembraneSpec* val) {
    int j = 0;
    for (int i = 0; (i < self->len); (i++)) {
        if (!__btrc_eq(self->data[i], val)) {
            (self->data[j] = self->data[i]);
            (j++);
        } else if (self->data[i]) {
            if ((--self->data[i]->__rc) <= 0) {
                MembraneSpec_destroy(self->data[i]);
            }
        }
    }
    (self->len = j);
}

static btrc_Vector_MembraneSpec* btrc_Vector_MembraneSpec_distinct(btrc_Vector_MembraneSpec* self) {
    btrc_Vector_MembraneSpec* result = btrc_Vector_MembraneSpec_new();
    for (int i = 0; (i < self->len); (i++)) {
        if (!btrc_Vector_MembraneSpec_contains(result, self->data[i])) {
            btrc_Vector_MembraneSpec_push(result, self->data[i]);
        }
    }
    return result;
}

static void btrc_Vector_MembraneSpec_sort(btrc_Vector_MembraneSpec* self) {
    for (int i = 1; (i < self->len); (i++)) {
        MembraneSpec* key = self->data[i];
        int j = (i - 1);
        while ((j >= 0) && __btrc_lt(key, self->data[j])) {
            (self->data[(j + 1)] = self->data[j]);
            (j = (j - 1));
        }
        (self->data[(j + 1)] = key);
    }
}

static btrc_Vector_MembraneSpec* btrc_Vector_MembraneSpec_sorted(btrc_Vector_MembraneSpec* self) {
    btrc_Vector_MembraneSpec* result = btrc_Vector_MembraneSpec_new();
    for (int i = 0; (i < self->len); (i++)) {
        btrc_Vector_MembraneSpec_push(result, self->data[i]);
    }
    btrc_Vector_MembraneSpec_sort(result);
    return result;
}

static MembraneSpec* btrc_Vector_MembraneSpec_min(btrc_Vector_MembraneSpec* self) {
    if (self->len <= 0) {
        fprintf(stderr, "Vector min on empty list\n");
        exit(1);
    }
    MembraneSpec* m = self->data[0];
    for (int i = 1; (i < self->len); (i++)) {
        if (__btrc_lt(self->data[i], m)) {
            (m = self->data[i]);
        }
    }
    return m;
}

static MembraneSpec* btrc_Vector_MembraneSpec_max(btrc_Vector_MembraneSpec* self) {
    if (self->len <= 0) {
        fprintf(stderr, "Vector max on empty list\n");
        exit(1);
    }
    MembraneSpec* m = self->data[0];
    for (int i = 1; (i < self->len); (i++)) {
        if (__btrc_gt(self->data[i], m)) {
            (m = self->data[i]);
        }
    }
    return m;
}

static btrc_Vector_MembraneSpec* btrc_Vector_MembraneSpec_filter(btrc_Vector_MembraneSpec* self, __btrc_fn_bool_MembraneSpec pred) {
    btrc_Vector_MembraneSpec* result = btrc_Vector_MembraneSpec_new();
    for (int i = 0; (i < self->len); (i++)) {
        if (pred(self->data[i])) {
            btrc_Vector_MembraneSpec_push(result, self->data[i]);
        }
    }
    return result;
}

static int btrc_Vector_MembraneSpec_findIndex(btrc_Vector_MembraneSpec* self, __btrc_fn_bool_MembraneSpec pred) {
    for (int i = 0; (i < self->len); (i++)) {
        if (pred(self->data[i])) {
            return i;
        }
    }
    return (-1);
}

static void btrc_Vector_MembraneSpec_forEach(btrc_Vector_MembraneSpec* self, __btrc_fn_void_MembraneSpec fn) {
    for (int i = 0; (i < self->len); (i++)) {
        fn(self->data[i]);
    }
}

static btrc_Vector_MembraneSpec* btrc_Vector_MembraneSpec_map(btrc_Vector_MembraneSpec* self, __btrc_fn_MembraneSpec_MembraneSpec fn) {
    btrc_Vector_MembraneSpec* result = btrc_Vector_MembraneSpec_new();
    for (int i = 0; (i < self->len); (i++)) {
        btrc_Vector_MembraneSpec_push(result, fn(self->data[i]));
    }
    return result;
}

static bool btrc_Vector_MembraneSpec_any(btrc_Vector_MembraneSpec* self, __btrc_fn_bool_MembraneSpec pred) {
    for (int i = 0; (i < self->len); (i++)) {
        if (pred(self->data[i])) {
            return true;
        }
    }
    return false;
}

static bool btrc_Vector_MembraneSpec_all(btrc_Vector_MembraneSpec* self, __btrc_fn_bool_MembraneSpec pred) {
    for (int i = 0; (i < self->len); (i++)) {
        if (!pred(self->data[i])) {
            return false;
        }
    }
    return true;
}

static MembraneSpec* btrc_Vector_MembraneSpec_reduce(btrc_Vector_MembraneSpec* self, MembraneSpec* init, __btrc_fn_MembraneSpec_MembraneSpec_MembraneSpec fn) {
    MembraneSpec* acc = init;
    for (int i = 0; (i < self->len); (i++)) {
        (acc = fn(acc, self->data[i]));
    }
    return acc;
}

static btrc_Vector_MembraneSpec* btrc_Vector_MembraneSpec_copy(btrc_Vector_MembraneSpec* self) {
    btrc_Vector_MembraneSpec* result = btrc_Vector_MembraneSpec_new();
    for (int i = 0; (i < self->len); (i++)) {
        btrc_Vector_MembraneSpec_push(result, self->data[i]);
    }
    return result;
}

static void btrc_Vector_MembraneSpec_removeAt(btrc_Vector_MembraneSpec* self, int idx) {
    btrc_Vector_MembraneSpec_remove(self, idx);
}

static int btrc_Vector_MembraneSpec_iterLen(btrc_Vector_MembraneSpec* self) {
    return self->len;
}

static MembraneSpec* btrc_Vector_MembraneSpec_iterGet(btrc_Vector_MembraneSpec* self, int i) {
    return self->data[i];
}

static void btrc_Vector_MembranePlan_init(btrc_Vector_MembranePlan* self) {
    self->__rc = 1;
    (self->data = NULL);
    (self->len = 0);
    (self->cap = 0);
}

static btrc_Vector_MembranePlan* btrc_Vector_MembranePlan_new(void) {
    btrc_Vector_MembranePlan* self = ((btrc_Vector_MembranePlan*)malloc(sizeof(btrc_Vector_MembranePlan)));
    memset(self, 0, sizeof(btrc_Vector_MembranePlan));
    btrc_Vector_MembranePlan_init(self);
    return self;
}

static void btrc_Vector_MembranePlan_destroy(btrc_Vector_MembranePlan* self) {
    free(self);
}

static void btrc_Vector_MembranePlan_push(btrc_Vector_MembranePlan* self, MembranePlan* val) {
    (val->__rc++);
    if (self->len >= self->cap) {
        (self->cap = ((self->cap == 0) ? 4 : (self->cap * 2)));
        (self->data = ((MembranePlan**)__btrc_safe_realloc(self->data, (sizeof(MembranePlan*) * self->cap))));
    }
    (self->data[self->len] = val);
    (self->len++);
}

static MembranePlan* btrc_Vector_MembranePlan_pop(btrc_Vector_MembranePlan* self) {
    if (self->len <= 0) {
        fprintf(stderr, "Vector pop from empty list\n");
        exit(1);
    }
    (self->len--);
    return self->data[self->len];
}

static MembranePlan* btrc_Vector_MembranePlan_get(btrc_Vector_MembranePlan* self, int i) {
    if ((i < 0) || (i >= self->len)) {
        fprintf(stderr, "Vector index out of bounds: %d (len=%d)\n", i, self->len);
        exit(1);
    }
    return self->data[i];
}

static void btrc_Vector_MembranePlan_set(btrc_Vector_MembranePlan* self, int i, MembranePlan* val) {
    if ((i < 0) || (i >= self->len)) {
        fprintf(stderr, "Vector index out of bounds: %d (len=%d)\n", i, self->len);
        exit(1);
    }
    if (self->data[i]) {
        if ((--self->data[i]->__rc) <= 0) {
            MembranePlan_destroy(self->data[i]);
        }
    }
    (val->__rc++);
    (self->data[i] = val);
}

static void btrc_Vector_MembranePlan_free(btrc_Vector_MembranePlan* self) {
    for (int i = 0; (i < self->len); (i++)) {
        if (self->data[i]) {
            if ((--self->data[i]->__rc) <= 0) {
                MembranePlan_destroy(self->data[i]);
            }
        }
    }
    free(self->data);
    (self->data = NULL);
    (self->len = 0);
    (self->cap = 0);
}

static void btrc_Vector_MembranePlan_remove(btrc_Vector_MembranePlan* self, int idx) {
    if ((idx < 0) || (idx >= self->len)) {
        fprintf(stderr, "Vector remove index out of bounds: %d (len=%d)\n", idx, self->len);
        exit(1);
    }
    if (self->data[idx]) {
        if ((--self->data[idx]->__rc) <= 0) {
            MembranePlan_destroy(self->data[idx]);
        }
    }
    for (int i = idx; (i < (self->len - 1)); (i++)) {
        (self->data[i] = self->data[(i + 1)]);
    }
    (self->len--);
}

static void btrc_Vector_MembranePlan_reverse(btrc_Vector_MembranePlan* self) {
    for (int i = 0; (i < (self->len / 2)); (i++)) {
        MembranePlan* tmp = self->data[i];
        (self->data[i] = self->data[((self->len - 1) - i)]);
        (self->data[((self->len - 1) - i)] = tmp);
    }
}

static btrc_Vector_MembranePlan* btrc_Vector_MembranePlan_reversed(btrc_Vector_MembranePlan* self) {
    btrc_Vector_MembranePlan* result = btrc_Vector_MembranePlan_new();
    for (int i = (self->len - 1); (i >= 0); (i--)) {
        btrc_Vector_MembranePlan_push(result, self->data[i]);
    }
    return result;
}

static void btrc_Vector_MembranePlan_swap(btrc_Vector_MembranePlan* self, int i, int j) {
    if ((((i < 0) || (i >= self->len)) || (j < 0)) || (j >= self->len)) {
        fprintf(stderr, "Vector swap index out of bounds\n");
        exit(1);
    }
    MembranePlan* tmp = self->data[i];
    (self->data[i] = self->data[j]);
    (self->data[j] = tmp);
}

static void btrc_Vector_MembranePlan_clear(btrc_Vector_MembranePlan* self) {
    for (int i = 0; (i < self->len); (i++)) {
        if (self->data[i]) {
            if ((--self->data[i]->__rc) <= 0) {
                MembranePlan_destroy(self->data[i]);
            }
        }
    }
    (self->len = 0);
}

static void btrc_Vector_MembranePlan_fill(btrc_Vector_MembranePlan* self, MembranePlan* val) {
    for (int i = 0; (i < self->len); (i++)) {
        if (self->data[i]) {
            if ((--self->data[i]->__rc) <= 0) {
                MembranePlan_destroy(self->data[i]);
            }
        }
        (val->__rc++);
        (self->data[i] = val);
    }
}

static int btrc_Vector_MembranePlan_size(btrc_Vector_MembranePlan* self) {
    return self->len;
}

static bool btrc_Vector_MembranePlan_isEmpty(btrc_Vector_MembranePlan* self) {
    return (self->len == 0);
}

static MembranePlan* btrc_Vector_MembranePlan_first(btrc_Vector_MembranePlan* self) {
    if (self->len == 0) {
        fprintf(stderr, "Vector.first() called on empty list\n");
        exit(1);
    }
    return self->data[0];
}

static MembranePlan* btrc_Vector_MembranePlan_last(btrc_Vector_MembranePlan* self) {
    if (self->len == 0) {
        fprintf(stderr, "Vector.last() called on empty list\n");
        exit(1);
    }
    return self->data[(self->len - 1)];
}

static btrc_Vector_MembranePlan* btrc_Vector_MembranePlan_slice(btrc_Vector_MembranePlan* self, int start, int end) {
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
    btrc_Vector_MembranePlan* result = btrc_Vector_MembranePlan_new();
    for (int i = start; (i < end); (i++)) {
        btrc_Vector_MembranePlan_push(result, self->data[i]);
    }
    return result;
}

static btrc_Vector_MembranePlan* btrc_Vector_MembranePlan_take(btrc_Vector_MembranePlan* self, int n) {
    if (n > self->len) {
        (n = self->len);
    }
    if (n < 0) {
        (n = 0);
    }
    return btrc_Vector_MembranePlan_slice(self, 0, n);
}

static btrc_Vector_MembranePlan* btrc_Vector_MembranePlan_drop(btrc_Vector_MembranePlan* self, int n) {
    if (n > self->len) {
        (n = self->len);
    }
    if (n < 0) {
        (n = 0);
    }
    return btrc_Vector_MembranePlan_slice(self, n, self->len);
}

static void btrc_Vector_MembranePlan_extend(btrc_Vector_MembranePlan* self, btrc_Vector_MembranePlan* other) {
    for (int i = 0; (i < other->len); (i++)) {
        btrc_Vector_MembranePlan_push(self, other->data[i]);
    }
}

static void btrc_Vector_MembranePlan_insert(btrc_Vector_MembranePlan* self, int idx, MembranePlan* val) {
    if ((idx < 0) || (idx > self->len)) {
        fprintf(stderr, "Vector insert index out of bounds: %d (size %d)\n", idx, self->len);
        exit(1);
    }
    (val->__rc++);
    if (self->len >= self->cap) {
        (self->cap = ((self->cap == 0) ? 4 : (self->cap * 2)));
        (self->data = ((MembranePlan**)__btrc_safe_realloc(self->data, (sizeof(MembranePlan*) * self->cap))));
    }
    for (int i = self->len; (i > idx); (i--)) {
        (self->data[i] = self->data[(i - 1)]);
    }
    (self->data[idx] = val);
    (self->len++);
}

static bool btrc_Vector_MembranePlan_contains(btrc_Vector_MembranePlan* self, MembranePlan* val) {
    for (int i = 0; (i < self->len); (i++)) {
        if (__btrc_eq(self->data[i], val)) {
            return true;
        }
    }
    return false;
}

static int btrc_Vector_MembranePlan_indexOf(btrc_Vector_MembranePlan* self, MembranePlan* val) {
    for (int i = 0; (i < self->len); (i++)) {
        if (__btrc_eq(self->data[i], val)) {
            return i;
        }
    }
    return (-1);
}

static int btrc_Vector_MembranePlan_lastIndexOf(btrc_Vector_MembranePlan* self, MembranePlan* val) {
    for (int i = (self->len - 1); (i >= 0); (i--)) {
        if (__btrc_eq(self->data[i], val)) {
            return i;
        }
    }
    return (-1);
}

static int btrc_Vector_MembranePlan_count(btrc_Vector_MembranePlan* self, MembranePlan* val) {
    int c = 0;
    for (int i = 0; (i < self->len); (i++)) {
        if (__btrc_eq(self->data[i], val)) {
            (c++);
        }
    }
    return c;
}

static void btrc_Vector_MembranePlan_removeAll(btrc_Vector_MembranePlan* self, MembranePlan* val) {
    int j = 0;
    for (int i = 0; (i < self->len); (i++)) {
        if (!__btrc_eq(self->data[i], val)) {
            (self->data[j] = self->data[i]);
            (j++);
        } else if (self->data[i]) {
            if ((--self->data[i]->__rc) <= 0) {
                MembranePlan_destroy(self->data[i]);
            }
        }
    }
    (self->len = j);
}

static btrc_Vector_MembranePlan* btrc_Vector_MembranePlan_distinct(btrc_Vector_MembranePlan* self) {
    btrc_Vector_MembranePlan* result = btrc_Vector_MembranePlan_new();
    for (int i = 0; (i < self->len); (i++)) {
        if (!btrc_Vector_MembranePlan_contains(result, self->data[i])) {
            btrc_Vector_MembranePlan_push(result, self->data[i]);
        }
    }
    return result;
}

static void btrc_Vector_MembranePlan_sort(btrc_Vector_MembranePlan* self) {
    for (int i = 1; (i < self->len); (i++)) {
        MembranePlan* key = self->data[i];
        int j = (i - 1);
        while ((j >= 0) && __btrc_lt(key, self->data[j])) {
            (self->data[(j + 1)] = self->data[j]);
            (j = (j - 1));
        }
        (self->data[(j + 1)] = key);
    }
}

static btrc_Vector_MembranePlan* btrc_Vector_MembranePlan_sorted(btrc_Vector_MembranePlan* self) {
    btrc_Vector_MembranePlan* result = btrc_Vector_MembranePlan_new();
    for (int i = 0; (i < self->len); (i++)) {
        btrc_Vector_MembranePlan_push(result, self->data[i]);
    }
    btrc_Vector_MembranePlan_sort(result);
    return result;
}

static MembranePlan* btrc_Vector_MembranePlan_min(btrc_Vector_MembranePlan* self) {
    if (self->len <= 0) {
        fprintf(stderr, "Vector min on empty list\n");
        exit(1);
    }
    MembranePlan* m = self->data[0];
    for (int i = 1; (i < self->len); (i++)) {
        if (__btrc_lt(self->data[i], m)) {
            (m = self->data[i]);
        }
    }
    return m;
}

static MembranePlan* btrc_Vector_MembranePlan_max(btrc_Vector_MembranePlan* self) {
    if (self->len <= 0) {
        fprintf(stderr, "Vector max on empty list\n");
        exit(1);
    }
    MembranePlan* m = self->data[0];
    for (int i = 1; (i < self->len); (i++)) {
        if (__btrc_gt(self->data[i], m)) {
            (m = self->data[i]);
        }
    }
    return m;
}

static btrc_Vector_MembranePlan* btrc_Vector_MembranePlan_filter(btrc_Vector_MembranePlan* self, __btrc_fn_bool_MembranePlan pred) {
    btrc_Vector_MembranePlan* result = btrc_Vector_MembranePlan_new();
    for (int i = 0; (i < self->len); (i++)) {
        if (pred(self->data[i])) {
            btrc_Vector_MembranePlan_push(result, self->data[i]);
        }
    }
    return result;
}

static int btrc_Vector_MembranePlan_findIndex(btrc_Vector_MembranePlan* self, __btrc_fn_bool_MembranePlan pred) {
    for (int i = 0; (i < self->len); (i++)) {
        if (pred(self->data[i])) {
            return i;
        }
    }
    return (-1);
}

static void btrc_Vector_MembranePlan_forEach(btrc_Vector_MembranePlan* self, __btrc_fn_void_MembranePlan fn) {
    for (int i = 0; (i < self->len); (i++)) {
        fn(self->data[i]);
    }
}

static btrc_Vector_MembranePlan* btrc_Vector_MembranePlan_map(btrc_Vector_MembranePlan* self, __btrc_fn_MembranePlan_MembranePlan fn) {
    btrc_Vector_MembranePlan* result = btrc_Vector_MembranePlan_new();
    for (int i = 0; (i < self->len); (i++)) {
        btrc_Vector_MembranePlan_push(result, fn(self->data[i]));
    }
    return result;
}

static bool btrc_Vector_MembranePlan_any(btrc_Vector_MembranePlan* self, __btrc_fn_bool_MembranePlan pred) {
    for (int i = 0; (i < self->len); (i++)) {
        if (pred(self->data[i])) {
            return true;
        }
    }
    return false;
}

static bool btrc_Vector_MembranePlan_all(btrc_Vector_MembranePlan* self, __btrc_fn_bool_MembranePlan pred) {
    for (int i = 0; (i < self->len); (i++)) {
        if (!pred(self->data[i])) {
            return false;
        }
    }
    return true;
}

static MembranePlan* btrc_Vector_MembranePlan_reduce(btrc_Vector_MembranePlan* self, MembranePlan* init, __btrc_fn_MembranePlan_MembranePlan_MembranePlan fn) {
    MembranePlan* acc = init;
    for (int i = 0; (i < self->len); (i++)) {
        (acc = fn(acc, self->data[i]));
    }
    return acc;
}

static btrc_Vector_MembranePlan* btrc_Vector_MembranePlan_copy(btrc_Vector_MembranePlan* self) {
    btrc_Vector_MembranePlan* result = btrc_Vector_MembranePlan_new();
    for (int i = 0; (i < self->len); (i++)) {
        btrc_Vector_MembranePlan_push(result, self->data[i]);
    }
    return result;
}

static void btrc_Vector_MembranePlan_removeAt(btrc_Vector_MembranePlan* self, int idx) {
    btrc_Vector_MembranePlan_remove(self, idx);
}

static int btrc_Vector_MembranePlan_iterLen(btrc_Vector_MembranePlan* self) {
    return self->len;
}

static MembranePlan* btrc_Vector_MembranePlan_iterGet(btrc_Vector_MembranePlan* self, int i) {
    return self->data[i];
}

static void btrc_Vector_MembraneVolume_init(btrc_Vector_MembraneVolume* self) {
    self->__rc = 1;
    (self->data = NULL);
    (self->len = 0);
    (self->cap = 0);
}

static btrc_Vector_MembraneVolume* btrc_Vector_MembraneVolume_new(void) {
    btrc_Vector_MembraneVolume* self = ((btrc_Vector_MembraneVolume*)malloc(sizeof(btrc_Vector_MembraneVolume)));
    memset(self, 0, sizeof(btrc_Vector_MembraneVolume));
    btrc_Vector_MembraneVolume_init(self);
    return self;
}

static void btrc_Vector_MembraneVolume_destroy(btrc_Vector_MembraneVolume* self) {
    free(self);
}

static void btrc_Vector_MembraneVolume_push(btrc_Vector_MembraneVolume* self, MembraneVolume* val) {
    (val->__rc++);
    if (self->len >= self->cap) {
        (self->cap = ((self->cap == 0) ? 4 : (self->cap * 2)));
        (self->data = ((MembraneVolume**)__btrc_safe_realloc(self->data, (sizeof(MembraneVolume*) * self->cap))));
    }
    (self->data[self->len] = val);
    (self->len++);
}

static MembraneVolume* btrc_Vector_MembraneVolume_pop(btrc_Vector_MembraneVolume* self) {
    if (self->len <= 0) {
        fprintf(stderr, "Vector pop from empty list\n");
        exit(1);
    }
    (self->len--);
    return self->data[self->len];
}

static MembraneVolume* btrc_Vector_MembraneVolume_get(btrc_Vector_MembraneVolume* self, int i) {
    if ((i < 0) || (i >= self->len)) {
        fprintf(stderr, "Vector index out of bounds: %d (len=%d)\n", i, self->len);
        exit(1);
    }
    return self->data[i];
}

static void btrc_Vector_MembraneVolume_set(btrc_Vector_MembraneVolume* self, int i, MembraneVolume* val) {
    if ((i < 0) || (i >= self->len)) {
        fprintf(stderr, "Vector index out of bounds: %d (len=%d)\n", i, self->len);
        exit(1);
    }
    if (self->data[i]) {
        if ((--self->data[i]->__rc) <= 0) {
            MembraneVolume_destroy(self->data[i]);
        }
    }
    (val->__rc++);
    (self->data[i] = val);
}

static void btrc_Vector_MembraneVolume_free(btrc_Vector_MembraneVolume* self) {
    for (int i = 0; (i < self->len); (i++)) {
        if (self->data[i]) {
            if ((--self->data[i]->__rc) <= 0) {
                MembraneVolume_destroy(self->data[i]);
            }
        }
    }
    free(self->data);
    (self->data = NULL);
    (self->len = 0);
    (self->cap = 0);
}

static void btrc_Vector_MembraneVolume_remove(btrc_Vector_MembraneVolume* self, int idx) {
    if ((idx < 0) || (idx >= self->len)) {
        fprintf(stderr, "Vector remove index out of bounds: %d (len=%d)\n", idx, self->len);
        exit(1);
    }
    if (self->data[idx]) {
        if ((--self->data[idx]->__rc) <= 0) {
            MembraneVolume_destroy(self->data[idx]);
        }
    }
    for (int i = idx; (i < (self->len - 1)); (i++)) {
        (self->data[i] = self->data[(i + 1)]);
    }
    (self->len--);
}

static void btrc_Vector_MembraneVolume_reverse(btrc_Vector_MembraneVolume* self) {
    for (int i = 0; (i < (self->len / 2)); (i++)) {
        MembraneVolume* tmp = self->data[i];
        (self->data[i] = self->data[((self->len - 1) - i)]);
        (self->data[((self->len - 1) - i)] = tmp);
    }
}

static btrc_Vector_MembraneVolume* btrc_Vector_MembraneVolume_reversed(btrc_Vector_MembraneVolume* self) {
    btrc_Vector_MembraneVolume* result = btrc_Vector_MembraneVolume_new();
    for (int i = (self->len - 1); (i >= 0); (i--)) {
        btrc_Vector_MembraneVolume_push(result, self->data[i]);
    }
    return result;
}

static void btrc_Vector_MembraneVolume_swap(btrc_Vector_MembraneVolume* self, int i, int j) {
    if ((((i < 0) || (i >= self->len)) || (j < 0)) || (j >= self->len)) {
        fprintf(stderr, "Vector swap index out of bounds\n");
        exit(1);
    }
    MembraneVolume* tmp = self->data[i];
    (self->data[i] = self->data[j]);
    (self->data[j] = tmp);
}

static void btrc_Vector_MembraneVolume_clear(btrc_Vector_MembraneVolume* self) {
    for (int i = 0; (i < self->len); (i++)) {
        if (self->data[i]) {
            if ((--self->data[i]->__rc) <= 0) {
                MembraneVolume_destroy(self->data[i]);
            }
        }
    }
    (self->len = 0);
}

static void btrc_Vector_MembraneVolume_fill(btrc_Vector_MembraneVolume* self, MembraneVolume* val) {
    for (int i = 0; (i < self->len); (i++)) {
        if (self->data[i]) {
            if ((--self->data[i]->__rc) <= 0) {
                MembraneVolume_destroy(self->data[i]);
            }
        }
        (val->__rc++);
        (self->data[i] = val);
    }
}

static int btrc_Vector_MembraneVolume_size(btrc_Vector_MembraneVolume* self) {
    return self->len;
}

static bool btrc_Vector_MembraneVolume_isEmpty(btrc_Vector_MembraneVolume* self) {
    return (self->len == 0);
}

static MembraneVolume* btrc_Vector_MembraneVolume_first(btrc_Vector_MembraneVolume* self) {
    if (self->len == 0) {
        fprintf(stderr, "Vector.first() called on empty list\n");
        exit(1);
    }
    return self->data[0];
}

static MembraneVolume* btrc_Vector_MembraneVolume_last(btrc_Vector_MembraneVolume* self) {
    if (self->len == 0) {
        fprintf(stderr, "Vector.last() called on empty list\n");
        exit(1);
    }
    return self->data[(self->len - 1)];
}

static btrc_Vector_MembraneVolume* btrc_Vector_MembraneVolume_slice(btrc_Vector_MembraneVolume* self, int start, int end) {
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
    btrc_Vector_MembraneVolume* result = btrc_Vector_MembraneVolume_new();
    for (int i = start; (i < end); (i++)) {
        btrc_Vector_MembraneVolume_push(result, self->data[i]);
    }
    return result;
}

static btrc_Vector_MembraneVolume* btrc_Vector_MembraneVolume_take(btrc_Vector_MembraneVolume* self, int n) {
    if (n > self->len) {
        (n = self->len);
    }
    if (n < 0) {
        (n = 0);
    }
    return btrc_Vector_MembraneVolume_slice(self, 0, n);
}

static btrc_Vector_MembraneVolume* btrc_Vector_MembraneVolume_drop(btrc_Vector_MembraneVolume* self, int n) {
    if (n > self->len) {
        (n = self->len);
    }
    if (n < 0) {
        (n = 0);
    }
    return btrc_Vector_MembraneVolume_slice(self, n, self->len);
}

static void btrc_Vector_MembraneVolume_extend(btrc_Vector_MembraneVolume* self, btrc_Vector_MembraneVolume* other) {
    for (int i = 0; (i < other->len); (i++)) {
        btrc_Vector_MembraneVolume_push(self, other->data[i]);
    }
}

static void btrc_Vector_MembraneVolume_insert(btrc_Vector_MembraneVolume* self, int idx, MembraneVolume* val) {
    if ((idx < 0) || (idx > self->len)) {
        fprintf(stderr, "Vector insert index out of bounds: %d (size %d)\n", idx, self->len);
        exit(1);
    }
    (val->__rc++);
    if (self->len >= self->cap) {
        (self->cap = ((self->cap == 0) ? 4 : (self->cap * 2)));
        (self->data = ((MembraneVolume**)__btrc_safe_realloc(self->data, (sizeof(MembraneVolume*) * self->cap))));
    }
    for (int i = self->len; (i > idx); (i--)) {
        (self->data[i] = self->data[(i - 1)]);
    }
    (self->data[idx] = val);
    (self->len++);
}

static bool btrc_Vector_MembraneVolume_contains(btrc_Vector_MembraneVolume* self, MembraneVolume* val) {
    for (int i = 0; (i < self->len); (i++)) {
        if (__btrc_eq(self->data[i], val)) {
            return true;
        }
    }
    return false;
}

static int btrc_Vector_MembraneVolume_indexOf(btrc_Vector_MembraneVolume* self, MembraneVolume* val) {
    for (int i = 0; (i < self->len); (i++)) {
        if (__btrc_eq(self->data[i], val)) {
            return i;
        }
    }
    return (-1);
}

static int btrc_Vector_MembraneVolume_lastIndexOf(btrc_Vector_MembraneVolume* self, MembraneVolume* val) {
    for (int i = (self->len - 1); (i >= 0); (i--)) {
        if (__btrc_eq(self->data[i], val)) {
            return i;
        }
    }
    return (-1);
}

static int btrc_Vector_MembraneVolume_count(btrc_Vector_MembraneVolume* self, MembraneVolume* val) {
    int c = 0;
    for (int i = 0; (i < self->len); (i++)) {
        if (__btrc_eq(self->data[i], val)) {
            (c++);
        }
    }
    return c;
}

static void btrc_Vector_MembraneVolume_removeAll(btrc_Vector_MembraneVolume* self, MembraneVolume* val) {
    int j = 0;
    for (int i = 0; (i < self->len); (i++)) {
        if (!__btrc_eq(self->data[i], val)) {
            (self->data[j] = self->data[i]);
            (j++);
        } else if (self->data[i]) {
            if ((--self->data[i]->__rc) <= 0) {
                MembraneVolume_destroy(self->data[i]);
            }
        }
    }
    (self->len = j);
}

static btrc_Vector_MembraneVolume* btrc_Vector_MembraneVolume_distinct(btrc_Vector_MembraneVolume* self) {
    btrc_Vector_MembraneVolume* result = btrc_Vector_MembraneVolume_new();
    for (int i = 0; (i < self->len); (i++)) {
        if (!btrc_Vector_MembraneVolume_contains(result, self->data[i])) {
            btrc_Vector_MembraneVolume_push(result, self->data[i]);
        }
    }
    return result;
}

static void btrc_Vector_MembraneVolume_sort(btrc_Vector_MembraneVolume* self) {
    for (int i = 1; (i < self->len); (i++)) {
        MembraneVolume* key = self->data[i];
        int j = (i - 1);
        while ((j >= 0) && __btrc_lt(key, self->data[j])) {
            (self->data[(j + 1)] = self->data[j]);
            (j = (j - 1));
        }
        (self->data[(j + 1)] = key);
    }
}

static btrc_Vector_MembraneVolume* btrc_Vector_MembraneVolume_sorted(btrc_Vector_MembraneVolume* self) {
    btrc_Vector_MembraneVolume* result = btrc_Vector_MembraneVolume_new();
    for (int i = 0; (i < self->len); (i++)) {
        btrc_Vector_MembraneVolume_push(result, self->data[i]);
    }
    btrc_Vector_MembraneVolume_sort(result);
    return result;
}

static MembraneVolume* btrc_Vector_MembraneVolume_min(btrc_Vector_MembraneVolume* self) {
    if (self->len <= 0) {
        fprintf(stderr, "Vector min on empty list\n");
        exit(1);
    }
    MembraneVolume* m = self->data[0];
    for (int i = 1; (i < self->len); (i++)) {
        if (__btrc_lt(self->data[i], m)) {
            (m = self->data[i]);
        }
    }
    return m;
}

static MembraneVolume* btrc_Vector_MembraneVolume_max(btrc_Vector_MembraneVolume* self) {
    if (self->len <= 0) {
        fprintf(stderr, "Vector max on empty list\n");
        exit(1);
    }
    MembraneVolume* m = self->data[0];
    for (int i = 1; (i < self->len); (i++)) {
        if (__btrc_gt(self->data[i], m)) {
            (m = self->data[i]);
        }
    }
    return m;
}

static btrc_Vector_MembraneVolume* btrc_Vector_MembraneVolume_filter(btrc_Vector_MembraneVolume* self, __btrc_fn_bool_MembraneVolume pred) {
    btrc_Vector_MembraneVolume* result = btrc_Vector_MembraneVolume_new();
    for (int i = 0; (i < self->len); (i++)) {
        if (pred(self->data[i])) {
            btrc_Vector_MembraneVolume_push(result, self->data[i]);
        }
    }
    return result;
}

static int btrc_Vector_MembraneVolume_findIndex(btrc_Vector_MembraneVolume* self, __btrc_fn_bool_MembraneVolume pred) {
    for (int i = 0; (i < self->len); (i++)) {
        if (pred(self->data[i])) {
            return i;
        }
    }
    return (-1);
}

static void btrc_Vector_MembraneVolume_forEach(btrc_Vector_MembraneVolume* self, __btrc_fn_void_MembraneVolume fn) {
    for (int i = 0; (i < self->len); (i++)) {
        fn(self->data[i]);
    }
}

static btrc_Vector_MembraneVolume* btrc_Vector_MembraneVolume_map(btrc_Vector_MembraneVolume* self, __btrc_fn_MembraneVolume_MembraneVolume fn) {
    btrc_Vector_MembraneVolume* result = btrc_Vector_MembraneVolume_new();
    for (int i = 0; (i < self->len); (i++)) {
        btrc_Vector_MembraneVolume_push(result, fn(self->data[i]));
    }
    return result;
}

static bool btrc_Vector_MembraneVolume_any(btrc_Vector_MembraneVolume* self, __btrc_fn_bool_MembraneVolume pred) {
    for (int i = 0; (i < self->len); (i++)) {
        if (pred(self->data[i])) {
            return true;
        }
    }
    return false;
}

static bool btrc_Vector_MembraneVolume_all(btrc_Vector_MembraneVolume* self, __btrc_fn_bool_MembraneVolume pred) {
    for (int i = 0; (i < self->len); (i++)) {
        if (!pred(self->data[i])) {
            return false;
        }
    }
    return true;
}

static MembraneVolume* btrc_Vector_MembraneVolume_reduce(btrc_Vector_MembraneVolume* self, MembraneVolume* init, __btrc_fn_MembraneVolume_MembraneVolume_MembraneVolume fn) {
    MembraneVolume* acc = init;
    for (int i = 0; (i < self->len); (i++)) {
        (acc = fn(acc, self->data[i]));
    }
    return acc;
}

static btrc_Vector_MembraneVolume* btrc_Vector_MembraneVolume_copy(btrc_Vector_MembraneVolume* self) {
    btrc_Vector_MembraneVolume* result = btrc_Vector_MembraneVolume_new();
    for (int i = 0; (i < self->len); (i++)) {
        btrc_Vector_MembraneVolume_push(result, self->data[i]);
    }
    return result;
}

static void btrc_Vector_MembraneVolume_removeAt(btrc_Vector_MembraneVolume* self, int idx) {
    btrc_Vector_MembraneVolume_remove(self, idx);
}

static int btrc_Vector_MembraneVolume_iterLen(btrc_Vector_MembraneVolume* self) {
    return self->len;
}

static MembraneVolume* btrc_Vector_MembraneVolume_iterGet(btrc_Vector_MembraneVolume* self, int i) {
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

char* Strings_fromInt(int n) {
    char* buf = ((char*)malloc(32));
    snprintf(buf, 32, "%d", n);
    return buf;
}

char* Strings_fromFloat(float f) {
    char* buf = ((char*)malloc(64));
    snprintf(buf, 64, "%g", f);
    return buf;
}

void Console_init(Console* self) {
    self->__rc = 1;
}

void Console_destroy(Console* self) {
    free(self);
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

Path* Path_new(void) {
    Path* self = ((Path*)malloc(sizeof(Path)));
    memset(self, 0, sizeof(Path));
    Path_init(self);
    return self;
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

bool ExecResult_ok(ExecResult* self) {
    __auto_type __btrc_ret_81 = (self->code == 0);
    return __btrc_ret_81;
}

char* ExecResult_stdout(ExecResult* self) {
    __auto_type __btrc_ret_82 = self->out;
    return __btrc_ret_82;
}

char* ExecResult_trimmed(ExecResult* self) {
    __auto_type __btrc_ret_83 = __btrc_str_track(__btrc_trim(self->out));
    return __btrc_ret_83;
}

void Command_init(Command* self, char* executable) {
    self->__rc = 1;
    (self->executable = executable);
    if (self->args != NULL) {
        if ((--self->args->__rc) <= 0) {
            btrc_Vector_string_free(self->args);
        }
    }
    btrc_Vector_string* __list_86 = btrc_Vector_string_new();
    (self->args = __list_86);
    btrc_Vector_string* __list_85 = btrc_Vector_string_new();
    (__list_85->__rc++);
    if (self->env != NULL) {
        if ((--self->env->__rc) <= 0) {
            btrc_Vector_string_free(self->env);
        }
    }
    btrc_Vector_string* __list_88 = btrc_Vector_string_new();
    (self->env = __list_88);
    btrc_Vector_string* __list_87 = btrc_Vector_string_new();
    (__list_87->__rc++);
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
    __auto_type __btrc_ret_89 = self;
    return __btrc_ret_89;
}

Command* Command_capture(Command* self, bool enabled) {
    (self->captureOutput = enabled);
    __auto_type __btrc_ret_95 = self;
    return __btrc_ret_95;
}

Command* Command_check(Command* self, bool enabled) {
    (self->checkStatus = enabled);
    __auto_type __btrc_ret_96 = self;
    return __btrc_ret_96;
}

char* Command_renderEnv(Command* self, char* item) {
    int split = Strings_find(item, "=", 0);
    if (split <= 0) {
        __auto_type __btrc_ret_99 = ShellWords_quote(item);
        return __btrc_ret_99;
    }
    char* name = __btrc_str_track(__btrc_substring(item, 0, split));
    char* value = __btrc_str_track(__btrc_substring(item, (split + 1), ((((int)strlen(item)) - split) - 1)));
    __auto_type __btrc_ret_100 = __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(name, "=")), ShellWords_quote(value)));
    return __btrc_ret_100;
}

char* Command_render(Command* self) {
    btrc_Vector_string* parts = btrc_Vector_string_new();
    int __n_102 = btrc_Vector_string_iterLen(self->env);
    for (int __i_101 = 0; (__i_101 < __n_102); (__i_101++)) {
        char* item = btrc_Vector_string_iterGet(self->env, __i_101);
        btrc_Vector_string_push(parts, Command_renderEnv(self, item));
    }
    if (self->useSudo) {
        btrc_Vector_string_push(parts, "sudo");
    }
    btrc_Vector_string_push(parts, ShellWords_quote(self->executable));
    int __n_104 = btrc_Vector_string_iterLen(self->args);
    for (int __i_103 = 0; (__i_103 < __n_104); (__i_103++)) {
        char* item = btrc_Vector_string_iterGet(self->args, __i_103);
        btrc_Vector_string_push(parts, ShellWords_quote(item));
    }
    if (self->mergeStderr) {
        btrc_Vector_string_push(parts, "2>&1");
    }
    __auto_type __btrc_ret_105 = btrc_Vector_string_join(parts, " ");
    return __btrc_ret_105;
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
    __auto_type __btrc_ret_106 = ShellWords_quote(raw);
    return __btrc_ret_106;
}

char* UnixShell_redactText(char* text, char* sensitive) {
    __auto_type __btrc_ret_107 = ShellWords_redact(text, sensitive);
    return __btrc_ret_107;
}

ExecResult* UnixShell_run(UnixShell* self, char* command) {
    __auto_type __btrc_ret_108 = UnixShell_runRaw(self, command, true, true, "");
    return __btrc_ret_108;
}

ExecResult* UnixShell_runCommand(UnixShell* self, Command* command) {
    __auto_type __btrc_ret_110 = UnixShell_runRaw(self, Command_render(command), command->captureOutput, command->checkStatus, command->sensitive);
    return __btrc_ret_110;
}

ExecResult* UnixShell_runRaw(UnixShell* self, char* command, bool captureOutput, bool checkStatus, char* sensitive) {
    char* rendered = command;
    if (((int)strlen(self->chrootPath)) > 0) {
        int __fstr_112_len = snprintf(NULL, 0, "nixos-enter --root %s --command %s", ShellWords_quote(self->chrootPath), ShellWords_quote(command));
        char* __fstr_112_buf = __btrc_str_track(((char*)malloc((__fstr_112_len + 1))));
        snprintf(__fstr_112_buf, (__fstr_112_len + 1), "nixos-enter --root %s --command %s", ShellWords_quote(self->chrootPath), ShellWords_quote(command));
        (rendered = __fstr_112_buf);
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
        __auto_type __btrc_ret_113 = ExecResult_new(code, "", "", rendered);
        return __btrc_ret_113;
    }
    UnixPipe* pipe = UnixProcess_pipe(rendered);
    if (!UnixPipe_ok(pipe)) {
        __auto_type __btrc_ret_114 = ExecResult_new(127, "", "popen failed", rendered);
        return __btrc_ret_114;
    }
    char* output = UnixPipe_readAll(pipe);
    ProcessStatus* status = UnixPipe_close(pipe);
    int code = ProcessStatus_code(status);
    if (checkStatus && (code != 0)) {
        fprintf(stderr, "Command failed (%d): %s\n", code, UnixShell_redactText(rendered, sensitive));
    }
    __auto_type __btrc_ret_115 = ExecResult_new(code, output, "", rendered);
    return __btrc_ret_115;
}

void PowerShell_init(PowerShell* self) {
    self->__rc = 1;
}

void PowerShell_destroy(PowerShell* self) {
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
    __auto_type __btrc_ret_117 = self->found;
    return __btrc_ret_117;
}

bool FileStatus_isDir(FileStatus* self) {
    __auto_type __btrc_ret_118 = (self->found && S_ISDIR(self->mode));
    return __btrc_ret_118;
}

bool FileStatus_isFile(FileStatus* self) {
    __auto_type __btrc_ret_119 = (self->found && S_ISREG(self->mode));
    return __btrc_ret_119;
}

bool FileStatus_isSymlink(FileStatus* self) {
    __auto_type __btrc_ret_120 = (self->linkFound && S_ISLNK(self->linkMode));
    return __btrc_ret_120;
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
        __auto_type __btrc_ret_121 = 127;
        return __btrc_ret_121;
    }
    if (raw > 255) {
        __auto_type __btrc_ret_122 = __btrc_div_int(raw, 256);
        return __btrc_ret_122;
    }
    return raw;
}

int UnixFileSystem_chmodPath(char* path, int mode) {
    __auto_type __btrc_ret_123 = chmod(path, ((mode_t)mode));
    return __btrc_ret_123;
}

int UnixFileSystem_mkdirPath(char* path, int mode) {
    __auto_type __btrc_ret_124 = mkdir(path, ((mode_t)mode));
    return __btrc_ret_124;
}

int UnixFileSystem_runShell(char* command) {
    __auto_type __btrc_ret_125 = UnixFileSystem_statusCode(system(command));
    return __btrc_ret_125;
}

int UnixFileSystem_mkdirp(char* path) {
    char* quoted = PathTools_shellQuote(path);
    int __fstr_126_len = snprintf(NULL, 0, "mkdir -p %s", quoted);
    char* __fstr_126_buf = __btrc_str_track(((char*)malloc((__fstr_126_len + 1))));
    snprintf(__fstr_126_buf, (__fstr_126_len + 1), "mkdir -p %s", quoted);
    __auto_type __btrc_ret_127 = UnixFileSystem_runShell(__fstr_126_buf);
    return __btrc_ret_127;
}

int UnixFileSystem_removeRecursive(char* path) {
    char* quoted = PathTools_shellQuote(path);
    int __fstr_128_len = snprintf(NULL, 0, "rm -rf %s", quoted);
    char* __fstr_128_buf = __btrc_str_track(((char*)malloc((__fstr_128_len + 1))));
    snprintf(__fstr_128_buf, (__fstr_128_len + 1), "rm -rf %s", quoted);
    __auto_type __btrc_ret_129 = UnixFileSystem_runShell(__fstr_128_buf);
    return __btrc_ret_129;
}

int UnixFileSystem_symlinkPath(char* target, char* linkPath) {
    __auto_type __btrc_ret_130 = symlink(target, linkPath);
    return __btrc_ret_130;
}

char* UnixFileSystem_readLink(char* path) {
    char buffer[4096];
    ssize_t length = readlink(path, buffer, 4095);
    if (length < 0) {
        __auto_type __btrc_ret_131 = "";
        return __btrc_ret_131;
    }
    (buffer[length] = '\0');
    __auto_type __btrc_ret_132 = Strings_copy(buffer);
    return __btrc_ret_132;
}

char* UnixFileSystem_tempDir(char* prefix) {
    char* base = Environment_get("TMPDIR", "/tmp");
    char* templatePath = PathTools_join(base, __btrc_str_track(__btrc_strcat(prefix, ".XXXXXX")));
    char* raw = Strings_copy(templatePath);
    char* result = mkdtemp(raw);
    if (result == NULL) {
        __auto_type __btrc_ret_133 = "";
        return __btrc_ret_133;
    }
    __auto_type __btrc_ret_134 = Strings_copy(result);
    return __btrc_ret_134;
}

void PathTools_init(PathTools* self) {
    self->__rc = 1;
}

void PathTools_destroy(PathTools* self) {
    free(self);
}

char* PathTools_shellQuote(char* raw) {
    __auto_type __btrc_ret_135 = ShellWords_quote(raw);
    return __btrc_ret_135;
}

char* PathTools_basename(char* path) {
    int len = ((int)strlen(path));
    if (len == 0) {
        __auto_type __btrc_ret_136 = "";
        return __btrc_ret_136;
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
        __auto_type __btrc_ret_137 = ".";
        return __btrc_ret_137;
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
            __auto_type __btrc_ret_138 = "/";
            return __btrc_ret_138;
        }
        __auto_type __btrc_ret_139 = ".";
        return __btrc_ret_139;
    }
    char* result = ((char*)malloc((end + 1)));
    memcpy(result, path, end);
    (result[end] = '\0');
    return result;
}

char* PathTools_join(char* left, char* right) {
    if (((int)strlen(left)) == 0) {
        __auto_type __btrc_ret_140 = Strings_copy(right);
        return __btrc_ret_140;
    }
    if (((int)strlen(right)) == 0) {
        __auto_type __btrc_ret_141 = Strings_copy(left);
        return __btrc_ret_141;
    }
    if (left[(((int)strlen(left)) - 1)] == '/') {
        int __fstr_142_len = snprintf(NULL, 0, "%s%s", left, right);
        char* __fstr_142_buf = __btrc_str_track(((char*)malloc((__fstr_142_len + 1))));
        snprintf(__fstr_142_buf, (__fstr_142_len + 1), "%s%s", left, right);
        __auto_type __btrc_ret_143 = __fstr_142_buf;
        return __btrc_ret_143;
    }
    int __fstr_144_len = snprintf(NULL, 0, "%s/%s", left, right);
    char* __fstr_144_buf = __btrc_str_track(((char*)malloc((__fstr_144_len + 1))));
    snprintf(__fstr_144_buf, (__fstr_144_len + 1), "%s/%s", left, right);
    __auto_type __btrc_ret_145 = __fstr_144_buf;
    return __btrc_ret_145;
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

bool FileSystem_isDir(char* path) {
    FileStatus* status = FileStatus_new(path);
    bool result = FileStatus_isDir(status);
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
    __auto_type __btrc_ret_146 = UnixFileSystem_chmodPath(path, mode);
    return __btrc_ret_146;
}

int FileSystem_mkdir(char* path, int mode) {
    __auto_type __btrc_ret_147 = UnixFileSystem_mkdirPath(path, mode);
    return __btrc_ret_147;
}

int FileSystem_mkdirp(char* path) {
    __auto_type __btrc_ret_148 = UnixFileSystem_mkdirp(path);
    return __btrc_ret_148;
}

int FileSystem_removeRecursive(char* path) {
    __auto_type __btrc_ret_149 = UnixFileSystem_removeRecursive(path);
    return __btrc_ret_149;
}

int FileSystem_symlink(char* target, char* linkPath) {
    __auto_type __btrc_ret_150 = UnixFileSystem_symlinkPath(target, linkPath);
    return __btrc_ret_150;
}

char* FileSystem_readLink(char* path) {
    __auto_type __btrc_ret_151 = UnixFileSystem_readLink(path);
    return __btrc_ret_151;
}

char* FileSystem_tempDir(char* prefix) {
    __auto_type __btrc_ret_152 = UnixFileSystem_tempDir(prefix);
    return __btrc_ret_152;
}

char* FileSystem_readText(char* path) {
    __auto_type __btrc_ret_153 = Path_readAll(path);
    return __btrc_ret_153;
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
        __auto_type __btrc_ret_154 = "";
        return __btrc_ret_154;
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
    int __n_161 = btrc_Map_string_string_iterLen(self->values);
    for (int __i_160 = 0; (__i_160 < __n_161); (__i_160++)) {
        char* key = btrc_Map_string_string_iterGet(self->values, __i_160);
        char* value = btrc_Map_string_string_iterValueAt(self->values, __i_160);
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
    __auto_type __btrc_ret_162 = __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("{", btrc_Vector_string_join(fields, ","))), "}"));
    return __btrc_ret_162;
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

void UnixPattern_init(UnixPattern* self) {
    self->__rc = 1;
}

void UnixPattern_destroy(UnixPattern* self) {
    free(self);
}

bool UnixPattern_matches(char* pattern, char* text) {
    __auto_type __btrc_ret_164 = (fnmatch(pattern, text, 0) == 0);
    return __btrc_ret_164;
}

void Pattern_init(Pattern* self) {
    self->__rc = 1;
}

void Pattern_destroy(Pattern* self) {
    free(self);
}

bool Pattern_matches(char* pattern, char* text) {
    __auto_type __btrc_ret_165 = UnixPattern_matches(pattern, text);
    return __btrc_ret_165;
}

void Math_init(Math* self) {
    self->__rc = 1;
}

void Math_destroy(Math* self) {
    free(self);
}

int Math_abs(int x) {
    if (x < 0) {
        __auto_type __btrc_ret_174 = (-x);
        return __btrc_ret_174;
    }
    return x;
}

int Math_factorial(int n) {
    if (n <= 1) {
        __auto_type __btrc_ret_178 = 1;
        return __btrc_ret_178;
    }
    __auto_type __btrc_ret_179 = (n * Math_factorial((n - 1)));
    return __btrc_ret_179;
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

Timer* Timer_new(void) {
    Timer* self = ((Timer*)malloc(sizeof(Timer)));
    memset(self, 0, sizeof(Timer));
    Timer_init(self);
    return self;
}

void Timer_destroy(Timer* self) {
    free(self);
}

void Timer_start(Timer* self) {
    (self->start_time = clock());
    (self->running = true);
}

float Timer_elapsed(Timer* self) {
    clock_t end = (self->running ? clock() : self->end_time);
    __auto_type __btrc_ret_217 = __btrc_div_double(((float)(end - self->start_time)), ((float)CLOCKS_PER_SEC));
    return __btrc_ret_217;
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
    __auto_type __btrc_ret_218 = (lo + (rand() % ((hi - lo) + 1)));
    return __btrc_ret_218;
}

float Random_random(Random* self) {
    if (!self->seeded) {
        Random_seedTime(self);
    }
    __auto_type __btrc_ret_219 = __btrc_div_double(((float)rand()), ((float)RAND_MAX));
    return __btrc_ret_219;
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
    __auto_type __btrc_ret_222 = self->message;
    return __btrc_ret_222;
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
    btrc_Vector_string* __list_224 = btrc_Vector_string_new();
    (self->values = __list_224);
    btrc_Vector_string* __list_223 = btrc_Vector_string_new();
    (__list_223->__rc++);
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
    __auto_type __btrc_ret_225 = self->values->len;
    return __btrc_ret_225;
}

char* CliArgs_get(CliArgs* self, int index) {
    __auto_type __btrc_ret_226 = btrc_Vector_string_get(self->values, index);
    return __btrc_ret_226;
}

char* CliArgs_command(CliArgs* self) {
    if (self->values->len == 0) {
        __auto_type __btrc_ret_227 = "";
        return __btrc_ret_227;
    }
    __auto_type __btrc_ret_228 = btrc_Vector_string_get(self->values, 0);
    return __btrc_ret_228;
}

void CliCommand_init(CliCommand* self, char* name) {
    self->__rc = 1;
    (self->name = name);
    if (self->aliases != NULL) {
        if ((--self->aliases->__rc) <= 0) {
            btrc_Vector_string_free(self->aliases);
        }
    }
    btrc_Vector_string* __list_239 = btrc_Vector_string_new();
    (self->aliases = __list_239);
    btrc_Vector_string* __list_238 = btrc_Vector_string_new();
    (__list_238->__rc++);
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
    __auto_type __btrc_ret_245 = "\033[90m";
    return __btrc_ret_245;
}

char* NixosLog_orange(void) {
    __auto_type __btrc_ret_246 = "\033[38;5;208m";
    return __btrc_ret_246;
}

char* NixosLog_red(void) {
    __auto_type __btrc_ret_247 = "\033[31m";
    return __btrc_ret_247;
}

char* NixosLog_reset(void) {
    __auto_type __btrc_ret_248 = "\033[0m";
    return __btrc_ret_248;
}

void NixosLog_fatal(char* message) {
    Console_error(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(NixosLog_red(), message)), NixosLog_reset())));
    exit(1);
}

void MembraneConstants_init(MembraneConstants* self) {
    self->__rc = 1;
}

void MembraneConstants_destroy(MembraneConstants* self) {
    free(self);
}

char* MembraneConstants_topBase(void) {
    __auto_type __btrc_ret_249 = "/run/semipermeable-membrane-top";
    return __btrc_ret_249;
}

char* MembraneConstants_dirs(void) {
    __auto_type __btrc_ret_250 = "dirs";
    return __btrc_ret_250;
}

char* MembraneConstants_meta(void) {
    __auto_type __btrc_ret_251 = ".semipermeable_membrane";
    return __btrc_ret_251;
}

char* MembraneConstants_files(void) {
    __auto_type __btrc_ret_252 = "files";
    return __btrc_ret_252;
}

char* MembraneConstants_next(void) {
    __auto_type __btrc_ret_253 = "NEXT";
    return __btrc_ret_253;
}

char* MembraneConstants_a(void) {
    __auto_type __btrc_ret_254 = "A";
    return __btrc_ret_254;
}

char* MembraneConstants_b(void) {
    __auto_type __btrc_ret_255 = "B";
    return __btrc_ret_255;
}

char* MembraneConstants_c(void) {
    __auto_type __btrc_ret_256 = "C";
    return __btrc_ret_256;
}

char* MembraneConstants_disabled(void) {
    __auto_type __btrc_ret_257 = "disabled";
    return __btrc_ret_257;
}

char* MembraneConstants_converge(void) {
    __auto_type __btrc_ret_258 = "converge";
    return __btrc_ret_258;
}

char* MembraneConstants_reset(void) {
    __auto_type __btrc_ret_259 = "reset";
    return __btrc_ret_259;
}

char* MembraneConstants_prepareOnly(void) {
    __auto_type __btrc_ret_260 = "prepare-only";
    return __btrc_ret_260;
}

char* MembraneConstants_snapshotOnly(void) {
    __auto_type __btrc_ret_261 = "snapshot-only";
    return __btrc_ret_261;
}

char* MembraneConstants_restoreA(void) {
    __auto_type __btrc_ret_262 = "restore-a";
    return __btrc_ret_262;
}

char* MembraneConstants_restoreB(void) {
    __auto_type __btrc_ret_263 = "restore-b";
    return __btrc_ret_263;
}

char* MembraneConstants_restoreC(void) {
    __auto_type __btrc_ret_264 = "restore-c";
    return __btrc_ret_264;
}

char* MembraneConstants_restorePrevious(void) {
    __auto_type __btrc_ret_265 = "restore-previous";
    return __btrc_ret_265;
}

char* MembraneConstants_restorePenultimate(void) {
    __auto_type __btrc_ret_266 = "restore-penultimate";
    return __btrc_ret_266;
}

void MembraneAbsPath_init(MembraneAbsPath* self, char* path, bool endedWithSlash) {
    self->__rc = 1;
    (self->path = path);
    (self->endedWithSlash = endedWithSlash);
}

MembraneAbsPath* MembraneAbsPath_new(char* path, bool endedWithSlash) {
    MembraneAbsPath* self = ((MembraneAbsPath*)malloc(sizeof(MembraneAbsPath)));
    memset(self, 0, sizeof(MembraneAbsPath));
    MembraneAbsPath_init(self, path, endedWithSlash);
    return self;
}

void MembraneAbsPath_destroy(MembraneAbsPath* self) {
    free(self);
}

void MembraneSpec_init(MembraneSpec* self, char* volume, char* mountPoint, char* absPath, char* relPath, char* kind) {
    self->__rc = 1;
    (self->volume = volume);
    (self->mountPoint = mountPoint);
    (self->absPath = absPath);
    (self->relPath = relPath);
    (self->kind = kind);
}

MembraneSpec* MembraneSpec_new(char* volume, char* mountPoint, char* absPath, char* relPath, char* kind) {
    MembraneSpec* self = ((MembraneSpec*)malloc(sizeof(MembraneSpec)));
    memset(self, 0, sizeof(MembraneSpec));
    MembraneSpec_init(self, volume, mountPoint, absPath, relPath, kind);
    return self;
}

void MembraneSpec_destroy(MembraneSpec* self) {
    free(self);
}

void MembraneVolume_init(MembraneVolume* self, char* name, char* mountPoint) {
    self->__rc = 1;
    (self->name = name);
    (self->mountPoint = mountPoint);
}

MembraneVolume* MembraneVolume_new(char* name, char* mountPoint) {
    MembraneVolume* self = ((MembraneVolume*)malloc(sizeof(MembraneVolume)));
    memset(self, 0, sizeof(MembraneVolume));
    MembraneVolume_init(self, name, mountPoint);
    return self;
}

void MembraneVolume_destroy(MembraneVolume* self) {
    free(self);
}

void MembranePlan_init(MembranePlan* self, MembraneSpec* spec, char* kind, char* store) {
    self->__rc = 1;
    if (self->spec != NULL) {
        if ((--self->spec->__rc) <= 0) {
            MembraneSpec_destroy(self->spec);
        }
    }
    (self->spec = spec);
    (spec->__rc++);
    (self->kind = kind);
    (self->store = store);
}

MembranePlan* MembranePlan_new(MembraneSpec* spec, char* kind, char* store) {
    MembranePlan* self = ((MembranePlan*)malloc(sizeof(MembranePlan)));
    memset(self, 0, sizeof(MembranePlan));
    MembranePlan_init(self, spec, kind, store);
    return self;
}

void MembranePlan_destroy(MembranePlan* self) {
    if (self->spec != NULL) {
        if ((--self->spec->__rc) <= 0) {
            MembraneSpec_destroy(self->spec);
        }
    }
    if (__btrc_tracking) {
        __btrc_mark_destroyed(self);
    }
    free(self);
}

void MembranePaths_init(MembranePaths* self) {
    self->__rc = 1;
}

void MembranePaths_destroy(MembranePaths* self) {
    free(self);
}

char* MembranePaths_trimSlashes(char* raw) {
    char* value = __btrc_str_track(__btrc_trim(raw));
    int start = 0;
    int end = ((int)strlen(value));
    while ((start < end) && (value[start] == '/')) {
        (start++);
    }
    while ((end > start) && (value[(end - 1)] == '/')) {
        (end--);
    }
    __auto_type __btrc_ret_267 = __btrc_str_track(__btrc_substring(value, start, (end - start)));
    return __btrc_ret_267;
}

char* MembranePaths_trimTrailingSlashes(char* raw) {
    char* value = __btrc_str_track(__btrc_trim(raw));
    int end = ((int)strlen(value));
    while ((end > 1) && (value[(end - 1)] == '/')) {
        (end--);
    }
    __auto_type __btrc_ret_268 = __btrc_str_track(__btrc_substring(value, 0, end));
    return __btrc_ret_268;
}

bool MembranePaths_validRel(char* path) {
    if (__btrc_isEmpty(path)) {
        __auto_type __btrc_ret_269 = false;
        return __btrc_ret_269;
    }
    if (__btrc_strContains(path, "\t") || __btrc_strContains(path, "!")) {
        __auto_type __btrc_ret_270 = false;
        return __btrc_ret_270;
    }
    int __n_272 = btrc_Vector_string_iterLen(Strings_split(path, "/"));
    for (int __i_271 = 0; (__i_271 < __n_272); (__i_271++)) {
        char* part = btrc_Vector_string_iterGet(Strings_split(path, "/"), __i_271);
        if ((__btrc_isEmpty(part) || (strcmp(part, ".") == 0)) || (strcmp(part, "..") == 0)) {
            __auto_type __btrc_ret_273 = false;
            return __btrc_ret_273;
        }
    }
    __auto_type __btrc_ret_274 = true;
    return __btrc_ret_274;
}

char* MembranePaths_normSubvol(char* raw) {
    char* value = MembranePaths_trimSlashes(raw);
    if (!MembranePaths_validRel(value)) {
        NixosLog_fatal(__btrc_str_track(__btrc_strcat("bad subvolume: ", raw)));
    }
    return value;
}

char* MembranePaths_normMount(char* raw) {
    char* value = __btrc_str_track(__btrc_trim(raw));
    if (__btrc_isEmpty(value) || (strcmp(value, "/") == 0)) {
        __auto_type __btrc_ret_275 = "/";
        return __btrc_ret_275;
    }
    (value = MembranePaths_trimTrailingSlashes(value));
    if (!__btrc_startsWith(value, "/")) {
        NixosLog_fatal(__btrc_str_track(__btrc_strcat("bad mount: ", raw)));
    }
    char* rel = __btrc_str_track(__btrc_substring(value, 1, (((int)strlen(value)) - 1)));
    if (!MembranePaths_validRel(rel)) {
        NixosLog_fatal(__btrc_str_track(__btrc_strcat("bad mount: ", raw)));
    }
    return value;
}

MembraneAbsPath* MembranePaths_normAbs(char* raw) {
    char* value = __btrc_str_track(__btrc_trim(raw));
    bool slash = ((((int)strlen(value)) > 1) && __btrc_endsWith(value, "/"));
    (value = MembranePaths_trimTrailingSlashes(value));
    if ((!__btrc_startsWith(value, "/")) || (strcmp(value, "/") == 0)) {
        NixosLog_fatal(__btrc_str_track(__btrc_strcat("bad keep path: ", raw)));
    }
    char* rel = __btrc_str_track(__btrc_substring(value, 1, (((int)strlen(value)) - 1)));
    if (!MembranePaths_validRel(rel)) {
        NixosLog_fatal(__btrc_str_track(__btrc_strcat("bad keep path: ", raw)));
    }
    __auto_type __btrc_ret_276 = MembraneAbsPath_new(value, slash);
    return __btrc_ret_276;
}

char* MembranePaths_relToMount(char* absPath, char* mountPoint) {
    if (strcmp(mountPoint, "/") == 0) {
        __auto_type __btrc_ret_277 = __btrc_str_track(__btrc_substring(absPath, 1, (((int)strlen(absPath)) - 1)));
        return __btrc_ret_277;
    }
    char* prefix = __btrc_str_track(__btrc_strcat(mountPoint, "/"));
    if (!__btrc_startsWith(absPath, prefix)) {
        NixosLog_fatal(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(absPath, " not under ")), mountPoint)));
    }
    __auto_type __btrc_ret_278 = __btrc_str_track(__btrc_substring(absPath, ((int)strlen(prefix)), (((int)strlen(absPath)) - ((int)strlen(prefix)))));
    return __btrc_ret_278;
}

char* MembranePaths_kind(char* raw) {
    char* value = __btrc_str_track(__btrc_trim(raw));
    if (strcmp(value, "auto") == 0) {
        __auto_type __btrc_ret_279 = "auto";
        return __btrc_ret_279;
    }
    if ((strcmp(value, "directory") == 0) || (strcmp(value, "dir") == 0)) {
        __auto_type __btrc_ret_280 = "dir";
        return __btrc_ret_280;
    }
    if (strcmp(value, "file") == 0) {
        __auto_type __btrc_ret_281 = "file";
        return __btrc_ret_281;
    }
    NixosLog_fatal(__btrc_str_track(__btrc_strcat("bad kind: ", raw)));
    __auto_type __btrc_ret_282 = "auto";
    return __btrc_ret_282;
}

bool MembranePaths_ancestor(char* parent, char* child) {
    __auto_type __btrc_ret_283 = __btrc_startsWith(child, __btrc_str_track(__btrc_strcat(parent, "/")));
    return __btrc_ret_283;
}

char* MembranePaths_key(char* absPath) {
    char* rel = __btrc_str_track(__btrc_substring(absPath, 1, (((int)strlen(absPath)) - 1)));
    __auto_type __btrc_ret_284 = Strings_replace(rel, "/", "!");
    return __btrc_ret_284;
}

char* MembranePaths_dirStore(char* persistRoot, char* absPath) {
    __auto_type __btrc_ret_285 = PathTools_join(PathTools_join(persistRoot, MembraneConstants_dirs()), MembranePaths_key(absPath));
    return __btrc_ret_285;
}

char* MembranePaths_fileStore(char* persistRoot, char* absPath) {
    char* root = PathTools_join(PathTools_join(persistRoot, MembraneConstants_meta()), MembraneConstants_files());
    __auto_type __btrc_ret_286 = PathTools_join(root, MembranePaths_key(absPath));
    return __btrc_ret_286;
}

void MembraneRun_init(MembraneRun* self, bool dryRun) {
    self->__rc = 1;
    if (self->shell != NULL) {
        if ((--self->shell->__rc) <= 0) {
            UnixShell_destroy(self->shell);
        }
    }
    (self->shell = UnixShell_new());
    (UnixShell_new()->__rc++);
    if (self->timer != NULL) {
        if ((--self->timer->__rc) <= 0) {
            Timer_destroy(self->timer);
        }
    }
    (self->timer = Timer_new());
    (Timer_new()->__rc++);
    Timer_start(self->timer);
    (self->top = Environment_get("SEMIPERMEABLE_MEMBRANE_MOUNT_PATH", __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(MembraneConstants_topBase(), ".")), Strings_fromInt(Platform_pid())))));
    (self->dryRun = dryRun);
}

MembraneRun* MembraneRun_new(bool dryRun) {
    MembraneRun* self = ((MembraneRun*)malloc(sizeof(MembraneRun)));
    memset(self, 0, sizeof(MembraneRun));
    MembraneRun_init(self, dryRun);
    return self;
}

void MembraneRun_destroy(MembraneRun* self) {
    if (self->shell != NULL) {
        if ((--self->shell->__rc) <= 0) {
            UnixShell_destroy(self->shell);
        }
    }
    if (self->timer != NULL) {
        if ((--self->timer->__rc) <= 0) {
            Timer_destroy(self->timer);
        }
    }
    if (__btrc_tracking) {
        __btrc_mark_destroyed(self);
    }
    free(self);
}

char* MembraneRun_elapsed(MembraneRun* self) {
    __auto_type __btrc_ret_287 = __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("[", Strings_fromFloat(Timer_elapsed(self->timer)))), "s]"));
    return __btrc_ret_287;
}

char* MembraneRun_path(MembraneRun* self, char* rel) {
    char* base = MembranePaths_trimTrailingSlashes(self->top);
    char* child = MembranePaths_trimSlashes(rel);
    if (__btrc_isEmpty(child)) {
        return base;
    }
    __auto_type __btrc_ret_288 = PathTools_join(base, child);
    return __btrc_ret_288;
}

void MembraneRun_log(MembraneRun* self, char* message) {
    Console_error(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(MembraneRun_elapsed(self), " ")), message)));
}

void MembraneRun_fatal(MembraneRun* self, char* message) {
    NixosLog_fatal(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(MembraneRun_elapsed(self), " ERR ")), message)));
}

ExecResult* MembraneRun_runCommand(MembraneRun* self, Command* command) {
    char* rendered = Command_render(command);
    if (self->dryRun) {
        MembraneRun_log(self, __btrc_str_track(__btrc_strcat("DRY ", rendered)));
        __auto_type __btrc_ret_289 = ExecResult_new(0, "", "", rendered);
        return __btrc_ret_289;
    }
    MembraneRun_log(self, rendered);
    __auto_type __btrc_ret_290 = UnixShell_runCommand(self->shell, command);
    return __btrc_ret_290;
}

ExecResult* MembraneRun_runRaw(MembraneRun* self, char* command, bool captureOutput, bool checkStatus) {
    if (self->dryRun) {
        MembraneRun_log(self, __btrc_str_track(__btrc_strcat("DRY ", command)));
        __auto_type __btrc_ret_291 = ExecResult_new(0, "", "", command);
        return __btrc_ret_291;
    }
    MembraneRun_log(self, command);
    __auto_type __btrc_ret_292 = UnixShell_runRaw(self->shell, command, captureOutput, checkStatus, "");
    return __btrc_ret_292;
}

void MembraneRun_requireCommand(MembraneRun* self, Command* command) {
    ExecResult* result = MembraneRun_runCommand(self, command);
    if (!ExecResult_ok(result)) {
        MembraneRun_fatal(self, __btrc_str_track(__btrc_strcat("Failed: ", Command_render(command))));
    }
}

void MembraneRun_requireRaw(MembraneRun* self, char* command) {
    ExecResult* result = MembraneRun_runRaw(self, command, false, true);
    if (!ExecResult_ok(result)) {
        MembraneRun_fatal(self, __btrc_str_track(__btrc_strcat("Failed: ", command)));
    }
}

void MembraneRun_mkdirp(MembraneRun* self, char* path) {
    if (self->dryRun) {
        MembraneRun_log(self, __btrc_str_track(__btrc_strcat("DRY mkdir -p ", UnixShell_quote(path))));
        return;
    }
    FileSystem_mkdirp(path);
}

void MembraneRun_removePath(MembraneRun* self, char* path) {
    if (self->dryRun) {
        MembraneRun_log(self, __btrc_str_track(__btrc_strcat("DRY rm -rf ", UnixShell_quote(path))));
        return;
    }
    if (FileSystem_exists(path) || FileSystem_isSymlink(path)) {
        FileSystem_removeRecursive(path);
    }
}

void MembraneRun_renamePath(MembraneRun* self, char* source, char* destination) {
    MembraneRun_requireRaw(self, __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("mv --no-copy ", UnixShell_quote(source))), " ")), UnixShell_quote(destination))));
}

void MembraneBtrfs_init(MembraneBtrfs* self) {
    self->__rc = 1;
}

void MembraneBtrfs_destroy(MembraneBtrfs* self) {
    free(self);
}

Command* MembraneBtrfs_show(char* path) {
    __auto_type __btrc_ret_293 = Command_check(Command_arg(Command_arg(Command_arg(Command_new("btrfs"), "subvolume"), "show"), path), false);
    return __btrc_ret_293;
}

Command* MembraneBtrfs_getReadonly(char* path) {
    __auto_type __btrc_ret_294 = Command_check(Command_arg(Command_arg(Command_arg(Command_arg(Command_arg(Command_new("btrfs"), "property"), "get"), "-ts"), path), "ro"), false);
    return __btrc_ret_294;
}

Command* MembraneBtrfs_listChildren(char* path) {
    __auto_type __btrc_ret_295 = Command_arg(Command_arg(Command_arg(Command_arg(Command_new("btrfs"), "subvolume"), "list"), "-o"), path);
    return __btrc_ret_295;
}

Command* MembraneBtrfs_deleteSubvol(char* path) {
    __auto_type __btrc_ret_296 = Command_capture(Command_arg(Command_arg(Command_arg(Command_new("btrfs"), "subvolume"), "delete"), path), false);
    return __btrc_ret_296;
}

Command* MembraneBtrfs_createSubvol(char* path) {
    __auto_type __btrc_ret_297 = Command_capture(Command_arg(Command_arg(Command_arg(Command_new("btrfs"), "subvolume"), "create"), path), false);
    return __btrc_ret_297;
}

Command* MembraneBtrfs_snapshot(char* source, char* destination, bool readonly) {
    Command* command = Command_arg(Command_arg(Command_new("btrfs"), "subvolume"), "snapshot");
    if (readonly) {
        Command_arg(command, "-r");
    }
    __auto_type __btrc_ret_298 = Command_capture(Command_arg(Command_arg(command, source), destination), false);
    return __btrc_ret_298;
}

Command* MembraneBtrfs_clearReadonly(char* path) {
    __auto_type __btrc_ret_299 = Command_capture(Command_arg(Command_arg(Command_arg(Command_arg(Command_arg(Command_arg(Command_new("btrfs"), "property"), "set"), "-ts"), path), "ro"), "false"), false);
    return __btrc_ret_299;
}

Command* MembraneBtrfs_sync(char* top) {
    __auto_type __btrc_ret_300 = Command_capture(Command_arg(Command_arg(Command_arg(Command_new("btrfs"), "filesystem"), "sync"), top), false);
    return __btrc_ret_300;
}

void MembraneFs_init(MembraneFs* self, MembraneRun* run) {
    self->__rc = 1;
    if (self->run != NULL) {
        if ((--self->run->__rc) <= 0) {
            MembraneRun_destroy(self->run);
        }
    }
    (self->run = run);
    (run->__rc++);
}

MembraneFs* MembraneFs_new(MembraneRun* run) {
    MembraneFs* self = ((MembraneFs*)malloc(sizeof(MembraneFs)));
    memset(self, 0, sizeof(MembraneFs));
    MembraneFs_init(self, run);
    return self;
}

void MembraneFs_destroy(MembraneFs* self) {
    if (self->run != NULL) {
        if ((--self->run->__rc) <= 0) {
            MembraneRun_destroy(self->run);
        }
    }
    if (__btrc_tracking) {
        __btrc_mark_destroyed(self);
    }
    free(self);
}

bool MembraneFs_exists(MembraneFs* self, char* path) {
    __auto_type __btrc_ret_301 = (FileSystem_exists(path) || FileSystem_isSymlink(path));
    return __btrc_ret_301;
}

bool MembraneFs_isSubvolume(MembraneFs* self, char* path) {
    if (!FileSystem_exists(path)) {
        __auto_type __btrc_ret_302 = false;
        return __btrc_ret_302;
    }
    ExecResult* result = UnixShell_runCommand(self->run->shell, MembraneBtrfs_show(path));
    __auto_type __btrc_ret_303 = ExecResult_ok(result);
    return __btrc_ret_303;
}

bool MembraneFs_isReadonly(MembraneFs* self, char* path) {
    ExecResult* result = UnixShell_runCommand(self->run->shell, MembraneBtrfs_getReadonly(path));
    __auto_type __btrc_ret_304 = (ExecResult_ok(result) && __btrc_strContains(ExecResult_stdout(result), "ro=true"));
    return __btrc_ret_304;
}

bool MembraneFs_mounted(MembraneFs* self, char* path) {
    Command* command = Command_check(Command_arg(Command_arg(Command_new("findmnt"), "--mountpoint"), path), false);
    ExecResult* result = UnixShell_runCommand(self->run->shell, command);
    __auto_type __btrc_ret_305 = ExecResult_ok(result);
    return __btrc_ret_305;
}

int MembraneFs_compareDeepestFirst(MembraneFs* self, char* left, char* right) {
    int leftDepth = Strings_count(left, "/");
    int rightDepth = Strings_count(right, "/");
    if (leftDepth != rightDepth) {
        __auto_type __btrc_ret_306 = (rightDepth - leftDepth);
        return __btrc_ret_306;
    }
    if (((int)strlen(left)) != ((int)strlen(right))) {
        __auto_type __btrc_ret_307 = (((int)strlen(right)) - ((int)strlen(left)));
        return __btrc_ret_307;
    }
    __auto_type __btrc_ret_308 = Strings_compare(left, right);
    return __btrc_ret_308;
}

void MembraneFs_pushChildSorted(MembraneFs* self, btrc_Vector_string* paths, char* path) {
    int index = 0;
    while ((index < paths->len) && (MembraneFs_compareDeepestFirst(self, btrc_Vector_string_get(paths, index), path) <= 0)) {
        (index++);
    }
    btrc_Vector_string_insert(paths, index, path);
}

btrc_Vector_string* MembraneFs_childSubvolumes(MembraneFs* self, char* path) {
    btrc_Vector_string* result = btrc_Vector_string_new();
    if (!FileSystem_isDir(path)) {
        return result;
    }
    ExecResult* listed = MembraneRun_runCommand(self->run, MembraneBtrfs_listChildren(path));
    if (!ExecResult_ok(listed)) {
        MembraneRun_fatal(self->run, __btrc_str_track(__btrc_strcat("list ", path)));
    }
    int __n_310 = btrc_Vector_string_iterLen(Strings_split(ExecResult_stdout(listed), "\n"));
    for (int __i_309 = 0; (__i_309 < __n_310); (__i_309++)) {
        char* line = btrc_Vector_string_iterGet(Strings_split(ExecResult_stdout(listed), "\n"), __i_309);
        int marker = Strings_find(line, " path ", 0);
        if (marker < 0) {
            continue;
        }
        char* child = __btrc_str_track(__btrc_trim(__btrc_str_track(__btrc_substring(line, (marker + 6), (((int)strlen(line)) - (marker + 6))))));
        char* childPath = MembraneRun_path(self->run, child);
        if (MembranePaths_ancestor(path, childPath)) {
            MembraneFs_pushChildSorted(self, result, childPath);
        }
    }
    return result;
}

void MembraneFs_deleteSubvolume(MembraneFs* self, char* path) {
    if (!FileSystem_isDir(path)) {
        return;
    }
    btrc_Vector_string* children = MembraneFs_childSubvolumes(self, path);
    int __n_312 = btrc_Vector_string_iterLen(children);
    for (int __i_311 = 0; (__i_311 < __n_312); (__i_311++)) {
        char* child = btrc_Vector_string_iterGet(children, __i_311);
        MembraneRun_requireCommand(self->run, MembraneBtrfs_deleteSubvol(child));
    }
    MembraneRun_requireCommand(self->run, MembraneBtrfs_deleteSubvol(path));
}

void MembraneFs_makeSubvolume(MembraneFs* self, char* path) {
    MembraneRun_mkdirp(self->run, PathTools_dirname(path));
    MembraneRun_requireCommand(self->run, MembraneBtrfs_createSubvol(path));
}

void MembraneFs_snapshot(MembraneFs* self, char* source, char* destination, bool readonly) {
    if (!FileSystem_isDir(source)) {
        MembraneRun_fatal(self->run, __btrc_str_track(__btrc_strcat("Source missing: ", source)));
    }
    MembraneFs_deleteSubvolume(self, destination);
    MembraneRun_requireCommand(self->run, MembraneBtrfs_snapshot(source, destination, readonly));
    if (!readonly) {
        MembraneRun_requireCommand(self->run, MembraneBtrfs_clearReadonly(destination));
    }
}

char* MembraneFs_sourcePath(MembraneFs* self, char* live, char* clean, char* relPath) {
    char* livePath = PathTools_join(live, relPath);
    if (MembraneFs_exists(self, livePath)) {
        return livePath;
    }
    char* cleanPath = PathTools_join(clean, relPath);
    if (MembraneFs_exists(self, cleanPath)) {
        return cleanPath;
    }
    __auto_type __btrc_ret_313 = "";
    return __btrc_ret_313;
}

void MembraneFs_copyPath(MembraneFs* self, char* source, char* destination) {
    MembraneRun_mkdirp(self->run, PathTools_dirname(destination));
    Command* command = Command_capture(Command_arg(Command_arg(Command_arg(Command_arg(Command_new("cp"), "--reflink=always"), "-a"), source), destination), false);
    MembraneRun_requireCommand(self->run, command);
}

void MembraneFs_copyDirectoryContents(MembraneFs* self, char* source, char* destination) {
    if (!FileSystem_isDir(source)) {
        return;
    }
    btrc_Vector_string* nested = MembraneFs_childSubvolumes(self, source);
    if (!btrc_Vector_string_isEmpty(nested)) {
        MembraneRun_fatal(self->run, __btrc_str_track(__btrc_strcat("nested subvolumes in persisted dir: ", btrc_Vector_string_join(nested, " "))));
    }
    MembraneRun_requireRaw(self->run, __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("cp --reflink=always -a ", UnixShell_quote(PathTools_join(source, ".")))), " ")), UnixShell_quote(destination))));
}

void MembranePlanner_init(MembranePlanner* self, MembraneFs* fs) {
    self->__rc = 1;
    if (self->fs != NULL) {
        if ((--self->fs->__rc) <= 0) {
            MembraneFs_destroy(self->fs);
        }
    }
    (self->fs = fs);
    (fs->__rc++);
}

MembranePlanner* MembranePlanner_new(MembraneFs* fs) {
    MembranePlanner* self = ((MembranePlanner*)malloc(sizeof(MembranePlanner)));
    memset(self, 0, sizeof(MembranePlanner));
    MembranePlanner_init(self, fs);
    return self;
}

void MembranePlanner_destroy(MembranePlanner* self) {
    if (self->fs != NULL) {
        if ((--self->fs->__rc) <= 0) {
            MembraneFs_destroy(self->fs);
        }
    }
    if (__btrc_tracking) {
        __btrc_mark_destroyed(self);
    }
    free(self);
}

int MembranePlanner_compareSpecs(MembranePlanner* self, MembraneSpec* left, MembraneSpec* right) {
    int volume = Strings_compare(left->volume, right->volume);
    if (volume != 0) {
        return volume;
    }
    int leftDepth = Strings_count(left->relPath, "/");
    int rightDepth = Strings_count(right->relPath, "/");
    if (leftDepth != rightDepth) {
        __auto_type __btrc_ret_314 = (leftDepth - rightDepth);
        return __btrc_ret_314;
    }
    if (((int)strlen(left->relPath)) != ((int)strlen(right->relPath))) {
        __auto_type __btrc_ret_315 = (((int)strlen(left->relPath)) - ((int)strlen(right->relPath)));
        return __btrc_ret_315;
    }
    __auto_type __btrc_ret_316 = Strings_compare(left->relPath, right->relPath);
    return __btrc_ret_316;
}

void MembranePlanner_pushSpecSorted(MembranePlanner* self, btrc_Vector_MembraneSpec* specs, MembraneSpec* spec) {
    int index = 0;
    while ((index < specs->len) && (MembranePlanner_compareSpecs(self, btrc_Vector_MembraneSpec_get(specs, index), spec) <= 0)) {
        (index++);
    }
    btrc_Vector_MembraneSpec_insert(specs, index, spec);
}

btrc_Vector_MembraneSpec* MembranePlanner_sortedSpecs(MembranePlanner* self, btrc_Vector_MembraneSpec* input) {
    btrc_Vector_MembraneSpec* sorted = btrc_Vector_MembraneSpec_new();
    int __n_318 = btrc_Vector_MembraneSpec_iterLen(input);
    for (int __i_317 = 0; (__i_317 < __n_318); (__i_317++)) {
        MembraneSpec* spec = btrc_Vector_MembraneSpec_iterGet(input, __i_317);
        MembranePlanner_pushSpecSorted(self, sorted, spec);
    }
    return sorted;
}

char* MembranePlanner_resolveKind(MembranePlanner* self, char* live, char* clean, MembraneSpec* spec) {
    char* path = MembraneFs_sourcePath(self->fs, live, clean, spec->relPath);
    if (__btrc_isEmpty(path)) {
        __auto_type __btrc_ret_319 = "file";
        return __btrc_ret_319;
    }
    if (FileSystem_isDir(path)) {
        __auto_type __btrc_ret_320 = "dir";
        return __btrc_ret_320;
    }
    __auto_type __btrc_ret_321 = "file";
    return __btrc_ret_321;
}

bool MembranePlanner_planAlreadyCovers(MembranePlanner* self, btrc_Vector_MembranePlan* plans, MembraneSpec* spec) {
    int __n_323 = btrc_Vector_MembranePlan_iterLen(plans);
    for (int __i_322 = 0; (__i_322 < __n_323); (__i_322++)) {
        MembranePlan* plan = btrc_Vector_MembranePlan_iterGet(plans, __i_322);
        if ((strcmp(plan->spec->volume, spec->volume) == 0) && (strcmp(plan->spec->relPath, spec->relPath) == 0)) {
            __auto_type __btrc_ret_324 = true;
            return __btrc_ret_324;
        }
        if (((strcmp(plan->spec->volume, spec->volume) == 0) && (strcmp(plan->kind, "dir") == 0)) && MembranePaths_ancestor(plan->spec->relPath, spec->relPath)) {
            __auto_type __btrc_ret_325 = true;
            return __btrc_ret_325;
        }
    }
    __auto_type __btrc_ret_326 = false;
    return __btrc_ret_326;
}

btrc_Vector_MembranePlan* MembranePlanner_makePlan(MembranePlanner* self, btrc_Vector_MembraneSpec* specs, char* live, char* clean, char* persistRoot) {
    btrc_Vector_MembranePlan* plans = btrc_Vector_MembranePlan_new();
    btrc_Vector_MembraneSpec* ordered = MembranePlanner_sortedSpecs(self, specs);
    int __n_328 = btrc_Vector_MembraneSpec_iterLen(ordered);
    for (int __i_327 = 0; (__i_327 < __n_328); (__i_327++)) {
        MembraneSpec* spec = btrc_Vector_MembraneSpec_iterGet(ordered, __i_327);
        char* kind = spec->kind;
        if (strcmp(kind, "auto") == 0) {
            (kind = MembranePlanner_resolveKind(self, live, clean, spec));
        }
        if (MembranePlanner_planAlreadyCovers(self, plans, spec)) {
            continue;
        }
        char* store = ((strcmp(kind, "dir") == 0) ? MembranePaths_dirStore(persistRoot, spec->absPath) : MembranePaths_fileStore(persistRoot, spec->absPath));
        btrc_Vector_MembranePlan_push(plans, MembranePlan_new(spec, kind, store));
    }
    return plans;
}

void MembraneMounts_init(MembraneMounts* self, MembraneRun* run, MembraneFs* fs) {
    self->__rc = 1;
    if (self->run != NULL) {
        if ((--self->run->__rc) <= 0) {
            MembraneRun_destroy(self->run);
        }
    }
    (self->run = run);
    (run->__rc++);
    if (self->fs != NULL) {
        if ((--self->fs->__rc) <= 0) {
            MembraneFs_destroy(self->fs);
        }
    }
    (self->fs = fs);
    (fs->__rc++);
}

MembraneMounts* MembraneMounts_new(MembraneRun* run, MembraneFs* fs) {
    MembraneMounts* self = ((MembraneMounts*)malloc(sizeof(MembraneMounts)));
    memset(self, 0, sizeof(MembraneMounts));
    MembraneMounts_init(self, run, fs);
    return self;
}

void MembraneMounts_destroy(MembraneMounts* self) {
    if (self->run != NULL) {
        if ((--self->run->__rc) <= 0) {
            MembraneRun_destroy(self->run);
        }
    }
    if (self->fs != NULL) {
        if ((--self->fs->__rc) <= 0) {
            MembraneFs_destroy(self->fs);
        }
    }
    if (__btrc_tracking) {
        __btrc_mark_destroyed(self);
    }
    free(self);
}

void MembraneMounts_mountTop(MembraneMounts* self, char* device, bool assumeMounted) {
    if (assumeMounted) {
        if (!FileSystem_isDir(self->run->top)) {
            MembraneRun_fatal(self->run, __btrc_str_track(__btrc_strcat("missing mount path: ", self->run->top)));
        }
        return;
    }
    MembraneRun_mkdirp(self->run, self->run->top);
    Command* command = Command_capture(Command_arg(Command_arg(Command_arg(Command_arg(Command_arg(Command_arg(Command_new("mount"), "-t"), "btrfs"), "-o"), "subvolid=5,user_subvol_rm_allowed"), device), self->run->top), false);
    MembraneRun_requireCommand(self->run, command);
}

void MembraneMounts_unmountTop(MembraneMounts* self, bool assumeMounted) {
    if (assumeMounted) {
        return;
    }
    Command* command = Command_check(Command_capture(Command_arg(Command_arg(Command_new("umount"), "-R"), self->run->top), false), false);
    MembraneRun_runCommand(self->run, command);
    if (__btrc_startsWith(self->run->top, __btrc_str_track(__btrc_strcat(MembraneConstants_topBase(), ".")))) {
        FileSystem_removeRecursive(self->run->top);
    }
}

bool MembraneMounts_coveredBy(MembraneMounts* self, btrc_Vector_string* selected, char* target) {
    int __n_330 = btrc_Vector_string_iterLen(selected);
    for (int __i_329 = 0; (__i_329 < __n_330); (__i_329++)) {
        char* s = btrc_Vector_string_iterGet(selected, __i_329);
        if (__btrc_startsWith(target, __btrc_str_track(__btrc_strcat(s, "/")))) {
            __auto_type __btrc_ret_331 = true;
            return __btrc_ret_331;
        }
    }
    __auto_type __btrc_ret_332 = false;
    return __btrc_ret_332;
}

void MembraneMounts_mountTarget(MembraneMounts* self, char* device, char* target, char* subvol) {
    MembraneRun_mkdirp(self->run, target);
    if (MembraneFs_mounted(self->fs, target)) {
        return;
    }
    Command* command = Command_capture(Command_arg(Command_arg(Command_arg(Command_arg(Command_arg(Command_arg(Command_new("mount"), "-t"), "btrfs"), "-o"), __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("subvol=", subvol)), ",compress=zstd,noatime"))), device), target), false);
    MembraneRun_requireCommand(self->run, command);
}

void MembraneMounts_mountPersist(MembraneMounts* self, char* device, char* persistRoot, btrc_Vector_MembraneSpec* specs, bool assumeMounted) {
    MembraneRun_log(self->run, __btrc_str_track(__btrc_strcat("Mounting persistent subvolumes dry_run=", (self->run->dryRun ? "true" : "false"))));
    bool mountedTop = false;
    if ((!assumeMounted) && (!MembraneFs_mounted(self->fs, self->run->top))) {
        MembraneRun_mkdirp(self->run, self->run->top);
        Command* m = Command_capture(Command_arg(Command_arg(Command_arg(Command_arg(Command_arg(Command_arg(Command_new("mount"), "-t"), "btrfs"), "-o"), "ro,subvolid=5"), device), self->run->top), false);
        MembraneRun_requireCommand(self->run, m);
        (mountedTop = true);
    }
    btrc_Vector_string* selected = btrc_Vector_string_new();
    int __n_334 = btrc_Vector_MembraneSpec_iterLen(specs);
    for (int __i_333 = 0; (__i_333 < __n_334); (__i_333++)) {
        MembraneSpec* spec = btrc_Vector_MembraneSpec_iterGet(specs, __i_333);
        char* target = spec->absPath;
        if (MembraneMounts_coveredBy(self, selected, target)) {
            continue;
        }
        char* subvol = PathTools_join(PathTools_join(persistRoot, MembraneConstants_dirs()), MembranePaths_key(target));
        if (!MembraneFs_isSubvolume(self->fs, MembraneRun_path(self->run, subvol))) {
            continue;
        }
        MembraneMounts_mountTarget(self, device, target, subvol);
        btrc_Vector_string_push(selected, target);
    }
    if (mountedTop) {
        Command* u = Command_check(Command_capture(Command_arg(Command_new("umount"), self->run->top), false), false);
        MembraneRun_runCommand(self->run, u);
    }
    MembraneRun_log(self->run, "Persistent mounts complete");
}

void MembraneMounts_snapshotCleanVolume(MembraneMounts* self, MembraneVolume* volume, char* snapshotsSubvolume, char* cleanName) {
    char* live = MembraneRun_path(self->run, volume->name);
    char* root = PathTools_join(snapshotsSubvolume, volume->name);
    char* clean = MembraneRun_path(self->run, PathTools_join(root, cleanName));
    if (!MembraneFs_exists(self->fs, live)) {
        MembraneRun_fatal(self->run, __btrc_str_track(__btrc_strcat("live missing: ", live)));
    }
    if (!MembraneFs_isSubvolume(self->fs, live)) {
        MembraneRun_fatal(self->run, __btrc_str_track(__btrc_strcat("live is not a subvolume: ", live)));
    }
    MembraneRun_log(self->run, __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("Re-capturing CLEAN baseline for ", volume->name)), " from live subvolume")));
    MembraneFs_snapshot(self->fs, live, clean, true);
}

void MembraneMounts_snapshotCleanAll(MembraneMounts* self, char* device, char* snapshotsSubvolume, char* cleanName, btrc_Vector_MembraneVolume* volumes, bool assumeMounted) {
    btrc_Vector_string* names = btrc_Vector_string_new();
    int __n_336 = btrc_Vector_MembraneVolume_iterLen(volumes);
    for (int __i_335 = 0; (__i_335 < __n_336); (__i_335++)) {
        MembraneVolume* volume = btrc_Vector_MembraneVolume_iterGet(volumes, __i_335);
        btrc_Vector_string_push(names, volume->name);
    }
    MembraneRun_log(self->run, __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("snapshot-clean dry_run=", (self->run->dryRun ? "true" : "false"))), " device=")), device)), " subvolumes=")), btrc_Vector_string_join(names, " "))));
    if (self->run->dryRun && (!assumeMounted)) {
        MembraneRun_log(self->run, "DRY add --assume-mounted to walk mounted test roots");
        return;
    }
    MembraneMounts_mountTop(self, device, assumeMounted);
    int __n_338 = btrc_Vector_MembraneVolume_iterLen(volumes);
    for (int __i_337 = 0; (__i_337 < __n_338); (__i_337++)) {
        MembraneVolume* volume = btrc_Vector_MembraneVolume_iterGet(volumes, __i_337);
        MembraneMounts_snapshotCleanVolume(self, volume, snapshotsSubvolume, cleanName);
    }
    MembraneRun_requireCommand(self->run, MembraneBtrfs_sync(self->run->top));
    MembraneMounts_unmountTop(self, assumeMounted);
    MembraneRun_log(self->run, "snapshot-clean complete");
}

void MembraneSlots_init(MembraneSlots* self, MembraneFs* fs, MembraneRun* run) {
    self->__rc = 1;
    if (self->fs != NULL) {
        if ((--self->fs->__rc) <= 0) {
            MembraneFs_destroy(self->fs);
        }
    }
    (self->fs = fs);
    (fs->__rc++);
    if (self->run != NULL) {
        if ((--self->run->__rc) <= 0) {
            MembraneRun_destroy(self->run);
        }
    }
    (self->run = run);
    (run->__rc++);
}

MembraneSlots* MembraneSlots_new(MembraneFs* fs, MembraneRun* run) {
    MembraneSlots* self = ((MembraneSlots*)malloc(sizeof(MembraneSlots)));
    memset(self, 0, sizeof(MembraneSlots));
    MembraneSlots_init(self, fs, run);
    return self;
}

void MembraneSlots_destroy(MembraneSlots* self) {
    if (self->fs != NULL) {
        if ((--self->fs->__rc) <= 0) {
            MembraneFs_destroy(self->fs);
        }
    }
    if (self->run != NULL) {
        if ((--self->run->__rc) <= 0) {
            MembraneRun_destroy(self->run);
        }
    }
    if (__btrc_tracking) {
        __btrc_mark_destroyed(self);
    }
    free(self);
}

void MembraneSlots_rotate(MembraneSlots* self, char* volume, char* live, char* snapshotsSubvolume) {
    char* root = PathTools_join(snapshotsSubvolume, volume);
    char* a = MembraneRun_path(self->run, PathTools_join(root, MembraneConstants_a()));
    char* b = MembraneRun_path(self->run, PathTools_join(root, MembraneConstants_b()));
    char* c = MembraneRun_path(self->run, PathTools_join(root, MembraneConstants_c()));
    MembraneFs_deleteSubvolume(self->fs, c);
    if (MembraneFs_exists(self->fs, b)) {
        MembraneFs_snapshot(self->fs, b, c, true);
        MembraneFs_deleteSubvolume(self->fs, b);
    }
    if (MembraneFs_exists(self->fs, a)) {
        MembraneFs_snapshot(self->fs, a, b, true);
        MembraneFs_deleteSubvolume(self->fs, a);
    }
    MembraneFs_snapshot(self->fs, live, a, true);
}

void MembraneSlots_publish(MembraneSlots* self, MembraneVolume* volume, char* live, char* next) {
    if (!MembraneFs_exists(self->fs, next)) {
        return;
    }
    if (((!self->run->dryRun) && (!(strcmp(volume->mountPoint, "/") == 0))) && MembraneFs_mounted(self->fs, volume->mountPoint)) {
        MembraneRun_log(self->run, __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("WRN ", volume->mountPoint)), " mounted; leaving NEXT for boot")));
        return;
    }
    if (!MembraneFs_exists(self->fs, live)) {
        MembraneRun_renamePath(self->run, next, live);
        return;
    }
    MembraneRun_requireRaw(self->run, __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("mv -T --exchange --no-copy ", UnixShell_quote(live))), " ")), UnixShell_quote(next))));
    MembraneFs_deleteSubvolume(self->fs, next);
}

void MembraneSlots_restoreSlot(MembraneSlots* self, MembraneVolume* volume, char* slot, char* snapshotsSubvolume) {
    char* root = PathTools_join(snapshotsSubvolume, volume->name);
    char* live = MembraneRun_path(self->run, volume->name);
    char* next = MembraneRun_path(self->run, PathTools_join(root, MembraneConstants_next()));
    char* source = MembraneRun_path(self->run, PathTools_join(root, slot));
    if ((!MembraneFs_exists(self->fs, source)) || (!MembraneFs_isSubvolume(self->fs, source))) {
        MembraneRun_fatal(self->run, __btrc_str_track(__btrc_strcat("restore source missing: ", source)));
    }
    MembraneFs_snapshot(self->fs, source, next, false);
    MembraneSlots_publish(self, volume, live, next);
}

void MembraneConfig_init(MembraneConfig* self) {
    self->__rc = 1;
}

void MembraneConfig_destroy(MembraneConfig* self) {
    free(self);
}

btrc_Vector_MembraneSpec* MembraneConfig_readSpecs(char* specFile) {
    btrc_Vector_MembraneSpec* result = btrc_Vector_MembraneSpec_new();
    char* content = FileSystem_readText(specFile);
    int __n_340 = btrc_Vector_string_iterLen(Strings_split(content, "\n"));
    for (int __i_339 = 0; (__i_339 < __n_340); (__i_339++)) {
        char* raw = btrc_Vector_string_iterGet(Strings_split(content, "\n"), __i_339);
        char* line = __btrc_str_track(__btrc_trim(raw));
        if (__btrc_isEmpty(line) || __btrc_startsWith(line, "#")) {
            continue;
        }
        btrc_Vector_string* parts = Strings_split(raw, "\t");
        if (parts->len != 4) {
            NixosLog_fatal(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(specFile, " expected 4 tab-separated fields: ")), raw)));
        }
        char* volume = MembranePaths_normSubvol(btrc_Vector_string_get(parts, 0));
        char* mountPoint = MembranePaths_normMount(btrc_Vector_string_get(parts, 1));
        MembraneAbsPath* abs = MembranePaths_normAbs(btrc_Vector_string_get(parts, 2));
        char* kind = MembranePaths_kind(btrc_Vector_string_get(parts, 3));
        if ((strcmp(kind, "auto") == 0) && abs->endedWithSlash) {
            (kind = "dir");
        }
        char* rel = MembranePaths_relToMount(abs->path, mountPoint);
        btrc_Vector_MembraneSpec_push(result, MembraneSpec_new(volume, mountPoint, abs->path, rel, kind));
    }
    return result;
}

bool MembraneConfig_volumeSeen(btrc_Vector_MembraneVolume* volumes, char* name) {
    int __n_342 = btrc_Vector_MembraneVolume_iterLen(volumes);
    for (int __i_341 = 0; (__i_341 < __n_342); (__i_341++)) {
        MembraneVolume* volume = btrc_Vector_MembraneVolume_iterGet(volumes, __i_341);
        if (strcmp(volume->name, name) == 0) {
            __auto_type __btrc_ret_343 = true;
            return __btrc_ret_343;
        }
    }
    __auto_type __btrc_ret_344 = false;
    return __btrc_ret_344;
}

btrc_Vector_MembraneVolume* MembraneConfig_readVolumes(btrc_Vector_string* args, btrc_Vector_MembraneSpec* specs) {
    btrc_Vector_MembraneVolume* result = btrc_Vector_MembraneVolume_new();
    if (btrc_Vector_string_isEmpty(args)) {
        int __n_346 = btrc_Vector_MembraneSpec_iterLen(specs);
        for (int __i_345 = 0; (__i_345 < __n_346); (__i_345++)) {
            MembraneSpec* spec = btrc_Vector_MembraneSpec_iterGet(specs, __i_345);
            if (!MembraneConfig_volumeSeen(result, spec->volume)) {
                btrc_Vector_MembraneVolume_push(result, MembraneVolume_new(spec->volume, "/"));
            }
        }
        return result;
    }
    int __n_348 = btrc_Vector_string_iterLen(args);
    for (int __i_347 = 0; (__i_347 < __n_348); (__i_347++)) {
        char* arg = btrc_Vector_string_iterGet(args, __i_347);
        int marker = Strings_find(arg, "=", 0);
        if (marker < 0) {
            NixosLog_fatal(__btrc_str_track(__btrc_strcat("bad volume arg: ", arg)));
        }
        char* name = __btrc_str_track(__btrc_substring(arg, 0, marker));
        char* mountPoint = __btrc_str_track(__btrc_substring(arg, (marker + 1), ((((int)strlen(arg)) - marker) - 1)));
        btrc_Vector_MembraneVolume_push(result, MembraneVolume_new(MembranePaths_normSubvol(name), MembranePaths_normMount(mountPoint)));
    }
    return result;
}

void SemipermeableMembrane_init(SemipermeableMembrane* self, bool dryRun, bool assumeMounted) {
    self->__rc = 1;
    if (self->run != NULL) {
        if ((--self->run->__rc) <= 0) {
            MembraneRun_destroy(self->run);
        }
    }
    (self->run = MembraneRun_new(dryRun));
    (MembraneRun_new(dryRun)->__rc++);
    if (self->fs != NULL) {
        if ((--self->fs->__rc) <= 0) {
            MembraneFs_destroy(self->fs);
        }
    }
    (self->fs = MembraneFs_new(self->run));
    (MembraneFs_new(self->run)->__rc++);
    if (self->planner != NULL) {
        if ((--self->planner->__rc) <= 0) {
            MembranePlanner_destroy(self->planner);
        }
    }
    (self->planner = MembranePlanner_new(self->fs));
    (MembranePlanner_new(self->fs)->__rc++);
    if (self->mounts != NULL) {
        if ((--self->mounts->__rc) <= 0) {
            MembraneMounts_destroy(self->mounts);
        }
    }
    (self->mounts = MembraneMounts_new(self->run, self->fs));
    (MembraneMounts_new(self->run, self->fs)->__rc++);
    if (self->slots != NULL) {
        if ((--self->slots->__rc) <= 0) {
            MembraneSlots_destroy(self->slots);
        }
    }
    (self->slots = MembraneSlots_new(self->fs, self->run));
    (MembraneSlots_new(self->fs, self->run)->__rc++);
    (self->device = "");
    (self->snapshotsSubvolume = "");
    (self->cleanName = "");
    (self->mode = "");
    (self->persistRoot = "");
    (self->specFile = "");
    if (self->specs != NULL) {
        if ((--self->specs->__rc) <= 0) {
            btrc_Vector_MembraneSpec_free(self->specs);
        }
    }
    btrc_Vector_MembraneSpec* __list_350 = btrc_Vector_MembraneSpec_new();
    (self->specs = __list_350);
    btrc_Vector_MembraneSpec* __list_349 = btrc_Vector_MembraneSpec_new();
    (__list_349->__rc++);
    if (self->volumes != NULL) {
        if ((--self->volumes->__rc) <= 0) {
            btrc_Vector_MembraneVolume_free(self->volumes);
        }
    }
    btrc_Vector_MembraneVolume* __list_352 = btrc_Vector_MembraneVolume_new();
    (self->volumes = __list_352);
    btrc_Vector_MembraneVolume* __list_351 = btrc_Vector_MembraneVolume_new();
    (__list_351->__rc++);
    (self->assumeMounted = assumeMounted);
}

SemipermeableMembrane* SemipermeableMembrane_new(bool dryRun, bool assumeMounted) {
    SemipermeableMembrane* self = ((SemipermeableMembrane*)malloc(sizeof(SemipermeableMembrane)));
    memset(self, 0, sizeof(SemipermeableMembrane));
    SemipermeableMembrane_init(self, dryRun, assumeMounted);
    return self;
}

void SemipermeableMembrane_destroy(SemipermeableMembrane* self) {
    if (self->run != NULL) {
        if ((--self->run->__rc) <= 0) {
            MembraneRun_destroy(self->run);
        }
    }
    if (self->fs != NULL) {
        if ((--self->fs->__rc) <= 0) {
            MembraneFs_destroy(self->fs);
        }
    }
    if (self->planner != NULL) {
        if ((--self->planner->__rc) <= 0) {
            MembranePlanner_destroy(self->planner);
        }
    }
    if (self->mounts != NULL) {
        if ((--self->mounts->__rc) <= 0) {
            MembraneMounts_destroy(self->mounts);
        }
    }
    if (self->slots != NULL) {
        if ((--self->slots->__rc) <= 0) {
            MembraneSlots_destroy(self->slots);
        }
    }
    if (self->specs != NULL) {
        if ((--self->specs->__rc) <= 0) {
            btrc_Vector_MembraneSpec_free(self->specs);
        }
    }
    if (self->volumes != NULL) {
        if ((--self->volumes->__rc) <= 0) {
            btrc_Vector_MembraneVolume_free(self->volumes);
        }
    }
    if (__btrc_tracking) {
        __btrc_mark_destroyed(self);
    }
    free(self);
}

bool SemipermeableMembrane_envDryRun(void) {
    char* value = Environment_get("SEMIPERMEABLE_MEMBRANE_DRY_RUN", "");
    __auto_type __btrc_ret_353 = ((((strcmp(value, "1") == 0) || (strcmp(value, "true") == 0)) || (strcmp(value, "yes") == 0)) || (strcmp(value, "on") == 0));
    return __btrc_ret_353;
}

void SemipermeableMembrane_ensureNamespaces(SemipermeableMembrane* self) {
    btrc_Vector_string* __list_354 = btrc_Vector_string_new();
    btrc_Vector_string_push(__list_354, self->persistRoot);
    btrc_Vector_string_push(__list_354, PathTools_join(self->persistRoot, MembraneConstants_dirs()));
    btrc_Vector_string_push(__list_354, PathTools_join(self->persistRoot, MembraneConstants_meta()));
    btrc_Vector_string_push(__list_354, PathTools_join(PathTools_join(self->persistRoot, MembraneConstants_meta()), MembraneConstants_files()));
    btrc_Vector_string* namespaces = __list_354;
    int __n_356 = btrc_Vector_string_iterLen(namespaces);
    for (int __i_355 = 0; (__i_355 < __n_356); (__i_355++)) {
        char* rel = btrc_Vector_string_iterGet(namespaces, __i_355);
        char* path = MembraneRun_path(self->run, rel);
        if (!MembraneFs_exists(self->fs, path)) {
            MembraneFs_makeSubvolume(self->fs, path);
        } else if (!MembraneFs_isSubvolume(self->fs, path)) {
            MembraneRun_fatal(self->run, __btrc_str_track(__btrc_strcat("not a subvolume: ", path)));
        }
    }
}

bool SemipermeableMembrane_keyInSpec(SemipermeableMembrane* self, btrc_Vector_string* validKeys, char* key) {
    int __n_358 = btrc_Vector_string_iterLen(validKeys);
    for (int __i_357 = 0; (__i_357 < __n_358); (__i_357++)) {
        char* v = btrc_Vector_string_iterGet(validKeys, __i_357);
        if (strcmp(v, key) == 0) {
            __auto_type __btrc_ret_359 = true;
            return __btrc_ret_359;
        }
    }
    __auto_type __btrc_ret_360 = false;
    return __btrc_ret_360;
}

void SemipermeableMembrane_pruneOrphans(SemipermeableMembrane* self) {
    btrc_Vector_string* validKeys = btrc_Vector_string_new();
    int __n_362 = btrc_Vector_MembraneSpec_iterLen(self->specs);
    for (int __i_361 = 0; (__i_361 < __n_362); (__i_361++)) {
        MembraneSpec* spec = btrc_Vector_MembraneSpec_iterGet(self->specs, __i_361);
        btrc_Vector_string_push(validKeys, MembranePaths_key(spec->absPath));
    }
    char* dirsPath = MembraneRun_path(self->run, PathTools_join(self->persistRoot, MembraneConstants_dirs()));
    btrc_Vector_string* children = MembraneFs_childSubvolumes(self->fs, dirsPath);
    int __n_364 = btrc_Vector_string_iterLen(children);
    for (int __i_363 = 0; (__i_363 < __n_364); (__i_363++)) {
        char* child = btrc_Vector_string_iterGet(children, __i_363);
        char* key = PathTools_basename(child);
        if (!SemipermeableMembrane_keyInSpec(self, validKeys, key)) {
            MembraneRun_log(self->run, __btrc_str_track(__btrc_strcat("Pruning orphaned persist subvolume ", key)));
            MembraneFs_deleteSubvolume(self->fs, child);
        }
    }
}

btrc_Vector_MembraneSpec* SemipermeableMembrane_specsForVolume(SemipermeableMembrane* self, char* volume) {
    btrc_Vector_MembraneSpec* result = btrc_Vector_MembraneSpec_new();
    int __n_366 = btrc_Vector_MembraneSpec_iterLen(self->specs);
    for (int __i_365 = 0; (__i_365 < __n_366); (__i_365++)) {
        MembraneSpec* spec = btrc_Vector_MembraneSpec_iterGet(self->specs, __i_365);
        if (strcmp(spec->volume, volume) == 0) {
            btrc_Vector_MembraneSpec_push(result, spec);
        }
    }
    return result;
}

void SemipermeableMembrane_resetVolume(SemipermeableMembrane* self, MembraneVolume* volume) {
    char* live = MembraneRun_path(self->run, volume->name);
    char* root = PathTools_join(self->snapshotsSubvolume, volume->name);
    char* clean = MembraneRun_path(self->run, PathTools_join(root, self->cleanName));
    char* next = MembraneRun_path(self->run, PathTools_join(root, MembraneConstants_next()));
    if (((!MembraneFs_exists(self->fs, clean)) || (!MembraneFs_isSubvolume(self->fs, clean))) || (!MembraneFs_isReadonly(self->fs, clean))) {
        MembraneRun_fatal(self->run, __btrc_str_track(__btrc_strcat("bad CLEAN: ", clean)));
    }
    if (!MembraneFs_exists(self->fs, live)) {
        MembraneSlots_publish(self->slots, volume, live, next);
    }
    if (!MembraneFs_exists(self->fs, live)) {
        MembraneRun_fatal(self->run, __btrc_str_track(__btrc_strcat("live missing: ", live)));
    }
    if (!MembraneFs_isSubvolume(self->fs, live)) {
        MembraneRun_fatal(self->run, __btrc_str_track(__btrc_strcat("live is not a subvolume: ", live)));
    }
    MembraneFs_deleteSubvolume(self->fs, next);
    btrc_Vector_MembranePlan* plans = MembranePlanner_makePlan(self->planner, SemipermeableMembrane_specsForVolume(self, volume->name), live, clean, self->persistRoot);
    SemipermeableMembrane_restorePersistentDirs(self, plans, live, clean);
    SemipermeableMembrane_restorePersistentFiles(self, plans, live, clean);
    MembraneSlots_rotate(self->slots, volume->name, live, self->snapshotsSubvolume);
    MembraneFs_snapshot(self->fs, clean, next, false);
    SemipermeableMembrane_populateNext(self, plans, next);
    MembraneSlots_publish(self->slots, volume, live, next);
}

void SemipermeableMembrane_restorePersistentDirs(SemipermeableMembrane* self, btrc_Vector_MembranePlan* plans, char* live, char* clean) {
    int __n_368 = btrc_Vector_MembranePlan_iterLen(plans);
    for (int __i_367 = 0; (__i_367 < __n_368); (__i_367++)) {
        MembranePlan* plan = btrc_Vector_MembranePlan_iterGet(plans, __i_367);
        if (!(strcmp(plan->kind, "dir") == 0)) {
            continue;
        }
        char* store = MembraneRun_path(self->run, plan->store);
        if (!MembraneFs_exists(self->fs, store)) {
            MembraneFs_makeSubvolume(self->fs, store);
            char* source = MembraneFs_sourcePath(self->fs, live, clean, plan->spec->relPath);
            if (!__btrc_isEmpty(source)) {
                MembraneFs_copyDirectoryContents(self->fs, source, store);
            }
        } else if (!MembraneFs_isSubvolume(self->fs, store)) {
            MembraneRun_fatal(self->run, __btrc_str_track(__btrc_strcat("not a subvolume: ", store)));
        }
    }
}

void SemipermeableMembrane_restorePersistentFiles(SemipermeableMembrane* self, btrc_Vector_MembranePlan* plans, char* live, char* clean) {
    int __n_370 = btrc_Vector_MembranePlan_iterLen(plans);
    for (int __i_369 = 0; (__i_369 < __n_370); (__i_369++)) {
        MembranePlan* plan = btrc_Vector_MembranePlan_iterGet(plans, __i_369);
        if (!(strcmp(plan->kind, "file") == 0)) {
            continue;
        }
        char* destination = MembraneRun_path(self->run, plan->store);
        char* source = MembraneFs_sourcePath(self->fs, live, clean, plan->spec->relPath);
        if (!__btrc_isEmpty(source)) {
            char* temporary = __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(destination, ".tmp.")), Strings_fromInt(Platform_pid())));
            MembraneRun_removePath(self->run, temporary);
            MembraneFs_copyPath(self->fs, source, temporary);
            MembraneRun_removePath(self->run, destination);
            MembraneRun_renamePath(self->run, temporary, destination);
        } else {
            MembraneRun_removePath(self->run, destination);
        }
    }
}

void SemipermeableMembrane_populateNext(SemipermeableMembrane* self, btrc_Vector_MembranePlan* plans, char* next) {
    int __n_372 = btrc_Vector_MembranePlan_iterLen(plans);
    for (int __i_371 = 0; (__i_371 < __n_372); (__i_371++)) {
        MembranePlan* plan = btrc_Vector_MembranePlan_iterGet(plans, __i_371);
        char* target = PathTools_join(next, plan->spec->relPath);
        MembraneRun_removePath(self->run, target);
        if (strcmp(plan->kind, "dir") == 0) {
            MembraneRun_mkdirp(self->run, target);
        } else {
            char* source = MembraneRun_path(self->run, plan->store);
            if (MembraneFs_exists(self->fs, source)) {
                MembraneFs_copyPath(self->fs, source, target);
            }
        }
    }
}

void SemipermeableMembrane_configure(SemipermeableMembrane* self, char* device, char* snapshotsSubvolume, char* cleanName, char* mode, char* persistRoot, char* specFile, btrc_Vector_string* volumeArgs) {
    (self->device = device);
    (self->snapshotsSubvolume = MembranePaths_normSubvol(snapshotsSubvolume));
    (self->cleanName = __btrc_str_track(__btrc_trim(cleanName)));
    (self->mode = __btrc_str_track(__btrc_trim(mode)));
    (self->persistRoot = MembranePaths_normSubvol(persistRoot));
    (self->specFile = specFile);
    if (self->specs != NULL) {
        if ((--self->specs->__rc) <= 0) {
            btrc_Vector_MembraneSpec_free(self->specs);
        }
    }
    (self->specs = MembraneConfig_readSpecs(self->specFile));
    (MembraneConfig_readSpecs(self->specFile)->__rc++);
    if (self->volumes != NULL) {
        if ((--self->volumes->__rc) <= 0) {
            btrc_Vector_MembraneVolume_free(self->volumes);
        }
    }
    (self->volumes = MembraneConfig_readVolumes(volumeArgs, self->specs));
    (MembraneConfig_readVolumes(volumeArgs, self->specs)->__rc++);
}

void SemipermeableMembrane_runAll(SemipermeableMembrane* self) {
    btrc_Vector_string* names = btrc_Vector_string_new();
    int __n_374 = btrc_Vector_MembraneVolume_iterLen(self->volumes);
    for (int __i_373 = 0; (__i_373 < __n_374); (__i_373++)) {
        MembraneVolume* volume = btrc_Vector_MembraneVolume_iterGet(self->volumes, __i_373);
        btrc_Vector_string_push(names, volume->name);
    }
    MembraneRun_log(self->run, __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("Mode=", self->mode)), " dry_run=")), (self->run->dryRun ? "true" : "false"))), " device=")), self->device)), " subvolumes=")), btrc_Vector_string_join(names, " "))));
    if (strcmp(self->mode, MembraneConstants_disabled()) == 0) {
        MembraneRun_log(self->run, "Semipermeable membrane disabled; skipping");
        return;
    }
    if (self->run->dryRun && (!self->assumeMounted)) {
        MembraneRun_log(self->run, "DRY add --assume-mounted to walk mounted test roots");
        return;
    }
    MembraneMounts_mountTop(self->mounts, self->device, self->assumeMounted);
    SemipermeableMembrane_ensureNamespaces(self);
    if (((strcmp(self->mode, MembraneConstants_converge()) == 0) || (strcmp(self->mode, MembraneConstants_reset()) == 0)) || (strcmp(self->mode, MembraneConstants_prepareOnly()) == 0)) {
        SemipermeableMembrane_pruneOrphans(self);
    }
    int __n_376 = btrc_Vector_MembraneVolume_iterLen(self->volumes);
    for (int __i_375 = 0; (__i_375 < __n_376); (__i_375++)) {
        MembraneVolume* volume = btrc_Vector_MembraneVolume_iterGet(self->volumes, __i_375);
        if (((strcmp(self->mode, MembraneConstants_converge()) == 0) || (strcmp(self->mode, MembraneConstants_reset()) == 0)) || (strcmp(self->mode, MembraneConstants_prepareOnly()) == 0)) {
            SemipermeableMembrane_resetVolume(self, volume);
        } else if (strcmp(self->mode, MembraneConstants_snapshotOnly()) == 0) {
            MembraneSlots_rotate(self->slots, volume->name, MembraneRun_path(self->run, volume->name), self->snapshotsSubvolume);
        } else if ((strcmp(self->mode, MembraneConstants_restoreA()) == 0) || (strcmp(self->mode, MembraneConstants_restorePrevious()) == 0)) {
            MembraneSlots_restoreSlot(self->slots, volume, MembraneConstants_a(), self->snapshotsSubvolume);
        } else if ((strcmp(self->mode, MembraneConstants_restoreB()) == 0) || (strcmp(self->mode, MembraneConstants_restorePenultimate()) == 0)) {
            MembraneSlots_restoreSlot(self->slots, volume, MembraneConstants_b(), self->snapshotsSubvolume);
        } else if (strcmp(self->mode, MembraneConstants_restoreC()) == 0) {
            MembraneSlots_restoreSlot(self->slots, volume, MembraneConstants_c(), self->snapshotsSubvolume);
        } else {
            MembraneRun_fatal(self->run, __btrc_str_track(__btrc_strcat("Unknown mode: ", self->mode)));
        }
    }
    MembraneRun_requireCommand(self->run, MembraneBtrfs_sync(self->run->top));
    MembraneMounts_unmountTop(self->mounts, self->assumeMounted);
    MembraneRun_log(self->run, "Semipermeable membrane complete");
}

void SemipermeableMembrane_configureMount(SemipermeableMembrane* self, char* device, char* persistRoot, char* specFile) {
    (self->device = device);
    (self->persistRoot = MembranePaths_normSubvol(persistRoot));
    (self->specFile = specFile);
    if (self->specs != NULL) {
        if ((--self->specs->__rc) <= 0) {
            btrc_Vector_MembraneSpec_free(self->specs);
        }
    }
    (self->specs = MembraneConfig_readSpecs(self->specFile));
    (MembraneConfig_readSpecs(self->specFile)->__rc++);
}

void SemipermeableMembrane_mountPersist(SemipermeableMembrane* self) {
    MembraneMounts_mountPersist(self->mounts, self->device, self->persistRoot, self->specs, self->assumeMounted);
}

void SemipermeableMembrane_configureSnapshotClean(SemipermeableMembrane* self, char* device, char* snapshotsSubvolume, char* cleanName, btrc_Vector_string* volumeArgs) {
    (self->device = device);
    (self->snapshotsSubvolume = MembranePaths_normSubvol(snapshotsSubvolume));
    (self->cleanName = __btrc_str_track(__btrc_trim(cleanName)));
    if (self->volumes != NULL) {
        if ((--self->volumes->__rc) <= 0) {
            btrc_Vector_MembraneVolume_free(self->volumes);
        }
    }
    (self->volumes = MembraneConfig_readVolumes(volumeArgs, self->specs));
    (MembraneConfig_readVolumes(volumeArgs, self->specs)->__rc++);
}

void SemipermeableMembrane_snapshotCleanAll(SemipermeableMembrane* self) {
    MembraneMounts_snapshotCleanAll(self->mounts, self->device, self->snapshotsSubvolume, self->cleanName, self->volumes, self->assumeMounted);
}

int SemipermeableMembrane_cli(CliArgs* args) {
    bool dryRun = SemipermeableMembrane_envDryRun();
    bool assumeMounted = false;
    int index = 0;
    while (index < CliArgs_count(args)) {
        char* value = CliArgs_get(args, index);
        if (strcmp(value, "--dry-run") == 0) {
            (dryRun = true);
            (index++);
            continue;
        }
        if (strcmp(value, "--assume-mounted") == 0) {
            (assumeMounted = true);
            (index++);
            continue;
        }
        break;
    }
    if ((index < CliArgs_count(args)) && (strcmp(CliArgs_get(args, index), "mount") == 0)) {
        if (CliArgs_count(args) < (index + 4)) {
            Console_error("Usage: semipermeable_membrane [--dry-run] mount <device> <persist_root> <spec_file>");
            __auto_type __btrc_ret_377 = 1;
            return __btrc_ret_377;
        }
        SemipermeableMembrane* mounter = SemipermeableMembrane_new(dryRun, assumeMounted);
        SemipermeableMembrane_configureMount(mounter, CliArgs_get(args, (index + 1)), CliArgs_get(args, (index + 2)), CliArgs_get(args, (index + 3)));
        SemipermeableMembrane_mountPersist(mounter);
        __auto_type __btrc_ret_378 = 0;
        if (mounter != NULL) {
            if ((--mounter->__rc) <= 0) {
                SemipermeableMembrane_destroy(mounter);
            }
        }
        return __btrc_ret_378;
        if (mounter != NULL) {
            if ((--mounter->__rc) <= 0) {
                SemipermeableMembrane_destroy(mounter);
            }
        }
    }
    if ((index < CliArgs_count(args)) && (strcmp(CliArgs_get(args, index), "snapshot-clean") == 0)) {
        if (CliArgs_count(args) < (index + 4)) {
            Console_error("Usage: semipermeable_membrane [--dry-run] [--assume-mounted] snapshot-clean <device> <snapshots> <clean> [name=mount ...]");
            __auto_type __btrc_ret_379 = 1;
            return __btrc_ret_379;
        }
        btrc_Vector_string* cleanVolumeArgs = btrc_Vector_string_new();
        for (int i = (index + 4); (i < CliArgs_count(args)); (i++)) {
            btrc_Vector_string_push(cleanVolumeArgs, CliArgs_get(args, i));
        }
        SemipermeableMembrane* snapshotter = SemipermeableMembrane_new(dryRun, assumeMounted);
        SemipermeableMembrane_configureSnapshotClean(snapshotter, CliArgs_get(args, (index + 1)), CliArgs_get(args, (index + 2)), CliArgs_get(args, (index + 3)), cleanVolumeArgs);
        SemipermeableMembrane_snapshotCleanAll(snapshotter);
        __auto_type __btrc_ret_380 = 0;
        if (snapshotter != NULL) {
            if ((--snapshotter->__rc) <= 0) {
                SemipermeableMembrane_destroy(snapshotter);
            }
        }
        return __btrc_ret_380;
        if (snapshotter != NULL) {
            if ((--snapshotter->__rc) <= 0) {
                SemipermeableMembrane_destroy(snapshotter);
            }
        }
    }
    if (CliArgs_count(args) < (index + 6)) {
        Console_error("Usage: semipermeable_membrane [--dry-run] [--assume-mounted] <device> <snapshots> <clean> <mode> <persist_root> <spec_file> [name=mount ...]");
        __auto_type __btrc_ret_381 = 1;
        return __btrc_ret_381;
    }
    btrc_Vector_string* volumeArgs = btrc_Vector_string_new();
    for (int i = (index + 6); (i < CliArgs_count(args)); (i++)) {
        btrc_Vector_string_push(volumeArgs, CliArgs_get(args, i));
    }
    SemipermeableMembrane* membrane = SemipermeableMembrane_new(dryRun, assumeMounted);
    SemipermeableMembrane_configure(membrane, CliArgs_get(args, index), CliArgs_get(args, (index + 1)), CliArgs_get(args, (index + 2)), CliArgs_get(args, (index + 3)), CliArgs_get(args, (index + 4)), CliArgs_get(args, (index + 5)), volumeArgs);
    SemipermeableMembrane_runAll(membrane);
    __auto_type __btrc_ret_382 = 0;
    if (membrane != NULL) {
        if ((--membrane->__rc) <= 0) {
            SemipermeableMembrane_destroy(membrane);
        }
    }
    return __btrc_ret_382;
    if (membrane != NULL) {
        if ((--membrane->__rc) <= 0) {
            SemipermeableMembrane_destroy(membrane);
        }
    }
}

int main(int argc, char** argv) {
    CliArgs* args = CliArgs_new(argc, argv);
    int code = SemipermeableMembrane_cli(args);
    if (args != NULL) {
        if ((--args->__rc) <= 0) {
            CliArgs_destroy(args);
        }
    }
    return code;
    if (args != NULL) {
        if ((--args->__rc) <= 0) {
            CliArgs_destroy(args);
        }
    }
}

