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
typedef struct SemipermeableMembrane SemipermeableMembrane;
void SemipermeableMembrane_destroy(SemipermeableMembrane* self);
typedef struct btrc_Vector_string btrc_Vector_string;
typedef struct btrc_Vector_bool btrc_Vector_bool;
typedef struct btrc_Vector_int btrc_Vector_int;
typedef struct btrc_Vector_float btrc_Vector_float;
typedef struct btrc_Vector_MembraneSpec btrc_Vector_MembraneSpec;
typedef struct btrc_Vector_MembraneVolume btrc_Vector_MembraneVolume;
typedef struct btrc_Vector_MembranePlan btrc_Vector_MembranePlan;
typedef struct btrc_Map_string_string btrc_Map_string_string;
typedef struct btrc_Map_string_bool btrc_Map_string_bool;
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
void Console_log(char* msg);
void Console_error(char* msg);
void Console_write(char* msg);
void Console_writeLine(char* msg);
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
bool Path_exists(char* path);
char* Path_readAll(char* path);
void Path_writeAll(char* path, char* content);
int UnixPlatform_pid(void);
int UnixPlatform_euid(void);
bool Platform_isUnix(void);
bool Platform_isWindows(void);
char* Platform_pathSeparator(void);
int Platform_pid(void);
int Platform_euid(void);
bool Platform_isRoot(void);
char* Environment_get(char* name, char* fallback);
bool Environment_has(char* name);
FILE* popen(const char* command, const char* mode);
int pclose(FILE* stream);
int ProcessStatus_code(ProcessStatus* self);
bool ProcessStatus_ok(ProcessStatus* self);
bool UnixPipe_ok(UnixPipe* self);
char* UnixPipe_readAll(UnixPipe* self);
ProcessStatus* UnixPipe_close(UnixPipe* self);
ProcessStatus* UnixProcess_system(char* command);
UnixPipe* UnixProcess_pipe(char* command);
bool ShellWords_isSafeArgChar(char c);
bool ShellWords_isSafeArg(char* raw);
char* ShellWords_quote(char* raw);
char* ShellWords_redact(char* text, char* sensitive);
bool ExecResult_ok(ExecResult* self);
char* ExecResult_stdout(ExecResult* self);
char* ExecResult_stderr(ExecResult* self);
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
char* UnixShell_quote(char* raw);
char* UnixShell_redactText(char* text, char* sensitive);
int UnixShell_statusCode(int rawStatus);
void UnixShell_chroot(UnixShell* self, char* path);
void UnixShell_clearChroot(UnixShell* self);
ExecResult* UnixShell_run(UnixShell* self, char* command);
ExecResult* UnixShell_runUnchecked(UnixShell* self, char* command);
ExecResult* UnixShell_runCommand(UnixShell* self, Command* command);
ExecResult* UnixShell_runRaw(UnixShell* self, char* command, bool captureOutput, bool checkStatus, char* sensitive);
ExecResult* PowerShell_run(PowerShell* self, char* command);
char* mkdtemp(char* templatePath);
bool FileStatus_exists(FileStatus* self);
bool FileStatus_isDir(FileStatus* self);
bool FileStatus_isFile(FileStatus* self);
bool FileStatus_isSymlink(FileStatus* self);
btrc_Vector_string* Directory_entries(Directory* self);
int UnixFileSystem_statusCode(int raw);
int UnixFileSystem_chmodPath(char* path, int mode);
int UnixFileSystem_mkdirPath(char* path, int mode);
int UnixFileSystem_runShell(char* command);
int UnixFileSystem_mkdirp(char* path);
int UnixFileSystem_removeRecursive(char* path);
int UnixFileSystem_symlinkPath(char* target, char* linkPath);
char* UnixFileSystem_readLink(char* path);
char* UnixFileSystem_tempDir(char* prefix);
char* PathTools_shellQuote(char* raw);
char* PathTools_basename(char* path);
char* PathTools_dirname(char* path);
char* PathTools_join(char* left, char* right);
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
bool UnixPattern_matches(char* pattern, char* text);
bool Pattern_matches(char* pattern, char* text);
bool Pattern_anyMatches(btrc_Vector_string* patterns, char* text);
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
DateTime* DateTime_now(void);
void DateTime_display(DateTime* self);
char* DateTime_format(DateTime* self);
char* DateTime_dateString(DateTime* self);
char* DateTime_timeString(DateTime* self);
void Timer_start(Timer* self);
void Timer_stop(Timer* self);
float Timer_elapsed(Timer* self);
void Timer_reset(Timer* self);
void Random_seed(Random* self, int s);
void Random_seedTime(Random* self);
int Random_randint(Random* self, int lo, int hi);
float Random_random(Random* self);
float Random_uniform(Random* self, float lo, float hi);
int Random_choice(Random* self, btrc_Vector_int* items);
void Random_shuffle(Random* self, btrc_Vector_int* items);
char* Error_toString(Error* self);
char* ValueError_toString(ValueError* self);
char* IOError_toString(IOError* self);
char* TypeError_toString(TypeError* self);
char* IndexError_toString(IndexError* self);
char* KeyError_toString(KeyError* self);
int CliArgs_count(CliArgs* self);
char* CliArgs_get(CliArgs* self, int index);
char* CliArgs_command(CliArgs* self);
bool CliArgs_has(CliArgs* self, char* flag);
char* CliArgs_valueAfter(CliArgs* self, char* flag, char* fallback);
bool CliArgs_commandIs(CliArgs* self, char* name);
char* CliArgs_valueAfterPrefix(CliArgs* self, char* prefix, char* fallback);
void CliCommand_alias(CliCommand* self, char* name);
bool CliCommand_matches(CliCommand* self, char* value);
char* NixosLog_gray(void);
char* NixosLog_orange(void);
char* NixosLog_red(void);
char* NixosLog_reset(void);
char* NixosLog_redact(char* text, char* sensitive);
void NixosLog_info(char* message);
void NixosLog_error(char* message);
void NixosLog_fatal(char* message);
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
bool SemipermeableMembrane_envDryRun(void);
bool SemipermeableMembrane_exists(SemipermeableMembrane* self, char* path);
bool SemipermeableMembrane_isSubvolume(SemipermeableMembrane* self, char* path);
bool SemipermeableMembrane_isReadonly(SemipermeableMembrane* self, char* path);
bool SemipermeableMembrane_mounted(SemipermeableMembrane* self, char* path);
int SemipermeableMembrane_compareDeepestFirst(SemipermeableMembrane* self, char* left, char* right);
void SemipermeableMembrane_pushChildSorted(SemipermeableMembrane* self, btrc_Vector_string* paths, char* path);
btrc_Vector_string* SemipermeableMembrane_childSubvolumes(SemipermeableMembrane* self, char* path);
void SemipermeableMembrane_deleteSubvolume(SemipermeableMembrane* self, char* path);
void SemipermeableMembrane_makeSubvolume(SemipermeableMembrane* self, char* path);
void SemipermeableMembrane_snapshot(SemipermeableMembrane* self, char* source, char* destination, bool readonly);
char* SemipermeableMembrane_sourcePath(SemipermeableMembrane* self, char* live, char* clean, char* relPath);
void SemipermeableMembrane_copyPath(SemipermeableMembrane* self, char* source, char* destination);
void SemipermeableMembrane_copyDirectoryContents(SemipermeableMembrane* self, char* source, char* destination);
void SemipermeableMembrane_ensureNamespaces(SemipermeableMembrane* self);
void SemipermeableMembrane_rotate(SemipermeableMembrane* self, char* volume, char* live);
void SemipermeableMembrane_publish(SemipermeableMembrane* self, MembraneVolume* volume, char* live, char* next);
int SemipermeableMembrane_compareSpecs(SemipermeableMembrane* self, MembraneSpec* left, MembraneSpec* right);
void SemipermeableMembrane_pushSpecSorted(SemipermeableMembrane* self, btrc_Vector_MembraneSpec* specs, MembraneSpec* spec);
btrc_Vector_MembraneSpec* SemipermeableMembrane_sortedSpecs(SemipermeableMembrane* self, btrc_Vector_MembraneSpec* input);
char* SemipermeableMembrane_resolveKind(SemipermeableMembrane* self, char* live, char* clean, MembraneSpec* spec);
bool SemipermeableMembrane_planAlreadyCovers(SemipermeableMembrane* self, btrc_Vector_MembranePlan* plans, MembraneSpec* spec);
btrc_Vector_MembranePlan* SemipermeableMembrane_makePlan(SemipermeableMembrane* self, btrc_Vector_MembraneSpec* specs, char* live, char* clean);
btrc_Vector_MembraneSpec* SemipermeableMembrane_specsForVolume(SemipermeableMembrane* self, char* volume);
void SemipermeableMembrane_resetVolume(SemipermeableMembrane* self, MembraneVolume* volume);
void SemipermeableMembrane_restoreSlot(SemipermeableMembrane* self, MembraneVolume* volume, char* slot);
void SemipermeableMembrane_readSpecs(SemipermeableMembrane* self);
bool SemipermeableMembrane_volumeSeen(SemipermeableMembrane* self, btrc_Vector_MembraneVolume* volumes, char* name);
void SemipermeableMembrane_readVolumes(SemipermeableMembrane* self, btrc_Vector_string* args);
void SemipermeableMembrane_mountTop(SemipermeableMembrane* self);
void SemipermeableMembrane_unmountTop(SemipermeableMembrane* self);
void SemipermeableMembrane_configure(SemipermeableMembrane* self, char* device, char* snapshotsSubvolume, char* cleanName, char* mode, char* persistRoot, char* specFile, btrc_Vector_string* volumeArgs);
void SemipermeableMembrane_runAll(SemipermeableMembrane* self);
bool SemipermeableMembrane_coveredBy(SemipermeableMembrane* self, btrc_Vector_string* selected, char* target);
void SemipermeableMembrane_mountTarget(SemipermeableMembrane* self, char* target, char* subvol);
void SemipermeableMembrane_configureMount(SemipermeableMembrane* self, char* device, char* persistRoot, char* specFile);
void SemipermeableMembrane_mountPersist(SemipermeableMembrane* self);
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
typedef bool (*__btrc_fn_bool_MembraneVolume)(MembraneVolume*);
typedef void (*__btrc_fn_void_MembraneVolume)(MembraneVolume*);
typedef MembraneVolume* (*__btrc_fn_MembraneVolume_MembraneVolume)(MembraneVolume*);
typedef MembraneVolume* (*__btrc_fn_MembraneVolume_MembraneVolume_MembraneVolume)(MembraneVolume*, MembraneVolume*);
typedef bool (*__btrc_fn_bool_MembranePlan)(MembranePlan*);
typedef void (*__btrc_fn_void_MembranePlan)(MembranePlan*);
typedef MembranePlan* (*__btrc_fn_MembranePlan_MembranePlan)(MembranePlan*);
typedef MembranePlan* (*__btrc_fn_MembranePlan_MembranePlan_MembranePlan)(MembranePlan*, MembranePlan*);

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

struct btrc_Vector_MembraneVolume {
    int __rc;
    MembraneVolume** data;
    int len;
    int cap;
};

