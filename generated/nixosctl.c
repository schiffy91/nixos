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

#define _DARWIN_C_SOURCE

typedef struct NixosLog NixosLog;
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
typedef struct DiffOptions DiffOptions;
void DiffOptions_destroy(DiffOptions* self);
typedef struct DiffScanner DiffScanner;
typedef struct Installer Installer;
void Installer_destroy(Installer* self);
typedef struct SecureBootManager SecureBootManager;
void SecureBootManager_destroy(SecureBootManager* self);
typedef struct Tpm2Manager Tpm2Manager;
void Tpm2Manager_destroy(Tpm2Manager* self);
typedef struct PasswordManager PasswordManager;
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
typedef struct SshClient SshClient;
void SshClient_destroy(SshClient* self);
typedef struct QemuSerial QemuSerial;
void QemuSerial_destroy(QemuSerial* self);
typedef struct QemuFirmware QemuFirmware;
void QemuFirmware_destroy(QemuFirmware* self);
typedef struct VmStateStore VmStateStore;
void VmStateStore_destroy(VmStateStore* self);
typedef struct VmProvisioner VmProvisioner;
void VmProvisioner_destroy(VmProvisioner* self);
typedef struct QemuCommandBuilder QemuCommandBuilder;
void QemuCommandBuilder_destroy(QemuCommandBuilder* self);
typedef struct VmAssets VmAssets;
void VmAssets_destroy(VmAssets* self);
typedef struct QemuE2eHarness QemuE2eHarness;
void QemuE2eHarness_destroy(QemuE2eHarness* self);
typedef struct VmOperationCatalog VmOperationCatalog;
typedef struct VmTestRunner VmTestRunner;
void VmTestRunner_destroy(VmTestRunner* self);
typedef struct VmGraphNode VmGraphNode;
void VmGraphNode_destroy(VmGraphNode* self);
typedef struct VmTestGraph VmTestGraph;
void VmTestGraph_destroy(VmTestGraph* self);
typedef struct VmGraphParser VmGraphParser;
typedef struct VmGraphRunner VmGraphRunner;
void VmGraphRunner_destroy(VmGraphRunner* self);
typedef struct E2eCli E2eCli;
typedef struct NixosCtl NixosCtl;
void NixosCtl_destroy(NixosCtl* self);
typedef struct btrc_Vector_ResetSubvolume btrc_Vector_ResetSubvolume;
typedef struct btrc_Vector_DisplayLayoutRule btrc_Vector_DisplayLayoutRule;
typedef struct btrc_Vector_AudioPreset btrc_Vector_AudioPreset;
typedef struct btrc_Vector_AudioSink btrc_Vector_AudioSink;
typedef struct btrc_Vector_DisplayOutput btrc_Vector_DisplayOutput;
typedef struct btrc_Vector_VmOperation btrc_Vector_VmOperation;
typedef struct btrc_Vector_VmGraphNode btrc_Vector_VmGraphNode;
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
Command* NixosConfig_nixCmd(NixosConfig* self);
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
char* PermissionsManager_ignorePredicate(PermissionsManager* self);
void PermissionsManager_findChmod(PermissionsManager* self, char* quotedRoot, char* ignores, char* predicate, char* mode);
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
void NixosRebuilder_convergeMembrane(NixosRebuilder* self, char* mode, bool dryRun);
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
btrc_Vector_string* DiffScanner_selectEphemeral(DiffScanner* self, btrc_Vector_string* ephemeral, btrc_Vector_string* keepList, btrc_Vector_string* mounts, DiffOptions* options);
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
void DisplayManager_applyGeometry(DisplayManager* self, DisplayOutput* output, char* block);
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
void SshClient_init(SshClient* self, VmTestSpec* spec);
SshClient* SshClient_new(VmTestSpec* spec);
char* SshClient_sshKeyPath(SshClient* self);
char* SshClient_qmpPath(SshClient* self);
char* SshClient_workspaceRoot(SshClient* self);
btrc_Vector_string* SshClient_sshOptionList(SshClient* self);
char* SshClient_sshOptionsForShell(SshClient* self);
Command* SshClient_addSshOptions(SshClient* self, Command* cmd);
ExecResult* SshClient_sshWithTimeout(SshClient* self, char* command, bool checkStatus, int timeoutSeconds);
ExecResult* SshClient_ssh(SshClient* self, char* command, bool checkStatus);
ExecResult* SshClient_host(SshClient* self, char* command, bool checkStatus);
ExecResult* SshClient_workspaceFileExists(SshClient* self, char* relativePath);
ExecResult* SshClient_nixEval(SshClient* self, char* attribute, int timeoutSeconds);
ExecResult* SshClient_qmp(SshClient* self, char* command, int timeoutSeconds);
void SshClient_copyWorkspace(SshClient* self, char* localPath, char* remotePath);
void SshClient_copyTo(SshClient* self, char* localPath, char* remotePath);
void SshClient_copyFrom(SshClient* self, char* remotePath, char* localPath);
void QemuSerial_init(QemuSerial* self, VmTestSpec* spec);
QemuSerial* QemuSerial_new(VmTestSpec* spec);
char* QemuSerial_serialBasePath(QemuSerial* self);
char* QemuSerial_serialInPath(QemuSerial* self);
char* QemuSerial_serialOutPath(QemuSerial* self);
char* QemuSerial_serialLogPath(QemuSerial* self);
char* QemuSerial_serialReaderPidPath(QemuSerial* self);
char* QemuSerial_bootDir(QemuSerial* self);
char* QemuSerial_bootKernelFile(QemuSerial* self);
char* QemuSerial_bootInitrdFile(QemuSerial* self);
char* QemuSerial_bootAppendFile(QemuSerial* self);
char* QemuSerial_stripLeadingSlash(QemuSerial* self, char* path);
char* QemuSerial_valueAfterLinePrefix(QemuSerial* self, char* text, char* prefix, int start);
void QemuSerial_extractBootSerial(QemuSerial* self);
void QemuSerial_prepareSerialPipe(QemuSerial* self);
void QemuSerial_startSerialReader(QemuSerial* self);
void QemuSerial_stopSerialReader(QemuSerial* self);
void QemuSerial_serialSend(QemuSerial* self, char* command);
void QemuFirmware_init(QemuFirmware* self, VmTestSpec* spec);
QemuFirmware* QemuFirmware_new(VmTestSpec* spec);
char* QemuFirmware_firmwareVarsPath(QemuFirmware* self);
char* QemuFirmware_firmwareVarsSnapshotPath(QemuFirmware* self, char* name);
char* QemuFirmware_tpmDir(QemuFirmware* self);
char* QemuFirmware_tpmStateDir(QemuFirmware* self);
char* QemuFirmware_tpmSocketPath(QemuFirmware* self);
char* QemuFirmware_tpmPidPath(QemuFirmware* self);
char* QemuFirmware_tpmLogPath(QemuFirmware* self);
char* QemuFirmware_qemuBinary(QemuFirmware* self);
bool QemuFirmware_argEnabled(QemuFirmware* self, char* key);
bool QemuFirmware_tpm2Enabled(QemuFirmware* self);
bool QemuFirmware_secureBootEnabled(QemuFirmware* self);
bool QemuFirmware_uefiEnabled(QemuFirmware* self);
bool QemuFirmware_shouldUseUefi(QemuFirmware* self, bool fromIso);
bool QemuFirmware_hostArchMatchesGuest(QemuFirmware* self);
char* QemuFirmware_qemuSharePath(QemuFirmware* self, char* fileName);
char* QemuFirmware_findFirst(QemuFirmware* self, char* findArgs);
char* QemuFirmware_firmwareCodePath(QemuFirmware* self);
char* QemuFirmware_secureFirmwareCodePath(QemuFirmware* self);
char* QemuFirmware_firmwareVarsTemplatePath(QemuFirmware* self);
void QemuFirmware_makeFirmwareVarsWritable(QemuFirmware* self);
void QemuFirmware_setupFirmwareVars(QemuFirmware* self);
bool QemuFirmware_commandExists(QemuFirmware* self, char* name);
void QemuFirmware_requireCommand(QemuFirmware* self, char* name);
bool QemuFirmware_qemuDeviceAvailable(QemuFirmware* self, char* deviceName);
char* QemuFirmware_tpmQemuDevice(QemuFirmware* self);
void QemuFirmware_requireTpm2Capability(QemuFirmware* self);
void QemuFirmware_requireUefiCapability(QemuFirmware* self);
void QemuFirmware_requireSecureBootCapability(QemuFirmware* self);
char* QemuFirmware_secureBootCapabilityReport(QemuFirmware* self);
bool QemuFirmware_isDarwin(QemuFirmware* self);
void QemuFirmware_startSwtpm(QemuFirmware* self);
void QemuFirmware_stopSwtpm(QemuFirmware* self);
void QemuFirmware_addFirmware(QemuFirmware* self, Command* cmd, bool fromIso);
void QemuFirmware_addTpm2(QemuFirmware* self, Command* cmd);
void VmStateStore_init(VmStateStore* self, VmTestSpec* spec, QemuFirmware* firmware);
VmStateStore* VmStateStore_new(VmTestSpec* spec, QemuFirmware* firmware);
char* VmStateStore_diskPath(VmStateStore* self);
char* VmStateStore_pidPath(VmStateStore* self);
char* VmStateStore_sshKeyPath(VmStateStore* self);
char* VmStateStore_sshPubKeyPath(VmStateStore* self);
char* VmStateStore_parentStateDir(VmStateStore* self);
char* VmStateStore_parentWorkDirFile(VmStateStore* self);
char* VmStateStore_backingDiskFile(VmStateStore* self);
void VmStateStore_ensureWorkDir(VmStateStore* self);
char* VmStateStore_absolutePath(VmStateStore* self, char* path);
void VmStateStore_cleanStateRecord(VmStateStore* self);
void VmStateStore_requireParentState(VmStateStore* self);
void VmStateStore_copyIfExists(VmStateStore* self, char* source, char* target);
void VmStateStore_copyTreeIfExists(VmStateStore* self, char* source, char* target);
void VmStateStore_inheritState(VmStateStore* self);
void VmStateStore_recordState(VmStateStore* self);
bool VmStateStore_isRunning(VmStateStore* self);
bool VmStateStore_hasSnapshot(VmStateStore* self, char* name);
bool VmStateStore_hasBackingDisk(VmStateStore* self);
void VmStateStore_printStatus(VmStateStore* self);
void VmProvisioner_init(VmProvisioner* self, VmTestSpec* spec, SshClient* remote);
VmProvisioner* VmProvisioner_new(VmTestSpec* spec, SshClient* remote);
char* VmProvisioner_pubKey(VmProvisioner* self);
void VmProvisioner_configureVmHost(VmProvisioner* self);
bool VmProvisioner_installNixosGuest(VmProvisioner* self);
void QemuCommandBuilder_init(QemuCommandBuilder* self, VmTestSpec* spec, QemuFirmware* firmware, QemuSerial* serial);
QemuCommandBuilder* QemuCommandBuilder_new(VmTestSpec* spec, QemuFirmware* firmware, QemuSerial* serial);
char* QemuCommandBuilder_diskPath(QemuCommandBuilder* self);
char* QemuCommandBuilder_monitorPath(QemuCommandBuilder* self);
char* QemuCommandBuilder_qmpPath(QemuCommandBuilder* self);
void QemuCommandBuilder_addAccelerator(QemuCommandBuilder* self, Command* cmd);
void QemuCommandBuilder_addMachine(QemuCommandBuilder* self, Command* cmd);
void QemuCommandBuilder_addMemoryCpus(QemuCommandBuilder* self, Command* cmd);
void QemuCommandBuilder_addStorageAndNetwork(QemuCommandBuilder* self, Command* cmd);
void QemuCommandBuilder_addConsole(QemuCommandBuilder* self, Command* cmd);
void QemuCommandBuilder_addBootMedia(QemuCommandBuilder* self, Command* cmd, bool fromIso);
Command* QemuCommandBuilder_build(QemuCommandBuilder* self, bool fromIso);
void VmAssets_init(VmAssets* self, VmTestSpec* spec, QemuSerial* serial);
VmAssets* VmAssets_new(VmTestSpec* spec, QemuSerial* serial);
char* VmAssets_sshKeyPath(VmAssets* self);
char* VmAssets_sshPubKeyPath(VmAssets* self);
char* VmAssets_diskPath(VmAssets* self);
void VmAssets_ensureWorkDir(VmAssets* self);
void VmAssets_downloadIso(VmAssets* self);
void VmAssets_createSshKey(VmAssets* self);
char* VmAssets_sshPubKey(VmAssets* self);
void VmAssets_createDisk(VmAssets* self);
void VmAssets_setup(VmAssets* self);
void QemuE2eHarness_init(QemuE2eHarness* self, VmTestSpec* spec);
QemuE2eHarness* QemuE2eHarness_new(VmTestSpec* spec);
char* QemuE2eHarness_diskPath(QemuE2eHarness* self);
char* QemuE2eHarness_pidPath(QemuE2eHarness* self);
char* QemuE2eHarness_monitorPath(QemuE2eHarness* self);
char* QemuE2eHarness_qmpPath(QemuE2eHarness* self);
void QemuE2eHarness_ensureWorkDir(QemuE2eHarness* self);
void QemuE2eHarness_downloadIso(QemuE2eHarness* self);
void QemuE2eHarness_createSshKey(QemuE2eHarness* self);
char* QemuE2eHarness_sshPubKey(QemuE2eHarness* self);
void QemuE2eHarness_createDisk(QemuE2eHarness* self);
void QemuE2eHarness_setup(QemuE2eHarness* self);
void QemuE2eHarness_resetState(QemuE2eHarness* self);
void QemuE2eHarness_cleanStateRecord(QemuE2eHarness* self);
void QemuE2eHarness_requireParentState(QemuE2eHarness* self);
void QemuE2eHarness_inheritState(QemuE2eHarness* self);
void QemuE2eHarness_recordState(QemuE2eHarness* self);
void QemuE2eHarness_printStatus(QemuE2eHarness* self);
void QemuE2eHarness_start(QemuE2eHarness* self, bool fromIso);
void QemuE2eHarness_stop(QemuE2eHarness* self);
void QemuE2eHarness_sleepSeconds(QemuE2eHarness* self, int seconds);
void QemuE2eHarness_bootstrapSsh(QemuE2eHarness* self);
void QemuE2eHarness_upFromIso(QemuE2eHarness* self);
void QemuE2eHarness_rebootDisk(QemuE2eHarness* self);
ExecResult* QemuE2eHarness_ssh(QemuE2eHarness* self, char* command, bool checkStatus);
ExecResult* QemuE2eHarness_sshWithTimeout(QemuE2eHarness* self, char* command, bool checkStatus, int timeoutSeconds);
ExecResult* QemuE2eHarness_host(QemuE2eHarness* self, char* command, bool checkStatus);
ExecResult* QemuE2eHarness_nixEval(QemuE2eHarness* self, char* attribute, int timeoutSeconds);
ExecResult* QemuE2eHarness_qmp(QemuE2eHarness* self, char* command, int timeoutSeconds);
ExecResult* QemuE2eHarness_workspaceFileExists(QemuE2eHarness* self, char* relativePath);
void QemuE2eHarness_copyWorkspace(QemuE2eHarness* self, char* localPath, char* remotePath);
void QemuE2eHarness_copyTo(QemuE2eHarness* self, char* localPath, char* remotePath);
void QemuE2eHarness_copyFrom(QemuE2eHarness* self, char* remotePath, char* localPath);
char* QemuE2eHarness_serialLogPath(QemuE2eHarness* self);
void QemuE2eHarness_serialSend(QemuE2eHarness* self, char* command);
void QemuE2eHarness_requireCommand(QemuE2eHarness* self, char* name);
void QemuE2eHarness_requireTpm2Capability(QemuE2eHarness* self);
void QemuE2eHarness_requireUefiCapability(QemuE2eHarness* self);
void QemuE2eHarness_requireSecureBootCapability(QemuE2eHarness* self);
char* QemuE2eHarness_secureBootCapabilityReport(QemuE2eHarness* self);
bool QemuE2eHarness_waitForSsh(QemuE2eHarness* self, int timeout);
void QemuE2eHarness_configureVmHost(QemuE2eHarness* self);
void QemuE2eHarness_installNixosGuest(QemuE2eHarness* self);
void QemuE2eHarness_snapshot(QemuE2eHarness* self, char* name);
void QemuE2eHarness_restore(QemuE2eHarness* self, char* name);
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
void E2eCli_init(E2eCli* self);
E2eCli* E2eCli_new(void);
char* E2eCli_tail(E2eCli* self, CliArgs* args, int startIndex);
void E2eCli_applySpecArgs(E2eCli* self, VmTestSpec* spec, CliArgs* args, int startIndex);
int E2eCli_runVm(E2eCli* self, CliArgs* args);
btrc_Map_string_string* E2eCli_graphArgs(E2eCli* self, CliArgs* args, int startIndex);
btrc_Vector_string* E2eCli_graphTargets(E2eCli* self, CliArgs* args, int startIndex);
int E2eCli_runGraph(E2eCli* self, CliArgs* args);
int E2eCli_runE2e(E2eCli* self, CliArgs* args);
void NixosCtl_init(NixosCtl* self);
NixosCtl* NixosCtl_new(void);
char* NixosCtl_env(char* name, char* fallback);
void NixosCtl_usage(NixosCtl* self);
bool NixosCtl_needsRoot(NixosCtl* self, char* command);
int NixosCtl_sudoSelf(NixosCtl* self, CliArgs* args);
int NixosCtl_run(NixosCtl* self, CliArgs* args);
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

struct SshClient {
    int __rc;
    VmTestSpec* spec;
    UnixShell* shell;
};

struct QemuSerial {
    int __rc;
    VmTestSpec* spec;
    UnixShell* shell;
};

struct QemuFirmware {
    int __rc;
    VmTestSpec* spec;
    UnixShell* shell;
};

struct VmStateStore {
    int __rc;
    VmTestSpec* spec;
    UnixShell* shell;
    QemuFirmware* firmware;
};

struct VmProvisioner {
    int __rc;
    VmTestSpec* spec;
    SshClient* remote;
};

struct QemuCommandBuilder {
    int __rc;
    VmTestSpec* spec;
    QemuFirmware* firmware;
    QemuSerial* serial;
};

struct VmAssets {
    int __rc;
    VmTestSpec* spec;
    UnixShell* shell;
    QemuSerial* serial;
};

struct QemuE2eHarness {
    int __rc;
    VmTestSpec* spec;
    UnixShell* shell;
    SshClient* remote;
    QemuSerial* serial;
    QemuFirmware* firmware;
    VmStateStore* state;
    VmProvisioner* provisioner;
    QemuCommandBuilder* qemuCmd;
    VmAssets* assets;
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

struct E2eCli {
    int __rc;
};

struct NixosCtl {
    int __rc;
    NixosConfig* config;
};

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
    return PathTools_join(self->root, "config.json");
}

char* NixosPaths_hostsPath(NixosPaths* self) {
    return PathTools_join(self->root, "modules/hosts");
}

char* NixosPaths_secretsPathFallback(NixosPaths* self) {
    return PathTools_join(self->root, "secrets");
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
    return FileSystem_exists(self->path);
}

JsonObject* LocalConfigFile_read(LocalConfigFile* self) {
    if (!LocalConfigFile_exists(self)) {
        return JsonObject_new();
    }
    return JsonObject_readFile(self->path);
}

void LocalConfigFile_overwrite(LocalConfigFile* self, JsonObject* data) {
    char* dir = PathTools_dirname(self->path);
    FileSystem_mkdirp(dir);
    JsonObject_writeFile(data, self->path);
}

char* LocalConfigFile_getString(LocalConfigFile* self, char* key, char* fallback) {
    JsonObject* data = LocalConfigFile_read(self);
    char* value = JsonObject_getString(data, key, fallback);
    return Strings_copy(value);
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
    return ExecResult_stdout(result);
}

bool Interactive_confirm(Interactive* self, char* prompt) {
    while (true) {
        char* response = __btrc_str_track(__btrc_trim(__btrc_str_track(__btrc_toLower(Interactive_ask(self, __btrc_str_track(__btrc_strcat(prompt, " (y/n):")))))));
        if ((strcmp(response, "y") == 0) || (strcmp(response, "yes") == 0)) {
            return true;
        }
        if ((strcmp(response, "n") == 0) || (strcmp(response, "no") == 0)) {
            return false;
        }
        Console_error("Invalid input. Enter 'y' or 'n'.");
    }
    return false;
}

char* Interactive_askPassword(Interactive* self, char* prompt) {
    char* command = __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("printf ", UnixShell_quote(__btrc_str_track(__btrc_strcat(prompt, ": "))))), " >&2; stty -echo; IFS= read -r value; stty echo; printf '\\n' >&2; printf '%s' \"$value\""));
    ExecResult* result = UnixShell_runRaw(self->shell, command, true, false, "");
    return ExecResult_stdout(result);
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
    return "";
}

char* Interactive_askHostPath(Interactive* self, char* hostsPath) {
    ExecResult* found = UnixShell_runUnchecked(self->shell, __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("find ", UnixShell_quote(hostsPath))), " -type f -name '*.nix' | sort")));
    btrc_Vector_string* candidates = btrc_Vector_string_new();
    int __n_65 = btrc_Vector_string_iterLen(Strings_split(ExecResult_stdout(found), "\n"));
    for (int __i_64 = 0; (__i_64 < __n_65); (__i_64++)) {
        char* line = btrc_Vector_string_iterGet(Strings_split(ExecResult_stdout(found), "\n"), __i_64);
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
            return btrc_Vector_string_get(candidates, selected);
        }
        NixosLog_error("Invalid choice.");
    }
    return btrc_Vector_string_get(candidates, 0);
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
    return btrc_Map_string_string_has(self->values, key);
}

char* NixEvalCache_get(NixEvalCache* self, char* key) {
    return btrc_Map_string_string_get(self->values, key);
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
    return LocalConfigFile_exists(self->local);
}

char* NixosConfig_hostPath(NixosConfig* self) {
    return LocalConfigFile_getString(self->local, "host_path", "");
}

char* NixosConfig_target(NixosConfig* self) {
    return LocalConfigFile_getString(self->local, "target", "Standard-Boot");
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
    return Strings_replace(base, ".nix", "");
}

char* NixosConfig_flakeRef(NixosConfig* self) {
    return __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(self->paths->root, "#")), NixosConfig_host(self))), "-")), NixosConfig_target(self)));
}

Command* NixosConfig_nixCmd(NixosConfig* self) {
    return Command_arg(Command_arg(Command_arg(Command_arg(Command_new("nix"), "--extra-experimental-features"), "nix-command"), "--extra-experimental-features"), "flakes");
}

