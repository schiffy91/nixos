// BTRFS immutable boot v2.
//
// V2 is an idempotent converger. Boot and update run the same pipeline:
// keep directories become persistent Btrfs subvolumes, keep files are saved as
// tiny payloads, READY is derived from CLEAN, NEXT is published as live, and
// old live subvolumes are moved to trash for bounded cleanup.
//
// Usage:
//   immutabilityv2 plan <persist_root> <spec_file>
//   immutabilityv2 [--dry-run] [--assume-mounted] <device> <snap_name> <clean_name> <mode> <persist_root> <spec_file> [volume=mount ...]
//
// Spec file format (tab-separated):
//   <volume_name> <mount_point> <absolute_keep_path> <auto|directory|file>
use std::{
    collections::HashSet,
    env, fs, io,
    os::unix::fs::MetadataExt,
    path::{Path, PathBuf},
    process::{self, Command},
    sync::Mutex,
    time::Instant,
};

const DEFAULT_MOUNT_PATH: &str = "/mnt";
const META_DIR: &str = ".immutabilityv2";
const FILES_DIR: &str = "files";
const STAGING: &str = "@staging";
const TRASH: &str = "@trash";
const READY: &str = "READY";
const NEXT: &str = "NEXT";
const CAPTURE: &str = "CAPTURE";

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
enum Kind {
    Auto,
    Directory,
    File,
}

impl Kind {
    fn parse(raw: &str) -> Result<Self, String> {
        match raw.trim() {
            "auto" => Ok(Self::Auto),
            "directory" | "dir" => Ok(Self::Directory),
            "file" => Ok(Self::File),
            other => Err(format!("unknown keep kind: {other}")),
        }
    }

    fn as_str(self) -> &'static str {
        match self {
            Self::Auto => "auto",
            Self::Directory => "directory",
            Self::File => "file",
        }
    }
}

#[derive(Clone, Debug)]
struct KeepSpec {
    volume: String,
    abs_path: String,
    rel_path: String,
    kind: Kind,
    trailing_slash: bool,
}

#[derive(Clone, Debug)]
struct KeepPlan {
    spec: KeepSpec,
    kind: Kind,
    subvol: String,
    files_to_persist: String,
}

#[derive(Clone, Debug)]
struct Volume {
    name: String,
}

#[derive(Debug)]
struct Cli {
    dry_run: bool,
    assume_mounted: bool,
    offset: usize,
}

#[derive(Debug)]
struct SubvolumeInfo {
    uuid: Option<String>,
    parent_uuid: Option<String>,
}

struct Runner {
    start: Instant,
    mount_path: String,
    dry_run: bool,
    planned_dirs: Mutex<HashSet<String>>,
    planned_files: Mutex<HashSet<String>>,
    planned_snapshots: Mutex<Vec<(String, String)>>,
}

impl Runner {
    fn new(dry_run: bool) -> Self {
        let mount_path =
            env::var("IMMUTABILITY_MOUNT_PATH").unwrap_or_else(|_| DEFAULT_MOUNT_PATH.to_string());
        Self {
            start: Instant::now(),
            mount_path,
            dry_run,
            planned_dirs: Mutex::new(HashSet::new()),
            planned_files: Mutex::new(HashSet::new()),
            planned_snapshots: Mutex::new(Vec::new()),
        }
    }

    fn elapsed(&self) -> String {
        format!("[{:.2}s]", self.start.elapsed().as_secs_f64())
    }

    fn run(&self, args: &[&str]) -> bool {
        println!("{} {}", self.elapsed(), args.join(" "));
        let output = Command::new(args[0])
            .args(&args[1..])
            .output()
            .expect("exec failed");
        for line in String::from_utf8_lossy(&output.stdout)
            .lines()
            .filter(|line| !line.is_empty())
        {
            println!("{}   {line}", self.elapsed());
        }
        for line in String::from_utf8_lossy(&output.stderr)
            .lines()
            .filter(|line| !line.is_empty())
        {
            eprintln!("{}   {line}", self.elapsed());
        }
        output.status.success()
    }

    fn output(&self, args: &[&str]) -> io::Result<std::process::Output> {
        println!("{} {}", self.elapsed(), args.join(" "));
        Command::new(args[0]).args(&args[1..]).output()
    }

    fn require(&self, args: &[&str]) {
        if !self.run(args) {
            self.die(&format!("Failed: {}", args.join(" ")));
        }
    }

    fn effect(&self, args: &[&str]) -> bool {
        if self.dry_run {
            println!("{} DRY {}", self.elapsed(), args.join(" "));
            true
        } else {
            self.run(args)
        }
    }

    fn require_effect(&self, args: &[&str]) {
        if !self.effect(args) {
            self.die(&format!("Failed: {}", args.join(" ")));
        }
    }

    fn create_dir_all(&self, path: &Path) {
        if self.dry_run {
            println!("{} DRY mkdir -p {}", self.elapsed(), path.display());
            return;
        }
        fs::create_dir_all(path)
            .unwrap_or_else(|e| self.die(&format!("cannot create {}: {e}", path.display())));
    }

    fn remove_file(&self, path: &Path) {
        if self.dry_run {
            println!("{} DRY rm -f {}", self.elapsed(), path.display());
            self.unmark_planned_file(path);
            return;
        }
        fs::remove_file(path).ok();
    }

    fn remove_dir_all(&self, path: &Path) {
        if self.dry_run {
            println!("{} DRY rm -rf {}", self.elapsed(), path.display());
            self.unmark_planned_dir(&path.display().to_string());
            return;
        }
        fs::remove_dir_all(path).ok();
    }

    fn rename(&self, source: &str, dest: &str) -> bool {
        if self.dry_run {
            println!("{} DRY mv {source} {dest}", self.elapsed());
            self.unmark_planned_dir(dest);
            self.unmark_planned_dir(source);
            self.mark_planned_dir(dest);
            return true;
        }
        match fs::rename(source, dest) {
            Ok(()) => true,
            Err(e) => {
                eprintln!("{} WRN rename {source} -> {dest}: {e}", self.elapsed());
                false
            }
        }
    }

    fn die(&self, message: &str) -> ! {
        eprintln!("{} ERR {message}", self.elapsed());
        panic!("immutabilityv2 fatal: {message}");
    }

    fn top(&self, rel: &str) -> String {
        format!("{}/{}", self.mount_path, rel.trim_start_matches('/'))
    }

    fn rel(&self, path: &str) -> String {
        let prefix = format!("{}/", self.mount_path.trim_end_matches('/'));
        path.strip_prefix(&prefix).unwrap_or(path).to_string()
    }