struct btrc_Vector_MembranePlan {
    int __rc;
    MembranePlan** data;
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

struct SemipermeableMembrane {
    int __rc;
    MembraneRun* run;
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

Strings* Strings_new(void) {
    Strings* self = ((Strings*)malloc(sizeof(Strings)));
    memset(self, 0, sizeof(Strings));
    Strings_init(self);
    return self;
}

void Strings_destroy(Strings* self) {
    free(self);
}

char* Strings_copy(char* s) {
    int __fstr_1_len = snprintf(NULL, 0, "%s", s);
    char* __fstr_1_buf = __btrc_str_track(((char*)malloc((__fstr_1_len + 1))));
    snprintf(__fstr_1_buf, (__fstr_1_len + 1), "%s", s);
    return __fstr_1_buf;
}

char* Strings_repeat(char* s, int count) {
    int slen = ((int)strlen(s));
    int total = (slen * count);
    char* result = ((char*)malloc((total + 1)));
    for (int i = 0; (i < count); (i++)) {
        memcpy((result + (i * slen)), s, slen);
    }
    (result[total] = '\0');
    return result;
}

char* Strings_join(btrc_Vector_string* items, char* sep) {
    if (items->len == 0) {
        return "";
    }
    int seplen = ((int)strlen(sep));
    int total = 0;
    for (int i = 0; (i < items->len); (i++)) {
        (total = (total + ((int)strlen(btrc_Vector_string_get(items, i)))));
    }
    (total = (total + (seplen * (items->len - 1))));
    char* result = ((char*)malloc((total + 1)));
    int pos = 0;
    int first_len = ((int)strlen(btrc_Vector_string_get(items, 0)));
    memcpy(result, btrc_Vector_string_get(items, 0), first_len);
    (pos = first_len);
    for (int i = 1; (i < items->len); (i++)) {
        memcpy((result + pos), sep, seplen);
        (pos = (pos + seplen));
        int item_len = ((int)strlen(btrc_Vector_string_get(items, i)));
        memcpy((result + pos), btrc_Vector_string_get(items, i), item_len);
        (pos = (pos + item_len));
    }
    (result[pos] = '\0');
    return result;
}

char* Strings_replace(char* s, char* old, char* replacement) {
    if (s == NULL) {
        return "";
    }
    if ((old == NULL) || (replacement == NULL)) {
        return Strings_copy(s);
    }
    int slen = ((int)strlen(s));
    int oldlen = ((int)strlen(old));
    if (oldlen == 0) {
        return Strings_copy(s);
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
    return ((c >= '0') && (c <= '9'));
}

bool Strings_isAlpha(char c) {
    return (((c >= 'a') && (c <= 'z')) || ((c >= 'A') && (c <= 'Z')));
}

bool Strings_isAlnum(char c) {
    return (Strings_isAlpha(c) || Strings_isDigit(c));
}

bool Strings_isSpace(char c) {
    return ((((c == ' ') || (c == '\t')) || (c == '\n')) || (c == '\r'));
}

int Strings_toInt(char* s) {
    if (s == NULL) {
        return 0;
    }
    char* value = __btrc_str_track(__btrc_trim(s));
    if (__btrc_isEmpty(value)) {
        return 0;
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
    return (result * sign);
}

float Strings_toFloat(char* s) {
    return ((float)atof(s));
}

int Strings_count(char* s, char* sub) {
    int slen = ((int)strlen(s));
    int sublen = ((int)strlen(sub));
    if (sublen == 0) {
        return 0;
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
    return (-1);
}

int Strings_rfind(char* s, char* sub) {
    int slen = ((int)strlen(s));
    int sublen = ((int)strlen(sub));
    if (sublen == 0) {
        return slen;
    }
    int i = (slen - sublen);
    while (i >= 0) {
        if (strncmp((s + i), sub, sublen) == 0) {
            return i;
        }
        (i--);
    }
    return (-1);
}

int Strings_compare(char* left, char* right) {
    if ((left == NULL) && (right == NULL)) {
        return 0;
    }
    if (left == NULL) {
        return (-1);
    }
    if (right == NULL) {
        return 1;
    }
    int i = 0;
    while ((left[i] != '\0') && (right[i] != '\0')) {
        if (left[i] < right[i]) {
            return (-1);
        }
        if (left[i] > right[i]) {
            return 1;
        }
        (i++);
    }
    if ((left[i] == '\0') && (right[i] == '\0')) {
        return 0;
    }
    if (left[i] == '\0') {
        return (-1);
    }
    return 1;
}

bool Strings_lessThan(char* left, char* right) {
    return (Strings_compare(left, right) < 0);
}

char* Strings_capitalize(char* s) {
    int slen = ((int)strlen(s));
    char* result = ((char*)malloc((slen + 1)));
    for (int i = 0; (i < slen); (i++)) {
        (result[i] = ((char)tolower(((unsigned char)s[i]))));
    }
    if (slen > 0) {
        (result[0] = ((char)toupper(((unsigned char)s[0]))));
    }
    (result[slen] = '\0');
    return result;
}

char* Strings_title(char* s) {
    int slen = ((int)strlen(s));
    char* result = ((char*)malloc((slen + 1)));
    bool newWord = true;
    for (int i = 0; (i < slen); (i++)) {
        char c = s[i];
        if ((((c == ' ') || (c == '\t')) || (c == '\n')) || (c == '\r')) {
            (result[i] = c);
            (newWord = true);
        } else {
            if (newWord) {
                (result[i] = ((char)toupper(((unsigned char)c))));
            } else {
                (result[i] = ((char)tolower(((unsigned char)c))));
            }
            (newWord = false);
        }
    }
    (result[slen] = '\0');
    return result;
}

char* Strings_swapCase(char* s) {
    int slen = ((int)strlen(s));
    char* result = ((char*)malloc((slen + 1)));
    for (int i = 0; (i < slen); (i++)) {
        char c = s[i];
        if ((c >= 'A') && (c <= 'Z')) {
            (result[i] = ((char)tolower(((unsigned char)c))));
        } else if ((c >= 'a') && (c <= 'z')) {
            (result[i] = ((char)toupper(((unsigned char)c))));
        } else {
            (result[i] = c);
        }
    }
    (result[slen] = '\0');
    return result;
}

char* Strings_padLeft(char* s, int width, char fill) {
    int slen = ((int)strlen(s));
    if (slen >= width) {
        return Strings_copy(s);
    }
    int pad = (width - slen);
    char* result = ((char*)malloc((width + 1)));
    for (int i = 0; (i < pad); (i++)) {
        (result[i] = fill);
    }
    memcpy((result + pad), s, slen);
    (result[width] = '\0');
    return result;
}

char* Strings_padRight(char* s, int width, char fill) {
    int slen = ((int)strlen(s));
    if (slen >= width) {
        return Strings_copy(s);
    }
    int pad = (width - slen);
    char* result = ((char*)malloc((width + 1)));
    memcpy(result, s, slen);
    for (int i = 0; (i < pad); (i++)) {
        (result[(slen + i)] = fill);
    }
    (result[width] = '\0');
    return result;
}

char* Strings_center(char* s, int width, char fill) {
    int slen = ((int)strlen(s));
    if (slen >= width) {
        return Strings_copy(s);
    }
    int total_pad = (width - slen);
    int left_pad = __btrc_div_int(total_pad, 2);
    int right_pad = (total_pad - left_pad);
    char* result = ((char*)malloc((width + 1)));
    for (int i = 0; (i < left_pad); (i++)) {
        (result[i] = fill);
    }
    memcpy((result + left_pad), s, slen);
    for (int i = 0; (i < right_pad); (i++)) {
        (result[((left_pad + slen) + i)] = fill);
    }
    (result[width] = '\0');
    return result;
}

char* Strings_lstrip(char* s) {
    int slen = ((int)strlen(s));
    int start = 0;
    while ((start < slen) && ((((s[start] == ' ') || (s[start] == '\t')) || (s[start] == '\n')) || (s[start] == '\r'))) {
        (start++);
    }
    int newlen = (slen - start);
    char* result = ((char*)malloc((newlen + 1)));
    memcpy(result, (s + start), newlen);
    (result[newlen] = '\0');
    return result;
}

char* Strings_rstrip(char* s) {
    int slen = ((int)strlen(s));
    int end = slen;
    while ((end > 0) && ((((s[(end - 1)] == ' ') || (s[(end - 1)] == '\t')) || (s[(end - 1)] == '\n')) || (s[(end - 1)] == '\r'))) {
        (end--);
    }
    char* result = ((char*)malloc((end + 1)));
    memcpy(result, s, end);
    (result[end] = '\0');
    return result;
}

char* Strings_removePrefix(char* s, char* prefix) {
    if (!__btrc_startsWith(s, prefix)) {
        return Strings_copy(s);
    }
    return __btrc_str_track(__btrc_substring(s, ((int)strlen(prefix)), (((int)strlen(s)) - ((int)strlen(prefix)))));
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

bool Strings_isDigitStr(char* s) {
    int slen = ((int)strlen(s));
    if (slen == 0) {
        return false;
    }
    for (int i = 0; (i < slen); (i++)) {
        if ((s[i] < '0') || (s[i] > '9')) {
            return false;
        }
    }
    return true;
}

bool Strings_isAlphaStr(char* s) {
    int slen = ((int)strlen(s));
    if (slen == 0) {
        return false;
    }
    for (int i = 0; (i < slen); (i++)) {
        char c = s[i];
        if (!(((c >= 'a') && (c <= 'z')) || ((c >= 'A') && (c <= 'Z')))) {
            return false;
        }
    }
    return true;
}

bool Strings_isBlank(char* s) {
    int slen = ((int)strlen(s));
    for (int i = 0; (i < slen); (i++)) {
        char c = s[i];
        if ((((c != ' ') && (c != '\t')) && (c != '\n')) && (c != '\r')) {
            return false;
        }
    }
    return true;
}

void Console_init(Console* self) {
    self->__rc = 1;
}

Console* Console_new(void) {
    Console* self = ((Console*)malloc(sizeof(Console)));
    memset(self, 0, sizeof(Console));
    Console_init(self);
    return self;
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

void Console_write(char* msg) {
    printf("%s", msg);
}

void Console_writeLine(char* msg) {
    printf("%s\n", msg);
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
    return self->is_open;
}

char* File_read(File* self) {
    if (!self->is_open) {
        return "";
    }
    fseek(self->handle, 0, SEEK_END);
    long size = ftell(self->handle);
    fseek(self->handle, 0, SEEK_SET);
    char* buf = ((char*)malloc((size + 1)));
    long n = ((long)fread(buf, 1, size, self->handle));
    (buf[n] = '\0');
    return buf;
}

char* File_readLine(File* self) {
    if (!self->is_open) {
        return "";
    }
    char buf[4096];
    if (fgets(buf, 4096, self->handle) != NULL) {
        int len = ((int)strlen(buf));
        if ((len > 0) && (buf[(len - 1)] == '\n')) {
            (buf[(len - 1)] = '\0');
        }
        return Strings_copy(buf);
    }
    return "";
}

btrc_Vector_string* File_readLines(File* self) {
    btrc_Vector_string* lines = btrc_Vector_string_new();
    if (!self->is_open) {
        return lines;
    }
    char buf[4096];
    while (fgets(buf, 4096, self->handle) != NULL) {
        int len = ((int)strlen(buf));
        if ((len > 0) && (buf[(len - 1)] == '\n')) {
            (buf[(len - 1)] = '\0');
        }
        btrc_Vector_string_push(lines, Strings_copy(buf));
    }
    return lines;
}

void File_setHandle(File* self, FILE* h) {
    (self->handle = h);
    (self->is_open = true);
}

void File_write(File* self, char* text) {
    if (!self->is_open) {
        return;
    }
    fputs(text, self->handle);
}

void File_writeLine(File* self, char* text) {
    if (!self->is_open) {
        return;
    }
    fputs(text, self->handle);
    fputc('\n', self->handle);
}

void File_close(File* self) {
    if (self->is_open) {
        if (((int)strlen(self->path)) > 0) {
            fclose(self->handle);
        }
        (self->is_open = false);
    }
}

bool File_eof(File* self) {
    if (!self->is_open) {
        return true;
    }
    return (feof(self->handle) != 0);
}

void File_flush(File* self) {
    if (self->is_open) {
        fflush(self->handle);
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

bool Path_exists(char* path) {
    FILE* f = fopen(path, "r");
    if (f != NULL) {
        fclose(f);
        return true;
    }
    return false;
}

char* Path_readAll(char* path) {
    File* f = File_new(path, "r");
    if (!File_ok(f)) {
        if (f != NULL) {
            if ((--f->__rc) <= 0) {
                File_destroy(f);
            }
        }
        return "";
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

UnixPlatform* UnixPlatform_new(void) {
    UnixPlatform* self = ((UnixPlatform*)malloc(sizeof(UnixPlatform)));
    memset(self, 0, sizeof(UnixPlatform));
    UnixPlatform_init(self);
    return self;
}

void UnixPlatform_destroy(UnixPlatform* self) {
    free(self);
}

int UnixPlatform_pid(void) {
    return ((int)getpid());
}

int UnixPlatform_euid(void) {
    return ((int)geteuid());
}

void Platform_init(Platform* self) {
    self->__rc = 1;
}

Platform* Platform_new(void) {
    Platform* self = ((Platform*)malloc(sizeof(Platform)));
    memset(self, 0, sizeof(Platform));
    Platform_init(self);
    return self;
}

void Platform_destroy(Platform* self) {
    free(self);
}

bool Platform_isUnix(void) {
    return true;
}

bool Platform_isWindows(void) {
    return false;
}

char* Platform_pathSeparator(void) {
    return "/";
}

int Platform_pid(void) {
    return UnixPlatform_pid();
}

int Platform_euid(void) {
    return UnixPlatform_euid();
}

bool Platform_isRoot(void) {
    return (Platform_euid() == 0);
}

void Environment_init(Environment* self) {
    self->__rc = 1;
}

Environment* Environment_new(void) {
    Environment* self = ((Environment*)malloc(sizeof(Environment)));
    memset(self, 0, sizeof(Environment));
    Environment_init(self);
    return self;
}

void Environment_destroy(Environment* self) {
    free(self);
}

char* Environment_get(char* name, char* fallback) {
    char* value = getenv(name);
    if ((value == NULL) || __btrc_isEmpty(value)) {
        return fallback;
    }
    return Strings_copy(value);
}

bool Environment_has(char* name) {
    char* value = getenv(name);
    return ((value != NULL) && (!__btrc_isEmpty(value)));
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
        return 127;
    }
    if (self->raw > 255) {
        return __btrc_div_int(self->raw, 256);
    }
    return self->raw;
}

bool ProcessStatus_ok(ProcessStatus* self) {
    return (ProcessStatus_code(self) == 0);
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
    return (self->handle != NULL);
}

char* UnixPipe_readAll(UnixPipe* self) {
    if (!UnixPipe_ok(self)) {
        return "";
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
        return ProcessStatus_new((-1));
    }
    int raw = pclose(self->handle);
    (self->handle = NULL);
    return ProcessStatus_new(raw);
}

void UnixProcess_init(UnixProcess* self) {
    self->__rc = 1;
}

UnixProcess* UnixProcess_new(void) {
    UnixProcess* self = ((UnixProcess*)malloc(sizeof(UnixProcess)));
    memset(self, 0, sizeof(UnixProcess));
    UnixProcess_init(self);
    return self;
}

void UnixProcess_destroy(UnixProcess* self) {
    free(self);
}

ProcessStatus* UnixProcess_system(char* command) {
    return ProcessStatus_new(system(command));
}

UnixPipe* UnixProcess_pipe(char* command) {
    return UnixPipe_new(command);
}

void ShellWords_init(ShellWords* self) {
    self->__rc = 1;
}

ShellWords* ShellWords_new(void) {
    ShellWords* self = ((ShellWords*)malloc(sizeof(ShellWords)));
    memset(self, 0, sizeof(ShellWords));
    ShellWords_init(self);
    return self;
}

void ShellWords_destroy(ShellWords* self) {
    free(self);
}

bool ShellWords_isSafeArgChar(char c) {
    if ((c >= 'a') && (c <= 'z')) {
        return true;
    }
    if ((c >= 'A') && (c <= 'Z')) {
        return true;
    }
    if ((c >= '0') && (c <= '9')) {
        return true;
    }
    return ((((((((c == '_') || (c == '-')) || (c == '.')) || (c == '/')) || (c == ':')) || (c == '=')) || (c == ',')) || (c == '+'));
}

bool ShellWords_isSafeArg(char* raw) {
    int len = ((int)strlen(raw));
    if (len == 0) {
        return false;
    }
    for (int __i_2 = 0; (raw[__i_2] != '\0'); (__i_2++)) {
        char ch = raw[__i_2];
        if (!ShellWords_isSafeArgChar(ch)) {
            return false;
        }
    }
    return true;
}

char* ShellWords_quote(char* raw) {
    if (ShellWords_isSafeArg(raw)) {
        return Strings_copy(raw);
    }
    char* escaped = Strings_replace(raw, "'", "'\\''");
    return __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("'", escaped)), "'"));
}

char* ShellWords_redact(char* text, char* sensitive) {
    if (__btrc_isEmpty(sensitive)) {
        return text;
    }
    return Strings_replace(text, sensitive, "***");
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
    return (self->code == 0);
}

char* ExecResult_stdout(ExecResult* self) {
    return self->out;
}

char* ExecResult_stderr(ExecResult* self) {
    return self->err;
}

void Command_init(Command* self, char* executable) {
    self->__rc = 1;
    (self->executable = executable);
    if (self->args != NULL) {
        if ((--self->args->__rc) <= 0) {
            btrc_Vector_string_free(self->args);
        }
    }
    btrc_Vector_string* __list_4 = btrc_Vector_string_new();
    (self->args = __list_4);
    btrc_Vector_string* __list_3 = btrc_Vector_string_new();
    (__list_3->__rc++);
    if (self->env != NULL) {
        if ((--self->env->__rc) <= 0) {
            btrc_Vector_string_free(self->env);
        }
    }
    btrc_Vector_string* __list_6 = btrc_Vector_string_new();
    (self->env = __list_6);
    btrc_Vector_string* __list_5 = btrc_Vector_string_new();
    (__list_5->__rc++);
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
    return self;
}

Command* Command_flag(Command* self, char* name, char* value) {
    btrc_Vector_string_push(self->args, name);
    btrc_Vector_string_push(self->args, value);
    return self;
}

Command* Command_envVar(Command* self, char* name, char* value) {
    int __fstr_8_len = snprintf(NULL, 0, "%s=%s", name, value);
    char* __fstr_8_buf = __btrc_str_track(((char*)malloc((__fstr_8_len + 1))));
    snprintf(__fstr_8_buf, (__fstr_8_len + 1), "%s=%s", name, value);
    btrc_Vector_string_push(self->env, __fstr_8_buf);
    return self;
}

Command* Command_sudo(Command* self, bool enabled) {
    (self->useSudo = enabled);
    return self;
}

Command* Command_capture(Command* self, bool enabled) {
    (self->captureOutput = enabled);
    return self;
}

Command* Command_check(Command* self, bool enabled) {
    (self->checkStatus = enabled);
    return self;
}

Command* Command_mergeError(Command* self, bool enabled) {
    (self->mergeStderr = enabled);
    return self;
}

Command* Command_redact(Command* self, char* value) {
    (self->sensitive = value);
    return self;
}

char* Command_renderEnv(Command* self, char* item) {
    int split = Strings_find(item, "=", 0);
    if (split <= 0) {
        return ShellWords_quote(item);
    }
    char* name = __btrc_str_track(__btrc_substring(item, 0, split));
    char* value = __btrc_str_track(__btrc_substring(item, (split + 1), ((((int)strlen(item)) - split) - 1)));
    return __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(name, "=")), ShellWords_quote(value)));
}

char* Command_render(Command* self) {
    btrc_Vector_string* parts = btrc_Vector_string_new();
    int __n_10 = btrc_Vector_string_iterLen(self->env);
    for (int __i_9 = 0; (__i_9 < __n_10); (__i_9++)) {
        char* item = btrc_Vector_string_iterGet(self->env, __i_9);
        btrc_Vector_string_push(parts, Command_renderEnv(self, item));
    }
    if (self->useSudo) {
        btrc_Vector_string_push(parts, "sudo");
    }
    btrc_Vector_string_push(parts, ShellWords_quote(self->executable));
    int __n_12 = btrc_Vector_string_iterLen(self->args);
    for (int __i_11 = 0; (__i_11 < __n_12); (__i_11++)) {
        char* item = btrc_Vector_string_iterGet(self->args, __i_11);
        btrc_Vector_string_push(parts, ShellWords_quote(item));
    }
    if (self->mergeStderr) {
        btrc_Vector_string_push(parts, "2>&1");
    }
    return btrc_Vector_string_join(parts, " ");
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
    return ShellWords_quote(raw);
}

char* UnixShell_redactText(char* text, char* sensitive) {
    return ShellWords_redact(text, sensitive);
}

int UnixShell_statusCode(int rawStatus) {
    ProcessStatus* status = ProcessStatus_new(rawStatus);
    int code = ProcessStatus_code(status);
    if (status != NULL) {
        if ((--status->__rc) <= 0) {
            ProcessStatus_destroy(status);
        }
    }
    return code;
    if (status != NULL) {
        if ((--status->__rc) <= 0) {
            ProcessStatus_destroy(status);
        }
    }
}

void UnixShell_chroot(UnixShell* self, char* path) {
    (self->chrootPath = path);
}

void UnixShell_clearChroot(UnixShell* self) {
    (self->chrootPath = "");
}

ExecResult* UnixShell_run(UnixShell* self, char* command) {
    return UnixShell_runRaw(self, command, true, true, "");
}

ExecResult* UnixShell_runUnchecked(UnixShell* self, char* command) {
    return UnixShell_runRaw(self, command, true, false, "");
}

ExecResult* UnixShell_runCommand(UnixShell* self, Command* command) {
    return UnixShell_runRaw(self, Command_render(command), command->captureOutput, command->checkStatus, command->sensitive);
}

ExecResult* UnixShell_runRaw(UnixShell* self, char* command, bool captureOutput, bool checkStatus, char* sensitive) {
    char* rendered = command;
    if (((int)strlen(self->chrootPath)) > 0) {
        int __fstr_13_len = snprintf(NULL, 0, "nixos-enter --root %s --command %s", ShellWords_quote(self->chrootPath), ShellWords_quote(command));
        char* __fstr_13_buf = __btrc_str_track(((char*)malloc((__fstr_13_len + 1))));
        snprintf(__fstr_13_buf, (__fstr_13_len + 1), "nixos-enter --root %s --command %s", ShellWords_quote(self->chrootPath), ShellWords_quote(command));
        (rendered = __fstr_13_buf);
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
        return ExecResult_new(code, "", "", rendered);
    }
    UnixPipe* pipe = UnixProcess_pipe(rendered);
    if (!UnixPipe_ok(pipe)) {
        return ExecResult_new(127, "", "popen failed", rendered);
    }
    char* output = UnixPipe_readAll(pipe);
    ProcessStatus* status = UnixPipe_close(pipe);
    int code = ProcessStatus_code(status);
    if (checkStatus && (code != 0)) {
        fprintf(stderr, "Command failed (%d): %s\n", code, UnixShell_redactText(rendered, sensitive));
    }
    return ExecResult_new(code, output, "", rendered);
}

void PowerShell_init(PowerShell* self) {
    self->__rc = 1;
}

PowerShell* PowerShell_new(void) {
    PowerShell* self = ((PowerShell*)malloc(sizeof(PowerShell)));
    memset(self, 0, sizeof(PowerShell));
    PowerShell_init(self);
    return self;
}

void PowerShell_destroy(PowerShell* self) {
    free(self);
}

ExecResult* PowerShell_run(PowerShell* self, char* command) {
    return ExecResult_new(127, "", "PowerShell support is TODO", command);
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
    return self->found;
}

bool FileStatus_isDir(FileStatus* self) {
    return (self->found && S_ISDIR(self->mode));
}

bool FileStatus_isFile(FileStatus* self) {
    return (self->found && S_ISREG(self->mode));
}

bool FileStatus_isSymlink(FileStatus* self) {
    return (self->linkFound && S_ISLNK(self->linkMode));
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

UnixFileSystem* UnixFileSystem_new(void) {
    UnixFileSystem* self = ((UnixFileSystem*)malloc(sizeof(UnixFileSystem)));
    memset(self, 0, sizeof(UnixFileSystem));
    UnixFileSystem_init(self);
    return self;
}

void UnixFileSystem_destroy(UnixFileSystem* self) {
    free(self);
}

int UnixFileSystem_statusCode(int raw) {
    if (raw == (-1)) {
        return 127;
    }
    if (raw > 255) {
        return __btrc_div_int(raw, 256);
    }
    return raw;
}

int UnixFileSystem_chmodPath(char* path, int mode) {
    return chmod(path, ((mode_t)mode));
}

int UnixFileSystem_mkdirPath(char* path, int mode) {
    return mkdir(path, ((mode_t)mode));
}

int UnixFileSystem_runShell(char* command) {
    return UnixFileSystem_statusCode(system(command));
}

int UnixFileSystem_mkdirp(char* path) {
    char* quoted = PathTools_shellQuote(path);
    int __fstr_14_len = snprintf(NULL, 0, "mkdir -p %s", quoted);
    char* __fstr_14_buf = __btrc_str_track(((char*)malloc((__fstr_14_len + 1))));
    snprintf(__fstr_14_buf, (__fstr_14_len + 1), "mkdir -p %s", quoted);
    return UnixFileSystem_runShell(__fstr_14_buf);
}

int UnixFileSystem_removeRecursive(char* path) {
    char* quoted = PathTools_shellQuote(path);
    int __fstr_15_len = snprintf(NULL, 0, "rm -rf %s", quoted);
    char* __fstr_15_buf = __btrc_str_track(((char*)malloc((__fstr_15_len + 1))));
    snprintf(__fstr_15_buf, (__fstr_15_len + 1), "rm -rf %s", quoted);
    return UnixFileSystem_runShell(__fstr_15_buf);
}

int UnixFileSystem_symlinkPath(char* target, char* linkPath) {
    return symlink(target, linkPath);
}

char* UnixFileSystem_readLink(char* path) {
    char buffer[4096];
    ssize_t length = readlink(path, buffer, 4095);
    if (length < 0) {
        return "";
    }
    (buffer[length] = '\0');
    return Strings_copy(buffer);
}

char* UnixFileSystem_tempDir(char* prefix) {
    char* base = Environment_get("TMPDIR", "/tmp");
    char* templatePath = PathTools_join(base, __btrc_str_track(__btrc_strcat(prefix, ".XXXXXX")));
    char* raw = Strings_copy(templatePath);
    char* result = mkdtemp(raw);
    if (result == NULL) {
        return "";
    }
    return Strings_copy(result);
}

void PathTools_init(PathTools* self) {
    self->__rc = 1;
}

PathTools* PathTools_new(void) {
    PathTools* self = ((PathTools*)malloc(sizeof(PathTools)));
    memset(self, 0, sizeof(PathTools));
    PathTools_init(self);
    return self;
}

void PathTools_destroy(PathTools* self) {
    free(self);
}

char* PathTools_shellQuote(char* raw) {
    return ShellWords_quote(raw);
}

char* PathTools_basename(char* path) {
    int len = ((int)strlen(path));
    if (len == 0) {
        return "";
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
        return ".";
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
            return "/";
        }
        return ".";
    }
    char* result = ((char*)malloc((end + 1)));
    memcpy(result, path, end);
    (result[end] = '\0');
    return result;
}

char* PathTools_join(char* left, char* right) {
    if (((int)strlen(left)) == 0) {
        return Strings_copy(right);
    }
    if (((int)strlen(right)) == 0) {
        return Strings_copy(left);
    }
    if (left[(((int)strlen(left)) - 1)] == '/') {
        int __fstr_16_len = snprintf(NULL, 0, "%s%s", left, right);
        char* __fstr_16_buf = __btrc_str_track(((char*)malloc((__fstr_16_len + 1))));
        snprintf(__fstr_16_buf, (__fstr_16_len + 1), "%s%s", left, right);
        return __fstr_16_buf;
    }
    int __fstr_17_len = snprintf(NULL, 0, "%s/%s", left, right);
    char* __fstr_17_buf = __btrc_str_track(((char*)malloc((__fstr_17_len + 1))));
    snprintf(__fstr_17_buf, (__fstr_17_len + 1), "%s/%s", left, right);
    return __fstr_17_buf;
}

void FileSystem_init(FileSystem* self) {
    self->__rc = 1;
}

FileSystem* FileSystem_new(void) {
    FileSystem* self = ((FileSystem*)malloc(sizeof(FileSystem)));
    memset(self, 0, sizeof(FileSystem));
    FileSystem_init(self);
    return self;
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

bool FileSystem_isFile(char* path) {
    FileStatus* status = FileStatus_new(path);
    bool result = FileStatus_isFile(status);
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
    return UnixFileSystem_chmodPath(path, mode);
}

int FileSystem_mkdir(char* path, int mode) {
    return UnixFileSystem_mkdirPath(path, mode);
}

int FileSystem_mkdirp(char* path) {
    return UnixFileSystem_mkdirp(path);
}

int FileSystem_removeRecursive(char* path) {
    return UnixFileSystem_removeRecursive(path);
}

int FileSystem_symlink(char* target, char* linkPath) {
    return UnixFileSystem_symlinkPath(target, linkPath);
}

char* FileSystem_readLink(char* path) {
    return UnixFileSystem_readLink(path);
}

char* FileSystem_tempDir(char* prefix) {
    return UnixFileSystem_tempDir(prefix);
}

btrc_Vector_string* FileSystem_listDir(char* path) {
    Directory* dir = Directory_new(path);
    btrc_Vector_string* result = Directory_entries(dir);
    if (dir != NULL) {
        if ((--dir->__rc) <= 0) {
            Directory_destroy(dir);
        }
    }
    return result;
    if (dir != NULL) {
        if ((--dir->__rc) <= 0) {
            Directory_destroy(dir);
        }
    }
}

char* FileSystem_readText(char* path) {
    return Path_readAll(path);
}

void FileSystem_writeText(char* path, char* content) {
    Path_writeAll(path, content);
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
        return "";
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

bool JsonObject_has(JsonObject* self, char* key) {
    return btrc_Map_string_string_has(self->values, key);
}

char* JsonObject_getString(JsonObject* self, char* key, char* fallback) {
    if (!btrc_Map_string_string_has(self->values, key)) {
        return fallback;
    }
    return btrc_Map_string_string_get(self->values, key);
}

bool JsonObject_getBool(JsonObject* self, char* key, bool fallback) {
    if (!btrc_Map_string_string_has(self->values, key)) {
        return fallback;
    }
    char* value = btrc_Map_string_string_get(self->values, key);
    if (strcmp(value, "true") == 0) {
        return true;
    }
    if (strcmp(value, "false") == 0) {
        return false;
    }
    return fallback;
}

int JsonObject_getInt(JsonObject* self, char* key, int fallback) {
    if (!btrc_Map_string_string_has(self->values, key)) {
        return fallback;
    }
    return Strings_toInt(btrc_Map_string_string_get(self->values, key));
}

char* JsonObject_stringify(JsonObject* self) {
    btrc_Vector_string* fields = btrc_Vector_string_new();
    int __n_19 = btrc_Map_string_string_iterLen(self->values);
    for (int __i_18 = 0; (__i_18 < __n_19); (__i_18++)) {
        char* key = btrc_Map_string_string_iterGet(self->values, __i_18);
        char* value = btrc_Map_string_string_iterValueAt(self->values, __i_18);
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
    return __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("{", btrc_Vector_string_join(fields, ","))), "}"));
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
    return JsonObject_parse(Path_readAll(path));
}

void JsonObject_writeFile(JsonObject* self, char* path) {
    Path_writeAll(path, JsonObject_stringify(self));
}

void UnixPattern_init(UnixPattern* self) {
    self->__rc = 1;
}

UnixPattern* UnixPattern_new(void) {
    UnixPattern* self = ((UnixPattern*)malloc(sizeof(UnixPattern)));
    memset(self, 0, sizeof(UnixPattern));
    UnixPattern_init(self);
    return self;
}

void UnixPattern_destroy(UnixPattern* self) {
    free(self);
}

bool UnixPattern_matches(char* pattern, char* text) {
    return (fnmatch(pattern, text, 0) == 0);
}

void Pattern_init(Pattern* self) {
    self->__rc = 1;
}

Pattern* Pattern_new(void) {
    Pattern* self = ((Pattern*)malloc(sizeof(Pattern)));
    memset(self, 0, sizeof(Pattern));
    Pattern_init(self);
    return self;
}

void Pattern_destroy(Pattern* self) {
    free(self);
}

bool Pattern_matches(char* pattern, char* text) {
    return UnixPattern_matches(pattern, text);
}

bool Pattern_anyMatches(btrc_Vector_string* patterns, char* text) {
    int __n_21 = btrc_Vector_string_iterLen(patterns);
    for (int __i_20 = 0; (__i_20 < __n_21); (__i_20++)) {
        char* pattern = btrc_Vector_string_iterGet(patterns, __i_20);
        if (Pattern_matches(pattern, text)) {
            return true;
        }
    }
    return false;
}

void Math_init(Math* self) {
    self->__rc = 1;
}

Math* Math_new(void) {
    Math* self = ((Math*)malloc(sizeof(Math)));
    memset(self, 0, sizeof(Math));
    Math_init(self);
    return self;
}

void Math_destroy(Math* self) {
    free(self);
}

float Math_PI(void) {
    return 3.14159265358979323846f;
}

float Math_E(void) {
    return 2.71828182845904523536f;
}

float Math_TAU(void) {
    return 6.28318530717958647692f;
}

float Math_INF(void) {
    float zero = 0.0f;
    return __btrc_div_double(1.0f, zero);
}

int Math_abs(int x) {
    if (x < 0) {
        return (-x);
    }
    return x;
}

float Math_fabs(float x) {
    if (x < 0.0f) {
        return (-x);
    }
    return x;
}

int Math_max(int a, int b) {
    if (a > b) {
        return a;
    }
    return b;
}

int Math_min(int a, int b) {
    if (a < b) {
        return a;
    }
    return b;
}

float Math_fmax(float a, float b) {
    if (a > b) {
        return a;
    }
    return b;
}

float Math_fmin(float a, float b) {
    if (a < b) {
        return a;
    }
    return b;
}

int Math_clamp(int x, int lo, int hi) {
    if (x < lo) {
        return lo;
    }
    if (x > hi) {
        return hi;
    }
    return x;
}

float Math_power(float base, int exp) {
    float result = 1.0f;
    bool negative = false;
    if (exp < 0) {
        (negative = true);
        (exp = (-exp));
    }
    for (int i = 0; (i < exp); (i++)) {
        (result = (result * base));
    }
    if (negative) {
        return __btrc_div_double(1.0f, result);
    }
    return result;
}

float Math_sqrt(float x) {
    return sqrt(x);
}

int Math_factorial(int n) {
    if (n <= 1) {
        return 1;
    }
    return (n * Math_factorial((n - 1)));
}

int Math_gcd(int a, int b) {
    while (b != 0) {
        int temp = b;
        (b = __btrc_mod_int(a, b));
        (a = temp);
    }
    return a;
}

int Math_lcm(int a, int b) {
    return (__btrc_div_int(Math_abs(a), Math_gcd(a, b)) * Math_abs(b));
}

int Math_fibonacci(int n) {
    if (n <= 0) {
        return 0;
    }
    if (n == 1) {
        return 1;
    }
    int a = 0;
    int b = 1;
    for (int i = 2; (i < (n + 1)); (i++)) {
        int temp = (a + b);
        (a = b);
        (b = temp);
    }
    return b;
}

bool Math_isPrime(int n) {
    if (n < 2) {
        return false;
    }
    if (n < 4) {
        return true;
    }
    if (__btrc_mod_int(n, 2) == 0) {
        return false;
    }
    int i = 3;
    while ((i * i) <= n) {
        if (__btrc_mod_int(n, i) == 0) {
            return false;
        }
        (i = (i + 2));
    }
    return true;
}

bool Math_isEven(int n) {
    return (__btrc_mod_int(n, 2) == 0);
}

bool Math_isOdd(int n) {
    return (__btrc_mod_int(n, 2) != 0);
}

int Math_sum(btrc_Vector_int* items) {
    int total = 0;
    for (int i = 0; (i < items->len); (i++)) {
        (total = (total + btrc_Vector_int_get(items, i)));
    }
    return total;
}

float Math_fsum(btrc_Vector_float* items) {
    float total = 0.0f;
    for (int i = 0; (i < items->len); (i++)) {
        (total = (total + btrc_Vector_float_get(items, i)));
    }
    return total;
}

float Math_sin(float x) {
    return sin(x);
}

float Math_cos(float x) {
    return cos(x);
}

float Math_tan(float x) {
    return tan(x);
}

float Math_asin(float x) {
    return asin(x);
}

float Math_acos(float x) {
    return acos(x);
}

float Math_atan(float x) {
    return atan(x);
}

float Math_atan2(float y, float x) {
    return atan2(y, x);
}

float Math_ceil(float x) {
    return ceil(x);
}

float Math_floor(float x) {
    return floor(x);
}

int Math_round(float x) {
    return ((int)round(x));
}

int Math_truncate(float x) {
    return ((int)trunc(x));
}

float Math_log(float x) {
    return log(x);
}

float Math_log10(float x) {
    return log10(x);
}

float Math_log2(float x) {
    return log2(x);
}

float Math_exp(float x) {
    return exp(x);
}

float Math_toRadians(float degrees) {
    return __btrc_div_double((degrees * 3.14159265358979323846f), 180.0f);
}

float Math_toDegrees(float radians) {
    return __btrc_div_double((radians * 180.0f), 3.14159265358979323846f);
}

float Math_fclamp(float val, float lo, float hi) {
    if (val < lo) {
        return lo;
    }
    if (val > hi) {
        return hi;
    }
    return val;
}

int Math_sign(int x) {
    if (x > 0) {
        return 1;
    }
    if (x < 0) {
        return (-1);
    }
    return 0;
}

float Math_fsign(float x) {
    if (x > 0.0f) {
        return 1.0f;
    }
    if (x < 0.0f) {
        return (-1.0f);
    }
    return 0.0f;
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

DateTime* DateTime_now(void) {
    time_t t = time(NULL);
    struct tm* tm = localtime((&t));
    return DateTime_new((tm->tm_year + 1900), (tm->tm_mon + 1), tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
}

void DateTime_display(DateTime* self) {
    printf("%04d-%02d-%02d %02d:%02d:%02d", self->year, self->month, self->day, self->hour, self->minute, self->second);
}

char* DateTime_format(DateTime* self) {
    char buf[64];
    snprintf(buf, 64, "%04d-%02d-%02d %02d:%02d:%02d", self->year, self->month, self->day, self->hour, self->minute, self->second);
    return Strings_copy(buf);
}

char* DateTime_dateString(DateTime* self) {
    char buf[32];
    snprintf(buf, 32, "%04d-%02d-%02d", self->year, self->month, self->day);
    return Strings_copy(buf);
}

char* DateTime_timeString(DateTime* self) {
    char buf[32];
    snprintf(buf, 32, "%02d:%02d:%02d", self->hour, self->minute, self->second);
    return Strings_copy(buf);
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

void Timer_stop(Timer* self) {
    (self->end_time = clock());
    (self->running = false);
}

float Timer_elapsed(Timer* self) {
    clock_t end = (self->running ? clock() : self->end_time);
    return __btrc_div_double(((float)(end - self->start_time)), ((float)CLOCKS_PER_SEC));
}

void Timer_reset(Timer* self) {
    (self->start_time = 0);
    (self->end_time = 0);
    (self->running = false);
}

void Random_init(Random* self) {
    self->__rc = 1;
    (self->seeded = false);
}

Random* Random_new(void) {
    Random* self = ((Random*)malloc(sizeof(Random)));
    memset(self, 0, sizeof(Random));
    Random_init(self);
    return self;
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
    return (lo + (rand() % ((hi - lo) + 1)));
}

float Random_random(Random* self) {
    if (!self->seeded) {
        Random_seedTime(self);
    }
    return __btrc_div_double(((float)rand()), ((float)RAND_MAX));
}

float Random_uniform(Random* self, float lo, float hi) {
    return (lo + (Random_random(self) * (hi - lo)));
}

int Random_choice(Random* self, btrc_Vector_int* items) {
    int idx = Random_randint(self, 0, (items->len - 1));
    return btrc_Vector_int_get(items, idx);
}

void Random_shuffle(Random* self, btrc_Vector_int* items) {
    for (int i = (items->len - 1); (i > 0); (i--)) {
        int j = Random_randint(self, 0, i);
        int tmp = btrc_Vector_int_get(items, i);
        btrc_Vector_int_set(items, i, btrc_Vector_int_get(items, j));
        btrc_Vector_int_set(items, j, tmp);
    }
}

void Error_init(Error* self, char* message, int code) {
    self->__rc = 1;
    (self->message = message);
    (self->code = code);
}

Error* Error_new(char* message, int code) {
    Error* self = ((Error*)malloc(sizeof(Error)));
    memset(self, 0, sizeof(Error));
    Error_init(self, message, code);
    return self;
}

void Error_destroy(Error* self) {
    free(self);
}

char* Error_toString(Error* self) {
    return self->message;
}

void ValueError_init(ValueError* self, char* message) {
    self->__rc = 1;
    (self->message = message);
    (self->code = 1);
}

ValueError* ValueError_new(char* message) {
    ValueError* self = ((ValueError*)malloc(sizeof(ValueError)));
    memset(self, 0, sizeof(ValueError));
    ValueError_init(self, message);
    return self;
}

void ValueError_destroy(ValueError* self) {
    free(self);
}

char* ValueError_toString(ValueError* self) {
    return Error_toString(((Error*)self));
}

void IOError_init(IOError* self, char* message) {
    self->__rc = 1;
    (self->message = message);
    (self->code = 2);
}

IOError* IOError_new(char* message) {
    IOError* self = ((IOError*)malloc(sizeof(IOError)));
    memset(self, 0, sizeof(IOError));
    IOError_init(self, message);
    return self;
}

void IOError_destroy(IOError* self) {
    free(self);
}

char* IOError_toString(IOError* self) {
    return Error_toString(((Error*)self));
}

void TypeError_init(TypeError* self, char* message) {
    self->__rc = 1;
    (self->message = message);
    (self->code = 3);
}

TypeError* TypeError_new(char* message) {
    TypeError* self = ((TypeError*)malloc(sizeof(TypeError)));
    memset(self, 0, sizeof(TypeError));
    TypeError_init(self, message);
    return self;
}

void TypeError_destroy(TypeError* self) {
    free(self);
}

char* TypeError_toString(TypeError* self) {
    return Error_toString(((Error*)self));
}

void IndexError_init(IndexError* self, char* message) {
    self->__rc = 1;
    (self->message = message);
    (self->code = 4);
}

IndexError* IndexError_new(char* message) {
    IndexError* self = ((IndexError*)malloc(sizeof(IndexError)));
    memset(self, 0, sizeof(IndexError));
    IndexError_init(self, message);
    return self;
}

void IndexError_destroy(IndexError* self) {
    free(self);
}

char* IndexError_toString(IndexError* self) {
    return Error_toString(((Error*)self));
}

void KeyError_init(KeyError* self, char* message) {
    self->__rc = 1;
    (self->message = message);
    (self->code = 5);
}

KeyError* KeyError_new(char* message) {
    KeyError* self = ((KeyError*)malloc(sizeof(KeyError)));
    memset(self, 0, sizeof(KeyError));
    KeyError_init(self, message);
    return self;
}

void KeyError_destroy(KeyError* self) {
    free(self);
}

char* KeyError_toString(KeyError* self) {
    return Error_toString(((Error*)self));
}

void CliArgs_init(CliArgs* self, int argc, char** argv) {
    self->__rc = 1;
    (self->program = ((argc > 0) ? Strings_copy(argv[0]) : ""));
    if (self->values != NULL) {
        if ((--self->values->__rc) <= 0) {
            btrc_Vector_string_free(self->values);
        }
    }
    btrc_Vector_string* __list_23 = btrc_Vector_string_new();
    (self->values = __list_23);
    btrc_Vector_string* __list_22 = btrc_Vector_string_new();
    (__list_22->__rc++);
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
    return self->values->len;
}

char* CliArgs_get(CliArgs* self, int index) {
    return btrc_Vector_string_get(self->values, index);
}

char* CliArgs_command(CliArgs* self) {
    if (self->values->len == 0) {
        return "";
    }
    return btrc_Vector_string_get(self->values, 0);
}

bool CliArgs_has(CliArgs* self, char* flag) {
    int __n_25 = btrc_Vector_string_iterLen(self->values);
    for (int __i_24 = 0; (__i_24 < __n_25); (__i_24++)) {
        char* value = btrc_Vector_string_iterGet(self->values, __i_24);
        if (strcmp(value, flag) == 0) {
            return true;
        }
    }
    return false;
}

char* CliArgs_valueAfter(CliArgs* self, char* flag, char* fallback) {
    for (int i = 0; (i < (self->values->len - 1)); (i++)) {
        if (strcmp(btrc_Vector_string_get(self->values, i), flag) == 0) {
            return btrc_Vector_string_get(self->values, (i + 1));
        }
    }
    return fallback;
}

bool CliArgs_commandIs(CliArgs* self, char* name) {
    return (strcmp(CliArgs_command(self), name) == 0);
}

char* CliArgs_valueAfterPrefix(CliArgs* self, char* prefix, char* fallback) {
    int __n_27 = btrc_Vector_string_iterLen(self->values);
    for (int __i_26 = 0; (__i_26 < __n_27); (__i_26++)) {
        char* value = btrc_Vector_string_iterGet(self->values, __i_26);
        if (__btrc_startsWith(value, prefix)) {
            return __btrc_str_track(__btrc_substring(value, ((int)strlen(prefix)), (((int)strlen(value)) - ((int)strlen(prefix)))));
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
    btrc_Vector_string* __list_29 = btrc_Vector_string_new();
    (self->aliases = __list_29);
    btrc_Vector_string* __list_28 = btrc_Vector_string_new();
    (__list_28->__rc++);
}

CliCommand* CliCommand_new(char* name) {
    CliCommand* self = ((CliCommand*)malloc(sizeof(CliCommand)));
    memset(self, 0, sizeof(CliCommand));
    CliCommand_init(self, name);
    return self;
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

void CliCommand_alias(CliCommand* self, char* name) {
    btrc_Vector_string_push(self->aliases, name);
}

bool CliCommand_matches(CliCommand* self, char* value) {
    if (strcmp(self->name, value) == 0) {
        return true;
    }
    int __n_31 = btrc_Vector_string_iterLen(self->aliases);
    for (int __i_30 = 0; (__i_30 < __n_31); (__i_30++)) {
        char* alias = btrc_Vector_string_iterGet(self->aliases, __i_30);
        if (strcmp(alias, value) == 0) {
            return true;
        }
    }
    return false;
}

void NixosLog_init(NixosLog* self) {
    self->__rc = 1;
}

NixosLog* NixosLog_new(void) {
    NixosLog* self = ((NixosLog*)malloc(sizeof(NixosLog)));
    memset(self, 0, sizeof(NixosLog));
    NixosLog_init(self);
    return self;
}

void NixosLog_destroy(NixosLog* self) {
    free(self);
}

char* NixosLog_gray(void) {
    return "\033[90m";
}

char* NixosLog_orange(void) {
    return "\033[38;5;208m";
}

char* NixosLog_red(void) {
    return "\033[31m";
}

char* NixosLog_reset(void) {
    return "\033[0m";
}

char* NixosLog_redact(char* text, char* sensitive) {
    if (__btrc_isEmpty(sensitive)) {
        return text;
    }
    return Strings_replace(text, sensitive, "***");
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

void MembraneConstants_init(MembraneConstants* self) {
    self->__rc = 1;
}

MembraneConstants* MembraneConstants_new(void) {
    MembraneConstants* self = ((MembraneConstants*)malloc(sizeof(MembraneConstants)));
    memset(self, 0, sizeof(MembraneConstants));
    MembraneConstants_init(self);
    return self;
}

void MembraneConstants_destroy(MembraneConstants* self) {
    free(self);
}

char* MembraneConstants_topBase(void) {
    return "/run/semipermeable-membrane-top";
}

char* MembraneConstants_dirs(void) {
    return "dirs";
}

char* MembraneConstants_meta(void) {
    return ".semipermeable_membrane";
}

char* MembraneConstants_files(void) {
    return "files";
}

char* MembraneConstants_next(void) {
    return "NEXT";
}

char* MembraneConstants_a(void) {
    return "A";
}

char* MembraneConstants_b(void) {
    return "B";
}

char* MembraneConstants_c(void) {
    return "C";
}

char* MembraneConstants_disabled(void) {
    return "disabled";
}

char* MembraneConstants_converge(void) {
    return "converge";
}

char* MembraneConstants_reset(void) {
    return "reset";
}

char* MembraneConstants_prepareOnly(void) {
    return "prepare-only";
}

char* MembraneConstants_snapshotOnly(void) {
    return "snapshot-only";
}

char* MembraneConstants_restoreA(void) {
    return "restore-a";
}

char* MembraneConstants_restoreB(void) {
    return "restore-b";
}

char* MembraneConstants_restoreC(void) {
    return "restore-c";
}

char* MembraneConstants_restorePrevious(void) {
    return "restore-previous";
}

char* MembraneConstants_restorePenultimate(void) {
    return "restore-penultimate";
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

MembranePaths* MembranePaths_new(void) {
    MembranePaths* self = ((MembranePaths*)malloc(sizeof(MembranePaths)));
    memset(self, 0, sizeof(MembranePaths));
    MembranePaths_init(self);
    return self;
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
    return __btrc_str_track(__btrc_substring(value, start, (end - start)));
}

char* MembranePaths_trimTrailingSlashes(char* raw) {
    char* value = __btrc_str_track(__btrc_trim(raw));
    int end = ((int)strlen(value));
    while ((end > 1) && (value[(end - 1)] == '/')) {
        (end--);
    }
    return __btrc_str_track(__btrc_substring(value, 0, end));
}

bool MembranePaths_validRel(char* path) {
    if (__btrc_isEmpty(path)) {
        return false;
    }
    if (__btrc_strContains(path, "\t") || __btrc_strContains(path, "!")) {
        return false;
    }
    int __n_33 = btrc_Vector_string_iterLen(Strings_split(path, "/"));
    for (int __i_32 = 0; (__i_32 < __n_33); (__i_32++)) {
        char* part = btrc_Vector_string_iterGet(Strings_split(path, "/"), __i_32);
        if ((__btrc_isEmpty(part) || (strcmp(part, ".") == 0)) || (strcmp(part, "..") == 0)) {
            return false;
        }
    }
    return true;
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
        return "/";
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
    return MembraneAbsPath_new(value, slash);
}

char* MembranePaths_relToMount(char* absPath, char* mountPoint) {
    if (strcmp(mountPoint, "/") == 0) {
        return __btrc_str_track(__btrc_substring(absPath, 1, (((int)strlen(absPath)) - 1)));
    }
    char* prefix = __btrc_str_track(__btrc_strcat(mountPoint, "/"));
    if (!__btrc_startsWith(absPath, prefix)) {
        NixosLog_fatal(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(absPath, " not under ")), mountPoint)));
    }
    return __btrc_str_track(__btrc_substring(absPath, ((int)strlen(prefix)), (((int)strlen(absPath)) - ((int)strlen(prefix)))));
}

char* MembranePaths_kind(char* raw) {
    char* value = __btrc_str_track(__btrc_trim(raw));
    if (strcmp(value, "auto") == 0) {
        return "auto";
    }
    if ((strcmp(value, "directory") == 0) || (strcmp(value, "dir") == 0)) {
        return "dir";
    }
    if (strcmp(value, "file") == 0) {
        return "file";
    }
    NixosLog_fatal(__btrc_str_track(__btrc_strcat("bad kind: ", raw)));
    return "auto";
}

bool MembranePaths_ancestor(char* parent, char* child) {
    return __btrc_startsWith(child, __btrc_str_track(__btrc_strcat(parent, "/")));
}

char* MembranePaths_key(char* absPath) {
    char* rel = __btrc_str_track(__btrc_substring(absPath, 1, (((int)strlen(absPath)) - 1)));
    return Strings_replace(rel, "/", "!");
}

char* MembranePaths_dirStore(char* persistRoot, char* absPath) {
    return PathTools_join(PathTools_join(persistRoot, MembraneConstants_dirs()), MembranePaths_key(absPath));
}

char* MembranePaths_fileStore(char* persistRoot, char* absPath) {
    char* root = PathTools_join(PathTools_join(persistRoot, MembraneConstants_meta()), MembraneConstants_files());
    return PathTools_join(root, MembranePaths_key(absPath));
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
    return __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("[", Strings_fromFloat(Timer_elapsed(self->timer)))), "s]"));
}

char* MembraneRun_path(MembraneRun* self, char* rel) {
    char* base = MembranePaths_trimTrailingSlashes(self->top);
    char* child = MembranePaths_trimSlashes(rel);
    if (__btrc_isEmpty(child)) {
        return base;
    }
    return PathTools_join(base, child);
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
        return ExecResult_new(0, "", "", rendered);
    }
    MembraneRun_log(self, rendered);
    return UnixShell_runCommand(self->shell, command);
}

ExecResult* MembraneRun_runRaw(MembraneRun* self, char* command, bool captureOutput, bool checkStatus) {
    if (self->dryRun) {
        MembraneRun_log(self, __btrc_str_track(__btrc_strcat("DRY ", command)));
        return ExecResult_new(0, "", "", command);
    }
    MembraneRun_log(self, command);
    return UnixShell_runRaw(self->shell, command, captureOutput, checkStatus, "");
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

void SemipermeableMembrane_init(SemipermeableMembrane* self, bool dryRun, bool assumeMounted) {
    self->__rc = 1;
    if (self->run != NULL) {
        if ((--self->run->__rc) <= 0) {
            MembraneRun_destroy(self->run);
        }
    }
    (self->run = MembraneRun_new(dryRun));
    (MembraneRun_new(dryRun)->__rc++);
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
    btrc_Vector_MembraneSpec* __list_35 = btrc_Vector_MembraneSpec_new();
    (self->specs = __list_35);
    btrc_Vector_MembraneSpec* __list_34 = btrc_Vector_MembraneSpec_new();
    (__list_34->__rc++);
    if (self->volumes != NULL) {
        if ((--self->volumes->__rc) <= 0) {
            btrc_Vector_MembraneVolume_free(self->volumes);
        }
    }
    btrc_Vector_MembraneVolume* __list_37 = btrc_Vector_MembraneVolume_new();
    (self->volumes = __list_37);
    btrc_Vector_MembraneVolume* __list_36 = btrc_Vector_MembraneVolume_new();
    (__list_36->__rc++);
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
    return ((((strcmp(value, "1") == 0) || (strcmp(value, "true") == 0)) || (strcmp(value, "yes") == 0)) || (strcmp(value, "on") == 0));
}

bool SemipermeableMembrane_exists(SemipermeableMembrane* self, char* path) {
    return (FileSystem_exists(path) || FileSystem_isSymlink(path));
}

bool SemipermeableMembrane_isSubvolume(SemipermeableMembrane* self, char* path) {
    if (!FileSystem_exists(path)) {
        return false;
    }
    Command* command = Command_check(Command_arg(Command_arg(Command_arg(Command_new("btrfs"), "subvolume"), "show"), path), false);
    ExecResult* result = UnixShell_runCommand(self->run->shell, command);
    return ExecResult_ok(result);
}

bool SemipermeableMembrane_isReadonly(SemipermeableMembrane* self, char* path) {
    Command* command = Command_check(Command_arg(Command_arg(Command_arg(Command_arg(Command_arg(Command_new("btrfs"), "property"), "get"), "-ts"), path), "ro"), false);
    ExecResult* result = UnixShell_runCommand(self->run->shell, command);
    return (ExecResult_ok(result) && __btrc_strContains(ExecResult_stdout(result), "ro=true"));
}

bool SemipermeableMembrane_mounted(SemipermeableMembrane* self, char* path) {
    Command* command = Command_check(Command_arg(Command_arg(Command_new("findmnt"), "--mountpoint"), path), false);
    ExecResult* result = UnixShell_runCommand(self->run->shell, command);
    return ExecResult_ok(result);
}

int SemipermeableMembrane_compareDeepestFirst(SemipermeableMembrane* self, char* left, char* right) {
    int leftDepth = Strings_count(left, "/");
    int rightDepth = Strings_count(right, "/");
    if (leftDepth != rightDepth) {
        return (rightDepth - leftDepth);
    }
    if (((int)strlen(left)) != ((int)strlen(right))) {
        return (((int)strlen(right)) - ((int)strlen(left)));
    }
    return Strings_compare(left, right);
}

void SemipermeableMembrane_pushChildSorted(SemipermeableMembrane* self, btrc_Vector_string* paths, char* path) {
    int index = 0;
    while ((index < paths->len) && (SemipermeableMembrane_compareDeepestFirst(self, btrc_Vector_string_get(paths, index), path) <= 0)) {
        (index++);
    }
    btrc_Vector_string_insert(paths, index, path);
}

btrc_Vector_string* SemipermeableMembrane_childSubvolumes(SemipermeableMembrane* self, char* path) {
    btrc_Vector_string* result = btrc_Vector_string_new();
    if (!FileSystem_isDir(path)) {
        return result;
    }
    Command* command = Command_arg(Command_arg(Command_arg(Command_arg(Command_new("btrfs"), "subvolume"), "list"), "-o"), path);
    ExecResult* listed = MembraneRun_runCommand(self->run, command);
    if (!ExecResult_ok(listed)) {
        MembraneRun_fatal(self->run, __btrc_str_track(__btrc_strcat("list ", path)));
    }
    int __n_39 = btrc_Vector_string_iterLen(Strings_split(ExecResult_stdout(listed), "\n"));
    for (int __i_38 = 0; (__i_38 < __n_39); (__i_38++)) {
        char* line = btrc_Vector_string_iterGet(Strings_split(ExecResult_stdout(listed), "\n"), __i_38);
        int marker = Strings_find(line, " path ", 0);
        if (marker < 0) {
            continue;
        }
        char* child = __btrc_str_track(__btrc_trim(__btrc_str_track(__btrc_substring(line, (marker + 6), (((int)strlen(line)) - (marker + 6))))));
        char* childPath = MembraneRun_path(self->run, child);
        if (MembranePaths_ancestor(path, childPath)) {
            SemipermeableMembrane_pushChildSorted(self, result, childPath);
        }
    }
    return result;
}

void SemipermeableMembrane_deleteSubvolume(SemipermeableMembrane* self, char* path) {
    if (!FileSystem_isDir(path)) {
        return;
    }
    btrc_Vector_string* children = SemipermeableMembrane_childSubvolumes(self, path);
    int __n_41 = btrc_Vector_string_iterLen(children);
    for (int __i_40 = 0; (__i_40 < __n_41); (__i_40++)) {
        char* child = btrc_Vector_string_iterGet(children, __i_40);
        Command* childCommand = Command_capture(Command_arg(Command_arg(Command_arg(Command_new("btrfs"), "subvolume"), "delete"), child), false);
        MembraneRun_requireCommand(self->run, childCommand);
    }
    Command* command = Command_capture(Command_arg(Command_arg(Command_arg(Command_new("btrfs"), "subvolume"), "delete"), path), false);
    MembraneRun_requireCommand(self->run, command);
}

void SemipermeableMembrane_makeSubvolume(SemipermeableMembrane* self, char* path) {
    MembraneRun_mkdirp(self->run, PathTools_dirname(path));
    Command* command = Command_capture(Command_arg(Command_arg(Command_arg(Command_new("btrfs"), "subvolume"), "create"), path), false);
    MembraneRun_requireCommand(self->run, command);
}

void SemipermeableMembrane_snapshot(SemipermeableMembrane* self, char* source, char* destination, bool readonly) {
    if (!FileSystem_isDir(source)) {
        MembraneRun_fatal(self->run, __btrc_str_track(__btrc_strcat("Source missing: ", source)));
    }
    SemipermeableMembrane_deleteSubvolume(self, destination);
    Command* command = Command_arg(Command_arg(Command_new("btrfs"), "subvolume"), "snapshot");
    if (readonly) {
        Command_arg(command, "-r");
    }
    Command_arg(command, source);
    Command_arg(command, destination);
    Command_capture(command, false);
    MembraneRun_requireCommand(self->run, command);
    if (!readonly) {
        Command* rw = Command_capture(Command_arg(Command_arg(Command_arg(Command_arg(Command_arg(Command_arg(Command_new("btrfs"), "property"), "set"), "-ts"), destination), "ro"), "false"), false);
        MembraneRun_requireCommand(self->run, rw);
    }
}

char* SemipermeableMembrane_sourcePath(SemipermeableMembrane* self, char* live, char* clean, char* relPath) {
    char* livePath = PathTools_join(live, relPath);
    if (SemipermeableMembrane_exists(self, livePath)) {
        return livePath;
    }
    char* cleanPath = PathTools_join(clean, relPath);
    if (SemipermeableMembrane_exists(self, cleanPath)) {
        return cleanPath;
    }
    return "";
}

void SemipermeableMembrane_copyPath(SemipermeableMembrane* self, char* source, char* destination) {
    MembraneRun_mkdirp(self->run, PathTools_dirname(destination));
    Command* command = Command_capture(Command_arg(Command_arg(Command_arg(Command_arg(Command_new("cp"), "--reflink=always"), "-a"), source), destination), false);
    MembraneRun_requireCommand(self->run, command);
}

void SemipermeableMembrane_copyDirectoryContents(SemipermeableMembrane* self, char* source, char* destination) {
    if (!FileSystem_isDir(source)) {
        return;
    }
    btrc_Vector_string* nested = SemipermeableMembrane_childSubvolumes(self, source);
    if (!btrc_Vector_string_isEmpty(nested)) {
        MembraneRun_fatal(self->run, __btrc_str_track(__btrc_strcat("nested subvolumes in persisted dir: ", btrc_Vector_string_join(nested, " "))));
    }
    MembraneRun_requireRaw(self->run, __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("cp --reflink=always -a ", UnixShell_quote(PathTools_join(source, ".")))), " ")), UnixShell_quote(destination))));
}

void SemipermeableMembrane_ensureNamespaces(SemipermeableMembrane* self) {
    btrc_Vector_string* __list_42 = btrc_Vector_string_new();
    btrc_Vector_string_push(__list_42, self->persistRoot);
    btrc_Vector_string_push(__list_42, PathTools_join(self->persistRoot, MembraneConstants_dirs()));
    btrc_Vector_string_push(__list_42, PathTools_join(self->persistRoot, MembraneConstants_meta()));
    btrc_Vector_string_push(__list_42, PathTools_join(PathTools_join(self->persistRoot, MembraneConstants_meta()), MembraneConstants_files()));
    btrc_Vector_string* namespaces = __list_42;
    int __n_44 = btrc_Vector_string_iterLen(namespaces);
    for (int __i_43 = 0; (__i_43 < __n_44); (__i_43++)) {
        char* rel = btrc_Vector_string_iterGet(namespaces, __i_43);
        char* path = MembraneRun_path(self->run, rel);
        if (!SemipermeableMembrane_exists(self, path)) {
            SemipermeableMembrane_makeSubvolume(self, path);
        } else if (!SemipermeableMembrane_isSubvolume(self, path)) {
            MembraneRun_fatal(self->run, __btrc_str_track(__btrc_strcat("not a subvolume: ", path)));
        }
    }
}

void SemipermeableMembrane_rotate(SemipermeableMembrane* self, char* volume, char* live) {
    char* root = PathTools_join(self->snapshotsSubvolume, volume);
    char* a = MembraneRun_path(self->run, PathTools_join(root, MembraneConstants_a()));
    char* b = MembraneRun_path(self->run, PathTools_join(root, MembraneConstants_b()));
    char* c = MembraneRun_path(self->run, PathTools_join(root, MembraneConstants_c()));
    SemipermeableMembrane_deleteSubvolume(self, c);
    if (SemipermeableMembrane_exists(self, b)) {
        SemipermeableMembrane_snapshot(self, b, c, true);
        SemipermeableMembrane_deleteSubvolume(self, b);
    }
    if (SemipermeableMembrane_exists(self, a)) {
        SemipermeableMembrane_snapshot(self, a, b, true);
        SemipermeableMembrane_deleteSubvolume(self, a);
    }
    SemipermeableMembrane_snapshot(self, live, a, true);
}

void SemipermeableMembrane_publish(SemipermeableMembrane* self, MembraneVolume* volume, char* live, char* next) {
    if (!SemipermeableMembrane_exists(self, next)) {
        return;
    }
    if (((!self->run->dryRun) && (!(strcmp(volume->mountPoint, "/") == 0))) && SemipermeableMembrane_mounted(self, volume->mountPoint)) {
        MembraneRun_log(self->run, __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("WRN ", volume->mountPoint)), " mounted; leaving NEXT for boot")));
        return;
    }
    if (!SemipermeableMembrane_exists(self, live)) {
        MembraneRun_renamePath(self->run, next, live);
        return;
    }
    MembraneRun_requireRaw(self->run, __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("mv -T --exchange --no-copy ", UnixShell_quote(live))), " ")), UnixShell_quote(next))));
    SemipermeableMembrane_deleteSubvolume(self, next);
}