char* NixosConfig_inputLockedRev(NixosConfig* self, char* inputName) {
    Command* cmd = Command_arg(Command_arg(Command_arg(Command_arg(Command_arg(Command_arg(NixosConfig_nixCmd(self), "flake"), "metadata"), inputName), "--json"), "-I"), self->paths->root);
    ExecResult* result = UnixShell_runCommand(self->shell, cmd);
    if (!ExecResult_ok(result)) {
        return "master";
    }
    char* json = ExecResult_stdout(result);
    char* marker = "\"rev\":\"";
    int start = Strings_find(json, marker, 0);
    if (start < 0) {
        return "master";
    }
    (start = (start + ((int)strlen(marker))));
    int end = start;
    while ((json[end] != '\0') && (json[end] != '"')) {
        (end++);
    }
    return JsonObject_slice(json, start, end);
}

char* NixosConfig_evalRaw(NixosConfig* self, char* attribute) {
    char* key = __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(NixosConfig_flakeRef(self), ":")), attribute));
    if (NixEvalCache_has(self->cache, key)) {
        return NixEvalCache_get(self->cache, key);
    }
    Command* cmd = Command_arg(Command_arg(NixosConfig_nixCmd(self), "eval"), __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(self->paths->root, "#nixosConfigurations.")), NixosConfig_host(self))), "-")), NixosConfig_target(self))), ".")), attribute)));
    ExecResult* result = UnixShell_runCommand(self->shell, cmd);
    if (!ExecResult_ok(result)) {
        NixosLog_fatal(__btrc_str_track(__btrc_strcat("nix eval failed for ", attribute)));
    }
    char* value = ExecResult_trimmed(result);
    (value = Strings_replace(value, "\"", ""));
    NixEvalCache_put(self->cache, key, value);
    return value;
}

bool NixosConfig_evalBool(NixosConfig* self, char* attribute) {
    char* value = NixosConfig_evalRaw(self, attribute);
    return (strcmp(value, "true") == 0);
}

char* NixosConfig_standardTarget(NixosConfig* self) {
    return "Standard-Boot";
}

char* NixosConfig_secureBootTarget(NixosConfig* self) {
    return "Secure-Boot";
}

char* NixosConfig_diskOperationTarget(NixosConfig* self) {
    return "Disk-Operation";
}

char* NixosConfig_username(NixosConfig* self) {
    return NixosConfig_evalRaw(self, "config.settings.user.admin.username");
}

char* NixosConfig_secretsPath(NixosConfig* self) {
    return NixosConfig_evalRaw(self, "config.settings.secrets.path");
}

char* NixosConfig_hashedPasswordPath(NixosConfig* self) {
    char* name = NixosConfig_evalRaw(self, "config.settings.secrets.hashedPasswordFile");
    return PathTools_join(NixosConfig_secretsPath(self), name);
}

char* NixosConfig_diskDevice(NixosConfig* self) {
    return NixosConfig_evalRaw(self, "config.settings.disk.device");
}

char* NixosConfig_rootPartLabelPath(NixosConfig* self) {
    return NixosConfig_evalRaw(self, "config.settings.disk.by.partlabel.root");
}

char* NixosConfig_tpmDevice(NixosConfig* self) {
    return NixosConfig_evalRaw(self, "config.settings.tpm.device");
}

char* NixosConfig_tpmVersionPath(NixosConfig* self) {
    return NixosConfig_evalRaw(self, "config.settings.tpm.versionPath");
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
    return FileSystem_exists(NixosConfig_hashedPasswordPath(self->config));
}

void SecretsManager_writeHashedPassword(SecretsManager* self, char* hashed) {
    FileSystem_mkdirp(PathTools_dirname(NixosConfig_hashedPasswordPath(self->config)));
    Path_writeAll(NixosConfig_hashedPasswordPath(self->config), hashed);
}

bool SecretsManager_needsPassword(SecretsManager* self, char* plainTextPasswordPath) {
    if (!SecretsManager_hasHashedPassword(self)) {
        return true;
    }
    if ((!__btrc_isEmpty(plainTextPasswordPath)) && (!FileSystem_exists(plainTextPasswordPath))) {
        return true;
    }
    return false;
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
    char* __btrc_ret_66 = ExecResult_trimmed(result);
    if (cmd != NULL) {
        if ((--cmd->__rc) <= 0) {
            Command_destroy(cmd);
        }
    }
    return __btrc_ret_66;
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

char* PermissionsManager_ignorePredicate(PermissionsManager* self) {
    return __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("\\( -path ", UnixShell_quote("*/secrets*"))), " -o -path ")), UnixShell_quote("*/.venv*"))), " -o -path ")), UnixShell_quote("*/.direnv*"))), " \\)"));
}

void PermissionsManager_findChmod(PermissionsManager* self, char* quotedRoot, char* ignores, char* predicate, char* mode) {
    UnixShell_runRaw(self->shell, __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("find ", quotedRoot)), " ")), ignores)), " -prune -o ")), predicate)), " -exec chmod ")), mode)), " {} +")), false, false, "");
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
    char* ignores = PermissionsManager_ignorePredicate(self);
    PermissionsManager_findChmod(self, quotedRoot, ignores, "-type d", "755");
    PermissionsManager_findChmod(self, quotedRoot, ignores, "-type f", "644");
    PermissionsManager_findChmod(self, quotedRoot, ignores, "\\( -path '*/scripts/*' -o -path '*/bin/*' \\)", "755");
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
        return self->rootPrefix;
    }
    if (__btrc_startsWith(path, "/")) {
        return __btrc_str_track(__btrc_strcat(self->rootPrefix, path));
    }
    return PathTools_join(self->rootPrefix, path);
}

char* SnapshotManager_snapshotsPath(SnapshotManager* self) {
    return NixosConfig_evalRaw(self->config, "config.settings.disk.subvolumes.snapshots.mountPoint");
}

char* SnapshotManager_cleanName(SnapshotManager* self) {
    return NixosConfig_evalRaw(self->config, "config.settings.disk.immutability.persist.snapshots.cleanName");
}

btrc_Vector_ResetSubvolume* SnapshotManager_resetSubvolumes(SnapshotManager* self) {
    btrc_Vector_ResetSubvolume* result = btrc_Vector_ResetSubvolume_new();
    char* raw = NixosConfig_evalRaw(self->config, "config.settings.disk.subvolumes.nameMountPointPairs.resetOnBoot");
    btrc_Vector_string* pairs = Strings_split(raw, " ");
    int __n_68 = btrc_Vector_string_iterLen(pairs);
    for (int __i_67 = 0; (__i_67 < __n_68); (__i_67++)) {
        char* pair = btrc_Vector_string_iterGet(pairs, __i_67);
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
    return PathTools_join(PathTools_join(SnapshotManager_snapshotsPath(self), subvolumeName), SnapshotManager_cleanName(self));
}

bool SnapshotManager_isSubvolume(SnapshotManager* self, char* path) {
    if (!FileSystem_exists(path)) {
        return false;
    }
    Command* cmd = Command_check(Command_arg(Command_arg(Command_arg(Command_new("btrfs"), "subvolume"), "show"), path), false);
    return ExecResult_ok(UnixShell_runCommand(self->shell, cmd));
}

bool SnapshotManager_isReadonly(SnapshotManager* self, char* path) {
    Command* cmd = Command_check(Command_arg(Command_arg(Command_arg(Command_arg(Command_arg(Command_new("btrfs"), "property"), "get"), "-ts"), path), "ro"), false);
    ExecResult* result = UnixShell_runCommand(self->shell, cmd);
    return (ExecResult_ok(result) && __btrc_strContains(ExecResult_stdout(result), "ro=true"));
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
    int __n_70 = btrc_Vector_string_iterLen(Strings_split(ExecResult_stdout(listed), "\n"));
    for (int __i_69 = 0; (__i_69 < __n_70); (__i_69++)) {
        char* line = btrc_Vector_string_iterGet(Strings_split(ExecResult_stdout(listed), "\n"), __i_69);
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
    int __n_72 = btrc_Vector_string_iterLen(SnapshotManager_childSubvolumes(self, path));
    for (int __i_71 = 0; (__i_71 < __n_72); (__i_71++)) {
        char* child = btrc_Vector_string_iterGet(SnapshotManager_childSubvolumes(self, path), __i_71);
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
    int __n_74 = btrc_Vector_ResetSubvolume_iterLen(volumes);
    for (int __i_73 = 0; (__i_73 < __n_74); (__i_73++)) {
        ResetSubvolume* volume = btrc_Vector_ResetSubvolume_iterGet(volumes, __i_73);
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

ExecResult* NixosRebuilder_runNixCollectGarbage(NixosRebuilder* self) {
    Command* cmd = Command_new("nix-collect-garbage");
    Command_arg(cmd, "-d");
    Command_capture(cmd, false);
    ExecResult* __btrc_ret_75 = UnixShell_runCommand(self->shell, cmd);
    if (cmd != NULL) {
        if ((--cmd->__rc) <= 0) {
            Command_destroy(cmd);
        }
    }
    return __btrc_ret_75;
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
    ExecResult* __btrc_ret_76 = UnixShell_runCommand(self->shell, cmd);
    if (cmd != NULL) {
        if ((--cmd->__rc) <= 0) {
            Command_destroy(cmd);
        }
    }
    return __btrc_ret_76;
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
    ExecResult* __btrc_ret_77 = UnixShell_runCommand(self->shell, cmd);
    if (cmd != NULL) {
        if ((--cmd->__rc) <= 0) {
            Command_destroy(cmd);
        }
    }
    return __btrc_ret_77;
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
    ExecResult* __btrc_ret_78 = UnixShell_runCommand(self->shell, cmd);
    if (cmd != NULL) {
        if ((--cmd->__rc) <= 0) {
            Command_destroy(cmd);
        }
    }
    return __btrc_ret_78;
    if (cmd != NULL) {
        if ((--cmd->__rc) <= 0) {
            Command_destroy(cmd);
        }
    }
}

char* NixosRebuilder_immutabilityDevice(NixosRebuilder* self) {
    if (NixosConfig_evalBool(self->config, "config.settings.disk.encryption.enable")) {
        return NixosConfig_evalRaw(self->config, "config.settings.disk.by.mapper.root");
    }
    return NixosConfig_rootPartLabelPath(self->config);
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
        return NixosConfig_evalRaw(self->config, "config.settings.disk.encryption.plainTextPasswordFile");
    }
    return "";
}

char* NixosRebuilder_homeManagerLogs(NixosRebuilder* self) {
    ExecResult* result = UnixShell_runUnchecked(self->shell, "journalctl -u 'home-manager-*.service' --no-pager -o cat -q -r");
    btrc_Vector_string* lines = btrc_Vector_string_new();
    int __n_80 = btrc_Vector_string_iterLen(Strings_split(ExecResult_stdout(result), "\n"));
    for (int __i_79 = 0; (__i_79 < __n_80); (__i_79++)) {
        char* line = btrc_Vector_string_iterGet(Strings_split(ExecResult_stdout(result), "\n"), __i_79);
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
    return __btrc_str_track(__btrc_trim(btrc_Vector_string_join(lines, "\n")));
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
    NixosRebuilder_convergeMembrane(self, mode, dryRun);
    if (dryRun) {
        NixosLog_info("DRY would start semipermeable-membrane-mounts.service");
        if (snapshots != NULL) {
            if ((--snapshots->__rc) <= 0) {
                SnapshotManager_destroy(snapshots);
            }
        }
        return;
    }
    UnixShell_runRaw(self->shell, "systemctl start semipermeable-membrane-mounts.service", false, false, "");
    if (snapshots != NULL) {
        if ((--snapshots->__rc) <= 0) {
            SnapshotManager_destroy(snapshots);
        }
    }
}

void NixosRebuilder_convergeMembrane(NixosRebuilder* self, char* mode, bool dryRun) {
    NixosLog_info("Converging semipermeable membrane after update");
    btrc_Vector_string* __list_81 = btrc_Vector_string_new();
    btrc_Vector_string_push(__list_81, "/run/current-system/sw/bin/semipermeable_membrane");
    btrc_Vector_string_push(__list_81, NixosRebuilder_immutabilityDevice(self));
    btrc_Vector_string_push(__list_81, NixosConfig_evalRaw(self->config, "config.settings.disk.subvolumes.snapshots.name"));
    btrc_Vector_string_push(__list_81, NixosConfig_evalRaw(self->config, "config.settings.disk.immutability.persist.snapshots.cleanName"));
    btrc_Vector_string_push(__list_81, mode);
    btrc_Vector_string_push(__list_81, NixosConfig_evalRaw(self->config, "config.settings.disk.immutability.semipermeable_membrane.persist.subvolumeRoot"));
    btrc_Vector_string_push(__list_81, "/etc/semipermeable_membrane/spec.tsv");
    btrc_Vector_string* args = __list_81;
    if (dryRun) {
        btrc_Vector_string_insert(args, 1, "--dry-run");
    }
    char* pairs = NixosConfig_evalRaw(self->config, "config.settings.disk.subvolumes.nameMountPointPairs.resetOnBoot");
    int __n_83 = btrc_Vector_string_iterLen(Strings_split(pairs, " "));
    for (int __i_82 = 0; (__i_82 < __n_83); (__i_82++)) {
        char* pair = btrc_Vector_string_iterGet(Strings_split(pairs, " "), __i_82);
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
    if (membrane != NULL) {
        if ((--membrane->__rc) <= 0) {
            Command_destroy(membrane);
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

btrc_Vector_string* DiffScanner_keepPaths(DiffScanner* self) {
    char* raw = NixosConfig_evalRaw(self->config, "config.settings.disk.immutability.persist.paths");
    (raw = Strings_replace(raw, "[", ""));
    (raw = Strings_replace(raw, "]", ""));
    btrc_Vector_string* result = btrc_Vector_string_new();
    int __n_85 = btrc_Vector_string_iterLen(Strings_split(__btrc_str_track(__btrc_trim(raw)), " "));
    for (int __i_84 = 0; (__i_84 < __n_85); (__i_84++)) {
        char* item = btrc_Vector_string_iterGet(Strings_split(__btrc_str_track(__btrc_trim(raw)), " "), __i_84);
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
    int __n_87 = btrc_Vector_string_iterLen(Strings_split(Path_readAll(path), "\n"));
    for (int __i_86 = 0; (__i_86 < __n_87); (__i_86++)) {
        char* line = btrc_Vector_string_iterGet(Strings_split(Path_readAll(path), "\n"), __i_86);
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
    int __n_89 = btrc_Vector_ResetSubvolume_iterLen(volumes);
    for (int __i_88 = 0; (__i_88 < __n_89); (__i_88++)) {
        ResetSubvolume* volume = btrc_Vector_ResetSubvolume_iterGet(volumes, __i_88);
        btrc_Vector_string_push(result, volume->mountPoint);
    }
    return result;
}

btrc_Vector_string* DiffScanner_changedFiles(DiffScanner* self) {
    btrc_Vector_string* result = btrc_Vector_string_new();
    btrc_Vector_ResetSubvolume* volumes = SnapshotManager_resetSubvolumes(self->snapshots);
    int __n_91 = btrc_Vector_ResetSubvolume_iterLen(volumes);
    for (int __i_90 = 0; (__i_90 < __n_91); (__i_90++)) {
        ResetSubvolume* volume = btrc_Vector_ResetSubvolume_iterGet(volumes, __i_90);
        char* tmp = PathTools_join(PathTools_join(SnapshotManager_snapshotsPath(self->snapshots), volume->name), "tmp");
        char* clean = SnapshotManager_cleanSnapshotPath(self->snapshots, volume->name);
        SnapshotManager_deleteSubvolume(self->snapshots, tmp);
        SnapshotManager_createReadonlySnapshot(self->snapshots, volume->mountPoint, tmp);
        ExecResult* tx = UnixShell_runUnchecked(self->shell, __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("echo \"$(sudo btrfs subvolume find-new ", UnixShell_quote(clean))), " 9999999)\" | cut -d' ' -f4")));
        char* transaction = ExecResult_trimmed(tx);
        ExecResult* changes = UnixShell_runUnchecked(self->shell, __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("btrfs subvolume find-new ", UnixShell_quote(tmp))), " ")), UnixShell_quote(transaction))), " | sed '$d' | cut -f17- -d' ' | sort | uniq")));
        if (ExecResult_ok(changes)) {
            int __n_93 = btrc_Vector_string_iterLen(Strings_split(ExecResult_stdout(changes), "\n"));
            for (int __i_92 = 0; (__i_92 < __n_93); (__i_92++)) {
                char* line = btrc_Vector_string_iterGet(Strings_split(ExecResult_stdout(changes), "\n"), __i_92);
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
    int __n_95 = btrc_Vector_string_iterLen(keepPaths);
    for (int __i_94 = 0; (__i_94 < __n_95); (__i_94++)) {
        char* keepPath = btrc_Vector_string_iterGet(keepPaths, __i_94);
        if ((strcmp(path, keepPath) == 0) || __btrc_startsWith(path, __btrc_str_track(__btrc_strcat(keepPath, "/")))) {
            return true;
        }
    }
    return false;
}

bool DiffScanner_matchesPattern(DiffScanner* self, char* path, char* pattern) {
    if (__btrc_isEmpty(pattern)) {
        return true;
    }
    return Pattern_matches(__btrc_str_track(__btrc_toLower(pattern)), __btrc_str_track(__btrc_toLower(path)));
}

bool DiffScanner_ignored(DiffScanner* self, char* path, btrc_Vector_string* patterns) {
    int __n_97 = btrc_Vector_string_iterLen(patterns);
    for (int __i_96 = 0; (__i_96 < __n_97); (__i_96++)) {
        char* pattern = btrc_Vector_string_iterGet(patterns, __i_96);
        if (Pattern_matches(pattern, path)) {
            return true;
        }
    }
    return false;
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
        int __n_99 = btrc_Vector_string_iterLen(keepList);
        for (int __i_98 = 0; (__i_98 < __n_99); (__i_98++)) {
            char* keepPath = btrc_Vector_string_iterGet(keepList, __i_98);
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
    int __n_101 = btrc_Vector_string_iterLen(paths);
    for (int __i_100 = 0; (__i_100 < __n_101); (__i_100++)) {
        char* path = btrc_Vector_string_iterGet(paths, __i_100);
        btrc_Vector_string_push(result, DiffScanner_topAncestor(self, path, keepList, mounts));
    }
    btrc_Vector_string* distinct = btrc_Vector_string_distinct(result);
    btrc_Vector_string_sort(distinct);
    return distinct;
}

btrc_Vector_string* DiffScanner_collapseToPersist(DiffScanner* self, btrc_Vector_string* persisted, btrc_Vector_string* keepList) {
    btrc_Vector_string* result = btrc_Vector_string_new();
    int __n_103 = btrc_Vector_string_iterLen(persisted);
    for (int __i_102 = 0; (__i_102 < __n_103); (__i_102++)) {
        char* path = btrc_Vector_string_iterGet(persisted, __i_102);
        int __n_105 = btrc_Vector_string_iterLen(keepList);
        for (int __i_104 = 0; (__i_104 < __n_105); (__i_104++)) {
            char* keepPath = btrc_Vector_string_iterGet(keepList, __i_104);
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
    int __n_107 = btrc_Vector_string_iterLen(bases);
    for (int __i_106 = 0; (__i_106 < __n_107); (__i_106++)) {
        char* base = btrc_Vector_string_iterGet(bases, __i_106);
        char* prefix = __btrc_str_track(__btrc_strcat(base, "/"));
        int __n_109 = btrc_Vector_string_iterLen(source);
        for (int __i_108 = 0; (__i_108 < __n_109); (__i_108++)) {
            char* path = btrc_Vector_string_iterGet(source, __i_108);
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
    int __n_111 = btrc_Vector_string_iterLen(input);
    for (int __i_110 = 0; (__i_110 < __n_111); (__i_110++)) {
        char* path = btrc_Vector_string_iterGet(input, __i_110);
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
    int __n_113 = btrc_Vector_string_iterLen(btrc_Map_string_string_keys(data->values));
    for (int __i_112 = 0; (__i_112 < __n_113); (__i_112++)) {
        char* key = btrc_Vector_string_iterGet(btrc_Map_string_string_keys(data->values), __i_112);
        btrc_Vector_string_push(result, key);
    }
    return result;
}

void DiffScanner_writeCache(DiffScanner* self, char* cachePath, btrc_Vector_string* paths) {
    JsonObject* data = JsonObject_new();
    int __n_115 = btrc_Vector_string_iterLen(paths);
    for (int __i_114 = 0; (__i_114 < __n_115); (__i_114++)) {
        char* path = btrc_Vector_string_iterGet(paths, __i_114);
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
    int __n_117 = btrc_Vector_string_iterLen(changed);
    for (int __i_116 = 0; (__i_116 < __n_117); (__i_116++)) {
        char* path = btrc_Vector_string_iterGet(changed, __i_116);
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
        (output = DiffScanner_selectEphemeral(self, ephemeral, keepList, mounts, options));
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
    int __n_119 = btrc_Vector_string_iterLen(output);
    for (int __i_118 = 0; (__i_118 < __n_119); (__i_118++)) {
        char* path = btrc_Vector_string_iterGet(output, __i_118);
        Console_error(path);
    }
}

btrc_Vector_string* DiffScanner_selectEphemeral(DiffScanner* self, btrc_Vector_string* ephemeral, btrc_Vector_string* keepList, btrc_Vector_string* mounts, DiffOptions* options) {
    btrc_Vector_string* top = DiffScanner_collapse(self, ephemeral, keepList, mounts);
    char* cachePath = "/tmp/etc/nixos/scripts/bin/diff/cache.json";
    btrc_Vector_string* previous = DiffScanner_previousCache(self, cachePath);
    DiffScanner_writeCache(self, cachePath, top);
    if (options->recent) {
        btrc_Vector_string* recent = btrc_Vector_string_new();
        int __n_121 = btrc_Vector_string_iterLen(top);
        for (int __i_120 = 0; (__i_120 < __n_121); (__i_120++)) {
            char* path = btrc_Vector_string_iterGet(top, __i_120);
            if (!btrc_Vector_string_contains(previous, path)) {
                btrc_Vector_string_push(recent, path);
            }
        }
        (top = recent);
    }
    if (!options->showSymlinks) {
        btrc_Vector_string* filtered = btrc_Vector_string_new();
        int __n_123 = btrc_Vector_string_iterLen(top);
        for (int __i_122 = 0; (__i_122 < __n_123); (__i_122++)) {
            char* path = btrc_Vector_string_iterGet(top, __i_122);
            if (!FileSystem_isSymlink(path)) {
                btrc_Vector_string_push(filtered, path);
            }
        }
        (top = filtered);
    }
    if (!__btrc_isEmpty(options->showChildren)) {
        bool covers = false;
        int __n_125 = btrc_Vector_string_iterLen(top);
        for (int __i_124 = 0; (__i_124 < __n_125); (__i_124++)) {
            char* path = btrc_Vector_string_iterGet(top, __i_124);
            if (((strcmp(path, options->showChildren) == 0) || __btrc_startsWith(options->showChildren, __btrc_str_track(__btrc_strcat(path, "/")))) || __btrc_startsWith(path, __btrc_str_track(__btrc_strcat(options->showChildren, "/")))) {
                (covers = true);
            }
        }
        btrc_Vector_string* bases = btrc_Vector_string_new();
        if (covers) {
            btrc_Vector_string_push(bases, options->showChildren);
        }
        return DiffScanner_atDepth(self, bases, ephemeral, options->depth);
    }
    return DiffScanner_atDepth(self, top, ephemeral, options->depth);
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
    return UnixShell_runRaw(self->shell, command, false, true, "");
}

ExecResult* Installer_mountDisk(Installer* self) {
    return Installer_runDisko(self, "mount", "");
}

ExecResult* Installer_eraseAndMountDisk(Installer* self) {
    return Installer_runDisko(self, "destroy,format,mount", "--yes-wipe-all-disks");
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
        return NixosConfig_evalRaw(self->config, "config.settings.disk.encryption.plainTextPasswordFile");
    }
    return "";
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
    char* output = ExecResult_trimmed(result);
    if (__btrc_isEmpty(output) || (strcmp(output, "null") == 0)) {
        NixosLog_error("No EFI binaries registered");
        return;
    }
    btrc_Vector_string* unsignedPaths = SecureBootManager_unsignedFromJson(self, output);
    if (btrc_Vector_string_isEmpty(unsignedPaths)) {
        NixosLog_info("All EFI binaries are signed");
        return;
    }
    int __n_127 = btrc_Vector_string_iterLen(unsignedPaths);
    for (int __i_126 = 0; (__i_126 < __n_127); (__i_126++)) {
        char* path = btrc_Vector_string_iterGet(unsignedPaths, __i_126);
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
        return false;
    }
    char* version = __btrc_str_track(__btrc_trim(Path_readAll(NixosConfig_tpmVersionPath(self->config))));
    return (strcmp(version, "2") == 0);
}

bool Tpm2Manager_diskEncrypted(Tpm2Manager* self) {
    Command* cmd = Command_new("cryptsetup");
    Command_arg(cmd, "isLuks");
    Command_arg(cmd, NixosConfig_rootPartLabelPath(self->config));
    Command_check(cmd, false);
    ExecResult* result = UnixShell_runCommand(self->shell, cmd);
    bool __btrc_ret_128 = ExecResult_ok(result);
    if (cmd != NULL) {
        if ((--cmd->__rc) <= 0) {
            Command_destroy(cmd);
        }
    }
    return __btrc_ret_128;
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
    bool __btrc_ret_129 = ExecResult_ok(UnixShell_runCommand(self->shell, cmd));
    if (cmd != NULL) {
        if ((--cmd->__rc) <= 0) {
            Command_destroy(cmd);
        }
    }
    return __btrc_ret_129;
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
    bool __btrc_ret_130 = ExecResult_ok(UnixShell_runCommand(self->shell, cmd));
    if (cmd != NULL) {
        if ((--cmd->__rc) <= 0) {
            Command_destroy(cmd);
        }
    }
    return __btrc_ret_130;
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

bool PasswordManager_changeLuksPassword(PasswordManager* self, char* oldPassword, char* newPassword) {
    char* command = __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("printf '%s\\n%s' ", UnixShell_quote(oldPassword))), " ")), UnixShell_quote(newPassword))), " | cryptsetup luksChangeKey ")), UnixShell_quote(NixosConfig_rootPartLabelPath(self->config))));
    ExecResult* result = UnixShell_runRaw(self->shell, command, false, false, oldPassword);
    return ExecResult_ok(result);
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
    return btrc_Map_string_string_getOrDefault(Toml_sectionMap(self->content, "displays"), name, name);
}

btrc_Vector_DisplayLayoutRule* LabelsConfig_layoutRules(LabelsConfig* self) {
    btrc_Vector_DisplayLayoutRule* result = btrc_Vector_DisplayLayoutRule_new();
    int __n_132 = btrc_Vector_Map_string_string_iterLen(Toml_tableArrayBlocks(self->content, "layout"));
    for (int __i_131 = 0; (__i_131 < __n_132); (__i_131++)) {
        btrc_Map_string_string* block = btrc_Vector_Map_string_string_iterGet(Toml_tableArrayBlocks(self->content, "layout"), __i_131);
        DisplayLayoutRule* rule = DisplayLayoutRule_new();
        (rule->display = btrc_Map_string_string_getOrDefault(block, "display", ""));
        (rule->position = btrc_Map_string_string_getOrDefault(block, "position", ""));
        (rule->relativeTo = btrc_Map_string_string_getOrDefault(block, "relative_to", ""));
        if (!__btrc_isEmpty(rule->display)) {
            btrc_Vector_DisplayLayoutRule_push(result, rule);
        }
        if (rule != NULL) {
            if ((--rule->__rc) <= 0) {
                DisplayLayoutRule_destroy(rule);
            }
        }
    }
    return result;
}

btrc_Vector_AudioPreset* LabelsConfig_audioPresets(LabelsConfig* self) {
    btrc_Vector_AudioPreset* result = btrc_Vector_AudioPreset_new();
    int __n_134 = btrc_Vector_Map_string_string_iterLen(Toml_tableArrayBlocks(self->content, "audio"));
    for (int __i_133 = 0; (__i_133 < __n_134); (__i_133++)) {
        btrc_Map_string_string* block = btrc_Vector_Map_string_string_iterGet(Toml_tableArrayBlocks(self->content, "audio"), __i_133);
        AudioPreset* preset = AudioPreset_new();
        (preset->label = btrc_Map_string_string_getOrDefault(block, "label", ""));
        (preset->card = btrc_Map_string_string_getOrDefault(block, "card", ""));
        (preset->profile = btrc_Map_string_string_getOrDefault(block, "profile", ""));
        (preset->sink = btrc_Map_string_string_getOrDefault(block, "sink", ""));
        (preset->volume = btrc_Map_string_string_getOrDefault(block, "volume", ""));
        if ((!__btrc_isEmpty(preset->sink)) || (!__btrc_isEmpty(preset->label))) {
            btrc_Vector_AudioPreset_push(result, preset);
        }
        if (preset != NULL) {
            if ((--preset->__rc) <= 0) {
                AudioPreset_destroy(preset);
            }
        }
    }
    return result;
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
    return ExecResult_trimmed(result);
}

char* AudioManager_jsonStringValue(AudioManager* self, char* line, char* key) {
    char* marker = __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("\"", key)), "\""));
    int pos = Strings_find(line, marker, 0);
    if (pos < 0) {
        return "";
    }
    int colon = Strings_find(line, ":", pos);
    if (colon < 0) {
        return "";
    }
    int quote = Strings_find(line, "\"", (colon + 1));
    if (quote < 0) {
        return "";
    }
    int end = (quote + 1);
    bool escaped = false;
    while (end < ((int)strlen(line))) {
        char c = line[end];
        if ((!escaped) && (c == ((char)34))) {
            return JsonObject_slice(line, (quote + 1), end);
        }
        (escaped = ((!escaped) && (c == '\\')));
        if (c != '\\') {
            (escaped = false);
        }
        (end++);
    }
    return "";
}

