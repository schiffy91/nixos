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
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <fnmatch.h>
#include <time.h>

#define _DARWIN_C_SOURCE

typedef struct NixosLog NixosLog;
typedef struct MembraneConstants MembraneConstants;
typedef struct MembraneAbsPath MembraneAbsPath;
typedef struct MembraneSpec MembraneSpec;
void MembraneSpec_destroy(MembraneSpec* self);
typedef struct MembraneVolume MembraneVolume;
void MembraneVolume_destroy(MembraneVolume* self);
typedef struct MembranePlan MembranePlan;
void MembranePlan_destroy(MembranePlan* self);
typedef struct MembranePaths MembranePaths;
typedef struct MembraneRun MembraneRun;
void MembraneRun_destroy(MembraneRun* self);
typedef struct MembraneBtrfs MembraneBtrfs;
typedef struct MembraneFs MembraneFs;
void MembraneFs_destroy(MembraneFs* self);
typedef struct MembranePlanner MembranePlanner;
void MembranePlanner_destroy(MembranePlanner* self);
typedef struct MembraneMounts MembraneMounts;
void MembraneMounts_destroy(MembraneMounts* self);
typedef struct MembraneSlots MembraneSlots;
void MembraneSlots_destroy(MembraneSlots* self);
typedef struct MembraneConfig MembraneConfig;
typedef struct MembraneOrphans MembraneOrphans;
void MembraneOrphans_destroy(MembraneOrphans* self);
typedef struct SemipermeableMembrane SemipermeableMembrane;
void SemipermeableMembrane_destroy(SemipermeableMembrane* self);
typedef struct btrc_Vector_MembraneSpec btrc_Vector_MembraneSpec;
typedef struct btrc_Vector_MembranePlan btrc_Vector_MembranePlan;
typedef struct btrc_Vector_MembraneVolume btrc_Vector_MembraneVolume;
char* NixosLog_red(void);
char* NixosLog_reset(void);
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
void MembraneAbsPath_init(MembraneAbsPath* self, char* path, bool endedWithSlash);
MembraneAbsPath* MembraneAbsPath_new(char* path, bool endedWithSlash);
void MembraneSpec_init(MembraneSpec* self, char* volume, char* mountPoint, char* absPath, char* relPath, char* kind);
MembraneSpec* MembraneSpec_new(char* volume, char* mountPoint, char* absPath, char* relPath, char* kind);
void MembraneVolume_init(MembraneVolume* self, char* name, char* mountPoint);
MembraneVolume* MembraneVolume_new(char* name, char* mountPoint);
void MembranePlan_init(MembranePlan* self, MembraneSpec* spec, char* kind, char* store);
MembranePlan* MembranePlan_new(MembraneSpec* spec, char* kind, char* store);
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
btrc_Vector_MembraneSpec* MembraneConfig_readSpecs(char* specFile);
bool MembraneConfig_volumeSeen(btrc_Vector_MembraneVolume* volumes, char* name);
btrc_Vector_MembraneVolume* MembraneConfig_readVolumes(btrc_Vector_string* args, btrc_Vector_MembraneSpec* specs);
void MembraneOrphans_init(MembraneOrphans* self, MembraneFs* fs, MembraneRun* run);
MembraneOrphans* MembraneOrphans_new(MembraneFs* fs, MembraneRun* run);
void MembraneOrphans_ensureNamespaces(MembraneOrphans* self, char* persistRoot);
bool MembraneOrphans_keyInSpec(MembraneOrphans* self, btrc_Vector_string* validKeys, char* key);
void MembraneOrphans_pruneOrphans(MembraneOrphans* self, char* persistRoot, btrc_Vector_MembraneSpec* specs);
void SemipermeableMembrane_init(SemipermeableMembrane* self, bool dryRun, bool assumeMounted);
SemipermeableMembrane* SemipermeableMembrane_new(bool dryRun, bool assumeMounted);
bool SemipermeableMembrane_envDryRun(void);
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

struct MembraneOrphans {
    int __rc;
    MembraneFs* fs;
    MembraneRun* run;
};