int SemipermeableMembrane_compareSpecs(SemipermeableMembrane* self, MembraneSpec* left, MembraneSpec* right) {
    int volume = Strings_compare(left->volume, right->volume);
    if (volume != 0) {
        return volume;
    }
    int leftDepth = Strings_count(left->relPath, "/");
    int rightDepth = Strings_count(right->relPath, "/");
    if (leftDepth != rightDepth) {
        return (leftDepth - rightDepth);
    }
    if (((int)strlen(left->relPath)) != ((int)strlen(right->relPath))) {
        return (((int)strlen(left->relPath)) - ((int)strlen(right->relPath)));
    }
    return Strings_compare(left->relPath, right->relPath);
}

void SemipermeableMembrane_pushSpecSorted(SemipermeableMembrane* self, btrc_Vector_MembraneSpec* specs, MembraneSpec* spec) {
    int index = 0;
    while ((index < specs->len) && (SemipermeableMembrane_compareSpecs(self, btrc_Vector_MembraneSpec_get(specs, index), spec) <= 0)) {
        (index++);
    }
    btrc_Vector_MembraneSpec_insert(specs, index, spec);
}

btrc_Vector_MembraneSpec* SemipermeableMembrane_sortedSpecs(SemipermeableMembrane* self, btrc_Vector_MembraneSpec* input) {
    btrc_Vector_MembraneSpec* sorted = btrc_Vector_MembraneSpec_new();
    int __n_46 = btrc_Vector_MembraneSpec_iterLen(input);
    for (int __i_45 = 0; (__i_45 < __n_46); (__i_45++)) {
        MembraneSpec* spec = btrc_Vector_MembraneSpec_iterGet(input, __i_45);
        SemipermeableMembrane_pushSpecSorted(self, sorted, spec);
    }
    return sorted;
}