btrc_Vector_AudioSink* AudioManager_sinks(AudioManager* self) {
    btrc_Vector_AudioSink* result = btrc_Vector_AudioSink_new();
    ExecResult* listed = UnixShell_runUnchecked(self->shell, "pactl -f json list sinks");
    AudioSink* current = AudioSink_new();
    int __n_136 = btrc_Vector_string_iterLen(Strings_split(ExecResult_stdout(listed), "\n"));
    for (int __i_135 = 0; (__i_135 < __n_136); (__i_135++)) {
        char* line = btrc_Vector_string_iterGet(Strings_split(ExecResult_stdout(listed), "\n"), __i_135);
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
    int __n_138 = btrc_Vector_AudioPreset_iterLen(presets);
    for (int __i_137 = 0; (__i_137 < __n_138); (__i_137++)) {
        AudioPreset* preset = btrc_Vector_AudioPreset_iterGet(presets, __i_137);
        char* name = (__btrc_isEmpty(preset->sink) ? preset->label : preset->sink);
        char* label = (__btrc_isEmpty(preset->label) ? name : preset->label);
        btrc_Vector_string_push(entries, AudioManager_entryJson(self, name, label, "", ((!__btrc_isEmpty(preset->sink)) && (strcmp(preset->sink, currentSink) == 0))));
        if (!__btrc_isEmpty(preset->sink)) {
            btrc_Vector_string_push(covered, preset->sink);
        }
    }
    btrc_Vector_AudioSink* allSinks = AudioManager_sinks(self);
    int __n_140 = btrc_Vector_AudioSink_iterLen(allSinks);
    for (int __i_139 = 0; (__i_139 < __n_140); (__i_139++)) {
        AudioSink* sink = btrc_Vector_AudioSink_iterGet(allSinks, __i_139);
        if (!btrc_Vector_string_contains(covered, sink->name)) {
            char* label = (__btrc_isEmpty(sink->description) ? sink->name : sink->description);
            btrc_Vector_string_push(entries, AudioManager_entryJson(self, sink->name, label, sink->description, (strcmp(sink->name, currentSink) == 0)));
        }
    }
    Console_log(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("[", btrc_Vector_string_join(entries, ","))), "]")));
}

AudioPreset* AudioManager_findPreset(AudioManager* self, char* selector) {
    btrc_Vector_AudioPreset* presets = LabelsConfig_audioPresets(self->labels);
    int __n_142 = btrc_Vector_AudioPreset_iterLen(presets);
    for (int __i_141 = 0; (__i_141 < __n_142); (__i_141++)) {
        AudioPreset* preset = btrc_Vector_AudioPreset_iterGet(presets, __i_141);
        if ((strcmp(selector, preset->label) == 0) || (strcmp(selector, preset->sink) == 0)) {
            return preset;
        }
    }
    return AudioPreset_new();
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
        return 0;
    }
    int existing = Strings_toInt(__btrc_str_track(__btrc_trim(Path_readAll(self->pidFile))));
    if (existing <= 0) {
        FileSystem_removeRecursive(self->pidFile);
        return 0;
    }
    ExecResult* alive = UnixShell_runUnchecked(self->shell, __btrc_str_track(__btrc_strcat("kill -0 ", Strings_fromInt(existing))));
    if (!ExecResult_ok(alive)) {
        FileSystem_removeRecursive(self->pidFile);
        return 0;
    }
    return existing;
}

bool CaffeineManager_enabled(CaffeineManager* self) {
    return (CaffeineManager_pid(self) > 0);
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
    return ExecResult_stdout(result);
}

bool DisplayManager_drmConnected(DisplayManager* self, char* name) {
    char* status = PathTools_join(PathTools_join("/sys/class/drm", __btrc_str_track(__btrc_strcat("card*-", name))), "status");
    ExecResult* result = UnixShell_runUnchecked(self->shell, __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("cat ", status)), " 2>/dev/null | head -n 1")));
    return (strcmp(ExecResult_trimmed(result), "connected") == 0);
}

char* DisplayManager_lineValue(DisplayManager* self, char* block, char* prefix) {
    int __n_144 = btrc_Vector_string_iterLen(Strings_split(block, "\n"));
    for (int __i_143 = 0; (__i_143 < __n_144); (__i_143++)) {
        char* line = btrc_Vector_string_iterGet(Strings_split(block, "\n"), __i_143);
        char* trimmed = __btrc_str_track(__btrc_trim(line));
        if (__btrc_startsWith(trimmed, prefix)) {
            return __btrc_str_track(__btrc_trim(__btrc_str_track(__btrc_substring(trimmed, ((int)strlen(prefix)), (((int)strlen(trimmed)) - ((int)strlen(prefix)))))));
        }
    }
    return "";
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
    btrc_Vector_string* __list_145 = btrc_Vector_string_new();
    btrc_Vector_string_push(__list_145, "HDMI");
    btrc_Vector_string_push(__list_145, "DisplayPort");
    btrc_Vector_string_push(__list_145, "VGA");
    btrc_Vector_string_push(__list_145, "DVI");
    btrc_Vector_string_push(__list_145, "Panel");
    btrc_Vector_string_push(__list_145, "TV");
    btrc_Vector_string_push(__list_145, "Unknown");
    int __n_147 = btrc_Vector_string_iterLen(__list_145);
    for (int __i_146 = 0; (__i_146 < __n_147); (__i_146++)) {
        btrc_Vector_string* __list_145 = btrc_Vector_string_new();
        btrc_Vector_string_push(__list_145, "HDMI");
        btrc_Vector_string_push(__list_145, "DisplayPort");
        btrc_Vector_string_push(__list_145, "VGA");
        btrc_Vector_string_push(__list_145, "DVI");
        btrc_Vector_string_push(__list_145, "Panel");
        btrc_Vector_string_push(__list_145, "TV");
        btrc_Vector_string_push(__list_145, "Unknown");
        char* kind = btrc_Vector_string_iterGet(__list_145, __i_146);
        if (__btrc_strContains(block, __btrc_str_track(__btrc_strcat("\t", kind)))) {
            (output->kind = kind);
        }
    }
    DisplayManager_applyGeometry(self, output, block);
    return output;
    if (output != NULL) {
        if ((--output->__rc) <= 0) {
            DisplayOutput_destroy(output);
        }
    }
}

void DisplayManager_applyGeometry(DisplayManager* self, DisplayOutput* output, char* block) {
    char* geometry = DisplayManager_lineValue(self, block, "Geometry: ");
    if (__btrc_isEmpty(geometry)) {
        return;
    }
    btrc_Vector_string* parts = Strings_split(geometry, " ");
    if (parts->len < 2) {
        return;
    }
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

btrc_Vector_DisplayOutput* DisplayManager_outputs(DisplayManager* self) {
    btrc_Vector_DisplayOutput* result = btrc_Vector_DisplayOutput_new();
    int __n_149 = btrc_Vector_string_iterLen(Strings_split(DisplayManager_kscreen(self), "Output: "));
    for (int __i_148 = 0; (__i_148 < __n_149); (__i_148++)) {
        char* block = btrc_Vector_string_iterGet(Strings_split(DisplayManager_kscreen(self), "Output: "), __i_148);
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
    int __n_151 = btrc_Vector_DisplayOutput_iterLen(outputs);
    for (int __i_150 = 0; (__i_150 < __n_151); (__i_150++)) {
        DisplayOutput* output = btrc_Vector_DisplayOutput_iterGet(outputs, __i_150);
        if (strcmp(output->name, name) == 0) {
            return output;
        }
    }
    return DisplayOutput_new();
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
    int __n_153 = btrc_Vector_DisplayLayoutRule_iterLen(rules);
    for (int __i_152 = 0; (__i_152 < __n_153); (__i_152++)) {
        DisplayLayoutRule* rule = btrc_Vector_DisplayLayoutRule_iterGet(rules, __i_152);
        DisplayManager_applyLayout(self, rule->display);
    }
}

void DisplayManager_applyLayout(DisplayManager* self, char* name) {
    DisplayLayoutRule* selected = DisplayLayoutRule_new();
    btrc_Vector_DisplayLayoutRule* rules = LabelsConfig_layoutRules(self->labels);
    int __n_155 = btrc_Vector_DisplayLayoutRule_iterLen(rules);
    for (int __i_154 = 0; (__i_154 < __n_155); (__i_154++)) {
        DisplayLayoutRule* rule = btrc_Vector_DisplayLayoutRule_iterGet(rules, __i_154);
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
    } else if (strcmp(selected->position, "right-of") == 0) {
        (px = (anchor->x + anchor->width));
    } else if (strcmp(selected->position, "above") == 0) {
        (py = (anchor->y - display->height));
    } else if (strcmp(selected->position, "below") == 0) {
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
    int __n_157 = btrc_Vector_DisplayOutput_iterLen(all);
    for (int __i_156 = 0; (__i_156 < __n_157); (__i_156++)) {
        DisplayOutput* output = btrc_Vector_DisplayOutput_iterGet(all, __i_156);
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
    btrc_Vector_VmOperation* __list_159 = btrc_Vector_VmOperation_new();
    (self->operations = __list_159);
    btrc_Vector_VmOperation* __list_158 = btrc_Vector_VmOperation_new();
    (__list_158->__rc++);
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
    return PathTools_join(self->stateRoot, self->state);
}

char* VmTestSpec_stateHashFile(VmTestSpec* self) {
    return PathTools_join(VmTestSpec_stateDir(self), "hash");
}

char* VmTestSpec_parentHashFile(VmTestSpec* self) {
    return PathTools_join(PathTools_join(self->stateRoot, self->parentState), "hash");
}

char* VmTestSpec_resolveParentHash(VmTestSpec* self) {
    if ((strcmp(self->parentState, "root") == 0) || __btrc_isEmpty(self->parentState)) {
        return "root";
    }
    char* path = VmTestSpec_parentHashFile(self);
    if (FileSystem_exists(path)) {
        return __btrc_str_track(__btrc_trim(Path_readAll(path)));
    }
    return __btrc_str_track(__btrc_strcat("missing:", self->parentState));
}

char* VmTestSpec_operationsMaterial(VmTestSpec* self) {
    btrc_Vector_string* lines = btrc_Vector_string_new();
    int __n_161 = btrc_Vector_VmOperation_iterLen(self->operations);
    for (int __i_160 = 0; (__i_160 < __n_161); (__i_160++)) {
        VmOperation* op = btrc_Vector_VmOperation_iterGet(self->operations, __i_160);
        btrc_Vector_string_push(lines, __btrc_str_track(__btrc_strcat("op=", op->kind)));
        btrc_Vector_string_push(lines, __btrc_str_track(__btrc_strcat("name=", op->name)));
        btrc_Vector_string_push(lines, __btrc_str_track(__btrc_strcat("command=", op->command)));
        btrc_Vector_string_push(lines, __btrc_str_track(__btrc_strcat("expect=", op->expect)));
        btrc_Vector_string_push(lines, __btrc_str_track(__btrc_strcat("local=", op->localPath)));
        btrc_Vector_string_push(lines, __btrc_str_track(__btrc_strcat("remote=", op->remotePath)));
        btrc_Vector_string_push(lines, __btrc_str_track(__btrc_strcat("timeout=", Strings_fromInt(op->timeout))));
    }
    return btrc_Vector_string_join(lines, "\n");
}

char* VmTestSpec_hashMaterial(VmTestSpec* self) {
    char* material = self->stateMaterial;
    if (__btrc_isEmpty(material)) {
        int __fstr_162_len = snprintf(NULL, 0, "%s:%s:%s:%s:%s:%d:%d", self->name, self->arch, self->isoUrl, self->diskSize, self->memory, self->cpus, self->sshPort);
        char* __fstr_162_buf = __btrc_str_track(((char*)malloc((__fstr_162_len + 1))));
        snprintf(__fstr_162_buf, (__fstr_162_len + 1), "%s:%s:%s:%s:%s:%d:%d", self->name, self->arch, self->isoUrl, self->diskSize, self->memory, self->cpus, self->sshPort);
        (material = __fstr_162_buf);
    }
    int __fstr_163_len = snprintf(NULL, 0, "parent=%s\nstate=%s\nmaterial=%s\noperations=\n%s\n", self->parentHash, self->state, material, VmTestSpec_operationsMaterial(self));
    char* __fstr_163_buf = __btrc_str_track(((char*)malloc((__fstr_163_len + 1))));
    snprintf(__fstr_163_buf, (__fstr_163_len + 1), "parent=%s\nstate=%s\nmaterial=%s\noperations=\n%s\n", self->parentHash, self->state, material, VmTestSpec_operationsMaterial(self));
    return __fstr_163_buf;
}

void VmTestSpec_computeStateHash(VmTestSpec* self) {
    (self->parentHash = VmTestSpec_resolveParentHash(self));
    char* command = __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("printf %s ", UnixShell_quote(VmTestSpec_hashMaterial(self)))), " | (if command -v sha256sum >/dev/null 2>&1; then sha256sum; else shasum -a 256; fi) | awk '{print $1}'"));
    ExecResult* result = UnixShell_run(UnixShell_new(), command);
    if (!ExecResult_ok(result)) {
        NixosLog_fatal(__btrc_str_track(__btrc_strcat("Failed to compute state hash for ", self->state)));
    }
    (self->stateHash = ExecResult_trimmed(result));
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
    int __n_165 = btrc_Vector_VmOperation_iterLen(self->operations);
    for (int __i_164 = 0; (__i_164 < __n_165); (__i_164++)) {
        VmOperation* op = btrc_Vector_VmOperation_iterGet(self->operations, __i_164);
        VmOperation_expandArgs(op, self->args);
    }
}

char* VmSpecParser_hostArch(void) {
    UnixShell* shell = UnixShell_new();
    ExecResult* result = UnixShell_run(shell, "uname -m");
    char* machine = ExecResult_trimmed(result);
    if (__btrc_strContains(machine, "arm64") || __btrc_strContains(machine, "aarch64")) {
        char* __btrc_ret_166 = "aarch64";
        if (shell != NULL) {
            if ((--shell->__rc) <= 0) {
                UnixShell_destroy(shell);
            }
        }
        return __btrc_ret_166;
    }
    char* __btrc_ret_167 = "x86_64";
    if (shell != NULL) {
        if ((--shell->__rc) <= 0) {
            UnixShell_destroy(shell);
        }
    }
    return __btrc_ret_167;
    if (shell != NULL) {
        if ((--shell->__rc) <= 0) {
            UnixShell_destroy(shell);
        }
    }
}

char* VmSpecParser_defaultIsoUrl(char* arch) {
    return __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("https://channels.nixos.org/nixos-unstable/latest-nixos-minimal-", arch)), "-linux.iso"));
}

char* VmSpecParser_expandArgs(char* text, btrc_Map_string_string* args) {
    char* result = Strings_copy(text);
    int __n_169 = btrc_Map_string_string_iterLen(args);
    for (int __i_168 = 0; (__i_168 < __n_169); (__i_168++)) {
        char* key = btrc_Map_string_string_iterGet(args, __i_168);
        char* value = btrc_Map_string_string_iterValueAt(args, __i_168);
        (result = Strings_replace(result, __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("{{", key)), "}}")), value));
    }
    int __n_171 = btrc_Map_string_string_iterLen(args);
    for (int __i_170 = 0; (__i_170 < __n_171); (__i_170++)) {
        char* key = btrc_Map_string_string_iterGet(args, __i_170);
        char* value = btrc_Map_string_string_iterValueAt(args, __i_170);
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
    return Strings_find(text, __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("\"", key)), "\"")), 0);
}

char* VmSpecParser_objectField(char* text, char* key) {
    int pos = VmSpecParser_keyPosition(text, key);
    if (pos < 0) {
        return "";
    }
    int colon = Strings_find(text, ":", pos);
    if (colon < 0) {
        return "";
    }
    int i = VmSpecParser_skipSpaces(text, (colon + 1));
    int len = ((int)strlen(text));
    if ((i >= len) || (text[i] != '{')) {
        return "";
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
                return JsonObject_slice(text, i, (j + 1));
            }
        }
    }
    return "";
}