    fn dir_exists(&self, path: &str) -> bool {
        Path::new(path).is_dir()
            || self
                .planned_dirs
                .lock()
                .map(|paths| paths.contains(path))
                .unwrap_or(false)
    }

    fn file_exists(&self, path: &str) -> bool {
        fs::symlink_metadata(path).is_ok()
            || self
                .planned_files
                .lock()
                .map(|paths| paths.contains(path))
                .unwrap_or(false)
    }

    fn path_exists(&self, path: &Path) -> bool {
        if path.exists() || fs::symlink_metadata(path).is_ok() {
            return true;
        }
        if !self.dry_run {
            return false;
        }
        self.resolve_virtual_path(path)
            .map(|mapped| mapped.exists() || fs::symlink_metadata(mapped).is_ok())
            .unwrap_or(false)
    }

    fn resolve_virtual_path(&self, path: &Path) -> Option<PathBuf> {
        let raw = path.display().to_string();
        let snapshots = self.planned_snapshots.lock().ok()?;
        for (source, destination) in snapshots.iter().rev() {
            if raw == *destination {
                return Some(PathBuf::from(source));
            }
            let prefix = format!("{destination}/");
            if let Some(rest) = raw.strip_prefix(&prefix) {
                return Some(PathBuf::from(format!("{source}/{rest}")));
            }
        }
        None
    }

    fn mark_planned_dir(&self, path: &str) {
        if self.dry_run {
            if let Ok(mut paths) = self.planned_dirs.lock() {
                paths.insert(path.to_string());
            }
        }
    }

    fn unmark_planned_dir(&self, path: &str) {
        if self.dry_run {
            if let Ok(mut paths) = self.planned_dirs.lock() {
                paths.retain(|planned| planned != path && !is_ancestor(path, planned));
            }
            if let Ok(mut paths) = self.planned_files.lock() {
                paths.retain(|planned| planned != path && !is_ancestor(path, planned));
            }
        }
    }

    fn mark_planned_file(&self, path: &str) {
        if self.dry_run {
            if let Ok(mut paths) = self.planned_files.lock() {
                paths.insert(path.to_string());
            }
        }
    }

    fn mark_planned_snapshot(&self, source: &str, destination: &str) {
        if self.dry_run {
            if let Ok(mut snapshots) = self.planned_snapshots.lock() {
                snapshots.push((source.to_string(), destination.to_string()));
            }
        }
    }

    fn unmark_planned_file(&self, path: &Path) {
        if self.dry_run {
            if let Ok(mut paths) = self.planned_files.lock() {
                paths.remove(&path.display().to_string());
            }
        }
    }
}

struct Unmounter<'a>(&'a Runner);
impl Drop for Unmounter<'_> {
    fn drop(&mut self) {
        self.0.run(&["umount", "-R", &self.0.mount_path]);
    }
}

fn truthy_env(name: &str) -> bool {
    env::var(name)
        .map(|value| matches!(value.as_str(), "1" | "true" | "yes" | "on"))
        .unwrap_or(false)
}

fn parse_cli_flags(args: &[String]) -> Cli {
    let mut dry_run = truthy_env("IMMUTABILITY_DRY_RUN");
    let mut assume_mounted = truthy_env("IMMUTABILITY_ASSUME_MOUNTED");
    let mut offset = 1;
    while let Some(arg) = args.get(offset) {
        match arg.as_str() {
            "--dry-run" => dry_run = true,
            "--assume-mounted" => assume_mounted = true,
            _ => break,
        }
        offset += 1;
    }
    Cli {
        dry_run,
        assume_mounted,
        offset,
    }
}

fn validate_relative_path(path: &str, label: &str) -> Result<(), String> {
    if path.contains('\0') || path.contains('\t') {
        return Err(format!("{label} cannot contain NUL or tab characters"));
    }
    for part in path.split('/') {
        if part.is_empty() || part == "." || part == ".." {
            return Err(format!("{label} contains unsafe path component: {path}"));
        }
    }
    Ok(())
}

fn normalize_abs(raw: &str) -> Result<(String, bool), String> {
    let trimmed = raw.trim();
    if !trimmed.starts_with('/') {
        return Err(format!("keep path must be absolute: {trimmed}"));
    }
    let trailing_slash = trimmed.len() > 1 && trimmed.ends_with('/');
    let normalized = if trailing_slash {
        trimmed.trim_end_matches('/').to_string()
    } else {
        trimmed.to_string()
    };
    if normalized.is_empty() || normalized == "/" {
        return Err("keep path cannot be filesystem root".to_string());
    }
    validate_relative_path(normalized.strip_prefix('/').unwrap(), "keep path")?;
    Ok((normalized, trailing_slash))
}

fn normalize_mount_point(raw: &str) -> Result<String, String> {
    let trimmed = raw.trim();
    if trimmed.is_empty() {
        return Ok("/".to_string());
    }
    if !trimmed.starts_with('/') {
        return Err(format!("mount point must be absolute: {trimmed}"));
    }
    let normalized = if trimmed == "/" {
        "/".to_string()
    } else {
        trimmed.trim_end_matches('/').to_string()
    };
    if normalized != "/" {
        validate_relative_path(normalized.strip_prefix('/').unwrap(), "mount point")?;
    }
    Ok(normalized)
}

fn normalize_subvolume_path(raw: &str, label: &str) -> Result<String, String> {
    let trimmed = raw.trim().trim_matches('/');
    if trimmed.is_empty() {
        return Err(format!("{label} cannot be empty"));
    }
    validate_relative_path(trimmed, label)?;
    Ok(trimmed.to_string())
}

fn normalize_path_component(raw: &str, label: &str) -> Result<String, String> {
    let trimmed = raw.trim();
    if trimmed.is_empty() || trimmed.contains('/') || trimmed == "." || trimmed == ".." {
        return Err(format!("{label} must be a single path component"));
    }
    if trimmed.contains('\0') || trimmed.contains('\t') {
        return Err(format!("{label} cannot contain NUL or tab characters"));
    }
    Ok(trimmed.to_string())
}

fn relative_to_mount(abs_path: &str, mount_point: &str) -> Result<String, String> {
    if mount_point == "/" {
        return Ok(abs_path.trim_start_matches('/').to_string());
    }
    if abs_path == mount_point {
        return Err(format!(
            "keep path cannot be the reset volume mount point: {abs_path}"
        ));
    }
    let prefix = format!("{}/", mount_point.trim_end_matches('/'));
    if let Some(rel) = abs_path.strip_prefix(&prefix) {
        return Ok(rel.to_string());
    }
    Err(format!("{abs_path} is not under mount point {mount_point}"))
}