char* SemipermeableMembrane_resolveKind(SemipermeableMembrane* self, char* live, char* clean, MembraneSpec* spec) {
    char* path = SemipermeableMembrane_sourcePath(self, live, clean, spec->relPath);
    if (__btrc_isEmpty(path)) {
        return "file";
    }
    if (FileSystem_isDir(path)) {
        return "dir";
    }
    return "file";
}

bool SemipermeableMembrane_planAlreadyCovers(SemipermeableMembrane* self, btrc_Vector_MembranePlan* plans, MembraneSpec* spec) {
    int __n_48 = btrc_Vector_MembranePlan_iterLen(plans);
    for (int __i_47 = 0; (__i_47 < __n_48); (__i_47++)) {
        MembranePlan* plan = btrc_Vector_MembranePlan_iterGet(plans, __i_47);
        if ((strcmp(plan->spec->volume, spec->volume) == 0) && (strcmp(plan->spec->relPath, spec->relPath) == 0)) {
            return true;
        }
        if (((strcmp(plan->spec->volume, spec->volume) == 0) && (strcmp(plan->kind, "dir") == 0)) && MembranePaths_ancestor(plan->spec->relPath, spec->relPath)) {
            return true;
        }
    }
    return false;
}

btrc_Vector_MembranePlan* SemipermeableMembrane_makePlan(SemipermeableMembrane* self, btrc_Vector_MembraneSpec* specs, char* live, char* clean) {
    btrc_Vector_MembranePlan* plans = btrc_Vector_MembranePlan_new();
    btrc_Vector_MembraneSpec* ordered = SemipermeableMembrane_sortedSpecs(self, specs);
    int __n_50 = btrc_Vector_MembraneSpec_iterLen(ordered);
    for (int __i_49 = 0; (__i_49 < __n_50); (__i_49++)) {
        MembraneSpec* spec = btrc_Vector_MembraneSpec_iterGet(ordered, __i_49);
        char* kind = spec->kind;
        if (strcmp(kind, "auto") == 0) {
            (kind = SemipermeableMembrane_resolveKind(self, live, clean, spec));
        }
        if (SemipermeableMembrane_planAlreadyCovers(self, plans, spec)) {
            continue;
        }
        char* store = ((strcmp(kind, "dir") == 0) ? MembranePaths_dirStore(self->persistRoot, spec->absPath) : MembranePaths_fileStore(self->persistRoot, spec->absPath));
        btrc_Vector_MembranePlan_push(plans, MembranePlan_new(spec, kind, store));
    }
    return plans;
}