btrc_Map_string_string* VmSpecParser_argsObject(char* text) {
    btrc_Map_string_string* result = btrc_Map_string_string_new();
    char* objectText = VmSpecParser_objectField(text, "args");
    if (__btrc_isEmpty(objectText)) {
        return result;
    }
    JsonObject* parsed = JsonObject_parse(objectText);
    int __n_173 = btrc_Map_string_string_iterLen(parsed->values);
    for (int __i_172 = 0; (__i_172 < __n_173); (__i_172++)) {
        char* key = btrc_Map_string_string_iterGet(parsed->values, __i_172);
        char* value = btrc_Map_string_string_iterValueAt(parsed->values, __i_172);
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
            return JsonObject_unescape(JsonObject_slice(text, start, i));
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
        return VmSpecParser_parseStringValue(text, i, fallback);
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
    return Strings_toInt(raw);
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
    return (((!__btrc_isEmpty(op->kind)) || (!__btrc_isEmpty(op->command))) || (!__btrc_isEmpty(op->name)));
}

char* VmSpecParser_yamlKey(char* line) {
    int pos = Strings_find(line, ":", 0);
    if (pos < 0) {
        return "";
    }
    return Toml_unquote(__btrc_str_track(__btrc_trim(__btrc_str_track(__btrc_substring(line, 0, pos)))));
}

char* VmSpecParser_yamlValue(char* line) {
    int pos = Strings_find(line, ":", 0);
    if (pos < 0) {
        return "";
    }
    return Toml_unquote(__btrc_str_track(__btrc_trim(Toml_stripInlineComment(__btrc_str_track(__btrc_substring(line, (pos + 1), ((((int)strlen(line)) - pos) - 1)))))));
}

VmTestSpec* VmSpecParser_parseToml(char* text) {
    VmTestSpec* spec = VmTestSpec_new();
    char* section = "root";
    VmOperation* op = VmOperation_new();
    btrc_Vector_string* lines = Strings_split(text, "\n");
    int __n_175 = btrc_Vector_string_iterLen(lines);
    for (int __i_174 = 0; (__i_174 < __n_175); (__i_174++)) {
        char* line = btrc_Vector_string_iterGet(lines, __i_174);
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
    int __n_177 = btrc_Vector_string_iterLen(lines);
    for (int __i_176 = 0; (__i_176 < __n_177); (__i_176++)) {
        char* raw = btrc_Vector_string_iterGet(lines, __i_176);
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
        return VmSpecParser_parseToml(text);
    }
    if (__btrc_endsWith(path, ".yaml") || __btrc_endsWith(path, ".yml")) {
        return VmSpecParser_parseYaml(text);
    }
    return VmSpecParser_parse(text);
}

void SshClient_init(SshClient* self, VmTestSpec* spec) {
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

SshClient* SshClient_new(VmTestSpec* spec) {
    SshClient* self = ((SshClient*)malloc(sizeof(SshClient)));
    memset(self, 0, sizeof(SshClient));
    SshClient_init(self, spec);
    return self;
}

void SshClient_destroy(SshClient* self) {
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

char* SshClient_sshKeyPath(SshClient* self) {
    return PathTools_join(self->spec->workDir, "id_ed25519");
}

char* SshClient_qmpPath(SshClient* self) {
    return PathTools_join(self->spec->workDir, "qmp.sock");
}

char* SshClient_workspaceRoot(SshClient* self) {
    return btrc_Map_string_string_getOrDefault(self->spec->args, "workspaceRoot", ".");
}

btrc_Vector_string* SshClient_sshOptionList(SshClient* self) {
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
    btrc_Vector_string_push(opts, SshClient_sshKeyPath(self));
    return opts;
}

char* SshClient_sshOptionsForShell(SshClient* self) {
    btrc_Vector_string* quoted = btrc_Vector_string_new();
    btrc_Vector_string* opts = SshClient_sshOptionList(self);
    int __n_179 = btrc_Vector_string_iterLen(opts);
    for (int __i_178 = 0; (__i_178 < __n_179); (__i_178++)) {
        char* opt = btrc_Vector_string_iterGet(opts, __i_178);
        btrc_Vector_string_push(quoted, UnixShell_quote(opt));
    }
    return btrc_Vector_string_join(quoted, " ");
}

Command* SshClient_addSshOptions(SshClient* self, Command* cmd) {
    btrc_Vector_string* opts = SshClient_sshOptionList(self);
    int __n_181 = btrc_Vector_string_iterLen(opts);
    for (int __i_180 = 0; (__i_180 < __n_181); (__i_180++)) {
        char* opt = btrc_Vector_string_iterGet(opts, __i_180);
        Command_arg(cmd, opt);
    }
    return cmd;
}

ExecResult* SshClient_sshWithTimeout(SshClient* self, char* command, bool checkStatus, int timeoutSeconds) {
    Command* cmd = Command_check(Command_arg(Command_arg(SshClient_addSshOptions(self, Command_arg(Command_arg(Command_arg(Command_arg(Command_new("timeout"), Strings_fromInt(timeoutSeconds)), "ssh"), "-p"), Strings_fromInt(self->spec->sshPort))), "root@localhost"), command), checkStatus);
    ExecResult* result = UnixShell_runCommand(self->shell, cmd);
    return result;
}

ExecResult* SshClient_ssh(SshClient* self, char* command, bool checkStatus) {
    return SshClient_sshWithTimeout(self, command, checkStatus, 5);
}

ExecResult* SshClient_host(SshClient* self, char* command, bool checkStatus) {
    return UnixShell_runRaw(self->shell, command, true, checkStatus, "");
}

ExecResult* SshClient_workspaceFileExists(SshClient* self, char* relativePath) {
    char* command = __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("cd ", UnixShell_quote(SshClient_workspaceRoot(self)))), " && test -f ")), UnixShell_quote(relativePath))), " && printf exists"));
    return UnixShell_runRaw(self->shell, command, true, false, "");
}

ExecResult* SshClient_nixEval(SshClient* self, char* attribute, int timeoutSeconds) {
    char* command = __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("cd ", UnixShell_quote(SshClient_workspaceRoot(self)))), " && timeout ")), Strings_fromInt(timeoutSeconds))), " nix eval ")), UnixShell_quote(__btrc_str_track(__btrc_strcat(".#", attribute))))), " --show-trace"));
    return UnixShell_runRaw(self->shell, command, true, false, "");
}

ExecResult* SshClient_qmp(SshClient* self, char* command, int timeoutSeconds) {
    if (!FileSystem_exists(SshClient_qmpPath(self))) {
        return ExecResult_new(1, "", __btrc_str_track(__btrc_strcat("QMP socket does not exist: ", SshClient_qmpPath(self))), "");
    }
    char* payload = __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("{\"execute\":\"qmp_capabilities\"}\n", command)), "\n"));
    char* rendered = __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("printf %s ", UnixShell_quote(payload))), " | timeout ")), Strings_fromInt(timeoutSeconds))), " socat - ")), UnixShell_quote(__btrc_str_track(__btrc_strcat("UNIX-CONNECT:", SshClient_qmpPath(self))))));
    return UnixShell_runRaw(self->shell, rendered, true, false, "");
}

void SshClient_copyWorkspace(SshClient* self, char* localPath, char* remotePath) {
    char* source = (__btrc_isEmpty(localPath) ? ".." : localPath);
    char* target = (__btrc_isEmpty(remotePath) ? "/etc/nixos" : remotePath);
    char* remote = __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("mkdir -p ", UnixShell_quote(target))), " && tar xzf - -C ")), UnixShell_quote(target)));
    char* tar = __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("tar czf - --exclude=.vm --exclude=build --exclude=secrets --exclude=.git --exclude=result --exclude='._*' -C ", UnixShell_quote(source))), " ."));
    char* ssh = __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("ssh -p ", Strings_fromInt(self->spec->sshPort))), " ")), SshClient_sshOptionsForShell(self))), " root@localhost ")), UnixShell_quote(remote)));
    ExecResult* result = UnixShell_runRaw(self->shell, __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(tar, " | ")), ssh)), false, true, "");
    if (!ExecResult_ok(result)) {
        NixosLog_fatal("Failed to copy workspace into VM");
    }
}

void SshClient_copyTo(SshClient* self, char* localPath, char* remotePath) {
    int __fstr_182_len = snprintf(NULL, 0, "root@localhost:%s", remotePath);
    char* __fstr_182_buf = __btrc_str_track(((char*)malloc((__fstr_182_len + 1))));
    snprintf(__fstr_182_buf, (__fstr_182_len + 1), "root@localhost:%s", remotePath);
    Command* cmd = Command_capture(Command_arg(Command_arg(SshClient_addSshOptions(self, Command_arg(Command_arg(Command_arg(Command_new("scp"), "-r"), "-P"), Strings_fromInt(self->spec->sshPort))), localPath), __fstr_182_buf), false);
    ExecResult* result = UnixShell_runCommand(self->shell, cmd);
    if (!ExecResult_ok(result)) {
        NixosLog_fatal("Failed to copy file to VM");
    }
}

void SshClient_copyFrom(SshClient* self, char* remotePath, char* localPath) {
    int __fstr_183_len = snprintf(NULL, 0, "root@localhost:%s", remotePath);
    char* __fstr_183_buf = __btrc_str_track(((char*)malloc((__fstr_183_len + 1))));
    snprintf(__fstr_183_buf, (__fstr_183_len + 1), "root@localhost:%s", remotePath);
    Command* cmd = Command_capture(Command_arg(Command_arg(SshClient_addSshOptions(self, Command_arg(Command_arg(Command_arg(Command_new("scp"), "-r"), "-P"), Strings_fromInt(self->spec->sshPort))), __fstr_183_buf), localPath), false);
    ExecResult* result = UnixShell_runCommand(self->shell, cmd);
    if (!ExecResult_ok(result)) {
        NixosLog_fatal("Failed to copy file from VM");
    }
}

void QemuSerial_init(QemuSerial* self, VmTestSpec* spec) {
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

QemuSerial* QemuSerial_new(VmTestSpec* spec) {
    QemuSerial* self = ((QemuSerial*)malloc(sizeof(QemuSerial)));
    memset(self, 0, sizeof(QemuSerial));
    QemuSerial_init(self, spec);
    return self;
}

void QemuSerial_destroy(QemuSerial* self) {
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

char* QemuSerial_serialBasePath(QemuSerial* self) {
    return PathTools_join(self->spec->workDir, "serial");
}

char* QemuSerial_serialInPath(QemuSerial* self) {
    return __btrc_str_track(__btrc_strcat(QemuSerial_serialBasePath(self), ".in"));
}

char* QemuSerial_serialOutPath(QemuSerial* self) {
    return __btrc_str_track(__btrc_strcat(QemuSerial_serialBasePath(self), ".out"));
}

char* QemuSerial_serialLogPath(QemuSerial* self) {
    return __btrc_str_track(__btrc_strcat(QemuSerial_serialBasePath(self), ".log"));
}

char* QemuSerial_serialReaderPidPath(QemuSerial* self) {
    return PathTools_join(self->spec->workDir, "serial-reader.pid");
}

char* QemuSerial_bootDir(QemuSerial* self) {
    return PathTools_join(self->spec->workDir, "boot");
}

char* QemuSerial_bootKernelFile(QemuSerial* self) {
    return PathTools_join(QemuSerial_bootDir(self), "kernel.path");
}

char* QemuSerial_bootInitrdFile(QemuSerial* self) {
    return PathTools_join(QemuSerial_bootDir(self), "initrd.path");
}

char* QemuSerial_bootAppendFile(QemuSerial* self) {
    return PathTools_join(QemuSerial_bootDir(self), "append.txt");
}

char* QemuSerial_stripLeadingSlash(QemuSerial* self, char* path) {
    if (__btrc_startsWith(path, "/")) {
        return __btrc_str_track(__btrc_substring(path, 1, (((int)strlen(path)) - 1)));
    }
    return path;
}

char* QemuSerial_valueAfterLinePrefix(QemuSerial* self, char* text, char* prefix, int start) {
    int pos = Strings_find(text, prefix, start);
    if (pos < 0) {
        return "";
    }
    (pos = (pos + ((int)strlen(prefix))));
    int end = pos;
    while ((text[end] != '\0') && (text[end] != '\n')) {
        (end++);
    }
    return __btrc_str_track(__btrc_trim(JsonObject_slice(text, pos, end)));
}

void QemuSerial_extractBootSerial(QemuSerial* self) {
    if (!(strcmp(self->spec->arch, "x86_64") == 0)) {
        return;
    }
    if (FileSystem_exists(QemuSerial_bootAppendFile(self))) {
        return;
    }
    FileSystem_mkdirp(QemuSerial_bootDir(self));
    Command* cfgExtract = Command_capture(Command_arg(Command_arg(Command_arg(Command_arg(Command_arg(Command_new("bsdtar"), "-xf"), self->spec->iso), "-C"), QemuSerial_bootDir(self)), "isolinux/isolinux.cfg"), false);
    ExecResult* cfgResult = UnixShell_runCommand(self->shell, cfgExtract);
    if (!ExecResult_ok(cfgResult)) {
        NixosLog_fatal("Failed to extract isolinux.cfg from ISO");
    }
    char* cfgPath = PathTools_join(PathTools_join(QemuSerial_bootDir(self), "isolinux"), "isolinux.cfg");
    char* cfg = Path_readAll(cfgPath);
    int label = Strings_find(cfg, "LABEL boot-serial", 0);
    if (label < 0) {
        NixosLog_fatal("Could not find boot-serial entry in installer ISO");
    }
    char* kernelIso = QemuSerial_valueAfterLinePrefix(self, cfg, "LINUX ", label);
    char* append = QemuSerial_valueAfterLinePrefix(self, cfg, "APPEND ", label);
    char* initrdIso = QemuSerial_valueAfterLinePrefix(self, cfg, "INITRD ", label);
    if ((__btrc_isEmpty(kernelIso) || __btrc_isEmpty(initrdIso)) || __btrc_isEmpty(append)) {
        NixosLog_fatal("Incomplete boot-serial entry in installer ISO");
    }
    char* kernelRel = QemuSerial_stripLeadingSlash(self, kernelIso);
    char* initrdRel = QemuSerial_stripLeadingSlash(self, initrdIso);
    Command* bootExtract = Command_capture(Command_arg(Command_arg(Command_arg(Command_arg(Command_arg(Command_arg(Command_new("bsdtar"), "-xf"), self->spec->iso), "-C"), QemuSerial_bootDir(self)), kernelRel), initrdRel), false);
    ExecResult* bootResult = UnixShell_runCommand(self->shell, bootExtract);
    if (!ExecResult_ok(bootResult)) {
        NixosLog_fatal("Failed to extract serial boot kernel/initrd from ISO");
    }
    Path_writeAll(QemuSerial_bootKernelFile(self), PathTools_join(QemuSerial_bootDir(self), kernelRel));
    Path_writeAll(QemuSerial_bootInitrdFile(self), PathTools_join(QemuSerial_bootDir(self), initrdRel));
    Path_writeAll(QemuSerial_bootAppendFile(self), append);
}

void QemuSerial_prepareSerialPipe(QemuSerial* self) {
    FileSystem_removeRecursive(QemuSerial_serialInPath(self));
    FileSystem_removeRecursive(QemuSerial_serialOutPath(self));
    Command* inPipe = Command_new("mkfifo");
    Command_arg(inPipe, QemuSerial_serialInPath(self));
    Command_capture(inPipe, false);
    ExecResult* inResult = UnixShell_runCommand(self->shell, inPipe);
    if (!ExecResult_ok(inResult)) {
        NixosLog_fatal("Failed to create VM serial input pipe");
    }
    Command* outPipe = Command_new("mkfifo");
    Command_arg(outPipe, QemuSerial_serialOutPath(self));
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

void QemuSerial_startSerialReader(QemuSerial* self) {
    FileSystem_removeRecursive(QemuSerial_serialReaderPidPath(self));
    FileSystem_removeRecursive(QemuSerial_serialLogPath(self));
    char* outPath = UnixShell_quote(QemuSerial_serialOutPath(self));
    char* logPath = UnixShell_quote(QemuSerial_serialLogPath(self));
    char* pidPath = UnixShell_quote(QemuSerial_serialReaderPidPath(self));
    UnixShell_runRaw(self->shell, __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("(cat ", outPath)), " >> ")), logPath)), ") & echo $! > ")), pidPath)), false, false, "");
}

void QemuSerial_stopSerialReader(QemuSerial* self) {
    if (!FileSystem_exists(QemuSerial_serialReaderPidPath(self))) {
        return;
    }
    char* pidPath = UnixShell_quote(QemuSerial_serialReaderPidPath(self));
    UnixShell_runRaw(self->shell, __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("pid=$(cat ", pidPath)), "); kill $pid 2>/dev/null || true")), false, false, "");
    FileSystem_removeRecursive(QemuSerial_serialReaderPidPath(self));
}

void QemuSerial_serialSend(QemuSerial* self, char* command) {
    char* payload = __btrc_str_track(__btrc_strcat(command, "\n"));
    UnixShell_runRaw(self->shell, __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("printf %s ", UnixShell_quote(payload))), " > ")), UnixShell_quote(QemuSerial_serialInPath(self)))), false, false, "");
}

void QemuFirmware_init(QemuFirmware* self, VmTestSpec* spec) {
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

QemuFirmware* QemuFirmware_new(VmTestSpec* spec) {
    QemuFirmware* self = ((QemuFirmware*)malloc(sizeof(QemuFirmware)));
    memset(self, 0, sizeof(QemuFirmware));
    QemuFirmware_init(self, spec);
    return self;
}

void QemuFirmware_destroy(QemuFirmware* self) {
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

char* QemuFirmware_firmwareVarsPath(QemuFirmware* self) {
    return PathTools_join(self->spec->workDir, "edk2-vars.fd");
}

char* QemuFirmware_firmwareVarsSnapshotPath(QemuFirmware* self, char* name) {
    return PathTools_join(self->spec->workDir, __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("edk2-vars.", name)), ".fd")));
}

char* QemuFirmware_tpmDir(QemuFirmware* self) {
    return PathTools_join(self->spec->workDir, "tpm");
}

char* QemuFirmware_tpmStateDir(QemuFirmware* self) {
    return PathTools_join(QemuFirmware_tpmDir(self), "state");
}

char* QemuFirmware_tpmSocketPath(QemuFirmware* self) {
    return PathTools_join(QemuFirmware_tpmDir(self), "swtpm.sock");
}

char* QemuFirmware_tpmPidPath(QemuFirmware* self) {
    return PathTools_join(QemuFirmware_tpmDir(self), "swtpm.pid");
}

char* QemuFirmware_tpmLogPath(QemuFirmware* self) {
    return PathTools_join(QemuFirmware_tpmDir(self), "swtpm.log");
}

char* QemuFirmware_qemuBinary(QemuFirmware* self) {
    int __fstr_184_len = snprintf(NULL, 0, "qemu-system-%s", self->spec->arch);
    char* __fstr_184_buf = __btrc_str_track(((char*)malloc((__fstr_184_len + 1))));
    snprintf(__fstr_184_buf, (__fstr_184_len + 1), "qemu-system-%s", self->spec->arch);
    return __fstr_184_buf;
}

bool QemuFirmware_argEnabled(QemuFirmware* self, char* key) {
    return (strcmp(btrc_Map_string_string_getOrDefault(self->spec->args, key, "false"), "true") == 0);
}

bool QemuFirmware_tpm2Enabled(QemuFirmware* self) {
    return QemuFirmware_argEnabled(self, "tpm2");
}

bool QemuFirmware_secureBootEnabled(QemuFirmware* self) {
    return QemuFirmware_argEnabled(self, "secureBoot");
}

bool QemuFirmware_uefiEnabled(QemuFirmware* self) {
    return ((QemuFirmware_argEnabled(self, "uefi") || QemuFirmware_argEnabled(self, "uefiDisk")) || QemuFirmware_secureBootEnabled(self));
}

bool QemuFirmware_shouldUseUefi(QemuFirmware* self, bool fromIso) {
    if (strcmp(self->spec->arch, "aarch64") == 0) {
        return true;
    }
    if (QemuFirmware_argEnabled(self, "uefi")) {
        return true;
    }
    if ((!fromIso) && QemuFirmware_uefiEnabled(self)) {
        return true;
    }
    if (fromIso && QemuFirmware_argEnabled(self, "uefiIso")) {
        return true;
    }
    return false;
}

bool QemuFirmware_hostArchMatchesGuest(QemuFirmware* self) {
    return (strcmp(VmSpecParser_hostArch(), self->spec->arch) == 0);
}

char* QemuFirmware_qemuSharePath(QemuFirmware* self, char* fileName) {
    char* script = __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("bin=$(command -v ", UnixShell_quote(QemuFirmware_qemuBinary(self)))), ") || exit 1; ")), "path=$(dirname $(dirname \"$bin\"))/share/qemu/")), UnixShell_quote(fileName))), "; ")), "test -f \"$path\" || exit 1; printf %s \"$path\""));
    ExecResult* result = UnixShell_runUnchecked(self->shell, script);
    if (!ExecResult_ok(result)) {
        return "";
    }
    return ExecResult_trimmed(result);
}

char* QemuFirmware_findFirst(QemuFirmware* self, char* findArgs) {
    return ExecResult_trimmed(UnixShell_runUnchecked(self->shell, __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("find ", findArgs)), " -print -quit 2>/dev/null"))));
}

char* QemuFirmware_firmwareCodePath(QemuFirmware* self) {
    if (QemuFirmware_secureBootEnabled(self)) {
        return QemuFirmware_secureFirmwareCodePath(self);
    }
    if (strcmp(self->spec->arch, "aarch64") == 0) {
        char* packaged = QemuFirmware_qemuSharePath(self, "edk2-aarch64-code.fd");
        if (!__btrc_isEmpty(packaged)) {
            return packaged;
        }
        if (FileSystem_exists("/opt/homebrew/share/qemu/edk2-aarch64-code.fd")) {
            return "/opt/homebrew/share/qemu/edk2-aarch64-code.fd";
        }
    }
    if (strcmp(self->spec->arch, "x86_64") == 0) {
        char* packaged = QemuFirmware_qemuSharePath(self, "edk2-x86_64-code.fd");
        if (!__btrc_isEmpty(packaged)) {
            return packaged;
        }
    }
    return QemuFirmware_findFirst(self, "/nix/store -maxdepth 4 -name OVMF_CODE.fd");
}