fn parse_spec_file(path: &str) -> Result<Vec<KeepSpec>, String> {
    let content =
        fs::read_to_string(path).map_err(|e| format!("cannot read spec file {path}: {e}"))?;
    let mut specs = Vec::new();
    for (index, line) in content.lines().enumerate() {
        let trimmed = line.trim();
        if trimmed.is_empty() || trimmed.starts_with('#') {
            continue;
        }
        let parts: Vec<&str> = line.split('\t').collect();
        if parts.len() != 4 {
            return Err(format!(
                "{}:{} expected 4 tab-separated fields",
                path,
                index + 1
            ));
        }
        let volume = normalize_subvolume_path(parts[0], "volume")?;
        let mount_point = normalize_mount_point(parts[1])?;
        let (abs_path, trailing_slash) = normalize_abs(parts[2])?;
        let kind = Kind::parse(parts[3])?;
        let rel_path = relative_to_mount(&abs_path, &mount_point)?;
        specs.push(KeepSpec {
            volume,
            abs_path,
            rel_path,
            kind,
            trailing_slash,
        });
    }
    Ok(specs)
}

fn sanitize(input: &str) -> String {
    let mut out = String::new();
    for ch in input.chars() {
        if ch.is_ascii_alphanumeric() || matches!(ch, '.' | '_' | '-') {
            out.push(ch);
        } else {
            out.push('_');
        }
        if out.len() >= 96 {
            break;
        }
    }
    if out.is_empty() {
        "root".to_string()
    } else {
        out
    }
}

fn is_ancestor(parent: &str, child: &str) -> bool {
    child.starts_with(parent) && child.as_bytes().get(parent.len()) == Some(&b'/')
}

fn subvol_name(persist_root: &str, spec: &KeepSpec) -> String {
    format!("{}/{}", persist_root, spec.abs_path.trim_start_matches('/'))
}

fn file_storage_name(persist_root: &str, spec: &KeepSpec) -> String {
    format!(
        "{}/{META_DIR}/{FILES_DIR}/{}",
        persist_root,
        spec.abs_path.trim_start_matches('/'),
    )
}

fn file_store_root(persist_root: &str) -> String {
    format!("{persist_root}/{META_DIR}/{FILES_DIR}")
}

fn file_store_rel(spec: &KeepSpec) -> &str {
    spec.abs_path.trim_start_matches('/')
}

fn plan_from_specs(
    specs: &[KeepSpec],
    persist_root: &str,
    resolve_auto: impl Fn(&KeepSpec) -> Kind,
) -> Vec<KeepPlan> {
    let mut sorted = specs.to_vec();
    sorted.sort_by(|a, b| {
        a.volume
            .cmp(&b.volume)
            .then(
                a.rel_path
                    .matches('/')
                    .count()
                    .cmp(&b.rel_path.matches('/').count()),
            )
            .then(a.rel_path.len().cmp(&b.rel_path.len()))
            .then(a.rel_path.cmp(&b.rel_path))
    });

    let mut plans: Vec<KeepPlan> = Vec::new();
    for spec in sorted {
        let kind = match spec.kind {
            Kind::Auto => resolve_auto(&spec),
            other => other,
        };
        if plans.iter().any(|existing| {
            existing.spec.volume == spec.volume && existing.spec.rel_path == spec.rel_path
        }) {
            continue;
        }
        if plans.iter().any(|existing| {
            existing.spec.volume == spec.volume
                && existing.kind == Kind::Directory
                && is_ancestor(&existing.spec.rel_path, &spec.rel_path)
        }) {
            continue;
        }
        let subvol = if kind == Kind::Directory {
            subvol_name(persist_root, &spec)
        } else {
            file_storage_name(persist_root, &spec)
        };
        let files_to_persist = if kind == Kind::File {
            spec.rel_path.clone()
        } else {
            "*".to_string()
        };
        plans.push(KeepPlan {
            spec,
            kind,
            subvol,
            files_to_persist,
        });
    }
    plans
}

fn plan_for_display(specs: &[KeepSpec], persist_root: &str) -> Vec<KeepPlan> {
    plan_from_specs(specs, persist_root, |spec| {
        if spec.trailing_slash {
            Kind::Directory
        } else {
            Kind::File
        }
    })
}

fn volume_specs(specs: &[KeepSpec], volume: &str) -> Vec<KeepSpec> {
    specs
        .iter()
        .filter(|spec| spec.volume == volume)
        .cloned()
        .collect()
}

fn parse_volumes(args: &[String], specs: &[KeepSpec], runner: &Runner) -> Vec<Volume> {
    let mut volumes = Vec::new();
    if args.is_empty() {
        for spec in specs {
            if volumes.iter().any(|volume: &Volume| volume.name == spec.volume) {
                continue;
            }
            volumes.push(Volume {
                name: spec.volume.clone(),
            });
        }
        return volumes;
    }
    for arg in args {
        let (name, mount) = arg
            .split_once('=')
            .unwrap_or_else(|| runner.die(&format!("volume argument must be name=mount: {arg}")));
        let name =
            normalize_subvolume_path(name, "volume argument").unwrap_or_else(|e| runner.die(&e));
        let _mount_point =
            normalize_mount_point(mount).unwrap_or_else(|e| runner.die(&format!("{arg}: {e}")));
        volumes.push(Volume { name });
    }
    volumes.sort_by(|a, b| a.name.cmp(&b.name));
    volumes.dedup_by(|a, b| a.name == b.name);
    volumes
}

fn is_subvolume(runner: &Runner, path: &str) -> bool {
    if runner.dry_run {
        if env::var("FAKE_BTRFS_ROOT").is_ok() {
            return Path::new(&format!("{path}/.fake-btrfs-subvolume")).is_file()
                || runner.dir_exists(path);
        }
        return fs::metadata(path)
            .map(|meta| meta.is_dir() && meta.ino() == 256)
            .unwrap_or_else(|_| runner.dir_exists(path));
    }
    Command::new("btrfs")
        .args(["subvolume", "show", path])
        .output()
        .map(|output| output.status.success())
        .unwrap_or(false)
}