struct SemipermeableMembrane {
    int __rc;
    MembraneRun* run;
    MembraneFs* fs;
    MembranePlanner* planner;
    MembraneMounts* mounts;
    MembraneSlots* slots;
    MembraneOrphans* orphans;
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

char* NixosLog_red(void) {
    return "\033[31m";
}

char* NixosLog_reset(void) {
    return "\033[0m";
}

void NixosLog_fatal(char* message) {
    Console_error(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(NixosLog_red(), message)), NixosLog_reset())));
    exit(1);
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
    int __n_34 = btrc_Vector_string_iterLen(Strings_split(path, "/"));
    for (int __i_33 = 0; (__i_33 < __n_34); (__i_33++)) {
        char* part = btrc_Vector_string_iterGet(Strings_split(path, "/"), __i_33);
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

Command* MembraneBtrfs_show(char* path) {
    return Command_check(Command_arg(Command_arg(Command_arg(Command_new("btrfs"), "subvolume"), "show"), path), false);
}

Command* MembraneBtrfs_getReadonly(char* path) {
    return Command_check(Command_arg(Command_arg(Command_arg(Command_arg(Command_arg(Command_new("btrfs"), "property"), "get"), "-ts"), path), "ro"), false);
}

Command* MembraneBtrfs_listChildren(char* path) {
    return Command_arg(Command_arg(Command_arg(Command_arg(Command_new("btrfs"), "subvolume"), "list"), "-o"), path);
}

Command* MembraneBtrfs_deleteSubvol(char* path) {
    return Command_capture(Command_arg(Command_arg(Command_arg(Command_new("btrfs"), "subvolume"), "delete"), path), false);
}

Command* MembraneBtrfs_createSubvol(char* path) {
    return Command_capture(Command_arg(Command_arg(Command_arg(Command_new("btrfs"), "subvolume"), "create"), path), false);
}

Command* MembraneBtrfs_snapshot(char* source, char* destination, bool readonly) {
    Command* command = Command_arg(Command_arg(Command_new("btrfs"), "subvolume"), "snapshot");
    if (readonly) {
        Command_arg(command, "-r");
    }
    return Command_capture(Command_arg(Command_arg(command, source), destination), false);
}

Command* MembraneBtrfs_clearReadonly(char* path) {
    return Command_capture(Command_arg(Command_arg(Command_arg(Command_arg(Command_arg(Command_arg(Command_new("btrfs"), "property"), "set"), "-ts"), path), "ro"), "false"), false);
}

Command* MembraneBtrfs_sync(char* top) {
    return Command_capture(Command_arg(Command_arg(Command_arg(Command_new("btrfs"), "filesystem"), "sync"), top), false);
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
    return (FileSystem_exists(path) || FileSystem_isSymlink(path));
}

bool MembraneFs_isSubvolume(MembraneFs* self, char* path) {
    if (!FileSystem_exists(path)) {
        return false;
    }
    ExecResult* result = UnixShell_runCommand(self->run->shell, MembraneBtrfs_show(path));
    return ExecResult_ok(result);
}

bool MembraneFs_isReadonly(MembraneFs* self, char* path) {
    ExecResult* result = UnixShell_runCommand(self->run->shell, MembraneBtrfs_getReadonly(path));
    return (ExecResult_ok(result) && __btrc_strContains(ExecResult_stdout(result), "ro=true"));
}

bool MembraneFs_mounted(MembraneFs* self, char* path) {
    Command* command = Command_check(Command_arg(Command_arg(Command_new("findmnt"), "--mountpoint"), path), false);
    ExecResult* result = UnixShell_runCommand(self->run->shell, command);
    return ExecResult_ok(result);
}

int MembraneFs_compareDeepestFirst(MembraneFs* self, char* left, char* right) {
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
    int __n_36 = btrc_Vector_string_iterLen(Strings_split(ExecResult_stdout(listed), "\n"));
    for (int __i_35 = 0; (__i_35 < __n_36); (__i_35++)) {
        char* line = btrc_Vector_string_iterGet(Strings_split(ExecResult_stdout(listed), "\n"), __i_35);
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
    int __n_38 = btrc_Vector_string_iterLen(children);
    for (int __i_37 = 0; (__i_37 < __n_38); (__i_37++)) {
        char* child = btrc_Vector_string_iterGet(children, __i_37);
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
    return "";
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
        return (leftDepth - rightDepth);
    }
    if (((int)strlen(left->relPath)) != ((int)strlen(right->relPath))) {
        return (((int)strlen(left->relPath)) - ((int)strlen(right->relPath)));
    }
    return Strings_compare(left->relPath, right->relPath);
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
    int __n_40 = btrc_Vector_MembraneSpec_iterLen(input);
    for (int __i_39 = 0; (__i_39 < __n_40); (__i_39++)) {
        MembraneSpec* spec = btrc_Vector_MembraneSpec_iterGet(input, __i_39);
        MembranePlanner_pushSpecSorted(self, sorted, spec);
    }
    return sorted;
}

char* MembranePlanner_resolveKind(MembranePlanner* self, char* live, char* clean, MembraneSpec* spec) {
    char* path = MembraneFs_sourcePath(self->fs, live, clean, spec->relPath);
    if (__btrc_isEmpty(path)) {
        return "file";
    }
    if (FileSystem_isDir(path)) {
        return "dir";
    }
    return "file";
}

bool MembranePlanner_planAlreadyCovers(MembranePlanner* self, btrc_Vector_MembranePlan* plans, MembraneSpec* spec) {
    int __n_42 = btrc_Vector_MembranePlan_iterLen(plans);
    for (int __i_41 = 0; (__i_41 < __n_42); (__i_41++)) {
        MembranePlan* plan = btrc_Vector_MembranePlan_iterGet(plans, __i_41);
        if ((strcmp(plan->spec->volume, spec->volume) == 0) && (strcmp(plan->spec->relPath, spec->relPath) == 0)) {
            return true;
        }
        if (((strcmp(plan->spec->volume, spec->volume) == 0) && (strcmp(plan->kind, "dir") == 0)) && MembranePaths_ancestor(plan->spec->relPath, spec->relPath)) {
            return true;
        }
    }
    return false;
}

btrc_Vector_MembranePlan* MembranePlanner_makePlan(MembranePlanner* self, btrc_Vector_MembraneSpec* specs, char* live, char* clean, char* persistRoot) {
    btrc_Vector_MembranePlan* plans = btrc_Vector_MembranePlan_new();
    btrc_Vector_MembraneSpec* ordered = MembranePlanner_sortedSpecs(self, specs);
    int __n_44 = btrc_Vector_MembraneSpec_iterLen(ordered);
    for (int __i_43 = 0; (__i_43 < __n_44); (__i_43++)) {
        MembraneSpec* spec = btrc_Vector_MembraneSpec_iterGet(ordered, __i_43);
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
    int __n_46 = btrc_Vector_string_iterLen(selected);
    for (int __i_45 = 0; (__i_45 < __n_46); (__i_45++)) {
        char* s = btrc_Vector_string_iterGet(selected, __i_45);
        if (__btrc_startsWith(target, __btrc_str_track(__btrc_strcat(s, "/")))) {
            return true;
        }
    }
    return false;
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
    int __n_48 = btrc_Vector_MembraneSpec_iterLen(specs);
    for (int __i_47 = 0; (__i_47 < __n_48); (__i_47++)) {
        MembraneSpec* spec = btrc_Vector_MembraneSpec_iterGet(specs, __i_47);
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
    int __n_50 = btrc_Vector_MembraneVolume_iterLen(volumes);
    for (int __i_49 = 0; (__i_49 < __n_50); (__i_49++)) {
        MembraneVolume* volume = btrc_Vector_MembraneVolume_iterGet(volumes, __i_49);
        btrc_Vector_string_push(names, volume->name);
    }
    MembraneRun_log(self->run, __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("snapshot-clean dry_run=", (self->run->dryRun ? "true" : "false"))), " device=")), device)), " subvolumes=")), btrc_Vector_string_join(names, " "))));
    if (self->run->dryRun && (!assumeMounted)) {
        MembraneRun_log(self->run, "DRY add --assume-mounted to walk mounted test roots");
        return;
    }
    MembraneMounts_mountTop(self, device, assumeMounted);
    int __n_52 = btrc_Vector_MembraneVolume_iterLen(volumes);
    for (int __i_51 = 0; (__i_51 < __n_52); (__i_51++)) {
        MembraneVolume* volume = btrc_Vector_MembraneVolume_iterGet(volumes, __i_51);
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

btrc_Vector_MembraneSpec* MembraneConfig_readSpecs(char* specFile) {
    btrc_Vector_MembraneSpec* result = btrc_Vector_MembraneSpec_new();
    char* content = FileSystem_readText(specFile);
    int __n_54 = btrc_Vector_string_iterLen(Strings_split(content, "\n"));
    for (int __i_53 = 0; (__i_53 < __n_54); (__i_53++)) {
        char* raw = btrc_Vector_string_iterGet(Strings_split(content, "\n"), __i_53);
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
    int __n_56 = btrc_Vector_MembraneVolume_iterLen(volumes);
    for (int __i_55 = 0; (__i_55 < __n_56); (__i_55++)) {
        MembraneVolume* volume = btrc_Vector_MembraneVolume_iterGet(volumes, __i_55);
        if (strcmp(volume->name, name) == 0) {
            return true;
        }
    }
    return false;
}

btrc_Vector_MembraneVolume* MembraneConfig_readVolumes(btrc_Vector_string* args, btrc_Vector_MembraneSpec* specs) {
    btrc_Vector_MembraneVolume* result = btrc_Vector_MembraneVolume_new();
    if (btrc_Vector_string_isEmpty(args)) {
        int __n_58 = btrc_Vector_MembraneSpec_iterLen(specs);
        for (int __i_57 = 0; (__i_57 < __n_58); (__i_57++)) {
            MembraneSpec* spec = btrc_Vector_MembraneSpec_iterGet(specs, __i_57);
            if (!MembraneConfig_volumeSeen(result, spec->volume)) {
                btrc_Vector_MembraneVolume_push(result, MembraneVolume_new(spec->volume, "/"));
            }
        }
        return result;
    }
    int __n_60 = btrc_Vector_string_iterLen(args);
    for (int __i_59 = 0; (__i_59 < __n_60); (__i_59++)) {
        char* arg = btrc_Vector_string_iterGet(args, __i_59);
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

void MembraneOrphans_init(MembraneOrphans* self, MembraneFs* fs, MembraneRun* run) {
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

MembraneOrphans* MembraneOrphans_new(MembraneFs* fs, MembraneRun* run) {
    MembraneOrphans* self = ((MembraneOrphans*)malloc(sizeof(MembraneOrphans)));
    memset(self, 0, sizeof(MembraneOrphans));
    MembraneOrphans_init(self, fs, run);
    return self;
}

void MembraneOrphans_destroy(MembraneOrphans* self) {
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

void MembraneOrphans_ensureNamespaces(MembraneOrphans* self, char* persistRoot) {
    btrc_Vector_string* __list_61 = btrc_Vector_string_new();
    btrc_Vector_string_push(__list_61, persistRoot);
    btrc_Vector_string_push(__list_61, PathTools_join(persistRoot, MembraneConstants_dirs()));
    btrc_Vector_string_push(__list_61, PathTools_join(persistRoot, MembraneConstants_meta()));
    btrc_Vector_string_push(__list_61, PathTools_join(PathTools_join(persistRoot, MembraneConstants_meta()), MembraneConstants_files()));
    btrc_Vector_string* namespaces = __list_61;
    int __n_63 = btrc_Vector_string_iterLen(namespaces);
    for (int __i_62 = 0; (__i_62 < __n_63); (__i_62++)) {
        char* rel = btrc_Vector_string_iterGet(namespaces, __i_62);
        char* path = MembraneRun_path(self->run, rel);
        if (!MembraneFs_exists(self->fs, path)) {
            MembraneFs_makeSubvolume(self->fs, path);
        } else if (!MembraneFs_isSubvolume(self->fs, path)) {
            MembraneRun_fatal(self->run, __btrc_str_track(__btrc_strcat("not a subvolume: ", path)));
        }
    }
}

bool MembraneOrphans_keyInSpec(MembraneOrphans* self, btrc_Vector_string* validKeys, char* key) {
    int __n_65 = btrc_Vector_string_iterLen(validKeys);
    for (int __i_64 = 0; (__i_64 < __n_65); (__i_64++)) {
        char* v = btrc_Vector_string_iterGet(validKeys, __i_64);
        if (strcmp(v, key) == 0) {
            return true;
        }
    }
    return false;
}

void MembraneOrphans_pruneOrphans(MembraneOrphans* self, char* persistRoot, btrc_Vector_MembraneSpec* specs) {
    btrc_Vector_string* validKeys = btrc_Vector_string_new();
    int __n_67 = btrc_Vector_MembraneSpec_iterLen(specs);
    for (int __i_66 = 0; (__i_66 < __n_67); (__i_66++)) {
        MembraneSpec* spec = btrc_Vector_MembraneSpec_iterGet(specs, __i_66);
        btrc_Vector_string_push(validKeys, MembranePaths_key(spec->absPath));
    }
    char* dirsPath = MembraneRun_path(self->run, PathTools_join(persistRoot, MembraneConstants_dirs()));
    btrc_Vector_string* children = MembraneFs_childSubvolumes(self->fs, dirsPath);
    int __n_69 = btrc_Vector_string_iterLen(children);
    for (int __i_68 = 0; (__i_68 < __n_69); (__i_68++)) {
        char* child = btrc_Vector_string_iterGet(children, __i_68);
        char* key = PathTools_basename(child);
        if (!MembraneOrphans_keyInSpec(self, validKeys, key)) {
            MembraneRun_log(self->run, __btrc_str_track(__btrc_strcat("Pruning orphaned persist subvolume ", key)));
            MembraneFs_deleteSubvolume(self->fs, child);
        }
    }
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
    if (self->orphans != NULL) {
        if ((--self->orphans->__rc) <= 0) {
            MembraneOrphans_destroy(self->orphans);
        }
    }
    (self->orphans = MembraneOrphans_new(self->fs, self->run));
    (MembraneOrphans_new(self->fs, self->run)->__rc++);
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
    btrc_Vector_MembraneSpec* __list_71 = btrc_Vector_MembraneSpec_new();
    (self->specs = __list_71);
    btrc_Vector_MembraneSpec* __list_70 = btrc_Vector_MembraneSpec_new();
    (__list_70->__rc++);
    if (self->volumes != NULL) {
        if ((--self->volumes->__rc) <= 0) {
            btrc_Vector_MembraneVolume_free(self->volumes);
        }
    }
    btrc_Vector_MembraneVolume* __list_73 = btrc_Vector_MembraneVolume_new();
    (self->volumes = __list_73);
    btrc_Vector_MembraneVolume* __list_72 = btrc_Vector_MembraneVolume_new();
    (__list_72->__rc++);
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
    if (self->orphans != NULL) {
        if ((--self->orphans->__rc) <= 0) {
            MembraneOrphans_destroy(self->orphans);
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

btrc_Vector_MembraneSpec* SemipermeableMembrane_specsForVolume(SemipermeableMembrane* self, char* volume) {
    btrc_Vector_MembraneSpec* result = btrc_Vector_MembraneSpec_new();
    int __n_75 = btrc_Vector_MembraneSpec_iterLen(self->specs);
    for (int __i_74 = 0; (__i_74 < __n_75); (__i_74++)) {
        MembraneSpec* spec = btrc_Vector_MembraneSpec_iterGet(self->specs, __i_74);
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
    int __n_77 = btrc_Vector_MembranePlan_iterLen(plans);
    for (int __i_76 = 0; (__i_76 < __n_77); (__i_76++)) {
        MembranePlan* plan = btrc_Vector_MembranePlan_iterGet(plans, __i_76);
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
    int __n_79 = btrc_Vector_MembranePlan_iterLen(plans);
    for (int __i_78 = 0; (__i_78 < __n_79); (__i_78++)) {
        MembranePlan* plan = btrc_Vector_MembranePlan_iterGet(plans, __i_78);
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
    int __n_81 = btrc_Vector_MembranePlan_iterLen(plans);
    for (int __i_80 = 0; (__i_80 < __n_81); (__i_80++)) {
        MembranePlan* plan = btrc_Vector_MembranePlan_iterGet(plans, __i_80);
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
    int __n_83 = btrc_Vector_MembraneVolume_iterLen(self->volumes);
    for (int __i_82 = 0; (__i_82 < __n_83); (__i_82++)) {
        MembraneVolume* volume = btrc_Vector_MembraneVolume_iterGet(self->volumes, __i_82);
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
    MembraneOrphans_ensureNamespaces(self->orphans, self->persistRoot);
    if (((strcmp(self->mode, MembraneConstants_converge()) == 0) || (strcmp(self->mode, MembraneConstants_reset()) == 0)) || (strcmp(self->mode, MembraneConstants_prepareOnly()) == 0)) {
        MembraneOrphans_pruneOrphans(self->orphans, self->persistRoot, self->specs);
    }
    int __n_85 = btrc_Vector_MembraneVolume_iterLen(self->volumes);
    for (int __i_84 = 0; (__i_84 < __n_85); (__i_84++)) {
        MembraneVolume* volume = btrc_Vector_MembraneVolume_iterGet(self->volumes, __i_84);
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
            return 1;
        }
        SemipermeableMembrane* mounter = SemipermeableMembrane_new(dryRun, assumeMounted);
        SemipermeableMembrane_configureMount(mounter, CliArgs_get(args, (index + 1)), CliArgs_get(args, (index + 2)), CliArgs_get(args, (index + 3)));
        SemipermeableMembrane_mountPersist(mounter);
        int __btrc_ret_86 = 0;
        if (mounter != NULL) {
            if ((--mounter->__rc) <= 0) {
                SemipermeableMembrane_destroy(mounter);
            }
        }
        return __btrc_ret_86;
        if (mounter != NULL) {
            if ((--mounter->__rc) <= 0) {
                SemipermeableMembrane_destroy(mounter);
            }
        }
    }
    if ((index < CliArgs_count(args)) && (strcmp(CliArgs_get(args, index), "snapshot-clean") == 0)) {
        if (CliArgs_count(args) < (index + 4)) {
            Console_error("Usage: semipermeable_membrane [--dry-run] [--assume-mounted] snapshot-clean <device> <snapshots> <clean> [name=mount ...]");
            return 1;
        }
        btrc_Vector_string* cleanVolumeArgs = btrc_Vector_string_new();
        for (int i = (index + 4); (i < CliArgs_count(args)); (i++)) {
            btrc_Vector_string_push(cleanVolumeArgs, CliArgs_get(args, i));
        }
        SemipermeableMembrane* snapshotter = SemipermeableMembrane_new(dryRun, assumeMounted);
        SemipermeableMembrane_configureSnapshotClean(snapshotter, CliArgs_get(args, (index + 1)), CliArgs_get(args, (index + 2)), CliArgs_get(args, (index + 3)), cleanVolumeArgs);
        SemipermeableMembrane_snapshotCleanAll(snapshotter);
        int __btrc_ret_87 = 0;
        if (snapshotter != NULL) {
            if ((--snapshotter->__rc) <= 0) {
                SemipermeableMembrane_destroy(snapshotter);
            }
        }
        return __btrc_ret_87;
        if (snapshotter != NULL) {
            if ((--snapshotter->__rc) <= 0) {
                SemipermeableMembrane_destroy(snapshotter);
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
    int __btrc_ret_88 = 0;
    if (membrane != NULL) {
        if ((--membrane->__rc) <= 0) {
            SemipermeableMembrane_destroy(membrane);
        }
    }
    return __btrc_ret_88;
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