char* QemuFirmware_secureFirmwareCodePath(QemuFirmware* self) {
    if (strcmp(self->spec->arch, "aarch64") == 0) {
        char* store = QemuFirmware_findFirst(self, "/nix/store -path '*aavmf-secboot*/AAVMF_CODE.secboot.fd'");
        if (!__btrc_isEmpty(store)) {
            return store;
        }
        return QemuFirmware_findFirst(self, "\"$(pwd)/.vm/firmware\" -name AAVMF_CODE.secboot.fd");
    }
    if (!(strcmp(self->spec->arch, "x86_64") == 0)) {
        return "";
    }
    char* packaged = QemuFirmware_qemuSharePath(self, "edk2-x86_64-secure-code.fd");
    if (!__btrc_isEmpty(packaged)) {
        return packaged;
    }
    return QemuFirmware_findFirst(self, "/nix/store -maxdepth 5 \\( -name OVMF_CODE.secboot.fd -o -name OVMF_CODE_4M.secboot.fd -o -name '*secure*CODE*.fd' \\)");
}

char* QemuFirmware_firmwareVarsTemplatePath(QemuFirmware* self) {
    if (QemuFirmware_secureBootEnabled(self) && (strcmp(self->spec->arch, "aarch64") == 0)) {
        char* store = QemuFirmware_findFirst(self, "/nix/store -path '*aavmf-secboot*/AAVMF_VARS.fd'");
        if (!__btrc_isEmpty(store)) {
            return store;
        }
        char* deb = QemuFirmware_findFirst(self, "\"$(pwd)/.vm/firmware\" -name AAVMF_VARS.fd");
        if (!__btrc_isEmpty(deb)) {
            return deb;
        }
    }
    if (strcmp(self->spec->arch, "aarch64") == 0) {
        char* packaged = QemuFirmware_qemuSharePath(self, "edk2-arm-vars.fd");
        if (!__btrc_isEmpty(packaged)) {
            return packaged;
        }
    }
    if (strcmp(self->spec->arch, "x86_64") == 0) {
        char* packaged = QemuFirmware_qemuSharePath(self, "edk2-i386-vars.fd");
        if (!__btrc_isEmpty(packaged)) {
            return packaged;
        }
    }
    return QemuFirmware_findFirst(self, "/nix/store -maxdepth 4 -name OVMF_VARS.fd");
}

void QemuFirmware_makeFirmwareVarsWritable(QemuFirmware* self) {
    Command* chmod = Command_check(Command_capture(Command_arg(Command_arg(Command_new("chmod"), "600"), QemuFirmware_firmwareVarsPath(self)), false), false);
    UnixShell_runCommand(self->shell, chmod);
}

void QemuFirmware_setupFirmwareVars(QemuFirmware* self) {
    if (FileSystem_exists(QemuFirmware_firmwareVarsPath(self))) {
        QemuFirmware_makeFirmwareVarsWritable(self);
        return;
    }
    char* template = QemuFirmware_firmwareVarsTemplatePath(self);
    if (!__btrc_isEmpty(template)) {
        Command* cp = Command_capture(Command_arg(Command_arg(Command_new("cp"), template), QemuFirmware_firmwareVarsPath(self)), false);
        ExecResult* result = UnixShell_runCommand(self->shell, cp);
        if (!ExecResult_ok(result)) {
            NixosLog_fatal("Failed to copy EDK2 vars file");
        }
        QemuFirmware_makeFirmwareVarsWritable(self);
        return;
    }
    int __fstr_186_len = snprintf(NULL, 0, "dd if=/dev/zero of=%s bs=1M count=64", UnixShell_quote(QemuFirmware_firmwareVarsPath(self)));
    char* __fstr_186_buf = __btrc_str_track(((char*)malloc((__fstr_186_len + 1))));
    snprintf(__fstr_186_buf, (__fstr_186_len + 1), "dd if=/dev/zero of=%s bs=1M count=64", UnixShell_quote(QemuFirmware_firmwareVarsPath(self)));
    UnixShell_runRaw(self->shell, __fstr_186_buf, false, true, "");
    QemuFirmware_makeFirmwareVarsWritable(self);
}

bool QemuFirmware_commandExists(QemuFirmware* self, char* name) {
    ExecResult* result = UnixShell_runRaw(self->shell, __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("command -v ", UnixShell_quote(name))), " >/dev/null 2>&1")), false, false, "");
    return ExecResult_ok(result);
}

void QemuFirmware_requireCommand(QemuFirmware* self, char* name) {
    if (!QemuFirmware_commandExists(self, name)) {
        NixosLog_fatal(__btrc_str_track(__btrc_strcat("Missing required command: ", name)));
    }
}

bool QemuFirmware_qemuDeviceAvailable(QemuFirmware* self, char* deviceName) {
    char* command = __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(QemuFirmware_qemuBinary(self), " -device help 2>/dev/null | grep -q ")), UnixShell_quote(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("name \"", deviceName)), "\"")))));
    ExecResult* result = UnixShell_runRaw(self->shell, command, false, false, "");
    return ExecResult_ok(result);
}

char* QemuFirmware_tpmQemuDevice(QemuFirmware* self) {
    if (strcmp(self->spec->arch, "aarch64") == 0) {
        return "tpm-tis-device";
    }
    return "tpm-tis";
}

void QemuFirmware_requireTpm2Capability(QemuFirmware* self) {
    QemuFirmware_requireCommand(self, "swtpm");
    char* deviceName = QemuFirmware_tpmQemuDevice(self);
    if (!QemuFirmware_qemuDeviceAvailable(self, deviceName)) {
        NixosLog_fatal(__btrc_str_track(__btrc_strcat("QEMU TPM2 device is unavailable: ", deviceName)));
    }
}

void QemuFirmware_requireUefiCapability(QemuFirmware* self) {
    char* codePath = QemuFirmware_firmwareCodePath(self);
    char* varsPath = QemuFirmware_firmwareVarsTemplatePath(self);
    if (__btrc_isEmpty(codePath) || __btrc_isEmpty(varsPath)) {
        NixosLog_fatal(__btrc_str_track(__btrc_strcat("QEMU UEFI firmware is unavailable for ", self->spec->arch)));
    }
}

void QemuFirmware_requireSecureBootCapability(QemuFirmware* self) {
    char* codePath = QemuFirmware_secureFirmwareCodePath(self);
    char* varsPath = QemuFirmware_firmwareVarsTemplatePath(self);
    if (__btrc_isEmpty(codePath) || __btrc_isEmpty(varsPath)) {
        NixosLog_fatal(__btrc_str_track(__btrc_strcat("QEMU Secure Boot firmware is unavailable for ", self->spec->arch)));
    }
    QemuFirmware_requireTpm2Capability(self);
}

char* QemuFirmware_secureBootCapabilityReport(QemuFirmware* self) {
    bool qemu = QemuFirmware_commandExists(self, QemuFirmware_qemuBinary(self));
    bool swtpm = QemuFirmware_commandExists(self, "swtpm");
    char* firmware = QemuFirmware_secureFirmwareCodePath(self);
    char* vars = QemuFirmware_firmwareVarsTemplatePath(self);
    char* device = QemuFirmware_tpmQemuDevice(self);
    bool tpmDevice = (qemu && QemuFirmware_qemuDeviceAvailable(self, device));
    bool available = ((((((strcmp(self->spec->arch, "x86_64") == 0) && qemu) && swtpm) && tpmDevice) && (!__btrc_isEmpty(firmware))) && (!__btrc_isEmpty(vars)));
    return __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("secureBootQemu=", (available ? "available" : "unavailable"))), "\narch=")), self->spec->arch)), "\nqemu=")), (qemu ? "yes" : "no"))), "\nswtpm=")), (swtpm ? "yes" : "no"))), "\ntpmDevice=")), (tpmDevice ? device : "missing"))), "\nfirmware=")), (__btrc_isEmpty(firmware) ? "missing" : firmware))), "\nvars=")), (__btrc_isEmpty(vars) ? "missing" : vars)));
}

bool QemuFirmware_isDarwin(QemuFirmware* self) {
    ExecResult* result = UnixShell_run(self->shell, "uname -s");
    return (strcmp(ExecResult_trimmed(result), "Darwin") == 0);
}

void QemuFirmware_startSwtpm(QemuFirmware* self) {
    if (!QemuFirmware_tpm2Enabled(self)) {
        return;
    }
    QemuFirmware_requireTpm2Capability(self);
    FileSystem_mkdirp(QemuFirmware_tpmStateDir(self));
    FileSystem_removeRecursive(QemuFirmware_tpmSocketPath(self));
    if (FileSystem_exists(QemuFirmware_tpmPidPath(self))) {
        ExecResult* running = UnixShell_runRaw(self->shell, __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("kill -0 $(cat ", UnixShell_quote(QemuFirmware_tpmPidPath(self)))), ") 2>/dev/null")), false, false, "");
        if (ExecResult_ok(running)) {
            return;
        }
        FileSystem_removeRecursive(QemuFirmware_tpmPidPath(self));
    }
    Command* cmd = Command_capture(Command_arg(Command_arg(Command_arg(Command_arg(Command_arg(Command_arg(Command_arg(Command_arg(Command_arg(Command_arg(Command_arg(Command_new("swtpm"), "socket"), "--tpm2"), "--tpmstate"), __btrc_str_track(__btrc_strcat("dir=", QemuFirmware_tpmStateDir(self)))), "--ctrl"), __btrc_str_track(__btrc_strcat("type=unixio,path=", QemuFirmware_tpmSocketPath(self)))), "--daemon"), "--pid"), __btrc_str_track(__btrc_strcat("file=", QemuFirmware_tpmPidPath(self)))), "--log"), __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("file=", QemuFirmware_tpmLogPath(self))), ",level=20"))), false);
    ExecResult* result = UnixShell_runCommand(self->shell, cmd);
    if (!ExecResult_ok(result)) {
        NixosLog_fatal("Failed to start swtpm");
    }
    char* waitCommand = __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("for i in 1 2 3 4 5 6 7 8 9 10; do test -S ", UnixShell_quote(QemuFirmware_tpmSocketPath(self)))), " && exit 0; sleep 1; done; exit 1"));
    ExecResult* ready = UnixShell_runRaw(self->shell, waitCommand, false, false, "");
    if (!ExecResult_ok(ready)) {
        NixosLog_fatal("swtpm socket did not become ready");
    }
}

void QemuFirmware_stopSwtpm(QemuFirmware* self) {
    if (FileSystem_exists(QemuFirmware_tpmPidPath(self))) {
        char* quotedPid = UnixShell_quote(QemuFirmware_tpmPidPath(self));
        UnixShell_runRaw(self->shell, __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("pid=$(cat ", quotedPid)), "); kill $pid 2>/dev/null || true")), false, false, "");
        FileSystem_removeRecursive(QemuFirmware_tpmPidPath(self));
    }
    FileSystem_removeRecursive(QemuFirmware_tpmSocketPath(self));
}

void QemuFirmware_addFirmware(QemuFirmware* self, Command* cmd, bool fromIso) {
    if (!QemuFirmware_shouldUseUefi(self, fromIso)) {
        return;
    }
    QemuFirmware_requireUefiCapability(self);
    if (QemuFirmware_secureBootEnabled(self)) {
        QemuFirmware_requireSecureBootCapability(self);
    }
    if (strcmp(self->spec->arch, "x86_64") == 0) {
        Command_arg(cmd, "-machine");
        Command_arg(cmd, "q35");
    }
    QemuFirmware_setupFirmwareVars(self);
    char* codePath = QemuFirmware_firmwareCodePath(self);
    if (__btrc_isEmpty(codePath)) {
        NixosLog_fatal(__btrc_str_track(__btrc_strcat("Could not find EDK2 firmware for ", self->spec->arch)));
    }
    Command_arg(cmd, "-drive");
    Command_arg(cmd, __btrc_str_track(__btrc_strcat("if=pflash,format=raw,readonly=on,file=", codePath)));
    Command_arg(cmd, "-drive");
    Command_arg(cmd, __btrc_str_track(__btrc_strcat("if=pflash,format=raw,file=", QemuFirmware_firmwareVarsPath(self))));
}

void QemuFirmware_addTpm2(QemuFirmware* self, Command* cmd) {
    if (!QemuFirmware_tpm2Enabled(self)) {
        return;
    }
    QemuFirmware_startSwtpm(self);
    Command_arg(cmd, "-chardev");
    Command_arg(cmd, __btrc_str_track(__btrc_strcat("socket,id=chrtpm,path=", QemuFirmware_tpmSocketPath(self))));
    Command_arg(cmd, "-tpmdev");
    Command_arg(cmd, "emulator,id=tpm0,chardev=chrtpm");
    Command_arg(cmd, "-device");
    Command_arg(cmd, __btrc_str_track(__btrc_strcat(QemuFirmware_tpmQemuDevice(self), ",tpmdev=tpm0")));
}

void VmStateStore_init(VmStateStore* self, VmTestSpec* spec, QemuFirmware* firmware) {
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
    if (self->firmware != NULL) {
        if ((--self->firmware->__rc) <= 0) {
            QemuFirmware_destroy(self->firmware);
        }
    }
    (self->firmware = firmware);
    (firmware->__rc++);
}

VmStateStore* VmStateStore_new(VmTestSpec* spec, QemuFirmware* firmware) {
    VmStateStore* self = ((VmStateStore*)malloc(sizeof(VmStateStore)));
    memset(self, 0, sizeof(VmStateStore));
    VmStateStore_init(self, spec, firmware);
    return self;
}

void VmStateStore_destroy(VmStateStore* self) {
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
    if (self->firmware != NULL) {
        if ((--self->firmware->__rc) <= 0) {
            QemuFirmware_destroy(self->firmware);
        }
    }
    if (__btrc_tracking) {
        __btrc_mark_destroyed(self);
    }
    free(self);
}

char* VmStateStore_diskPath(VmStateStore* self) {
    return PathTools_join(self->spec->workDir, "disk.qcow2");
}

char* VmStateStore_pidPath(VmStateStore* self) {
    return PathTools_join(self->spec->workDir, "qemu.pid");
}

char* VmStateStore_sshKeyPath(VmStateStore* self) {
    return PathTools_join(self->spec->workDir, "id_ed25519");
}

char* VmStateStore_sshPubKeyPath(VmStateStore* self) {
    return __btrc_str_track(__btrc_strcat(VmStateStore_sshKeyPath(self), ".pub"));
}

char* VmStateStore_parentStateDir(VmStateStore* self) {
    return PathTools_join(self->spec->stateRoot, self->spec->parentState);
}

char* VmStateStore_parentWorkDirFile(VmStateStore* self) {
    return PathTools_join(VmStateStore_parentStateDir(self), "workDir");
}

char* VmStateStore_backingDiskFile(VmStateStore* self) {
    return PathTools_join(self->spec->workDir, "backing-disk");
}

void VmStateStore_ensureWorkDir(VmStateStore* self) {
    FileSystem_mkdirp(self->spec->workDir);
    FileSystem_mkdirp(PathTools_dirname(self->spec->iso));
}

char* VmStateStore_absolutePath(VmStateStore* self, char* path) {
    Command* command = Command_arg(Command_arg(Command_new("sh"), "-c"), __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("cd ", UnixShell_quote(PathTools_dirname(path)))), " && printf '%s/%s' \"$PWD\" ")), UnixShell_quote(PathTools_basename(path)))));
    ExecResult* result = UnixShell_runCommand(self->shell, command);
    if (!ExecResult_ok(result)) {
        NixosLog_fatal(__btrc_str_track(__btrc_strcat("Failed to resolve path ", path)));
    }
    return ExecResult_trimmed(result);
}

void VmStateStore_cleanStateRecord(VmStateStore* self) {
    FileSystem_removeRecursive(VmTestSpec_stateDir(self->spec));
}

void VmStateStore_requireParentState(VmStateStore* self) {
    if ((strcmp(self->spec->parentState, "root") == 0) || __btrc_isEmpty(self->spec->parentState)) {
        return;
    }
    if (!FileSystem_exists(VmTestSpec_parentHashFile(self->spec))) {
        int __fstr_188_len = snprintf(NULL, 0, "Missing parent state %s; run its test first", self->spec->parentState);
        char* __fstr_188_buf = __btrc_str_track(((char*)malloc((__fstr_188_len + 1))));
        snprintf(__fstr_188_buf, (__fstr_188_len + 1), "Missing parent state %s; run its test first", self->spec->parentState);
        NixosLog_fatal(__fstr_188_buf);
    }
}

void VmStateStore_copyIfExists(VmStateStore* self, char* source, char* target) {
    if (!FileSystem_exists(source)) {
        return;
    }
    Command* cp = Command_capture(Command_arg(Command_arg(Command_new("cp"), source), target), false);
    ExecResult* result = UnixShell_runCommand(self->shell, cp);
    if (!ExecResult_ok(result)) {
        int __fstr_190_len = snprintf(NULL, 0, "Failed to copy state artifact %s", source);
        char* __fstr_190_buf = __btrc_str_track(((char*)malloc((__fstr_190_len + 1))));
        snprintf(__fstr_190_buf, (__fstr_190_len + 1), "Failed to copy state artifact %s", source);
        NixosLog_fatal(__fstr_190_buf);
    }
}

void VmStateStore_copyTreeIfExists(VmStateStore* self, char* source, char* target) {
    if (!FileSystem_exists(source)) {
        return;
    }
    FileSystem_removeRecursive(target);
    Command* cp = Command_capture(Command_arg(Command_arg(Command_arg(Command_new("cp"), "-R"), source), target), false);
    ExecResult* result = UnixShell_runCommand(self->shell, cp);
    if (!ExecResult_ok(result)) {
        int __fstr_192_len = snprintf(NULL, 0, "Failed to copy state tree %s", source);
        char* __fstr_192_buf = __btrc_str_track(((char*)malloc((__fstr_192_len + 1))));
        snprintf(__fstr_192_buf, (__fstr_192_len + 1), "Failed to copy state tree %s", source);
        NixosLog_fatal(__fstr_192_buf);
    }
}

void VmStateStore_inheritState(VmStateStore* self) {
    VmStateStore_requireParentState(self);
    if ((strcmp(self->spec->parentState, "root") == 0) || __btrc_isEmpty(self->spec->parentState)) {
        return;
    }
    if (!FileSystem_exists(VmStateStore_parentWorkDirFile(self))) {
        int __fstr_194_len = snprintf(NULL, 0, "Parent state %s has no workDir record", self->spec->parentState);
        char* __fstr_194_buf = __btrc_str_track(((char*)malloc((__fstr_194_len + 1))));
        snprintf(__fstr_194_buf, (__fstr_194_len + 1), "Parent state %s has no workDir record", self->spec->parentState);
        NixosLog_fatal(__fstr_194_buf);
    }
    char* parentWorkDir = __btrc_str_track(__btrc_trim(Path_readAll(VmStateStore_parentWorkDirFile(self))));
    VmStateStore_ensureWorkDir(self);
    char* parentDisk = PathTools_join(parentWorkDir, "disk.qcow2");
    if ((!FileSystem_exists(VmStateStore_diskPath(self))) && FileSystem_exists(parentDisk)) {
        char* backing = VmStateStore_absolutePath(self, parentDisk);
        Command* create = Command_capture(Command_arg(Command_arg(Command_arg(Command_arg(Command_arg(Command_arg(Command_arg(Command_arg(Command_new("qemu-img"), "create"), "-f"), "qcow2"), "-F"), "qcow2"), "-b"), backing), VmStateStore_diskPath(self)), false);
        ExecResult* result = UnixShell_runCommand(self->shell, create);
        if (!ExecResult_ok(result)) {
            NixosLog_fatal(__btrc_str_track(__btrc_strcat("Failed to create qcow2 state delta for ", self->spec->state)));
        }
        Path_writeAll(VmStateStore_backingDiskFile(self), __btrc_str_track(__btrc_strcat(backing, "\n")));
    }
    VmStateStore_copyIfExists(self, PathTools_join(parentWorkDir, "id_ed25519"), VmStateStore_sshKeyPath(self));
    VmStateStore_copyIfExists(self, PathTools_join(parentWorkDir, "id_ed25519.pub"), VmStateStore_sshPubKeyPath(self));
    VmStateStore_copyIfExists(self, PathTools_join(parentWorkDir, "edk2-vars.fd"), QemuFirmware_firmwareVarsPath(self->firmware));
    VmStateStore_copyTreeIfExists(self, PathTools_join(parentWorkDir, "tpm"), QemuFirmware_tpmDir(self->firmware));
}

void VmStateStore_recordState(VmStateStore* self) {
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
    JsonObject_setString(metadata, "disk", VmStateStore_diskPath(self));
    JsonObject_setString(metadata, "uefi", (QemuFirmware_uefiEnabled(self->firmware) ? "true" : "false"));
    JsonObject_setString(metadata, "secureBoot", (QemuFirmware_secureBootEnabled(self->firmware) ? "true" : "false"));
    JsonObject_setString(metadata, "tpm2", (QemuFirmware_tpm2Enabled(self->firmware) ? "true" : "false"));
    if (FileSystem_exists(VmStateStore_backingDiskFile(self))) {
        JsonObject_setString(metadata, "delta", "qcow2-backing");
        JsonObject_setString(metadata, "backingDisk", __btrc_str_track(__btrc_trim(Path_readAll(VmStateStore_backingDiskFile(self)))));
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

bool VmStateStore_isRunning(VmStateStore* self) {
    if (!FileSystem_exists(VmStateStore_pidPath(self))) {
        return false;
    }
    ExecResult* result = UnixShell_runRaw(self->shell, __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("kill -0 $(cat ", UnixShell_quote(VmStateStore_pidPath(self)))), ") 2>/dev/null")), false, false, "");
    return ExecResult_ok(result);
}

bool VmStateStore_hasSnapshot(VmStateStore* self, char* name) {
    if (!FileSystem_exists(VmStateStore_diskPath(self))) {
        return false;
    }
    ExecResult* result = UnixShell_runUnchecked(self->shell, __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("qemu-img snapshot -l ", UnixShell_quote(VmStateStore_diskPath(self)))), " | awk '{print $2}'")));
    if (!ExecResult_ok(result)) {
        return false;
    }
    btrc_Vector_string* lines = Strings_split(ExecResult_stdout(result), "\n");
    int __n_196 = btrc_Vector_string_iterLen(lines);
    for (int __i_195 = 0; (__i_195 < __n_196); (__i_195++)) {
        char* line = btrc_Vector_string_iterGet(lines, __i_195);
        if (strcmp(__btrc_str_track(__btrc_trim(line)), name) == 0) {
            return true;
        }
    }
    return false;
}

bool VmStateStore_hasBackingDisk(VmStateStore* self) {
    return FileSystem_exists(VmStateStore_backingDiskFile(self));
}

void VmStateStore_printStatus(VmStateStore* self) {
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
    Console_log(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("Disk:      ", (FileSystem_exists(VmStateStore_diskPath(self)) ? "yes" : "no"))), " ")), VmStateStore_diskPath(self))));
    Console_log(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("SSH key:   ", (FileSystem_exists(VmStateStore_sshKeyPath(self)) ? "yes" : "no"))), " ")), VmStateStore_sshKeyPath(self))));
    Console_log(__btrc_str_track(__btrc_strcat("Running:   ", (VmStateStore_isRunning(self) ? "yes" : "no"))));
    Console_log(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("Recorded:  ", recorded)), " ")), VmTestSpec_stateDir(self->spec))));
}