fn subvolume_info(runner: &Runner, path: &str) -> Option<SubvolumeInfo> {
    if runner.dry_run {
        if runner.dir_exists(path) {
            return Some(SubvolumeInfo {
                uuid: Some(path.to_string()),
                parent_uuid: None,
            });
        }
        return None;
    }
    let output = runner.output(&["btrfs", "subvolume", "show", path]).ok()?;
    if !output.status.success() {
        return None;
    }
    let mut info = SubvolumeInfo {
        uuid: None,
        parent_uuid: None,
    };
    for line in String::from_utf8_lossy(&output.stdout).lines() {
        let trimmed = line.trim();
        if let Some(value) = trimmed.strip_prefix("UUID:") {
            info.uuid = Some(value.trim().to_string());
        }
        if let Some(value) = trimmed.strip_prefix("Parent UUID:") {
            let value = value.trim();
            if value != "-" {
                info.parent_uuid = Some(value.to_string());
            }
        }
    }
    Some(info)
}

fn is_readonly(runner: &Runner, path: &str) -> bool {
    if runner.dry_run {
        return true;
    }
    runner
        .output(&["btrfs", "property", "get", "-ts", path, "ro"])
        .map(|output| {
            output.status.success()
                && String::from_utf8_lossy(&output.stdout).contains("ro=true")
        })
        .unwrap_or(false)
}

fn set_readonly(runner: &Runner, path: &str, readonly: bool) {
    let value = if readonly { "true" } else { "false" };
    runner.require_effect(&["btrfs", "property", "set", "-ts", path, "ro", value]);
}

fn create_subvolume(runner: &Runner, path: &str) {
    if let Some(parent) = Path::new(path).parent() {
        runner.create_dir_all(parent);
    }
    runner.require_effect(&["btrfs", "subvolume", "create", path]);
    runner.mark_planned_dir(path);
}

fn list_child_subvolumes(runner: &Runner, path: &str) -> Vec<String> {
    if runner.dry_run || !Path::new(path).is_dir() {
        return Vec::new();
    }
    let output = Command::new("btrfs")
        .args(["subvolume", "list", "-o", path])
        .output()
        .unwrap();
    String::from_utf8_lossy(&output.stdout)
        .lines()
        .filter_map(|line| line.split(" path ").nth(1))
        .map(|child| format!("{}/{}", runner.mount_path, child))
        .collect()
}

fn delete_subvolume(runner: &Runner, path: &str) {
    if !runner.dir_exists(path) {
        return;
    }
    for child in list_child_subvolumes(runner, path) {
        delete_subvolume(runner, &child);
    }
    runner.require_effect(&["btrfs", "subvolume", "delete", path]);
    runner.unmark_planned_dir(path);
}

fn delete_children(runner: &Runner, path: &str, budget: Option<usize>) -> usize {
    let mut deleted = 0;
    for child in list_child_subvolumes(runner, path) {
        if budget.map(|limit| deleted >= limit).unwrap_or(false) {
            break;
        }
        delete_subvolume(runner, &child);
        deleted += 1;
    }
    deleted
}

fn create_snapshot(runner: &Runner, source: &str, destination: &str, readonly: bool) {
    if !runner.dir_exists(source) {
        runner.die(&format!("Source missing: {source}"));
    }
    delete_subvolume(runner, destination);
    runner.require_effect(&["btrfs", "subvolume", "snapshot", source, destination]);
    runner.mark_planned_dir(destination);
    runner.mark_planned_snapshot(source, destination);
    set_readonly(runner, destination, readonly);
}

fn copy_directory_payload(runner: &Runner, source: &Path, destination: &str) {
    if !runner.path_exists(source) {
        return;
    }
    runner.require_effect(&[
        "cp",
        "--reflink=always",
        "-a",
        &format!("{}/.", source.display()),
        destination,
    ]);
}

fn copy_file_payload(runner: &Runner, source: &Path, store_root: &str, rel: &str) -> bool {
    if !runner.path_exists(source) {
        return false;
    }
    let destination = PathBuf::from(format!("{store_root}/{rel}"));
    if let Some(parent) = destination.parent() {
        runner.create_dir_all(parent);
    }
    if let Ok(meta) = fs::symlink_metadata(&destination) {
        if meta.is_dir() {
            runner.remove_dir_all(&destination);
        } else {
            runner.remove_file(&destination);
        }
    }
    runner.require_effect(&[
        "cp",
        "--reflink=always",
        "-a",
        &source.display().to_string(),
        &destination.display().to_string(),
    ]);
    runner.mark_planned_file(&destination.display().to_string());
    true
}

fn restore_file_payload(
    runner: &Runner,
    store_root: &str,
    source_rel: &str,
    target_root: &str,
    target_rel: &str,
) {
    let source = PathBuf::from(format!("{store_root}/{source_rel}"));
    let target = PathBuf::from(format!("{target_root}/{target_rel}"));
    if !runner.file_exists(&source.display().to_string()) {
        if let Ok(meta) = fs::symlink_metadata(&target) {
            if meta.is_dir() {
                runner.remove_dir_all(&target);
            } else {
                runner.remove_file(&target);
            }
        }
        return;
    }
    if let Some(parent) = target.parent() {
        runner.create_dir_all(parent);
    }
    if let Ok(meta) = fs::symlink_metadata(&target) {
        if meta.is_dir() {
            runner.remove_dir_all(&target);
        } else {
            runner.remove_file(&target);
        }
    }
    runner.require_effect(&[
        "cp",
        "--reflink=always",
        "-a",
        &source.display().to_string(),
        &target.display().to_string(),
    ]);
    runner.mark_planned_file(&target.display().to_string());
}

fn resolve_kind(runner: &Runner, volume: &str, clean: &str, spec: &KeepSpec) -> Kind {
    let live_path = PathBuf::from(format!(
        "{}/{}/{}",
        runner.mount_path, volume, spec.rel_path
    ));
    let clean_path = PathBuf::from(format!("{clean}/{}", spec.rel_path));
    for path in [&live_path, &clean_path] {
        if let Ok(meta) = fs::symlink_metadata(path) {
            return if meta.is_dir() {
                Kind::Directory
            } else {
                Kind::File
            };
        }
    }
    if spec.trailing_slash {
        Kind::Directory
    } else {
        Kind::File
    }
}

fn ensure_namespace(runner: &Runner, rel: &str) {
    let path = runner.top(rel);
    if Path::new(&path).exists() {
        if !Path::new(&path).is_dir() {
            runner.die(&format!("managed path exists but is not a directory: {path}"));
        }
        if !is_subvolume(runner, &path) {
            runner.die(&format!("managed path exists but is not a subvolume: {path}"));
        }
        return;
    }
    create_subvolume(runner, &path);
}