btrc_Vector_MembraneSpec* SemipermeableMembrane_specsForVolume(SemipermeableMembrane* self, char* volume) {
    btrc_Vector_MembraneSpec* result = btrc_Vector_MembraneSpec_new();
    int __n_52 = btrc_Vector_MembraneSpec_iterLen(self->specs);
    for (int __i_51 = 0; (__i_51 < __n_52); (__i_51++)) {
        MembraneSpec* spec = btrc_Vector_MembraneSpec_iterGet(self->specs, __i_51);
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
    if (((!SemipermeableMembrane_exists(self, clean)) || (!SemipermeableMembrane_isSubvolume(self, clean))) || (!SemipermeableMembrane_isReadonly(self, clean))) {
        MembraneRun_fatal(self->run, __btrc_str_track(__btrc_strcat("bad CLEAN: ", clean)));
    }
    if (!SemipermeableMembrane_exists(self, live)) {
        SemipermeableMembrane_publish(self, volume, live, next);
    }
    if (!SemipermeableMembrane_exists(self, live)) {
        MembraneRun_fatal(self->run, __btrc_str_track(__btrc_strcat("live missing: ", live)));
    }
    if (!SemipermeableMembrane_isSubvolume(self, live)) {
        MembraneRun_fatal(self->run, __btrc_str_track(__btrc_strcat("live is not a subvolume: ", live)));
    }
    SemipermeableMembrane_deleteSubvolume(self, next);
    btrc_Vector_MembranePlan* plans = SemipermeableMembrane_makePlan(self, SemipermeableMembrane_specsForVolume(self, volume->name), live, clean);
    int __n_54 = btrc_Vector_MembranePlan_iterLen(plans);
    for (int __i_53 = 0; (__i_53 < __n_54); (__i_53++)) {
        MembranePlan* plan = btrc_Vector_MembranePlan_iterGet(plans, __i_53);
        if (!(strcmp(plan->kind, "dir") == 0)) {
            continue;
        }
        char* store = MembraneRun_path(self->run, plan->store);
        if (!SemipermeableMembrane_exists(self, store)) {
            SemipermeableMembrane_makeSubvolume(self, store);
            char* source = SemipermeableMembrane_sourcePath(self, live, clean, plan->spec->relPath);
            if (!__btrc_isEmpty(source)) {
                SemipermeableMembrane_copyDirectoryContents(self, source, store);
            }
        } else if (!SemipermeableMembrane_isSubvolume(self, store)) {
            MembraneRun_fatal(self->run, __btrc_str_track(__btrc_strcat("not a subvolume: ", store)));
        }
    }
    int __n_56 = btrc_Vector_MembranePlan_iterLen(plans);
    for (int __i_55 = 0; (__i_55 < __n_56); (__i_55++)) {
        MembranePlan* plan = btrc_Vector_MembranePlan_iterGet(plans, __i_55);
        if (!(strcmp(plan->kind, "file") == 0)) {
            continue;
        }
        char* destination = MembraneRun_path(self->run, plan->store);
        char* source = SemipermeableMembrane_sourcePath(self, live, clean, plan->spec->relPath);
        if (!__btrc_isEmpty(source)) {
            char* temporary = __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(destination, ".tmp.")), Strings_fromInt(Platform_pid())));
            MembraneRun_removePath(self->run, temporary);
            SemipermeableMembrane_copyPath(self, source, temporary);
            MembraneRun_removePath(self->run, destination);
            MembraneRun_renamePath(self->run, temporary, destination);
        } else {
            MembraneRun_removePath(self->run, destination);
        }
    }
    SemipermeableMembrane_rotate(self, volume->name, live);
    SemipermeableMembrane_snapshot(self, clean, next, false);
    int __n_58 = btrc_Vector_MembranePlan_iterLen(plans);
    for (int __i_57 = 0; (__i_57 < __n_58); (__i_57++)) {
        MembranePlan* plan = btrc_Vector_MembranePlan_iterGet(plans, __i_57);
        char* target = PathTools_join(next, plan->spec->relPath);
        MembraneRun_removePath(self->run, target);
        if (strcmp(plan->kind, "dir") == 0) {
            MembraneRun_mkdirp(self->run, target);
        } else {
            char* source = MembraneRun_path(self->run, plan->store);
            if (SemipermeableMembrane_exists(self, source)) {
                SemipermeableMembrane_copyPath(self, source, target);
            }
        }
    }
    SemipermeableMembrane_publish(self, volume, live, next);
}