void VmProvisioner_init(VmProvisioner* self, VmTestSpec* spec, SshClient* remote) {
    self->__rc = 1;
    if (self->spec != NULL) {
        if ((--self->spec->__rc) <= 0) {
            VmTestSpec_destroy(self->spec);
        }
    }
    (self->spec = spec);
    (spec->__rc++);
    if (self->remote != NULL) {
        if ((--self->remote->__rc) <= 0) {
            SshClient_destroy(self->remote);
        }
    }
    (self->remote = remote);
    (remote->__rc++);
}

VmProvisioner* VmProvisioner_new(VmTestSpec* spec, SshClient* remote) {
    VmProvisioner* self = ((VmProvisioner*)malloc(sizeof(VmProvisioner)));
    memset(self, 0, sizeof(VmProvisioner));
    VmProvisioner_init(self, spec, remote);
    return self;
}

void VmProvisioner_destroy(VmProvisioner* self) {
    if (self->spec != NULL) {
        if ((--self->spec->__rc) <= 0) {
            VmTestSpec_destroy(self->spec);
        }
    }
    if (self->remote != NULL) {
        if ((--self->remote->__rc) <= 0) {
            SshClient_destroy(self->remote);
        }
    }
    if (__btrc_tracking) {
        __btrc_mark_destroyed(self);
    }
    free(self);
}

char* VmProvisioner_pubKey(VmProvisioner* self) {
    return __btrc_str_track(__btrc_trim(Path_readAll(PathTools_join(self->spec->workDir, "id_ed25519.pub"))));
}

void VmProvisioner_configureVmHost(VmProvisioner* self) {
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
    char* pubkey = VmProvisioner_pubKey(self);
    char* passwordHash = "$6$vmtest$zfOOVFBtEpiYVE486Cybmppi9zXd0QKYfRZ5FRLsMV26K7vK6QHIpZpUo1sxVt7liGoR9C/W1I5ih4VCt34n3.";
    char* secureBootSettings = (secureBootTarget ? "  settings.boot.pkiBundle = \"/var/lib/sbctl\";\n" : "");
    char* secureBootModulePath = "/etc/nixos/modules/system/vm-test-secure-boot.nix";
    char* secureBootModule = __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("{ config, lib, pkgs, ... }: {\n", "  config = lib.mkIf (config.networking.hostName == \"VM-TEST\" && config.settings.boot.method == \"Secure-Boot\") {\n")), "    boot.loader.timeout = lib.mkForce 0;\n")), "    boot.lanzaboote.autoGenerateKeys.enable = true;\n")), "    boot.lanzaboote.autoEnrollKeys.enable = true;\n")), "    boot.lanzaboote.allowUnsigned = true;\n")), "    boot.lanzaboote.autoEnrollKeys.includeMicrosoftKeys = true;\n")), "    boot.lanzaboote.autoEnrollKeys.autoReboot = false;\n")), "    environment.systemPackages = [ pkgs.sbctl ];\n")), "  };\n")), "}\n"));
    char* secureBootModuleCommand = (secureBootTarget ? __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("printf %s ", UnixShell_quote(secureBootModule))), " > ")), UnixShell_quote(secureBootModulePath))), "; ")) : "");
    char* hostNix = __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("{ lib, ... }: {\n", "  networking.hostName = \"VM-TEST\";\n")), "  networking.firewall.enable = lib.mkForce false;\n")), "  users.users.root.openssh.authorizedKeys.keys = [ \"")), pubkey)), "\" ];\n")), "  boot.kernelParams = [ \"console=")), serialConsole)), "\" ];\n")), "  boot.initrd.availableKernelModules = ")), initrdModules)), ";\n")), "  boot.plymouth.enable = lib.mkForce false;\n")), "  services.getty.autologinUser = lib.mkForce \"root\";\n")), "  services.openssh.enable = lib.mkForce true;\n")), "  services.openssh.settings.PermitRootLogin = lib.mkForce \"yes\";\n")), "  services.openssh.settings.UseDns = lib.mkForce false;\n")), "  settings.user.admin.autoUnlockWallet.enabled = false;\n")), "  settings.user.admin.homeManager.enable = false;\n")), "  settings.networking.identityAgent = \"none\";\n")), "  settings.networking.lanSubnet = \"10.0.2.0/24\";\n")), "  settings.disk.device = \"/dev/vda\";\n")), "  settings.disk.encryption.enable = ")), encryption)), ";\n")), "  settings.disk.immutability.enable = ")), enabled)), ";\n")), "  settings.disk.immutability.mode = \"")), mode)), "\";\n")), "  settings.disk.immutability.persist.paths = lib.mkForce [ \"/etc/machine-id\" \"/etc/nixos\" \"/var/lib/nixos\" \"/var/lib/sbctl\" \"/var/log\" ];\n")), "  settings.disk.immutability.semipermeable_membrane.mode = \"")), mode)), "\";\n")), "  settings.desktop.enable = ")), ((strcmp(desktop, "none") == 0) ? "false" : "true"))), ";\n")), "  settings.disk.swap.size = \"2G\";\n")), secureBootSettings)), "}\n"));
    char* localConfig = __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("{\"host_path\":\"modules/hosts/", self->spec->arch)), "/VM-TEST/VM-TEST.nix\",\"target\":\"")), bootTargetName)), "\"}\n"));
    char* luksCommand = ((strcmp(encryption, "true") == 0) ? __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("printf %s ", UnixShell_quote(luksPass))), " > /tmp/plain_text_password.txt; chmod 600 /tmp/plain_text_password.txt; ")) : "");
    char* command = __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("set -e; ", luksCommand)), "rm -rf /etc/nixos/modules/hosts/")), otherArch)), "; ")), "rm -f ")), UnixShell_quote(secureBootModulePath))), "; ")), secureBootModuleCommand)), "find /etc/nixos/modules/apps -maxdepth 1 -type f -name '*.nix' ! -name 'nixosctl.nix' -delete; ")), "find /etc/nixos/modules/apps/home-manager -maxdepth 1 -type f -name '*.nix' -delete 2>/dev/null || true; ")), "mkdir -p /etc/nixos/modules/apps/home-manager; ")), "touch /etc/nixos/modules/apps/home-manager/.keep; ")), "mkdir -p /etc/nixos/secrets; ")), "printf %s ")), UnixShell_quote(passwordHash))), " > /etc/nixos/secrets/hashed_password.txt; ")), "chmod 600 /etc/nixos/secrets/hashed_password.txt; ")), "mkdir -p ")), UnixShell_quote(hostDir))), "; ")), "printf %s ")), UnixShell_quote(hostNix))), " > ")), UnixShell_quote(hostPath))), "; ")), "printf %s ")), UnixShell_quote(localConfig))), " > /etc/nixos/config.json; ")), "git -C /etc/nixos init; ")), "git config --global --add safe.directory /etc/nixos; ")), "git -C /etc/nixos add -A; ")), "git -C /etc/nixos -c user.name=test -c user.email=test@test commit -m init --allow-empty"));
    ExecResult* result = SshClient_sshWithTimeout(self->remote, command, false, 120);
    if (!ExecResult_ok(result)) {
        Console_error(ExecResult_stdout(result));
        NixosLog_fatal("Failed to configure VM host");
    }
}

bool VmProvisioner_installNixosGuest(VmProvisioner* self) {
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
    ExecResult* result = SshClient_sshWithTimeout(self->remote, command, false, timeoutSeconds);
    if ((!ExecResult_ok(result)) || (!__btrc_strContains(ExecResult_stdout(result), "INSTALL_OK"))) {
        Console_error(ExecResult_stdout(result));
        return false;
    }
    return true;
}

void QemuCommandBuilder_init(QemuCommandBuilder* self, VmTestSpec* spec, QemuFirmware* firmware, QemuSerial* serial) {
    self->__rc = 1;
    if (self->spec != NULL) {
        if ((--self->spec->__rc) <= 0) {
            VmTestSpec_destroy(self->spec);
        }
    }
    (self->spec = spec);
    (spec->__rc++);
    if (self->firmware != NULL) {
        if ((--self->firmware->__rc) <= 0) {
            QemuFirmware_destroy(self->firmware);
        }
    }
    (self->firmware = firmware);
    (firmware->__rc++);
    if (self->serial != NULL) {
        if ((--self->serial->__rc) <= 0) {
            QemuSerial_destroy(self->serial);
        }
    }
    (self->serial = serial);
    (serial->__rc++);
}

QemuCommandBuilder* QemuCommandBuilder_new(VmTestSpec* spec, QemuFirmware* firmware, QemuSerial* serial) {
    QemuCommandBuilder* self = ((QemuCommandBuilder*)malloc(sizeof(QemuCommandBuilder)));
    memset(self, 0, sizeof(QemuCommandBuilder));
    QemuCommandBuilder_init(self, spec, firmware, serial);
    return self;
}

void QemuCommandBuilder_destroy(QemuCommandBuilder* self) {
    if (self->spec != NULL) {
        if ((--self->spec->__rc) <= 0) {
            VmTestSpec_destroy(self->spec);
        }
    }
    if (self->firmware != NULL) {
        if ((--self->firmware->__rc) <= 0) {
            QemuFirmware_destroy(self->firmware);
        }
    }
    if (self->serial != NULL) {
        if ((--self->serial->__rc) <= 0) {
            QemuSerial_destroy(self->serial);
        }
    }
    if (__btrc_tracking) {
        __btrc_mark_destroyed(self);
    }
    free(self);
}

char* QemuCommandBuilder_diskPath(QemuCommandBuilder* self) {
    return PathTools_join(self->spec->workDir, "disk.qcow2");
}

char* QemuCommandBuilder_monitorPath(QemuCommandBuilder* self) {
    return PathTools_join(self->spec->workDir, "monitor.sock");
}

char* QemuCommandBuilder_qmpPath(QemuCommandBuilder* self) {
    return PathTools_join(self->spec->workDir, "qmp.sock");
}

void QemuCommandBuilder_addAccelerator(QemuCommandBuilder* self, Command* cmd) {
    if (QemuFirmware_isDarwin(self->firmware)) {
        Command_arg(cmd, "-accel");
        Command_arg(cmd, (QemuFirmware_hostArchMatchesGuest(self->firmware) ? "hvf" : "tcg"));
    } else if (QemuFirmware_hostArchMatchesGuest(self->firmware)) {
        Command_arg(cmd, "-enable-kvm");
    } else {
        Command_arg(cmd, "-accel");
        Command_arg(cmd, "tcg");
    }
}

void QemuCommandBuilder_addMachine(QemuCommandBuilder* self, Command* cmd) {
    if (!(strcmp(self->spec->arch, "aarch64") == 0)) {
        return;
    }
    Command_arg(cmd, "-machine");
    Command_arg(cmd, "virt");
    Command_arg(cmd, "-cpu");
    Command_arg(cmd, (QemuFirmware_hostArchMatchesGuest(self->firmware) ? "host" : "max"));
    Command_arg(cmd, "-device");
    Command_arg(cmd, "virtio-rng-pci");
}

void QemuCommandBuilder_addMemoryCpus(QemuCommandBuilder* self, Command* cmd) {
    Command_arg(cmd, "-m");
    Command_arg(cmd, self->spec->memory);
    Command_arg(cmd, "-smp");
    Command_arg(cmd, Strings_fromInt(self->spec->cpus));
}

void QemuCommandBuilder_addStorageAndNetwork(QemuCommandBuilder* self, Command* cmd) {
    Command_arg(cmd, "-drive");
    Command_arg(cmd, __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("file=", QemuCommandBuilder_diskPath(self))), ",format=qcow2,if=virtio,cache=none")));
    Command_arg(cmd, "-netdev");
    Command_arg(cmd, __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("user,id=net0,hostfwd=tcp::", Strings_fromInt(self->spec->sshPort))), "-:22")));
    Command_arg(cmd, "-device");
    Command_arg(cmd, "virtio-net-pci,netdev=net0");
}

void QemuCommandBuilder_addConsole(QemuCommandBuilder* self, Command* cmd) {
    Command_arg(cmd, "-display");
    Command_arg(cmd, "none");
    Command_arg(cmd, "-serial");
    Command_arg(cmd, __btrc_str_track(__btrc_strcat("pipe:", QemuSerial_serialBasePath(self->serial))));
    Command_arg(cmd, "-monitor");
    Command_arg(cmd, __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("unix:", QemuCommandBuilder_monitorPath(self))), ",server,nowait")));
    Command_arg(cmd, "-qmp");
    Command_arg(cmd, __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("unix:", QemuCommandBuilder_qmpPath(self))), ",server,nowait")));
}

void QemuCommandBuilder_addBootMedia(QemuCommandBuilder* self, Command* cmd, bool fromIso) {
    if (!fromIso) {
        return;
    }
    if ((strcmp(self->spec->arch, "x86_64") == 0) && (!QemuFirmware_shouldUseUefi(self->firmware, fromIso))) {
        QemuSerial_extractBootSerial(self->serial);
        Command_arg(cmd, "-kernel");
        Command_arg(cmd, __btrc_str_track(__btrc_trim(Path_readAll(QemuSerial_bootKernelFile(self->serial)))));
        Command_arg(cmd, "-initrd");
        Command_arg(cmd, __btrc_str_track(__btrc_trim(Path_readAll(QemuSerial_bootInitrdFile(self->serial)))));
        Command_arg(cmd, "-append");
        Command_arg(cmd, __btrc_str_track(__btrc_trim(Path_readAll(QemuSerial_bootAppendFile(self->serial)))));
        Command_arg(cmd, "-cdrom");
        Command_arg(cmd, self->spec->iso);
    } else {
        Command_arg(cmd, "-cdrom");
        Command_arg(cmd, self->spec->iso);
        Command_arg(cmd, "-boot");
        Command_arg(cmd, "d");
    }
}

Command* QemuCommandBuilder_build(QemuCommandBuilder* self, bool fromIso) {
    Command* cmd = Command_new(QemuFirmware_qemuBinary(self->firmware));
    QemuCommandBuilder_addAccelerator(self, cmd);
    QemuCommandBuilder_addMachine(self, cmd);
    QemuFirmware_addFirmware(self->firmware, cmd, fromIso);
    QemuCommandBuilder_addMemoryCpus(self, cmd);
    QemuCommandBuilder_addStorageAndNetwork(self, cmd);
    QemuCommandBuilder_addConsole(self, cmd);
    QemuFirmware_addTpm2(self->firmware, cmd);
    QemuCommandBuilder_addBootMedia(self, cmd, fromIso);
    return cmd;
    if (cmd != NULL) {
        if ((--cmd->__rc) <= 0) {
            Command_destroy(cmd);
        }
    }
}

void VmAssets_init(VmAssets* self, VmTestSpec* spec, QemuSerial* serial) {
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
    if (self->serial != NULL) {
        if ((--self->serial->__rc) <= 0) {
            QemuSerial_destroy(self->serial);
        }
    }
    (self->serial = serial);
    (serial->__rc++);
}

VmAssets* VmAssets_new(VmTestSpec* spec, QemuSerial* serial) {
    VmAssets* self = ((VmAssets*)malloc(sizeof(VmAssets)));
    memset(self, 0, sizeof(VmAssets));
    VmAssets_init(self, spec, serial);
    return self;
}

void VmAssets_destroy(VmAssets* self) {
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
    if (self->serial != NULL) {
        if ((--self->serial->__rc) <= 0) {
            QemuSerial_destroy(self->serial);
        }
    }
    if (__btrc_tracking) {
        __btrc_mark_destroyed(self);
    }
    free(self);
}

char* VmAssets_sshKeyPath(VmAssets* self) {
    return PathTools_join(self->spec->workDir, "id_ed25519");
}

char* VmAssets_sshPubKeyPath(VmAssets* self) {
    return __btrc_str_track(__btrc_strcat(VmAssets_sshKeyPath(self), ".pub"));
}

char* VmAssets_diskPath(VmAssets* self) {
    return PathTools_join(self->spec->workDir, "disk.qcow2");
}

void VmAssets_ensureWorkDir(VmAssets* self) {
    FileSystem_mkdirp(self->spec->workDir);
    FileSystem_mkdirp(PathTools_dirname(self->spec->iso));
}

void VmAssets_downloadIso(VmAssets* self) {
    VmAssets_ensureWorkDir(self);
    if (FileSystem_exists(self->spec->iso)) {
        return;
    }
    if (__btrc_isEmpty(self->spec->isoUrl)) {
        NixosLog_fatal("No iso or isoUrl in VM spec");
    }
    int __fstr_197_len = snprintf(NULL, 0, "%s.tmp", self->spec->iso);
    char* __fstr_197_buf = __btrc_str_track(((char*)malloc((__fstr_197_len + 1))));
    snprintf(__fstr_197_buf, (__fstr_197_len + 1), "%s.tmp", self->spec->iso);
    char* tmp = __fstr_197_buf;
    Command* curl = Command_capture(Command_arg(Command_arg(Command_arg(Command_arg(Command_new("curl"), "-L"), "-o"), tmp), self->spec->isoUrl), false);
    ExecResult* result = UnixShell_runCommand(self->shell, curl);
    if (!ExecResult_ok(result)) {
        NixosLog_fatal("Failed to download installer ISO");
    }
    Command* mv = Command_capture(Command_arg(Command_arg(Command_new("mv"), tmp), self->spec->iso), false);
    UnixShell_runCommand(self->shell, mv);
}

void VmAssets_createSshKey(VmAssets* self) {
    VmAssets_ensureWorkDir(self);
    if (FileSystem_exists(VmAssets_sshKeyPath(self))) {
        return;
    }
    Command* cmd = Command_capture(Command_arg(Command_arg(Command_arg(Command_arg(Command_arg(Command_arg(Command_arg(Command_new("ssh-keygen"), "-t"), "ed25519"), "-N"), ""), "-f"), VmAssets_sshKeyPath(self)), "-q"), false);
    ExecResult* result = UnixShell_runCommand(self->shell, cmd);
    if (!ExecResult_ok(result)) {
        NixosLog_fatal("Failed to create VM SSH key");
    }
}

char* VmAssets_sshPubKey(VmAssets* self) {
    return __btrc_str_track(__btrc_trim(Path_readAll(VmAssets_sshPubKeyPath(self))));
}

void VmAssets_createDisk(VmAssets* self) {
    VmAssets_ensureWorkDir(self);
    if (FileSystem_exists(VmAssets_diskPath(self))) {
        return;
    }
    Command* cmd = Command_capture(Command_arg(Command_arg(Command_arg(Command_arg(Command_arg(Command_new("qemu-img"), "create"), "-f"), "qcow2"), VmAssets_diskPath(self)), self->spec->diskSize), false);
    ExecResult* result = UnixShell_runCommand(self->shell, cmd);
    if (!ExecResult_ok(result)) {
        NixosLog_fatal("Failed to create VM disk");
    }
}

void VmAssets_setup(VmAssets* self) {
    VmAssets_downloadIso(self);
    VmAssets_createSshKey(self);
    VmAssets_createDisk(self);
    QemuSerial_extractBootSerial(self->serial);
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
    if (self->remote != NULL) {
        if ((--self->remote->__rc) <= 0) {
            SshClient_destroy(self->remote);
        }
    }
    (self->remote = SshClient_new(spec));
    (SshClient_new(spec)->__rc++);
    if (self->serial != NULL) {
        if ((--self->serial->__rc) <= 0) {
            QemuSerial_destroy(self->serial);
        }
    }
    (self->serial = QemuSerial_new(spec));
    (QemuSerial_new(spec)->__rc++);
    if (self->firmware != NULL) {
        if ((--self->firmware->__rc) <= 0) {
            QemuFirmware_destroy(self->firmware);
        }
    }
    (self->firmware = QemuFirmware_new(spec));
    (QemuFirmware_new(spec)->__rc++);
    if (self->state != NULL) {
        if ((--self->state->__rc) <= 0) {
            VmStateStore_destroy(self->state);
        }
    }
    (self->state = VmStateStore_new(spec, self->firmware));
    (VmStateStore_new(spec, self->firmware)->__rc++);
    if (self->provisioner != NULL) {
        if ((--self->provisioner->__rc) <= 0) {
            VmProvisioner_destroy(self->provisioner);
        }
    }
    (self->provisioner = VmProvisioner_new(spec, self->remote));
    (VmProvisioner_new(spec, self->remote)->__rc++);
    if (self->qemuCmd != NULL) {
        if ((--self->qemuCmd->__rc) <= 0) {
            QemuCommandBuilder_destroy(self->qemuCmd);
        }
    }
    (self->qemuCmd = QemuCommandBuilder_new(spec, self->firmware, self->serial));
    (QemuCommandBuilder_new(spec, self->firmware, self->serial)->__rc++);
    if (self->assets != NULL) {
        if ((--self->assets->__rc) <= 0) {
            VmAssets_destroy(self->assets);
        }
    }
    (self->assets = VmAssets_new(spec, self->serial));
    (VmAssets_new(spec, self->serial)->__rc++);
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
    if (self->remote != NULL) {
        if ((--self->remote->__rc) <= 0) {
            SshClient_destroy(self->remote);
        }
    }
    if (self->serial != NULL) {
        if ((--self->serial->__rc) <= 0) {
            QemuSerial_destroy(self->serial);
        }
    }
    if (self->firmware != NULL) {
        if ((--self->firmware->__rc) <= 0) {
            QemuFirmware_destroy(self->firmware);
        }
    }
    if (self->state != NULL) {
        if ((--self->state->__rc) <= 0) {
            VmStateStore_destroy(self->state);
        }
    }
    if (self->provisioner != NULL) {
        if ((--self->provisioner->__rc) <= 0) {
            VmProvisioner_destroy(self->provisioner);
        }
    }
    if (self->qemuCmd != NULL) {
        if ((--self->qemuCmd->__rc) <= 0) {
            QemuCommandBuilder_destroy(self->qemuCmd);
        }
    }
    if (self->assets != NULL) {
        if ((--self->assets->__rc) <= 0) {
            VmAssets_destroy(self->assets);
        }
    }
    if (__btrc_tracking) {
        __btrc_mark_destroyed(self);
    }
    free(self);
}

char* QemuE2eHarness_diskPath(QemuE2eHarness* self) {
    return PathTools_join(self->spec->workDir, "disk.qcow2");
}

char* QemuE2eHarness_pidPath(QemuE2eHarness* self) {
    return PathTools_join(self->spec->workDir, "qemu.pid");
}

char* QemuE2eHarness_monitorPath(QemuE2eHarness* self) {
    return PathTools_join(self->spec->workDir, "monitor.sock");
}

char* QemuE2eHarness_qmpPath(QemuE2eHarness* self) {
    return PathTools_join(self->spec->workDir, "qmp.sock");
}

void QemuE2eHarness_ensureWorkDir(QemuE2eHarness* self) {
    FileSystem_mkdirp(self->spec->workDir);
    FileSystem_mkdirp(PathTools_dirname(self->spec->iso));
}

void QemuE2eHarness_downloadIso(QemuE2eHarness* self) {
    VmAssets_downloadIso(self->assets);
}