fn ensure_namespaces(runner: &Runner, snapshots_name: &str, persist_root: &str, volumes: &[Volume]) {
    ensure_namespace(runner, persist_root);
    ensure_namespace(runner, &file_store_root(persist_root));
    ensure_namespace(runner, snapshots_name);
    ensure_namespace(runner, STAGING);
    ensure_namespace(runner, TRASH);
    for volume in volumes {
        ensure_namespace(runner, &format!("{snapshots_name}/{}", volume.name));
    }
}

fn clean_path(runner: &Runner, snapshots_name: &str, volume: &str, clean_name: &str) -> String {
    runner.top(&format!("{snapshots_name}/{volume}/{clean_name}"))
}

fn ready_path(runner: &Runner, snapshots_name: &str, volume: &str) -> String {
    runner.top(&format!("{snapshots_name}/{volume}/{READY}"))
}

fn next_path(runner: &Runner, snapshots_name: &str, volume: &str) -> String {
    runner.top(&format!("{snapshots_name}/{volume}/{NEXT}"))
}

fn capture_path(runner: &Runner, volume: &str) -> String {
    runner.top(&format!("{STAGING}/{}.{}", sanitize(volume), CAPTURE))
}

fn retained_path(runner: &Runner, snapshots_name: &str, volume: &str, label: &str) -> String {
    runner.top(&format!("{snapshots_name}/{volume}/{label}"))
}

fn trash_path(runner: &Runner, label: &str) -> String {
    runner.top(&format!("{TRASH}/{}", sanitize(label)))
}

fn top_plan(runner: &Runner, mut plan: KeepPlan) -> KeepPlan {
    plan.subvol = runner.top(&plan.subvol);
    plan
}

fn plans_for_volume(
    runner: &Runner,
    volume: &Volume,
    snapshots_name: &str,
    clean_name: &str,
    persist_root: &str,
    specs: &[KeepSpec],
) -> Vec<KeepPlan> {
    let raw_specs = volume_specs(specs, &volume.name);
    let clean = clean_path(runner, snapshots_name, &volume.name, clean_name);
    plan_from_specs(&raw_specs, persist_root, |spec| {
        resolve_kind(runner, &volume.name, &clean, spec)
    })
    .into_iter()
    .map(|plan| top_plan(runner, plan))
    .collect()
}

fn ensure_persist_dirs(runner: &Runner, capture: &str, clean: &str, plans: &[KeepPlan]) {
    for plan in plans {
        if plan.kind != Kind::Directory {
            continue;
        }
        if Path::new(&plan.subvol).exists() {
            if !is_subvolume(runner, &plan.subvol) {
                runner.die(&format!(
                    "persist path exists but is not a subvolume: {}",
                    plan.subvol
                ));
            }
            println!("{} Keep exists: {}", runner.elapsed(), plan.subvol);
            continue;
        }
        let stage = runner.top(&format!(
            "{STAGING}/persist.{}",
            sanitize(&runner.rel(&plan.subvol))
        ));
        delete_subvolume(runner, &stage);
        create_subvolume(runner, &stage);
        let capture_source = PathBuf::from(format!("{capture}/{}", plan.spec.rel_path));
        let clean_source = PathBuf::from(format!("{clean}/{}", plan.spec.rel_path));
        if runner.path_exists(&capture_source) {
            copy_directory_payload(runner, &capture_source, &stage);
        } else {
            copy_directory_payload(runner, &clean_source, &stage);
        }
        if let Some(parent) = Path::new(&plan.subvol).parent() {
            runner.create_dir_all(parent);
        }
        if !runner.rename(&stage, &plan.subvol) {
            runner.die(&format!("cannot publish persistent subvolume {}", plan.subvol));
        }
        println!(
            "{} Promoted {} -> {}",
            runner.elapsed(),
            plan.spec.abs_path,
            plan.subvol
        );
    }
}

fn save_files(runner: &Runner, capture: &str, clean: &str, store_root: &str, plans: &[KeepPlan]) {
    for plan in plans {
        if plan.kind != Kind::File {
            continue;
        }
        let rel = file_store_rel(&plan.spec);
        let capture_source = PathBuf::from(format!("{capture}/{}", plan.spec.rel_path));
        let clean_source = PathBuf::from(format!("{clean}/{}", plan.spec.rel_path));
        let copied = if runner.path_exists(&capture_source) {
            copy_file_payload(runner, &capture_source, store_root, rel)
        } else {
            copy_file_payload(runner, &clean_source, store_root, rel)
        };
        if !copied {
            runner.remove_file(Path::new(&format!("{store_root}/{rel}")));
        }
        println!(
            "{} Saved file {} -> {store_root}/{rel}",
            runner.elapsed(),
            plan.spec.abs_path,
        );
    }
}

fn ready_valid(runner: &Runner, clean: &str, ready: &str, plans: &[KeepPlan]) -> bool {
    if !runner.dir_exists(ready) || !is_readonly(runner, ready) {
        return false;
    }
    let clean_uuid = subvolume_info(runner, clean).and_then(|info| info.uuid);
    let ready_parent = subvolume_info(runner, ready).and_then(|info| info.parent_uuid);
    if clean_uuid.is_some() && ready_parent.is_some() && clean_uuid != ready_parent {
        return false;
    }
    plans
        .iter()
        .filter(|plan| plan.kind == Kind::Directory)
        .all(|plan| Path::new(&format!("{ready}/{}", plan.spec.rel_path)).is_dir())
}

fn ensure_ready(runner: &Runner, clean: &str, ready: &str, volume: &str, plans: &[KeepPlan]) {
    if ready_valid(runner, clean, ready, plans) {
        println!("{} READY exists: {ready}", runner.elapsed());
        return;
    }
    let stage = runner.top(&format!("{STAGING}/{}.{}", sanitize(volume), READY));
    create_snapshot(runner, clean, &stage, false);
    for plan in plans {
        if plan.kind != Kind::Directory {
            continue;
        }
        let target = PathBuf::from(format!("{stage}/{}", plan.spec.rel_path));
        if let Ok(meta) = fs::symlink_metadata(&target) {
            if !meta.is_dir() {
                runner.remove_file(&target);
            }
        }
        runner.create_dir_all(&target);
    }
    set_readonly(runner, &stage, true);
    delete_subvolume(runner, ready);
    if !runner.rename(&stage, ready) {
        runner.die(&format!("cannot publish READY for {volume}"));
    }
}