void SemipermeableMembrane_restoreSlot(SemipermeableMembrane* self, MembraneVolume* volume, char* slot) {
    char* root = PathTools_join(self->snapshotsSubvolume, volume->name);
    char* live = MembraneRun_path(self->run, volume->name);
    char* next = MembraneRun_path(self->run, PathTools_join(root, MembraneConstants_next()));
    char* source = MembraneRun_path(self->run, PathTools_join(root, slot));
    if ((!SemipermeableMembrane_exists(self, source)) || (!SemipermeableMembrane_isSubvolume(self, source))) {
        MembraneRun_fatal(self->run, __btrc_str_track(__btrc_strcat("restore source missing: ", source)));
    }
    SemipermeableMembrane_snapshot(self, source, next, false);
    SemipermeableMembrane_publish(self, volume, live, next);
}

void SemipermeableMembrane_readSpecs(SemipermeableMembrane* self) {
    char* content = FileSystem_readText(self->specFile);
    int __n_60 = btrc_Vector_string_iterLen(Strings_split(content, "\n"));
    for (int __i_59 = 0; (__i_59 < __n_60); (__i_59++)) {
        char* raw = btrc_Vector_string_iterGet(Strings_split(content, "\n"), __i_59);
        char* line = __btrc_str_track(__btrc_trim(raw));
        if (__btrc_isEmpty(line) || __btrc_startsWith(line, "#")) {
            continue;
        }
        btrc_Vector_string* parts = Strings_split(raw, "\t");
        if (parts->len != 4) {
            NixosLog_fatal(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(self->specFile, " expected 4 tab-separated fields: ")), raw)));
        }
        char* volume = MembranePaths_normSubvol(btrc_Vector_string_get(parts, 0));
        char* mountPoint = MembranePaths_normMount(btrc_Vector_string_get(parts, 1));
        MembraneAbsPath* abs = MembranePaths_normAbs(btrc_Vector_string_get(parts, 2));
        char* kind = MembranePaths_kind(btrc_Vector_string_get(parts, 3));
        if ((strcmp(kind, "auto") == 0) && abs->endedWithSlash) {
            (kind = "dir");
        }
        char* rel = MembranePaths_relToMount(abs->path, mountPoint);
        btrc_Vector_MembraneSpec_push(self->specs, MembraneSpec_new(volume, mountPoint, abs->path, rel, kind));
    }
}