void QemuE2eHarness_createSshKey(QemuE2eHarness* self) {
    VmAssets_createSshKey(self->assets);
}

char* QemuE2eHarness_sshPubKey(QemuE2eHarness* self) {
    return VmAssets_sshPubKey(self->assets);
}

void QemuE2eHarness_createDisk(QemuE2eHarness* self) {
    VmAssets_createDisk(self->assets);
}

void QemuE2eHarness_setup(QemuE2eHarness* self) {
    VmAssets_setup(self->assets);
}

void QemuE2eHarness_resetState(QemuE2eHarness* self) {
    QemuE2eHarness_stop(self);
    FileSystem_removeRecursive(self->spec->workDir);
    QemuE2eHarness_ensureWorkDir(self);
}

void QemuE2eHarness_cleanStateRecord(QemuE2eHarness* self) {
    VmStateStore_cleanStateRecord(self->state);
}

void QemuE2eHarness_requireParentState(QemuE2eHarness* self) {
    VmStateStore_requireParentState(self->state);
}

void QemuE2eHarness_inheritState(QemuE2eHarness* self) {
    VmStateStore_inheritState(self->state);
}

void QemuE2eHarness_recordState(QemuE2eHarness* self) {
    VmStateStore_recordState(self->state);
}

void QemuE2eHarness_printStatus(QemuE2eHarness* self) {
    VmStateStore_printStatus(self->state);
}

void QemuE2eHarness_start(QemuE2eHarness* self, bool fromIso) {
    QemuE2eHarness_ensureWorkDir(self);
    if (FileSystem_exists(QemuE2eHarness_pidPath(self))) {
        NixosLog_info("QEMU pidfile exists; assuming VM is already running");
        return;
    }
    Command* cmd = QemuCommandBuilder_build(self->qemuCmd, fromIso);
    QemuSerial_prepareSerialPipe(self->serial);
    QemuSerial_startSerialReader(self->serial);
    Command_arg(cmd, "-daemonize");
    Command_arg(cmd, "-pidfile");
    Command_arg(cmd, QemuE2eHarness_pidPath(self));
    Command_capture(cmd, false);
    ExecResult* result = UnixShell_runCommand(self->shell, cmd);
    if (!ExecResult_ok(result)) {
        QemuSerial_stopSerialReader(self->serial);
        QemuFirmware_stopSwtpm(self->firmware);
        NixosLog_fatal("Failed to start QEMU");
    }
}

void QemuE2eHarness_stop(QemuE2eHarness* self) {
    QemuSerial_stopSerialReader(self->serial);
    if (FileSystem_exists(QemuE2eHarness_pidPath(self))) {
        char* quotedPid = UnixShell_quote(QemuE2eHarness_pidPath(self));
        UnixShell_runRaw(self->shell, __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("pid=$(cat ", quotedPid)), "); kill $pid 2>/dev/null || true; sleep 2; kill -0 $pid 2>/dev/null && kill -9 $pid 2>/dev/null || true")), false, false, "");
        FileSystem_removeRecursive(QemuE2eHarness_pidPath(self));
    }
    FileSystem_removeRecursive(QemuE2eHarness_monitorPath(self));
    FileSystem_removeRecursive(QemuE2eHarness_qmpPath(self));
    FileSystem_removeRecursive(QemuSerial_serialInPath(self->serial));
    FileSystem_removeRecursive(QemuSerial_serialOutPath(self->serial));
    QemuFirmware_stopSwtpm(self->firmware);
}

void QemuE2eHarness_sleepSeconds(QemuE2eHarness* self, int seconds) {
    UnixShell_runRaw(self->shell, __btrc_str_track(__btrc_strcat("sleep ", Strings_fromInt(seconds))), false, false, "");
}

void QemuE2eHarness_bootstrapSsh(QemuE2eHarness* self) {
    QemuE2eHarness_createSshKey(self);
    char* key = QemuE2eHarness_sshPubKey(self);
    QemuSerial_serialSend(self->serial, "");
    QemuSerial_serialSend(self->serial, __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("sudo install -d -m 700 /root/.ssh; echo ", UnixShell_quote(key))), " | sudo tee /root/.ssh/authorized_keys > /dev/null; sudo chmod 600 /root/.ssh/authorized_keys; sudo systemctl restart sshd")));
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

ExecResult* QemuE2eHarness_ssh(QemuE2eHarness* self, char* command, bool checkStatus) {
    return SshClient_ssh(self->remote, command, checkStatus);
}

ExecResult* QemuE2eHarness_sshWithTimeout(QemuE2eHarness* self, char* command, bool checkStatus, int timeoutSeconds) {
    return SshClient_sshWithTimeout(self->remote, command, checkStatus, timeoutSeconds);
}

ExecResult* QemuE2eHarness_host(QemuE2eHarness* self, char* command, bool checkStatus) {
    return SshClient_host(self->remote, command, checkStatus);
}

ExecResult* QemuE2eHarness_nixEval(QemuE2eHarness* self, char* attribute, int timeoutSeconds) {
    return SshClient_nixEval(self->remote, attribute, timeoutSeconds);
}

ExecResult* QemuE2eHarness_qmp(QemuE2eHarness* self, char* command, int timeoutSeconds) {
    return SshClient_qmp(self->remote, command, timeoutSeconds);
}

ExecResult* QemuE2eHarness_workspaceFileExists(QemuE2eHarness* self, char* relativePath) {
    return SshClient_workspaceFileExists(self->remote, relativePath);
}

void QemuE2eHarness_copyWorkspace(QemuE2eHarness* self, char* localPath, char* remotePath) {
    SshClient_copyWorkspace(self->remote, localPath, remotePath);
}

void QemuE2eHarness_copyTo(QemuE2eHarness* self, char* localPath, char* remotePath) {
    SshClient_copyTo(self->remote, localPath, remotePath);
}

void QemuE2eHarness_copyFrom(QemuE2eHarness* self, char* remotePath, char* localPath) {
    SshClient_copyFrom(self->remote, remotePath, localPath);
}

char* QemuE2eHarness_serialLogPath(QemuE2eHarness* self) {
    return QemuSerial_serialLogPath(self->serial);
}

void QemuE2eHarness_serialSend(QemuE2eHarness* self, char* command) {
    QemuSerial_serialSend(self->serial, command);
}

void QemuE2eHarness_requireCommand(QemuE2eHarness* self, char* name) {
    QemuFirmware_requireCommand(self->firmware, name);
}

void QemuE2eHarness_requireTpm2Capability(QemuE2eHarness* self) {
    QemuFirmware_requireTpm2Capability(self->firmware);
}

void QemuE2eHarness_requireUefiCapability(QemuE2eHarness* self) {
    QemuFirmware_requireUefiCapability(self->firmware);
}

void QemuE2eHarness_requireSecureBootCapability(QemuE2eHarness* self) {
    QemuFirmware_requireSecureBootCapability(self->firmware);
}

char* QemuE2eHarness_secureBootCapabilityReport(QemuE2eHarness* self) {
    return QemuFirmware_secureBootCapabilityReport(self->firmware);
}

bool QemuE2eHarness_waitForSsh(QemuE2eHarness* self, int timeout) {
    int elapsed = 0;
    while (elapsed < timeout) {
        ExecResult* result = QemuE2eHarness_ssh(self, "true", false);
        if (ExecResult_ok(result)) {
            return true;
        }
        QemuE2eHarness_sleepSeconds(self, 3);
        (elapsed = (elapsed + 8));
        if ((elapsed > 0) && (__btrc_mod_int(elapsed, 24) == 0)) {
            QemuE2eHarness_bootstrapSsh(self);
        }
    }
    return false;
}

void QemuE2eHarness_configureVmHost(QemuE2eHarness* self) {
    VmProvisioner_configureVmHost(self->provisioner);
}

void QemuE2eHarness_installNixosGuest(QemuE2eHarness* self) {
    if (!VmProvisioner_installNixosGuest(self->provisioner)) {
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
    if (FileSystem_exists(QemuFirmware_firmwareVarsPath(self->firmware))) {
        Command* cp = Command_capture(Command_arg(Command_arg(Command_new("cp"), QemuFirmware_firmwareVarsPath(self->firmware)), QemuFirmware_firmwareVarsSnapshotPath(self->firmware, name)), false);
        UnixShell_runCommand(self->shell, cp);
    }
}

void QemuE2eHarness_restore(QemuE2eHarness* self, char* name) {
    QemuE2eHarness_stop(self);
    if ((!VmStateStore_hasSnapshot(self->state, name)) && VmStateStore_hasBackingDisk(self->state)) {
        NixosLog_info(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("Snapshot ", name)), " is inherited through qcow2 backing; restore is a no-op")));
        return;
    }
    Command* cmd = Command_capture(Command_arg(Command_arg(Command_arg(Command_arg(Command_new("qemu-img"), "snapshot"), "-a"), name), QemuE2eHarness_diskPath(self)), false);
    ExecResult* result = UnixShell_runCommand(self->shell, cmd);
    if (!ExecResult_ok(result)) {
        NixosLog_fatal(__btrc_str_track(__btrc_strcat("Failed to restore VM snapshot ", name)));
    }
    if (FileSystem_exists(QemuFirmware_firmwareVarsSnapshotPath(self->firmware, name))) {
        Command* cp = Command_capture(Command_arg(Command_arg(Command_new("cp"), QemuFirmware_firmwareVarsSnapshotPath(self->firmware, name)), QemuFirmware_firmwareVarsPath(self->firmware)), false);
        UnixShell_runCommand(self->shell, cp);
        QemuFirmware_makeFirmwareVarsWritable(self->firmware);
    }
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
        return true;
    }
    return __btrc_strContains(ExecResult_stdout(result), expect);
}

void VmTestRunner_assertResult(VmTestRunner* self, char* label, ExecResult* result, char* expect) {
    if (!ExecResult_ok(result)) {
        Console_error(ExecResult_trimmed(result));
        VmTestRunner_fail(self, __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(label, " failed with exit code ")), Strings_fromInt(result->code))));
        return;
    }
    if (!VmTestRunner_outputMatches(self, result, expect)) {
        Console_error(ExecResult_trimmed(result));
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
    int __n_199 = btrc_Vector_VmOperation_iterLen(self->spec->operations);
    for (int __i_198 = 0; (__i_198 < __n_199); (__i_198++)) {
        VmOperation* op = btrc_Vector_VmOperation_iterGet(self->spec->operations, __i_198);
        if (self->failures > 0) {
            break;
        }
        VmTestRunner_runOperation(self, op);
    }
    if (self->failures > 0) {
        QemuE2eHarness_stop(self->vm);
        return 1;
    }
    NixosLog_info(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("e2e ", self->spec->name)), ": pass")));
    return 0;
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
    btrc_Vector_string* __list_201 = btrc_Vector_string_new();
    (self->after = __list_201);
    btrc_Vector_string* __list_200 = btrc_Vector_string_new();
    (__list_200->__rc++);
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
    btrc_Vector_string* __list_203 = btrc_Vector_string_new();
    (self->defaults = __list_203);
    btrc_Vector_string* __list_202 = btrc_Vector_string_new();
    (__list_202->__rc++);
    if (self->nodes != NULL) {
        if ((--self->nodes->__rc) <= 0) {
            btrc_Vector_VmGraphNode_free(self->nodes);
        }
    }
    btrc_Vector_VmGraphNode* __list_205 = btrc_Vector_VmGraphNode_new();
    (self->nodes = __list_205);
    btrc_Vector_VmGraphNode* __list_204 = btrc_Vector_VmGraphNode_new();
    (__list_204->__rc++);
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
    int __n_207 = btrc_Vector_VmGraphNode_iterLen(self->nodes);
    for (int __i_206 = 0; (__i_206 < __n_207); (__i_206++)) {
        VmGraphNode* node = btrc_Vector_VmGraphNode_iterGet(self->nodes, __i_206);
        if (strcmp(node->id, id) == 0) {
            return node;
        }
    }
    NixosLog_fatal(__btrc_str_track(__btrc_strcat("Unknown graph node: ", id)));
    return VmGraphNode_new();
}

char* VmTestGraph_resolvedSpecPath(VmTestGraph* self, VmGraphNode* node) {
    if (__btrc_startsWith(node->specPath, "/")) {
        return node->specPath;
    }
    if (FileSystem_exists(node->specPath)) {
        return node->specPath;
    }
    return PathTools_join(self->baseDir, node->specPath);
}

char* VmTestGraph_resolvedWorkspaceRoot(VmTestGraph* self) {
    if (__btrc_startsWith(self->workspaceRoot, "/")) {
        return self->workspaceRoot;
    }
    return PathTools_join(self->baseDir, self->workspaceRoot);
}

btrc_Vector_string* VmTestGraph_defaultTargets(VmTestGraph* self) {
    if (!btrc_Vector_string_isEmpty(self->defaults)) {
        return self->defaults;
    }
    btrc_Vector_string* result = btrc_Vector_string_new();
    int __n_209 = btrc_Vector_VmGraphNode_iterLen(self->nodes);
    for (int __i_208 = 0; (__i_208 < __n_209); (__i_208++)) {
        VmGraphNode* node = btrc_Vector_VmGraphNode_iterGet(self->nodes, __i_208);
        btrc_Vector_string_push(result, node->id);
    }
    return result;
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
        int __n_211 = btrc_Map_string_string_iterLen(parsed->values);
        for (int __i_210 = 0; (__i_210 < __n_211); (__i_210++)) {
            char* key = btrc_Map_string_string_iterGet(parsed->values, __i_210);
            char* value = btrc_Map_string_string_iterValueAt(parsed->values, __i_210);
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
    int __n_213 = btrc_Vector_string_iterLen(VmGraphParser_objectArray(text, "nodes"));
    for (int __i_212 = 0; (__i_212 < __n_213); (__i_212++)) {
        char* objectText = btrc_Vector_string_iterGet(VmGraphParser_objectArray(text, "nodes"), __i_212);
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
    btrc_Vector_string* __list_215 = btrc_Vector_string_new();
    (self->done = __list_215);
    btrc_Vector_string* __list_214 = btrc_Vector_string_new();
    (__list_214->__rc++);
    if (self->visiting != NULL) {
        if ((--self->visiting->__rc) <= 0) {
            btrc_Vector_string_free(self->visiting);
        }
    }
    btrc_Vector_string* __list_217 = btrc_Vector_string_new();
    (self->visiting = __list_217);
    btrc_Vector_string* __list_216 = btrc_Vector_string_new();
    (__list_216->__rc++);
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
        return self->sourceHashValue;
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
        (toolHash = ExecResult_trimmed(tool));
    }
    (self->sourceHashValue = __btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(ExecResult_trimmed(result), ":nixosctl=")), toolHash)));
    return self->sourceHashValue;
}

VmTestSpec* VmGraphRunner_specFor(VmGraphRunner* self, VmGraphNode* node) {
    VmTestSpec* spec = VmSpecParser_readFile(VmTestGraph_resolvedSpecPath(self->graph, node));
    VmTestSpec_setArg(spec, "graphName", self->graph->name);
    VmTestSpec_setArg(spec, "nodeId", node->id);
    VmTestSpec_setArg(spec, "workspaceRoot", VmTestGraph_resolvedWorkspaceRoot(self->graph));
    VmTestSpec_setArg(spec, "sourceHash", VmGraphRunner_sourceHash(self));
    VmGraphRunner_applyStructuralOverrides(self, spec, node->args);
    VmGraphRunner_applyStructuralOverrides(self, spec, self->args);
    int __n_219 = btrc_Map_string_string_iterLen(node->args);
    for (int __i_218 = 0; (__i_218 < __n_219); (__i_218++)) {
        char* key = btrc_Map_string_string_iterGet(node->args, __i_218);
        char* value = btrc_Map_string_string_iterValueAt(node->args, __i_218);
        VmTestSpec_setArg(spec, key, value);
    }
    int __n_221 = btrc_Map_string_string_iterLen(self->args);
    for (int __i_220 = 0; (__i_220 < __n_221); (__i_220++)) {
        char* key = btrc_Map_string_string_iterGet(self->args, __i_220);
        char* value = btrc_Map_string_string_iterValueAt(self->args, __i_220);
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
    int __n_223 = btrc_Vector_VmGraphNode_iterLen(self->graph->nodes);
    for (int __i_222 = 0; (__i_222 < __n_223); (__i_222++)) {
        VmGraphNode* node = btrc_Vector_VmGraphNode_iterGet(self->graph->nodes, __i_222);
        char* parents = (btrc_Vector_string_isEmpty(node->after) ? "root" : btrc_Vector_string_join(node->after, ","));
        Console_log(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(node->id, " <- ")), parents)), " :: ")), VmTestGraph_resolvedSpecPath(self->graph, node))));
    }
}

void VmGraphRunner_status(VmGraphRunner* self) {
    int __n_225 = btrc_Vector_VmGraphNode_iterLen(self->graph->nodes);
    for (int __i_224 = 0; (__i_224 < __n_225); (__i_224++)) {
        VmGraphNode* node = btrc_Vector_VmGraphNode_iterGet(self->graph->nodes, __i_224);
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
    int __n_227 = btrc_Vector_VmGraphNode_iterLen(self->graph->nodes);
    for (int __i_226 = 0; (__i_226 < __n_227); (__i_226++)) {
        VmGraphNode* node = btrc_Vector_VmGraphNode_iterGet(self->graph->nodes, __i_226);
        VmTestSpec* spec = VmSpecParser_readFile(VmTestGraph_resolvedSpecPath(self->graph, node));
        int __n_229 = btrc_Vector_VmOperation_iterLen(spec->operations);
        for (int __i_228 = 0; (__i_228 < __n_229); (__i_228++)) {
            VmOperation* op = btrc_Vector_VmOperation_iterGet(spec->operations, __i_228);
            if ((!__btrc_isEmpty(op->kind)) && (!btrc_Vector_string_contains(covered, op->kind))) {
                btrc_Vector_string_push(covered, op->kind);
            }
        }
    }
    btrc_Vector_string* missing = btrc_Vector_string_new();
    int __n_231 = btrc_Vector_string_iterLen(VmOperationCatalog_all());
    for (int __i_230 = 0; (__i_230 < __n_231); (__i_230++)) {
        char* kind = btrc_Vector_string_iterGet(VmOperationCatalog_all(), __i_230);
        if (!btrc_Vector_string_contains(covered, kind)) {
            btrc_Vector_string_push(missing, kind);
        }
    }
    if (!btrc_Vector_string_isEmpty(missing)) {
        Console_error(__btrc_str_track(__btrc_strcat("Missing e2e operation coverage: ", btrc_Vector_string_join(missing, ", "))));
        return 1;
    }
    Console_log(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat(__btrc_str_track(__btrc_strcat("E2E operation coverage: ", Strings_fromInt(covered->len))), "/")), Strings_fromInt(VmOperationCatalog_all()->len))));
    return 0;
}

bool VmGraphRunner_force(VmGraphRunner* self) {
    return (strcmp(btrc_Map_string_string_getOrDefault(self->args, "force", "false"), "true") == 0);
}

bool VmGraphRunner_ready(VmGraphRunner* self, VmTestSpec* spec) {
    if (!FileSystem_exists(VmTestSpec_stateHashFile(spec))) {
        return false;
    }
    char* saved = __btrc_str_track(__btrc_trim(Path_readAll(VmTestSpec_stateHashFile(spec))));
    return (strcmp(saved, spec->stateHash) == 0);
}