fn restore_files(runner: &Runner, next: &str, store_root: &str, plans: &[KeepPlan]) {
    for plan in plans {
        if plan.kind == Kind::File {
            restore_file_payload(
                runner,
                store_root,
                file_store_rel(&plan.spec),
                next,
                &plan.spec.rel_path,
            );
        }
    }
}

fn rotate_retained(runner: &Runner, snapshots_name: &str, volume: &str, capture: &str) {
    let c = retained_path(runner, snapshots_name, volume, "C");
    let b = retained_path(runner, snapshots_name, volume, "B");
    let a = retained_path(runner, snapshots_name, volume, "A");
    delete_subvolume(runner, &c);
    if runner.dir_exists(&b) && !runner.rename(&b, &c) {
        runner.die(&format!("cannot rotate {b} -> {c}"));
    }
    if runner.dir_exists(&a) && !runner.rename(&a, &b) {
        runner.die(&format!("cannot rotate {a} -> {b}"));
    }
    if !runner.rename(capture, &a) {
        runner.die(&format!("cannot rotate {capture} -> {a}"));
    }
}

fn publish_next(runner: &Runner, volume: &str, next: &str) {
    let live = runner.top(volume);
    if !runner.dir_exists(next) {
        return;
    }
    if !runner.dir_exists(&live) {
        if runner.rename(next, &live) {
            return;
        }
        runner.die(&format!("live volume missing and cannot publish {next}"));
    }
    let old = trash_path(
        runner,
        &format!(
            "{}.{}",
            sanitize(volume),
            fs::metadata(&live)
                .map(|meta| meta.ino().to_string())
                .unwrap_or_else(|_| "old".to_string())
        ),
    );
    if !runner.rename(&live, &old) {
        eprintln!(
            "{} WRN live volume busy; leaving NEXT for a later converge: {next}",
            runner.elapsed()
        );
        return;
    }
    if !runner.rename(next, &live) {
        runner.die(&format!("live moved to trash but NEXT could not publish: {next}"));
    }
}

fn cleanup_staging(runner: &Runner) {
    let staging = runner.top(STAGING);
    delete_children(runner, &staging, None);
}

fn cleanup_trash(runner: &Runner) {
    let budget = env::var("IMMUTABILITY_TRASH_DELETE_BUDGET")
        .ok()
        .and_then(|value| value.parse::<usize>().ok())
        .unwrap_or(32);
    let trash = runner.top(TRASH);
    delete_children(runner, &trash, Some(budget));
}

fn remove_empty_parents(path: &Path, stop: &Path) {
    let mut current = path.parent();
    while let Some(parent) = current {
        if parent == stop {
            break;
        }
        if fs::remove_dir(parent).is_err() {
            break;
        }
        current = parent.parent();
    }
}

fn delete_stale_file_payloads(runner: &Runner, store_root: &str, desired: &HashSet<String>) {
    if runner.dry_run || !Path::new(store_root).is_dir() {
        return;
    }
    let root = PathBuf::from(store_root);
    let mut stack = vec![root.clone()];
    while let Some(dir) = stack.pop() {
        let Ok(entries) = fs::read_dir(&dir) else {
            continue;
        };
        for entry in entries.flatten() {
            let path = entry.path();
            if env::var("FAKE_BTRFS_ROOT").is_ok()
                && path
                    .file_name()
                    .and_then(|name| name.to_str())
                    .map(|name| name.starts_with(".fake-btrfs-"))
                    .unwrap_or(false)
            {
                continue;
            }
            if path.is_dir() && !path.is_symlink() {
                stack.push(path);
                continue;
            }
            let rel = path.strip_prefix(&root).unwrap();
            let rel = rel.to_string_lossy().to_string();
            if !desired.contains(&rel) {
                runner.remove_file(&path);
                remove_empty_parents(&path, &root);
            }
        }
    }
}

fn list_subvolume_rels_under(runner: &Runner, root_rel: &str) -> Vec<String> {
    let root = runner.top(root_rel);
    if runner.dry_run || !Path::new(&root).is_dir() {
        return Vec::new();
    }
    let output = Command::new("btrfs")
        .args(["subvolume", "list", "-o", &root])
        .output()
        .unwrap();
    String::from_utf8_lossy(&output.stdout)
        .lines()
        .filter_map(|line| line.split(" path ").nth(1))
        .map(|path| path.to_string())
        .collect()
}

fn delete_obsolete_persists(
    runner: &Runner,
    persist_root: &str,
    desired_dirs: &HashSet<String>,
) {
    let meta = format!("{persist_root}/{META_DIR}");
    let file_store = file_store_root(persist_root);
    for subvol in list_subvolume_rels_under(runner, persist_root) {
        if subvol == persist_root || subvol == meta || subvol == file_store {
            continue;
        }
        if subvol.starts_with(&format!("{meta}/")) {
            continue;
        }
        let covered_by_desired = desired_dirs
            .iter()
            .any(|desired| subvol == *desired || is_ancestor(desired, &subvol));
        if covered_by_desired {
            continue;
        }
        let source = runner.top(&subvol);
        let dest = trash_path(runner, &format!("obsolete.{subvol}"));
        if !runner.rename(&source, &dest) {
            runner.die(&format!("cannot move obsolete persist to trash: {source}"));
        }
    }
}

fn converge_volume(
    runner: &Runner,
    volume: &Volume,
    snapshots_name: &str,
    clean_name: &str,
    persist_root: &str,
    specs: &[KeepSpec],
) {
    let live = runner.top(&volume.name);
    let clean = clean_path(runner, snapshots_name, &volume.name, clean_name);
    let ready = ready_path(runner, snapshots_name, &volume.name);
    let next = next_path(runner, snapshots_name, &volume.name);
    let capture = capture_path(runner, &volume.name);
    if !runner.dir_exists(&clean) {
        runner.die(&format!("CLEAN missing: {clean}"));
    }
    if !runner.dir_exists(&live) {
        publish_next(runner, &volume.name, &next);
    }
    if !runner.dir_exists(&live) {
        let retained = retained_path(runner, snapshots_name, &volume.name, "A");
        if runner.dir_exists(&retained) {
            create_snapshot(runner, &retained, &live, false);
        }
    }
    if !runner.dir_exists(&live) {
        runner.die(&format!("live volume missing: {live}"));
    }
    if runner.dir_exists(&next) {
        delete_subvolume(runner, &next);
    }

    let plans = plans_for_volume(
        runner,
        volume,
        snapshots_name,
        clean_name,
        persist_root,
        specs,
    );
    create_snapshot(runner, &live, &capture, true);
    ensure_persist_dirs(runner, &capture, &clean, &plans);
    let store_root = runner.top(&file_store_root(persist_root));
    save_files(runner, &capture, &clean, &store_root, &plans);
    ensure_ready(runner, &clean, &ready, &volume.name, &plans);
    create_snapshot(runner, &ready, &next, false);
    restore_files(runner, &next, &store_root, &plans);
    rotate_retained(runner, snapshots_name, &volume.name, &capture);
    publish_next(runner, &volume.name, &next);
}