bool SemipermeableMembrane_volumeSeen(SemipermeableMembrane* self, btrc_Vector_MembraneVolume* volumes, char* name) {
    int __n_62 = btrc_Vector_MembraneVolume_iterLen(volumes);
    for (int __i_61 = 0; (__i_61 < __n_62); (__i_61++)) {
        MembraneVolume* volume = btrc_Vector_MembraneVolume_iterGet(volumes, __i_61);
        if (strcmp(volume->name, name) == 0) {
            return true;
        }
    }
    return false;
}

void SemipermeableMembrane_readVolumes(SemipermeableMembrane* self, btrc_Vector_string* args) {
    if (btrc_Vector_string_isEmpty(args)) {
        int __n_64 = btrc_Vector_MembraneSpec_iterLen(self->specs);
        for (int __i_63 = 0; (__i_63 < __n_64); (__i_63++)) {
            MembraneSpec* spec = btrc_Vector_MembraneSpec_iterGet(self->specs, __i_63);
            if (!SemipermeableMembrane_volumeSeen(self, self->volumes, spec->volume)) {
                btrc_Vector_MembraneVolume_push(self->volumes, MembraneVolume_new(spec->volume, "/"));
            }
        }
        return;
    }
    int __n_66 = btrc_Vector_string_iterLen(args);
    for (int __i_65 = 0; (__i_65 < __n_66); (__i_65++)) {
        char* arg = btrc_Vector_string_iterGet(args, __i_65);
        int marker = Strings_find(arg, "=", 0);
        if (marker < 0) {
            NixosLog_fatal(__btrc_str_track(__btrc_strcat("bad volume arg: ", arg)));
        }
        char* name = __btrc_str_track(__btrc_substring(arg, 0, marker));
        char* mountPoint = __btrc_str_track(__btrc_substring(arg, (marker + 1), ((((int)strlen(arg)) - marker) - 1)));
        btrc_Vector_MembraneVolume_push(self->volumes, MembraneVolume_new(MembranePaths_normSubvol(name), MembranePaths_normMount(mountPoint)));
    }
}