int VmGraphRunner_runNode(VmGraphRunner* self, char* id) {
    if (btrc_Vector_string_contains(self->done, id)) {
        return 0;
    }
    if (btrc_Vector_string_contains(self->visiting, id)) {
        NixosLog_fatal(__btrc_str_track(__btrc_strcat("Cycle in graph at ", id)));
    }
    btrc_Vector_string_push(self->visiting, id);
    VmGraphNode* node = VmTestGraph_node(self->graph, id);
    int __n_233 = btrc_Vector_string_iterLen(node->after);
    for (int __i_232 = 0; (__i_232 < __n_233); (__i_232++)) {
        char* parent = btrc_Vector_string_iterGet(node->after, __i_232);
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
        return 0;
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
    int __btrc_ret_234 = 0;
    if (runner != NULL) {
        if ((--runner->__rc) <= 0) {
            VmTestRunner_destroy(runner);
        }
    }
    return __btrc_ret_234;
    if (runner != NULL) {
        if ((--runner->__rc) <= 0) {
            VmTestRunner_destroy(runner);
        }
    }
}

int VmGraphRunner_run(VmGraphRunner* self, btrc_Vector_string* targets) {
    btrc_Vector_string* selected = (btrc_Vector_string_isEmpty(targets) ? VmTestGraph_defaultTargets(self->graph) : targets);
    int __n_236 = btrc_Vector_string_iterLen(selected);
    for (int __i_235 = 0; (__i_235 < __n_236); (__i_235++)) {
        char* id = btrc_Vector_string_iterGet(selected, __i_235);
        int result = VmGraphRunner_runNode(self, id);
        if (result != 0) {
            return result;
        }
    }
    return 0;
}

void E2eCli_init(E2eCli* self) {
    self->__rc = 1;
}

E2eCli* E2eCli_new(void) {
    E2eCli* self = ((E2eCli*)malloc(sizeof(E2eCli)));
    memset(self, 0, sizeof(E2eCli));
    E2eCli_init(self);
    return self;
}

char* E2eCli_tail(E2eCli* self, CliArgs* args, int startIndex) {
    btrc_Vector_string* parts = btrc_Vector_string_new();
    for (int i = startIndex; (i < CliArgs_count(args)); (i++)) {
        btrc_Vector_string_push(parts, CliArgs_get(args, i));
    }
    return btrc_Vector_string_join(parts, " ");
}

void E2eCli_applySpecArgs(E2eCli* self, VmTestSpec* spec, CliArgs* args, int startIndex) {
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

int E2eCli_runVm(E2eCli* self, CliArgs* args) {
    if (CliArgs_count(args) < 3) {
        NixosLog_fatal("Usage: nixosctl vm <spec.json> <status|hash|setup|up|boot-iso|boot-disk|bootstrap-ssh|wait-ssh|ssh|snapshot|restore|stop|reset-state|clean-state>");
    }
    VmTestSpec* spec = VmSpecParser_readFile(CliArgs_get(args, 1));
    E2eCli_applySpecArgs(self, spec, args, 3);
    QemuE2eHarness* vm = QemuE2eHarness_new(spec);
    char* action = CliArgs_get(args, 2);
    if (strcmp(action, "status") == 0) {
        QemuE2eHarness_printStatus(vm);
        int __btrc_ret_237 = 0;
        if (vm != NULL) {
            if ((--vm->__rc) <= 0) {
                QemuE2eHarness_destroy(vm);
            }
        }
        return __btrc_ret_237;
    }
    if (strcmp(action, "hash") == 0) {
        Console_log(spec->stateHash);
        int __btrc_ret_238 = 0;
        if (vm != NULL) {
            if ((--vm->__rc) <= 0) {
                QemuE2eHarness_destroy(vm);
            }
        }
        return __btrc_ret_238;
    }
    if (strcmp(action, "setup") == 0) {
        QemuE2eHarness_setup(vm);
        int __btrc_ret_239 = 0;
        if (vm != NULL) {
            if ((--vm->__rc) <= 0) {
                QemuE2eHarness_destroy(vm);
            }
        }
        return __btrc_ret_239;
    }
    if (strcmp(action, "download-iso") == 0) {
        QemuE2eHarness_downloadIso(vm);
        int __btrc_ret_240 = 0;
        if (vm != NULL) {
            if ((--vm->__rc) <= 0) {
                QemuE2eHarness_destroy(vm);
            }
        }
        return __btrc_ret_240;
    }
    if (strcmp(action, "create-key") == 0) {
        QemuE2eHarness_createSshKey(vm);
        int __btrc_ret_241 = 0;
        if (vm != NULL) {
            if ((--vm->__rc) <= 0) {
                QemuE2eHarness_destroy(vm);
            }
        }
        return __btrc_ret_241;
    }
    if (strcmp(action, "create-disk") == 0) {
        QemuE2eHarness_createDisk(vm);
        int __btrc_ret_242 = 0;
        if (vm != NULL) {
            if ((--vm->__rc) <= 0) {
                QemuE2eHarness_destroy(vm);
            }
        }
        return __btrc_ret_242;
    }
    if (strcmp(action, "up") == 0) {
        QemuE2eHarness_upFromIso(vm);
        int __btrc_ret_243 = 0;
        if (vm != NULL) {
            if ((--vm->__rc) <= 0) {
                QemuE2eHarness_destroy(vm);
            }
        }
        return __btrc_ret_243;
    }
    if (strcmp(action, "boot-iso") == 0) {
        QemuE2eHarness_start(vm, true);
        int __btrc_ret_244 = 0;
        if (vm != NULL) {
            if ((--vm->__rc) <= 0) {
                QemuE2eHarness_destroy(vm);
            }
        }
        return __btrc_ret_244;
    }
    if (strcmp(action, "boot-disk") == 0) {
        QemuE2eHarness_start(vm, false);
        int __btrc_ret_245 = 0;
        if (vm != NULL) {
            if ((--vm->__rc) <= 0) {
                QemuE2eHarness_destroy(vm);
            }
        }
        return __btrc_ret_245;
    }
    if (strcmp(action, "bootstrap-ssh") == 0) {
        QemuE2eHarness_bootstrapSsh(vm);
        int __btrc_ret_246 = 0;
        if (vm != NULL) {
            if ((--vm->__rc) <= 0) {
                QemuE2eHarness_destroy(vm);
            }
        }
        return __btrc_ret_246;
    }
    if (strcmp(action, "wait-ssh") == 0) {
        int timeout = Strings_toInt(CliArgs_valueAfter(args, "--timeout", "180"));
        if (!QemuE2eHarness_waitForSsh(vm, timeout)) {
            int __btrc_ret_247 = 1;
            if (vm != NULL) {
                if ((--vm->__rc) <= 0) {
                    QemuE2eHarness_destroy(vm);
                }
            }
            return __btrc_ret_247;
        }
        int __btrc_ret_248 = 0;
        if (vm != NULL) {
            if ((--vm->__rc) <= 0) {
                QemuE2eHarness_destroy(vm);
            }
        }
        return __btrc_ret_248;
    }
    if (strcmp(action, "ssh") == 0) {
        char* command = E2eCli_tail(self, args, 3);
        if (__btrc_isEmpty(command)) {
            NixosLog_fatal("Usage: nixosctl vm <spec.json> ssh <command>");
        }
        ExecResult* result = QemuE2eHarness_ssh(vm, command, false);
        Console_log(ExecResult_trimmed(result));
        int __btrc_ret_249 = result->code;
        if (vm != NULL) {
            if ((--vm->__rc) <= 0) {
                QemuE2eHarness_destroy(vm);
            }
        }
        return __btrc_ret_249;
    }
    if (strcmp(action, "host") == 0) {
        char* command = E2eCli_tail(self, args, 3);
        if (__btrc_isEmpty(command)) {
            NixosLog_fatal("Usage: nixosctl vm <spec.json> host <command>");
        }
        ExecResult* result = QemuE2eHarness_host(vm, command, false);
        Console_log(ExecResult_trimmed(result));
        int __btrc_ret_250 = result->code;
        if (vm != NULL) {
            if ((--vm->__rc) <= 0) {
                QemuE2eHarness_destroy(vm);
            }
        }
        return __btrc_ret_250;
    }
    if (strcmp(action, "copy-workspace") == 0) {
        QemuE2eHarness_copyWorkspace(vm, CliArgs_valueAfter(args, "--local", ".."), CliArgs_valueAfter(args, "--remote", "/etc/nixos"));
        int __btrc_ret_251 = 0;
        if (vm != NULL) {
            if ((--vm->__rc) <= 0) {
                QemuE2eHarness_destroy(vm);
            }
        }
        return __btrc_ret_251;
    }
    if (strcmp(action, "configure-vm-host") == 0) {
        QemuE2eHarness_configureVmHost(vm);
        int __btrc_ret_252 = 0;
        if (vm != NULL) {
            if ((--vm->__rc) <= 0) {
                QemuE2eHarness_destroy(vm);
            }
        }
        return __btrc_ret_252;
    }
    if (strcmp(action, "install-nixos") == 0) {
        QemuE2eHarness_installNixosGuest(vm);
        int __btrc_ret_253 = 0;
        if (vm != NULL) {
            if ((--vm->__rc) <= 0) {
                QemuE2eHarness_destroy(vm);
            }
        }
        return __btrc_ret_253;
    }
    if (strcmp(action, "reboot-disk") == 0) {
        QemuE2eHarness_rebootDisk(vm);
        int __btrc_ret_254 = 0;
        if (vm != NULL) {
            if ((--vm->__rc) <= 0) {
                QemuE2eHarness_destroy(vm);
            }
        }
        return __btrc_ret_254;
    }
    if (strcmp(action, "snapshot") == 0) {
        QemuE2eHarness_snapshot(vm, CliArgs_valueAfter(args, "--name", "manual"));
        int __btrc_ret_255 = 0;
        if (vm != NULL) {
            if ((--vm->__rc) <= 0) {
                QemuE2eHarness_destroy(vm);
            }
        }
        return __btrc_ret_255;
    }
    if (strcmp(action, "restore") == 0) {
        QemuE2eHarness_restore(vm, CliArgs_valueAfter(args, "--name", "manual"));
        int __btrc_ret_256 = 0;
        if (vm != NULL) {
            if ((--vm->__rc) <= 0) {
                QemuE2eHarness_destroy(vm);
            }
        }
        return __btrc_ret_256;
    }
    if (strcmp(action, "record-state") == 0) {
        QemuE2eHarness_recordState(vm);
        int __btrc_ret_257 = 0;
        if (vm != NULL) {
            if ((--vm->__rc) <= 0) {
                QemuE2eHarness_destroy(vm);
            }
        }
        return __btrc_ret_257;
    }
    if (strcmp(action, "stop") == 0) {
        QemuE2eHarness_stop(vm);
        int __btrc_ret_258 = 0;
        if (vm != NULL) {
            if ((--vm->__rc) <= 0) {
                QemuE2eHarness_destroy(vm);
            }
        }
        return __btrc_ret_258;
    }
    if (strcmp(action, "reset-state") == 0) {
        QemuE2eHarness_resetState(vm);
        int __btrc_ret_259 = 0;
        if (vm != NULL) {
            if ((--vm->__rc) <= 0) {
                QemuE2eHarness_destroy(vm);
            }
        }
        return __btrc_ret_259;
    }
    if (strcmp(action, "clean-state") == 0) {
        QemuE2eHarness_resetState(vm);
        QemuE2eHarness_cleanStateRecord(vm);
        int __btrc_ret_260 = 0;
        if (vm != NULL) {
            if ((--vm->__rc) <= 0) {
                QemuE2eHarness_destroy(vm);
            }
        }
        return __btrc_ret_260;
    }
    NixosLog_fatal(__btrc_str_track(__btrc_strcat("Unknown vm action: ", action)));
    int __btrc_ret_261 = 1;
    if (vm != NULL) {
        if ((--vm->__rc) <= 0) {
            QemuE2eHarness_destroy(vm);
        }
    }
    return __btrc_ret_261;
    if (vm != NULL) {
        if ((--vm->__rc) <= 0) {
            QemuE2eHarness_destroy(vm);
        }
    }
}

btrc_Map_string_string* E2eCli_graphArgs(E2eCli* self, CliArgs* args, int startIndex) {
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

btrc_Vector_string* E2eCli_graphTargets(E2eCli* self, CliArgs* args, int startIndex) {
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

int E2eCli_runGraph(E2eCli* self, CliArgs* args) {
    if (CliArgs_count(args) < 3) {
        NixosLog_fatal("Usage: nixosctl graph <graph.json> <list|status|coverage|run> [node ...] [--arg key=value]");
    }
    VmTestGraph* graph = VmGraphParser_readFile(CliArgs_get(args, 1));
    char* action = CliArgs_get(args, 2);
    btrc_Map_string_string* overrides = E2eCli_graphArgs(self, args, 3);
    VmGraphRunner* runner = VmGraphRunner_new(graph, overrides);
    if (strcmp(action, "list") == 0) {
        VmGraphRunner_list(runner);
        int __btrc_ret_262 = 0;
        if (runner != NULL) {
            if ((--runner->__rc) <= 0) {
                VmGraphRunner_destroy(runner);
            }
        }
        return __btrc_ret_262;
    }
    if (strcmp(action, "status") == 0) {
        VmGraphRunner_status(runner);
        int __btrc_ret_263 = 0;
        if (runner != NULL) {
            if ((--runner->__rc) <= 0) {
                VmGraphRunner_destroy(runner);
            }
        }
        return __btrc_ret_263;
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
        int code = VmGraphRunner_run(runner, E2eCli_graphTargets(self, args, 3));
        if (runner != NULL) {
            if ((--runner->__rc) <= 0) {
                VmGraphRunner_destroy(runner);
            }
        }
        return code;
    }
    NixosLog_fatal("Usage: nixosctl graph <graph.json> <list|status|coverage|run> [node ...] [--arg key=value]");
    int __btrc_ret_264 = 1;
    if (runner != NULL) {
        if ((--runner->__rc) <= 0) {
            VmGraphRunner_destroy(runner);
        }
    }
    return __btrc_ret_264;
    if (runner != NULL) {
        if ((--runner->__rc) <= 0) {
            VmGraphRunner_destroy(runner);
        }
    }
}

int E2eCli_runE2e(E2eCli* self, CliArgs* args) {
    if (CliArgs_count(args) < 2) {
        NixosLog_fatal("Usage: nixosctl e2e <spec.json>");
    }
    VmTestSpec* spec = VmSpecParser_readFile(CliArgs_get(args, 1));
    E2eCli_applySpecArgs(self, spec, args, 2);
    VmTestRunner* runner = VmTestRunner_new(spec);
    int __btrc_ret_265 = VmTestRunner_run(runner);
    if (runner != NULL) {
        if ((--runner->__rc) <= 0) {
            VmTestRunner_destroy(runner);
        }
    }
    return __btrc_ret_265;
    if (runner != NULL) {
        if ((--runner->__rc) <= 0) {
            VmTestRunner_destroy(runner);
        }
    }
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
    return Environment_get(name, fallback);
}

void NixosCtl_usage(NixosCtl* self) {
    Console_log("Usage: nixosctl <eval|update|upgrade|install|snapshot|diff|fix-permissions|change-password|secure-boot|tpm2|displays|audio|caffeine|system|vm|e2e|graph> [args]");
}

bool NixosCtl_needsRoot(NixosCtl* self, char* command) {
    if (strcmp(NixosCtl_env("NIXOSCTL_ASSUME_ROOT_FOR_TESTS", "false"), "true") == 0) {
        return false;
    }
    return (((((((((strcmp(command, "update") == 0) || (strcmp(command, "upgrade") == 0)) || (strcmp(command, "install") == 0)) || (strcmp(command, "snapshot") == 0)) || (strcmp(command, "diff") == 0)) || (strcmp(command, "fix-permissions") == 0)) || (strcmp(command, "change-password") == 0)) || (strcmp(command, "secure-boot") == 0)) || (strcmp(command, "tpm2") == 0));
}

int NixosCtl_sudoSelf(NixosCtl* self, CliArgs* args) {
    Command* sudo = Command_new("sudo");
    Command_arg(sudo, args->program);
    int __n_267 = btrc_Vector_string_iterLen(args->values);
    for (int __i_266 = 0; (__i_266 < __n_267); (__i_266++)) {
        char* value = btrc_Vector_string_iterGet(args->values, __i_266);
        Command_arg(sudo, value);
    }
    Command_capture(sudo, false);
    ExecResult* result = UnixShell_runCommand(UnixShell_new(), sudo);
    int __btrc_ret_268 = result->code;
    if (sudo != NULL) {
        if ((--sudo->__rc) <= 0) {
            Command_destroy(sudo);
        }
    }
    return __btrc_ret_268;
    if (sudo != NULL) {
        if ((--sudo->__rc) <= 0) {
            Command_destroy(sudo);
        }
    }
}

int NixosCtl_run(NixosCtl* self, CliArgs* args) {
    if (CliArgs_count(args) == 0) {
        NixosCtl_usage(self);
        return 1;
    }
    char* cmd = CliArgs_command(args);
    if (NixosCtl_needsRoot(self, cmd) && (!Platform_isRoot())) {
        return NixosCtl_sudoSelf(self, args);
    }
    if (strcmp(cmd, "eval") == 0) {
        if (CliArgs_count(args) < 2) {
            NixosLog_fatal("Usage: nixosctl eval <attribute>");
        }
        Console_log(NixosConfig_evalRaw(self->config, CliArgs_get(args, 1)));
        return 0;
    }
    if ((strcmp(cmd, "update") == 0) || (strcmp(cmd, "upgrade") == 0)) {
        RebuildOptions* options = RebuildOptions_new();
        (options->rebuildFileSystem = CliArgs_has(args, "--rebuild-filesystem"));
        (options->reboot = CliArgs_has(args, "--reboot"));
        (options->clean = ((CliArgs_has(args, "--clean") || CliArgs_has(args, "--upgrade")) || (strcmp(cmd, "upgrade") == 0)));
        (options->upgrade = (CliArgs_has(args, "--upgrade") || (strcmp(cmd, "upgrade") == 0)));
        NixosRebuilder_update(NixosRebuilder_new(self->config), options);
        int __btrc_ret_269 = 0;
        if (options != NULL) {
            if ((--options->__rc) <= 0) {
                RebuildOptions_destroy(options);
            }
        }
        return __btrc_ret_269;
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
                int __btrc_ret_270 = installed->code;
                if (installer != NULL) {
                    if ((--installer->__rc) <= 0) {
                        Installer_destroy(installer);
                    }
                }
                return __btrc_ret_270;
            }
        } else if (Interactive_confirm(installer->interactive, "Permission NixOS?")) {
            Installer_permissionNixos(installer);
        }
        Interactive_askToReboot(installer->interactive);
        int __btrc_ret_271 = 0;
        if (installer != NULL) {
            if ((--installer->__rc) <= 0) {
                Installer_destroy(installer);
            }
        }
        return __btrc_ret_271;
        if (installer != NULL) {
            if ((--installer->__rc) <= 0) {
                Installer_destroy(installer);
            }
        }
    }
    if (strcmp(cmd, "snapshot") == 0) {
        SnapshotManager_createInitialSnapshots(SnapshotManager_new(self->config));
        return 0;
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
        int __btrc_ret_272 = 0;
        if (options != NULL) {
            if ((--options->__rc) <= 0) {
                DiffOptions_destroy(options);
            }
        }
        return __btrc_ret_272;
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
        return 0;
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
        int __btrc_ret_273 = 0;
        if (interactive != NULL) {
            if ((--interactive->__rc) <= 0) {
                Interactive_destroy(interactive);
            }
        }
        return __btrc_ret_273;
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
            int __btrc_ret_274 = 0;
            if (manager != NULL) {
                if ((--manager->__rc) <= 0) {
                    SecureBootManager_destroy(manager);
                }
            }
            return __btrc_ret_274;
        }
        if (strcmp(action, "disable") == 0) {
            SecureBootManager_disable(manager);
            int __btrc_ret_275 = 0;
            if (manager != NULL) {
                if ((--manager->__rc) <= 0) {
                    SecureBootManager_destroy(manager);
                }
            }
            return __btrc_ret_275;
        }
        if (strcmp(action, "status") == 0) {
            SecureBootManager_status(manager);
            int __btrc_ret_276 = 0;
            if (manager != NULL) {
                if ((--manager->__rc) <= 0) {
                    SecureBootManager_destroy(manager);
                }
            }
            return __btrc_ret_276;
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
            int __btrc_ret_277 = 0;
            if (tpm != NULL) {
                if ((--tpm->__rc) <= 0) {
                    Tpm2Manager_destroy(tpm);
                }
            }
            return __btrc_ret_277;
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
            int __btrc_ret_278 = 0;
            if (tpm != NULL) {
                if ((--tpm->__rc) <= 0) {
                    Tpm2Manager_destroy(tpm);
                }
            }
            return __btrc_ret_278;
        }
        if (strcmp(action, "disable") == 0) {
            if (!Tpm2Manager_wipe(tpm)) {
                NixosLog_fatal("TPM2 wipe failed");
            }
            int __btrc_ret_279 = 0;
            if (tpm != NULL) {
                if ((--tpm->__rc) <= 0) {
                    Tpm2Manager_destroy(tpm);
                }
            }
            return __btrc_ret_279;
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
            int __btrc_ret_280 = 0;
            if (displays != NULL) {
                if ((--displays->__rc) <= 0) {
                    DisplayManager_destroy(displays);
                }
            }
            return __btrc_ret_280;
        }
        if (strcmp(action, "layout") == 0) {
            DisplayManager_layout(displays);
            int __btrc_ret_281 = 0;
            if (displays != NULL) {
                if ((--displays->__rc) <= 0) {
                    DisplayManager_destroy(displays);
                }
            }
            return __btrc_ret_281;
        }
        if (CliArgs_count(args) < 3) {
            NixosLog_fatal("Missing display argument");
        }
        if (strcmp(action, "enable") == 0) {
            DisplayManager_enable(displays, CliArgs_get(args, 2));
            int __btrc_ret_282 = 0;
            if (displays != NULL) {
                if ((--displays->__rc) <= 0) {
                    DisplayManager_destroy(displays);
                }
            }
            return __btrc_ret_282;
        }
        if (strcmp(action, "disable") == 0) {
            DisplayManager_disable(displays, CliArgs_get(args, 2));
            int __btrc_ret_283 = 0;
            if (displays != NULL) {
                if ((--displays->__rc) <= 0) {
                    DisplayManager_destroy(displays);
                }
            }
            return __btrc_ret_283;
        }
        if (strcmp(action, "primary") == 0) {
            DisplayManager_primary(displays, CliArgs_get(args, 2));
            int __btrc_ret_284 = 0;
            if (displays != NULL) {
                if ((--displays->__rc) <= 0) {
                    DisplayManager_destroy(displays);
                }
            }
            return __btrc_ret_284;
        }
        if (strcmp(action, "dpms") == 0) {
            DisplayManager_dpms(displays, CliArgs_get(args, 2));
            int __btrc_ret_285 = 0;
            if (displays != NULL) {
                if ((--displays->__rc) <= 0) {
                    DisplayManager_destroy(displays);
                }
            }
            return __btrc_ret_285;
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
            int __btrc_ret_286 = 0;
            if (audio != NULL) {
                if ((--audio->__rc) <= 0) {
                    AudioManager_destroy(audio);
                }
            }
            return __btrc_ret_286;
        }
        if (strcmp(action, "current") == 0) {
            Console_log(AudioManager_current(audio));
            int __btrc_ret_287 = 0;
            if (audio != NULL) {
                if ((--audio->__rc) <= 0) {
                    AudioManager_destroy(audio);
                }
            }
            return __btrc_ret_287;
        }
        if (strcmp(action, "set") == 0) {
            if (CliArgs_count(args) < 3) {
                NixosLog_fatal("Usage: nixosctl audio set <sink>");
            }
            AudioManager_set(audio, CliArgs_get(args, 2));
            int __btrc_ret_288 = 0;
            if (audio != NULL) {
                if ((--audio->__rc) <= 0) {
                    AudioManager_destroy(audio);
                }
            }
            return __btrc_ret_288;
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
            int __btrc_ret_289 = (CaffeineManager_enabled(caffeine) ? 0 : 1);
            if (caffeine != NULL) {
                if ((--caffeine->__rc) <= 0) {
                    CaffeineManager_destroy(caffeine);
                }
            }
            return __btrc_ret_289;
        }
        if (strcmp(action, "enable") == 0) {
            CaffeineManager_enable(caffeine);
            int __btrc_ret_290 = 0;
            if (caffeine != NULL) {
                if ((--caffeine->__rc) <= 0) {
                    CaffeineManager_destroy(caffeine);
                }
            }
            return __btrc_ret_290;
        }
        if (strcmp(action, "disable") == 0) {
            CaffeineManager_disable(caffeine);
            int __btrc_ret_291 = 0;
            if (caffeine != NULL) {
                if ((--caffeine->__rc) <= 0) {
                    CaffeineManager_destroy(caffeine);
                }
            }
            return __btrc_ret_291;
        }
        if (strcmp(action, "toggle") == 0) {
            CaffeineManager_toggle(caffeine);
            int __btrc_ret_292 = 0;
            if (caffeine != NULL) {
                if ((--caffeine->__rc) <= 0) {
                    CaffeineManager_destroy(caffeine);
                }
            }
            return __btrc_ret_292;
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
            int __btrc_ret_293 = 0;
            if (system != NULL) {
                if ((--system->__rc) <= 0) {
                    SystemUi_destroy(system);
                }
            }
            return __btrc_ret_293;
        }
        if (strcmp(action, "upgrade") == 0) {
            SystemUi_upgrade(system);
            int __btrc_ret_294 = 0;
            if (system != NULL) {
                if ((--system->__rc) <= 0) {
                    SystemUi_destroy(system);
                }
            }
            return __btrc_ret_294;
        }
        if (system != NULL) {
            if ((--system->__rc) <= 0) {
                SystemUi_destroy(system);
            }
        }
    }
    if (strcmp(cmd, "vm") == 0) {
        return E2eCli_runVm(E2eCli_new(), args);
    }
    if (strcmp(cmd, "e2e") == 0) {
        return E2eCli_runE2e(E2eCli_new(), args);
    }
    if (strcmp(cmd, "graph") == 0) {
        return E2eCli_runGraph(E2eCli_new(), args);
    }
    NixosCtl_usage(self);
    return 1;
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