fn converge(
    runner: &Runner,
    snapshots_name: &str,
    clean_name: &str,
    persist_root: &str,
    specs: &[KeepSpec],
    volumes: &[Volume],
) {
    ensure_namespaces(runner, snapshots_name, persist_root, volumes);
    cleanup_staging(runner);
    for volume in volumes {
        println!("{} Converging {}", runner.elapsed(), volume.name);
        converge_volume(
            runner,
            volume,
            snapshots_name,
            clean_name,
            persist_root,
            specs,
        );
    }
    let mut desired_dirs = HashSet::new();
    let mut desired_files = HashSet::new();
    for volume in volumes {
        for plan in plans_for_volume(
            runner,
            volume,
            snapshots_name,
            clean_name,
            persist_root,
            specs,
        ) {
            if plan.kind == Kind::Directory {
                desired_dirs.insert(runner.rel(&plan.subvol));
            } else {
                desired_files.insert(file_store_rel(&plan.spec).to_string());
            }
        }
    }
    delete_obsolete_persists(runner, persist_root, &desired_dirs);
    delete_stale_file_payloads(runner, &runner.top(&file_store_root(persist_root)), &desired_files);
    cleanup_trash(runner);
}

fn snapshot_only(runner: &Runner, volume: &Volume, snapshots_name: &str, clean_name: &str) {
    let live = runner.top(&volume.name);
    let clean = clean_path(runner, snapshots_name, &volume.name, clean_name);
    if !runner.dir_exists(&clean) {
        runner.die(&format!("CLEAN missing: {clean}"));
    }
    let capture = capture_path(runner, &volume.name);
    create_snapshot(runner, &live, &capture, true);
    rotate_retained(runner, snapshots_name, &volume.name, &capture);
}

fn restore(runner: &Runner, volume: &Volume, snapshots_name: &str, label: &str) {
    let live = runner.top(&volume.name);
    let source = retained_path(runner, snapshots_name, &volume.name, label);
    if !runner.dir_exists(&source) {
        runner.die(&format!("Cannot restore: {source}"));
    }
    println!("{} Restoring {} from {label}", runner.elapsed(), volume.name);
    create_snapshot(runner, &source, &live, false);
}

fn print_plan(spec_file: &str, persist_root: &str) {
    let persist_root = normalize_subvolume_path(persist_root, "persist root").unwrap_or_else(|e| {
        eprintln!("ERR {e}");
        process::exit(1);
    });
    let specs = parse_spec_file(spec_file).unwrap_or_else(|e| {
        eprintln!("ERR {e}");
        process::exit(1);
    });
    for plan in plan_for_display(&specs, &persist_root) {
        println!(
            "{}\t{}\t{}\t{}\t{}\t{}",
            plan.kind.as_str(),
            plan.spec.volume,
            plan.spec.rel_path,
            plan.spec.abs_path,
            plan.subvol,
            plan.files_to_persist,
        );
    }
}

fn mount_top_level<'a>(runner: &'a Runner, device: &str, assume_mounted: bool) -> Option<Unmounter<'a>> {
    if assume_mounted {
        if !Path::new(&runner.mount_path).is_dir() {
            runner.die(&format!(
                "mount path must already exist for --assume-mounted: {}",
                runner.mount_path
            ));
        }
        return None;
    }
    fs::create_dir_all(&runner.mount_path).unwrap_or_else(|e| {
        runner.die(&format!(
            "cannot create mount path {}: {e}",
            runner.mount_path
        ))
    });
    if runner.dry_run {
        runner.require(&[
            "mount",
            "-t",
            "btrfs",
            "-o",
            "ro,subvolid=5",
            device,
            &runner.mount_path,
        ]);
    } else {
        runner.require(&[
            "mount",
            "-t",
            "btrfs",
            "-o",
            "subvolid=5,user_subvol_rm_allowed",
            device,
            &runner.mount_path,
        ]);
    }
    Some(Unmounter(runner))
}

fn main() {
    let args: Vec<String> = env::args().collect();
    if args.get(1).map(|s| s.as_str()) == Some("plan") {
        if args.len() != 4 {
            eprintln!("Usage: {} plan <persist_root> <spec_file>", args[0]);
            process::exit(1);
        }
        print_plan(&args[3], &args[2]);
        return;
    }

    let cli = parse_cli_flags(&args);
    if args.len() < cli.offset + 6 {
        eprintln!("Usage: {} [--dry-run] [--assume-mounted] <device> <snapshots_name> <clean_name> <mode> <persist_root> <spec_file> [volume=mount ...]", args[0]);
        process::exit(1);
    }

    let runner = Runner::new(cli.dry_run);
    if cli.assume_mounted && !runner.dry_run {
        runner.die("--assume-mounted is only supported with --dry-run");
    }
    let device = &args[cli.offset];
    let snapshots_name = normalize_subvolume_path(&args[cli.offset + 1], "snapshots subvolume")
        .unwrap_or_else(|e| runner.die(&e));
    let clean_name = normalize_path_component(&args[cli.offset + 2], "clean snapshot name")
        .unwrap_or_else(|e| runner.die(&e));
    let mode = args[cli.offset + 3].as_str();
    let persist_root = normalize_subvolume_path(&args[cli.offset + 4], "persist root")
        .unwrap_or_else(|e| runner.die(&e));
    let spec_file = &args[cli.offset + 5];
    let specs = parse_spec_file(spec_file).unwrap_or_else(|e| runner.die(&e));
    let volumes = parse_volumes(&args[(cli.offset + 6)..], &specs, &runner);

    println!(
        "{} Mode={mode} dry_run={} assume_mounted={} device={device} persist_root={persist_root} subvolumes={}",
        runner.elapsed(),
        runner.dry_run,
        cli.assume_mounted,
        volumes.iter().map(|volume| volume.name.as_str()).collect::<Vec<_>>().join(" "),
    );
    let _unmount = mount_top_level(&runner, device, cli.assume_mounted);
    match mode {
        "disabled" => println!("{} Immutability v2 disabled; skipping", runner.elapsed()),
        "converge" | "reset" | "prepare-only" => converge(
            &runner,
            &snapshots_name,
            &clean_name,
            &persist_root,
            &specs,
            &volumes,
        ),
        "snapshot-only" => {
            for volume in &volumes {
                snapshot_only(&runner, volume, &snapshots_name, &clean_name);
            }
        }
        "restore-a" => {
            for volume in &volumes {
                restore(&runner, volume, &snapshots_name, "A");
            }
        }
        "restore-b" | "restore-previous" => {
            for volume in &volumes {
                restore(&runner, volume, &snapshots_name, "B");
            }
        }
        "restore-c" | "restore-penultimate" => {
            for volume in &volumes {
                restore(&runner, volume, &snapshots_name, "C");
            }
        }
        _ => runner.die(&format!("Unknown mode: {mode}")),
    }
    if !matches!(mode, "disabled") {
        runner.require_effect(&["btrfs", "filesystem", "sync", &runner.mount_path]);
    }
    println!("{} Immutability v2 complete", runner.elapsed());
}