void SemipermeableMembrane_mountTop(SemipermeableMembrane* self) {
    if (self->assumeMounted) {
        if (!FileSystem_isDir(self->run->top)) {
            MembraneRun_fatal(self->run, __btrc_str_track(__btrc_strcat("missing mount path: ", self->run->top)));
        }
        return;
    }
    MembraneRun_mkdirp(self->run, self->run->top);
    Command* command = Command_capture(Command_arg(Command_arg(Command_arg(Command_arg(Command_arg(Command_arg(Command_new("mount"), "-t"), "btrfs"), "-o"), "subvolid=5,user_subvol_rm_allowed"), self->device), self->run->top), false);
    MembraneRun_requireCommand(self->run, command);
}

void SemipermeableMembrane_unmountTop(SemipermeableMembrane* self) {
    if (self->assumeMounted) {
        return;
    }
    Command* command = Command_check(Command_capture(Command_arg(Command_arg(Command_new("umount"), "-R"), self->run->top), false), false);
    MembraneRun_runCommand(self->run, command);
    if (__btrc_startsWith(self->run->top, __btrc_str_track(__btrc_strcat(MembraneConstants_topBase(), ".")))) {
        FileSystem_removeRecursive(self->run->top);
    }
}

void SemipermeableMembrane_configure(SemipermeableMembrane* self, char* device, char* snapshotsSubvolume, char* cleanName, char* mode, char* persistRoot, char* specFile, btrc_Vector_string* volumeArgs) {
    (self->device = device);
    (self->snapshotsSubvolume = MembranePaths_normSubvol(snapshotsSubvolume));
    (self->cleanName = __btrc_str_track(__btrc_trim(cleanName)));
    (self->mode = __btrc_str_track(__btrc_trim(mode)));
    (self->persistRoot = MembranePaths_normSubvol(persistRoot));
    (self->specFile = specFile);
    SemipermeableMembrane_readSpecs(self);
    SemipermeableMembrane_readVolumes(self, volumeArgs);
}

void SemipermeableMembrane_runAll(SemipermeableMembrane* self) {
    btrc_Vector_string* names = btrc_Vector_string_new();
    int __n_68 = btrc_Vector_MembraneVolume_iterLen(self->volumes);
    for (int __i_67 = 0; (__i_67 < __n_68); (__i_67++)) {
        MembraneVolume* volume = btrc_Vector_MembraneVolume_iterGet(self->volumes, __i_67);
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
    SemipermeableMembrane_mountTop(self);
    SemipermeableMembrane_ensureNamespaces(self);
    int __n_70 = btrc_Vector_MembraneVolume_iterLen(self->volumes);
    for (int __i_69 = 0; (__i_69 < __n_70); (__i_69++)) {
        MembraneVolume* volume = btrc_Vector_MembraneVolume_iterGet(self->volumes, __i_69);
        if (((strcmp(self->mode, MembraneConstants_converge()) == 0) || (strcmp(self->mode, MembraneConstants_reset()) == 0)) || (strcmp(self->mode, MembraneConstants_prepareOnly()) == 0)) {
            SemipermeableMembrane_resetVolume(self, volume);
        } else if (strcmp(self->mode, MembraneConstants_snapshotOnly()) == 0) {
            SemipermeableMembrane_rotate(self, volume->name, MembraneRun_path(self->run, volume->name));
        } else if ((strcmp(self->mode, MembraneConstants_restoreA()) == 0) || (strcmp(self->mode, MembraneConstants_restorePrevious()) == 0)) {
            SemipermeableMembrane_restoreSlot(self, volume, MembraneConstants_a());
        } else if ((strcmp(self->mode, MembraneConstants_restoreB()) == 0) || (strcmp(self->mode, MembraneConstants_restorePenultimate()) == 0)) {
            SemipermeableMembrane_restoreSlot(self, volume, MembraneConstants_b());
        } else if (strcmp(self->mode, MembraneConstants_restoreC()) == 0) {
            SemipermeableMembrane_restoreSlot(self, volume, MembraneConstants_c());
        } else {
            MembraneRun_fatal(self->run, __btrc_str_track(__btrc_strcat("Unknown mode: ", self->mode)));
        }
    }
    Command* sync = Command_capture(Command_arg(Command_arg(Command_arg(Command_new("btrfs"), "filesystem"), "sync"), self->run->top), false);
    MembraneRun_requireCommand(self->run, sync);
    SemipermeableMembrane_unmountTop(self);
    MembraneRun_log(self->run, "Semipermeable membrane complete");
}

bool SemipermeableMembrane_coveredBy(SemipermeableMembrane* self, btrc_Vector_string* selected, char* target) {
    int __n_72 = btrc_Vector_string_iterLen(selected);
    for (int __i_71 = 0; (__i_71 < __n_72); (__i_71++)) {
        char* s = btrc_Vector_string_iterGet(selected, __i_71);
        if (__btrc_startsWith(target, __btrc_str_track(__btrc_strcat(s, "/")))) {
            return true;
        }
    }
    return false;
}

void SemipermeableMembrane_mountTarget(SemipermeableMembrane* self, char* target, char* subvol) {
    MembraneRun_mkdirp(self->run, target);
    if (SemipermeableMembrane_mounted(self, target)) {
        return;
    }
    Command* command = Command_capture(Command_arg(Command_arg(Command_arg(Command_arg(Command_arg(Command_arg(Command_new("mount"), "-t"), "btrfs"), "-o"), __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("subvol=", subvol)), ",compress=zstd,noatime"))), self->device), target), false);
    MembraneRun_requireCommand(self->run, command);
}

void SemipermeableMembrane_configureMount(SemipermeableMembrane* self, char* device, char* persistRoot, char* specFile) {
    (self->device = device);
    (self->persistRoot = MembranePaths_normSubvol(persistRoot));
    (self->specFile = specFile);
    SemipermeableMembrane_readSpecs(self);
}

void SemipermeableMembrane_mountPersist(SemipermeableMembrane* self) {
    MembraneRun_log(self->run, __btrc_str_track(__btrc_strcat("Mounting persistent subvolumes dry_run=", (self->run->dryRun ? "true" : "false"))));
    bool mountedTop = false;
    if ((!self->assumeMounted) && (!SemipermeableMembrane_mounted(self, self->run->top))) {
        MembraneRun_mkdirp(self->run, self->run->top);
        Command* m = Command_capture(Command_arg(Command_arg(Command_arg(Command_arg(Command_arg(Command_arg(Command_new("mount"), "-t"), "btrfs"), "-o"), "ro,subvolid=5"), self->device), self->run->top), false);
        MembraneRun_requireCommand(self->run, m);
        (mountedTop = true);
    }
    btrc_Vector_string* selected = btrc_Vector_string_new();
    int __n_74 = btrc_Vector_MembraneSpec_iterLen(self->specs);
    for (int __i_73 = 0; (__i_73 < __n_74); (__i_73++)) {
        MembraneSpec* spec = btrc_Vector_MembraneSpec_iterGet(self->specs, __i_73);
        char* target = spec->absPath;
        if (SemipermeableMembrane_coveredBy(self, selected, target)) {
            continue;
        }
        char* subvol = PathTools_join(PathTools_join(self->persistRoot, MembraneConstants_dirs()), MembranePaths_key(target));
        if (!SemipermeableMembrane_isSubvolume(self, MembraneRun_path(self->run, subvol))) {
            continue;
        }
        SemipermeableMembrane_mountTarget(self, target, subvol);
        btrc_Vector_string_push(selected, target);
    }
    if (mountedTop) {
        Command* u = Command_check(Command_capture(Command_arg(Command_new("umount"), self->run->top), false), false);
        MembraneRun_runCommand(self->run, u);
    }
    MembraneRun_log(self->run, "Persistent mounts complete");
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
            return 1;
        }
        SemipermeableMembrane* mounter = SemipermeableMembrane_new(dryRun, assumeMounted);
        SemipermeableMembrane_configureMount(mounter, CliArgs_get(args, (index + 1)), CliArgs_get(args, (index + 2)), CliArgs_get(args, (index + 3)));
        SemipermeableMembrane_mountPersist(mounter);
        if (mounter != NULL) {
            if ((--mounter->__rc) <= 0) {
                SemipermeableMembrane_destroy(mounter);
            }
        }
        return 0;
        if (mounter != NULL) {
            if ((--mounter->__rc) <= 0) {
                SemipermeableMembrane_destroy(mounter);
            }
        }
    }
    if (CliArgs_count(args) < (index + 6)) {
        Console_error("Usage: semipermeable_membrane [--dry-run] [--assume-mounted] <device> <snapshots> <clean> <mode> <persist_root> <spec_file> [name=mount ...]");
        return 1;
    }
    btrc_Vector_string* volumeArgs = btrc_Vector_string_new();
    for (int i = (index + 6); (i < CliArgs_count(args)); (i++)) {
        btrc_Vector_string_push(volumeArgs, CliArgs_get(args, i));
    }
    SemipermeableMembrane* membrane = SemipermeableMembrane_new(dryRun, assumeMounted);
    SemipermeableMembrane_configure(membrane, CliArgs_get(args, index), CliArgs_get(args, (index + 1)), CliArgs_get(args, (index + 2)), CliArgs_get(args, (index + 3)), CliArgs_get(args, (index + 4)), CliArgs_get(args, (index + 5)), volumeArgs);
    SemipermeableMembrane_runAll(membrane);
    if (membrane != NULL) {
        if ((--membrane->__rc) <= 0) {
            SemipermeableMembrane_destroy(membrane);
        }
    }
    return 0;
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