#[cfg(test)]
mod tests {
    use super::*;

    fn temp_path(name: &str) -> PathBuf {
        let mut path = env::temp_dir();
        path.push(format!(
            "immutabilityv2-unit-{}-{name}",
            process::id()
        ));
        let _ = fs::remove_dir_all(&path);
        path
    }

    fn spec(volume: &str, mount: &str, abs: &str, kind: Kind) -> KeepSpec {
        let (abs_path, trailing_slash) = normalize_abs(abs).unwrap();
        KeepSpec {
            volume: volume.to_string(),
            abs_path: abs_path.clone(),
            rel_path: relative_to_mount(&abs_path, mount).unwrap(),
            kind,
            trailing_slash,
        }
    }

    #[test]
    fn pure_validation_edges_are_rejected() {
        assert_eq!(Kind::parse("bad").unwrap_err(), "unknown keep kind: bad");
        assert_eq!(Kind::Auto.as_str(), "auto");
        assert!(validate_relative_path("a\tb", "path").is_err());
        assert!(normalize_abs("relative").is_err());
        assert_eq!(normalize_mount_point("").unwrap(), "/");
        assert!(normalize_subvolume_path("/", "subvol").is_err());
        assert!(normalize_path_component("a/b", "component").is_err());
        assert!(normalize_path_component("a\tb", "component").is_err());
        assert!(relative_to_mount("/home", "/home").is_err());
        assert!(relative_to_mount("/var/log", "/home").is_err());
    }

    #[test]
    fn spec_parsing_comments_errors_and_sanitizing() {
        let path = temp_path("spec.tsv");
        fs::create_dir_all(path.parent().unwrap()).unwrap();
        fs::write(
            &path,
            "# ignored\n\n@home\t/home\t/home/alex/file\tauto\n",
        )
        .unwrap();
        let specs = parse_spec_file(path.to_str().unwrap()).unwrap();
        assert_eq!(specs.len(), 1);

        fs::write(&path, "@home\t/home\t/home/alex/file\n").unwrap();
        assert!(parse_spec_file(path.to_str().unwrap()).is_err());
        assert_eq!(sanitize(""), "root");
        assert_eq!(sanitize("a/b c"), "a_b_c");
        assert_eq!(sanitize(&"x".repeat(200)).len(), 96);
        let _ = fs::remove_file(path);
    }

    #[test]
    fn planning_deduplicates_and_parse_volumes_can_infer_specs() {
        let specs = vec![
            spec("@home", "/home", "/home/alex/.bash_history", Kind::File),
            spec("@home", "/home", "/home/alex/.bash_history", Kind::File),
            spec("@home", "/home", "/home/alex/.config/", Kind::Directory),
            spec("@home", "/home", "/home/alex/.config/kwinrc", Kind::File),
            spec("@home", "/home", "/home/alex/.config/", Kind::Directory),
            spec("@root", "/", "/etc/nixos", Kind::Auto),
        ];
        let plans = plan_from_specs(&specs, "@persist", |spec| {
            if spec.trailing_slash {
                Kind::Directory
            } else {
                Kind::File
            }
        });
        assert_eq!(plans.len(), 3);

        let runner = Runner::new(true);
        let volumes = parse_volumes(&[], &specs, &runner);
        assert_eq!(volumes.len(), 2);
    }

    #[test]
    fn dry_run_helpers_track_virtual_state() {
        let root = temp_path("virtual");
        let source = root.join("source");
        fs::create_dir_all(source.join("child")).unwrap();
        fs::write(source.join("child/file"), "value").unwrap();
        let destination = root.join("destination");
        let runner = Runner::new(true);
        runner.mark_planned_snapshot(
            source.to_str().unwrap(),
            destination.to_str().unwrap(),
        );
        assert!(runner.path_exists(&destination));
        assert!(runner.path_exists(&destination.join("child/file")));
        assert!(!runner.path_exists(&root.join("missing")));
        runner.mark_planned_file(destination.to_str().unwrap());
        runner.remove_file(&destination);
        runner.remove_dir_all(&destination);
        let _ = fs::remove_dir_all(root);
    }

    #[test]
    fn dry_run_subvolume_checks_without_fake_btrfs() {
        let root = temp_path("subvolume");
        fs::create_dir_all(&root).unwrap();
        let runner = Runner::new(true);
        assert!(!is_subvolume(&runner, root.to_str().unwrap()));
        assert!(subvolume_info(&runner, root.to_str().unwrap()).is_some());
        assert!(subvolume_info(&runner, root.join("missing").to_str().unwrap()).is_none());
        let _ = fs::remove_dir_all(root);
    }

    #[test]
    fn dry_run_fake_subvolume_and_cleanup_edges() {
        let root = temp_path("fake-subvolume");
        fs::create_dir_all(&root).unwrap();
        let runner = Runner::new(true);
        env::set_var("FAKE_BTRFS_ROOT", root.to_str().unwrap());
        runner.mark_planned_dir(root.to_str().unwrap());
        assert!(is_subvolume(&runner, root.to_str().unwrap()));
        assert!(is_readonly(&runner, root.to_str().unwrap()));
        env::remove_var("FAKE_BTRFS_ROOT");

        let stop = root.join("files");
        let parent = stop.join("home/alex");
        fs::create_dir_all(&parent).unwrap();
        fs::write(parent.join("gone"), "old").unwrap();
        fs::write(parent.join("sibling"), "keeps-parent-non-empty").unwrap();
        remove_empty_parents(&parent.join("gone"), &stop);
        assert!(parent.exists());
        let _ = fs::remove_dir_all(root);
    }
}
