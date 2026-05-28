// BTRFS semi-permeable membrane.
// KISS: save kept paths, snapshot CLEAN to NEXT, overlay kept paths, then
// atomically exchange NEXT with the live subvolume.
//
// Directory keeps: /home/alex/Downloads -> @persist/dirs/home!alex!Downloads
// File keeps:      /home/alex/history   -> @persist/.semipermeable_membrane/files/home!alex!history
use std::{env, fs, path::{Path, PathBuf}, process::{self, Command}, time::Instant};

const TOP: &str = "/run/semipermeable-membrane-top";
const DIRS: &str = "dirs";
const META: &str = ".semipermeable_membrane";
const FILES: &str = "files";
const NEXT: &str = "NEXT";
const A: &str = "A";
const B: &str = "B";
const C: &str = "C";
const PATH_KEY_SEPARATOR: char = '!';
const PATH_KEY_SEPARATOR_STR: &str = "!";
const ENV_MOUNT_PATH: &str = "SEMIPERMEABLE_MEMBRANE_MOUNT_PATH";
const ENV_DRY_RUN: &str = "SEMIPERMEABLE_MEMBRANE_DRY_RUN";
const MODE_DISABLED: &str = "disabled";
const MODE_CONVERGE: &str = "converge";
const MODE_RESET: &str = "reset";
const MODE_PREPARE_ONLY: &str = "prepare-only";
const MODE_SNAPSHOT_ONLY: &str = "snapshot-only";
const MODE_RESTORE_A: &str = "restore-a";

#[derive(Clone, Copy, Eq, PartialEq)]
enum Kind { Auto, Dir, File }

#[derive(Clone)]
struct Spec { vol: String, abs: String, rel: String, kind: Kind }

#[derive(Clone)]
struct Vol { name: String, mount: String }

struct Plan { spec: Spec, kind: Kind, store: String }
struct Run { start: Instant, top: String, dry: bool }

impl Run {
	fn new(dry: bool) -> Self {
		let top = env::var(ENV_MOUNT_PATH)
			.unwrap_or_else(|_| format!("{TOP}.{}", process::id()));
		Self { start: Instant::now(), top, dry }
	}
	fn e(&self) -> String { format!("[{:.2}s]", self.start.elapsed().as_secs_f64()) }
	fn top(&self, rel: &str) -> String {
		format!("{}/{}", self.top.trim_end_matches('/'), rel.trim_start_matches('/'))
	}
	fn log_run(&self, args: &[&str]) -> bool {
		println!("{} {}", self.e(), args.join(" "));
		let out = Command::new(args[0]).args(&args[1..]).output().expect("exec failed");
		for line in String::from_utf8_lossy(&out.stdout).lines().filter(|l| !l.is_empty()) {
			println!("{}   {line}", self.e());
		}
		for line in String::from_utf8_lossy(&out.stderr).lines().filter(|l| !l.is_empty()) {
			eprintln!("{}   {line}", self.e());
		}
		out.status.success()
	}
	fn run(&self, args: &[&str]) -> bool {
		if self.dry { println!("{} DRY {}", self.e(), args.join(" ")); true } else { self.log_run(args) }
	}
	fn req(&self, args: &[&str]) {
		if !self.run(args) { self.die(&format!("Failed: {}", args.join(" "))); }
	}
	fn mkdir(&self, path: &Path) {
		if self.dry { println!("{} DRY mkdir -p {}", self.e(), path.display()); return; }
		fs::create_dir_all(path).unwrap_or_else(|e| self.die(&format!("mkdir {}: {e}", path.display())));
	}
	fn rm(&self, path: &Path) {
		if self.dry { println!("{} DRY rm -rf {}", self.e(), path.display()); return; }
		match fs::symlink_metadata(path) {
			Ok(meta) if meta.is_dir() => { fs::remove_dir_all(path).ok(); }
			Ok(_) => { fs::remove_file(path).ok(); }
			Err(_) => {}
		}
	}
	fn rename(&self, a: &Path, b: &Path) {
		if self.dry { println!("{} DRY mv {} {}", self.e(), a.display(), b.display()); return; }
		fs::rename(a, b).unwrap_or_else(|e| self.die(&format!("mv {} {}: {e}", a.display(), b.display())));
	}
	fn die(&self, msg: &str) -> ! {
		eprintln!("{} ERR {msg}", self.e());
		panic!("semipermeable_membrane fatal: {msg}");
	}
}

struct Unmount<'a>(&'a Run);
impl Drop for Unmount<'_> {
	fn drop(&mut self) {
		self.0.log_run(&["umount", "-R", &self.0.top]);
		if self.0.top.starts_with(&format!("{TOP}.")) { fs::remove_dir(&self.0.top).ok(); }
	}
}

fn valid_rel(path: &str) -> bool {
	!path.contains(['\0', '\t', PATH_KEY_SEPARATOR]) && path.split('/').all(|p| !p.is_empty() && p != "." && p != "..")
}
fn norm_subvol(raw: &str) -> Result<String, String> {
	let s = raw.trim().trim_matches('/');
	if s.is_empty() || !valid_rel(s) { Err(format!("bad subvolume: {raw}")) } else { Ok(s.to_string()) }
}
fn norm_mount(raw: &str) -> Result<String, String> {
	let s = raw.trim();
	if s.is_empty() || s == "/" { return Ok("/".to_string()); }
	if s.starts_with('/') && valid_rel(s.trim_start_matches('/')) { Ok(s.trim_end_matches('/').to_string()) }
	else { Err(format!("bad mount: {raw}")) }
}
fn norm_abs(raw: &str) -> Result<(String, bool), String> {
	let s = raw.trim();
	if !s.starts_with('/') || s == "/" { return Err(format!("bad keep path: {raw}")); }
	let slash = s.len() > 1 && s.ends_with('/');
	let s = if slash { s.trim_end_matches('/') } else { s };
	if valid_rel(s.trim_start_matches('/')) { Ok((s.to_string(), slash)) } else { Err(format!("bad keep path: {raw}")) }
}
fn rel_to_mount(abs: &str, mount: &str) -> Result<String, String> {
	if mount == "/" { return Ok(abs.trim_start_matches('/').to_string()); }
	abs.strip_prefix(&format!("{mount}/")).map(|s| s.to_string()).ok_or_else(|| format!("{abs} not under {mount}"))
}
fn parse_kind(raw: &str) -> Result<Kind, String> {
	match raw.trim() { "auto" => Ok(Kind::Auto), "directory" | "dir" => Ok(Kind::Dir), "file" => Ok(Kind::File), _ => Err(format!("bad kind: {raw}")) }
}
fn exists(path: &str) -> bool { fs::symlink_metadata(path).is_ok() }
fn ancestor(parent: &str, child: &str) -> bool { child.starts_with(parent) && child.as_bytes().get(parent.len()) == Some(&b'/') }
fn key(abs: &str) -> String { abs.trim_start_matches('/').replace('/', PATH_KEY_SEPARATOR_STR) }
fn dir_store(persist: &str, abs: &str) -> String { format!("{persist}/{DIRS}/{}", key(abs)) }
fn file_store(persist: &str, abs: &str) -> String { format!("{persist}/{META}/{FILES}/{}", key(abs)) }

fn read_specs(path: &str) -> Result<Vec<Spec>, String> {
	let content = fs::read_to_string(path).map_err(|e| format!("read {path}: {e}"))?;
	let mut specs = Vec::new();
	for (i, line) in content.lines().enumerate() {
		let trimmed = line.trim();
		if trimmed.is_empty() || trimmed.starts_with('#') { continue; }
		let parts: Vec<&str> = line.split('\t').collect();
		if parts.len() != 4 { return Err(format!("{path}:{} expected 4 tab-separated fields", i + 1)); }
		let vol = norm_subvol(parts[0])?;
		let mount = norm_mount(parts[1])?;
		let (abs, slash) = norm_abs(parts[2])?;
		let mut kind = parse_kind(parts[3])?;
		if kind == Kind::Auto && slash { kind = Kind::Dir; }
		specs.push(Spec { vol, rel: rel_to_mount(&abs, &mount)?, abs, kind });
	}
	Ok(specs)
}
fn make_plan(specs: &[Spec], persist: &str, resolve: impl Fn(&Spec) -> Kind) -> Vec<Plan> {
	let mut specs = specs.to_vec();
	specs.sort_by(|a, b| {
		a.vol.cmp(&b.vol)
			.then(a.rel.matches('/').count().cmp(&b.rel.matches('/').count()))
			.then(a.rel.len().cmp(&b.rel.len()))
			.then(a.rel.cmp(&b.rel))
	});
	let mut plans = Vec::new();
	for spec in specs {
		let kind = if spec.kind == Kind::Auto { resolve(&spec) } else { spec.kind };
		if plans.iter().any(|p: &Plan| p.spec.vol == spec.vol && p.spec.rel == spec.rel) { continue; }
		if plans.iter().any(|p: &Plan| p.spec.vol == spec.vol && p.kind == Kind::Dir && ancestor(&p.spec.rel, &spec.rel)) { continue; }
		let store = if kind == Kind::Dir { dir_store(persist, &spec.abs) } else { file_store(persist, &spec.abs) };
		plans.push(Plan { spec, kind, store });
	}
	plans
}

fn subvol(path: &str) -> bool {
	Command::new("btrfs").args(["subvolume", "show", path]).output().map(|out| out.status.success()).unwrap_or(false)
}
fn readonly(path: &str) -> bool {
	Command::new("btrfs").args(["property", "get", "-ts", path, "ro"]).output()
		.map(|out| out.status.success() && String::from_utf8_lossy(&out.stdout).contains("ro=true")).unwrap_or(false)
}
fn mounted(path: &str) -> bool {
	Command::new("findmnt").args(["--mountpoint", path]).output().map(|out| out.status.success()).unwrap_or(false)
}
fn children(run: &Run, path: &str) -> Vec<String> {
	if !Path::new(path).is_dir() { return vec![]; }
	let out = Command::new("btrfs").args(["subvolume", "list", "-o", path]).output()
		.unwrap_or_else(|e| run.die(&format!("list {path}: {e}")));
	if !out.status.success() { run.die(&format!("list {path}")); }
	let mut children: Vec<String> = String::from_utf8_lossy(&out.stdout).lines()
		.filter_map(|line| line.split(" path ").nth(1)).map(|path| run.top(path)).collect();
	children.sort_by(|a, b| b.matches('/').count().cmp(&a.matches('/').count()).then(b.len().cmp(&a.len())).then(a.cmp(b)));
	children
}
fn del_subvol(run: &Run, path: &str) {
	if !Path::new(path).is_dir() { return; }
	for child in children(run, path) { del_subvol(run, &child); }
	run.req(&["btrfs", "subvolume", "delete", path]);
}
fn mk_subvol(run: &Run, path: &str) {
	if let Some(parent) = Path::new(path).parent() { run.mkdir(parent); }
	run.req(&["btrfs", "subvolume", "create", path]);
}
fn snap(run: &Run, from: &str, to: &str, ro: bool) {
	if !Path::new(from).is_dir() { run.die(&format!("Source missing: {from}")); }
	del_subvol(run, to);
	if ro { run.req(&["btrfs", "subvolume", "snapshot", "-r", from, to]); }
	else { run.req(&["btrfs", "subvolume", "snapshot", from, to]); run.req(&["btrfs", "property", "set", "-ts", to, "ro", "false"]); }
}

fn source(live: &str, clean: &str, rel: &str) -> Option<PathBuf> {
	for root in [live, clean] {
		let path = PathBuf::from(format!("{root}/{rel}"));
		if exists(&path.display().to_string()) { return Some(path); }
	}
	None
}
fn copy(run: &Run, src: &Path, dst: &Path) {
	if let Some(parent) = dst.parent() { run.mkdir(parent); }
	run.req(&["cp", "--reflink=always", "-a", &src.display().to_string(), &dst.display().to_string()]);
}
fn copy_dir(run: &Run, src: &Path, dst: &str) {
	if !src.is_dir() { return; }
	let nested = children(run, &src.display().to_string());
	if !nested.is_empty() { run.die(&format!("nested subvolumes in persisted dir: {}", nested.join(" "))); }
	run.req(&["cp", "--reflink=always", "-a", &format!("{}/.", src.display()), dst]);
}
fn namespaces(run: &Run, persist: &str) {
	for rel in [persist.to_string(), format!("{persist}/{DIRS}"), format!("{persist}/{META}"), format!("{persist}/{META}/{FILES}")] {
		let path = run.top(&rel);
		if !exists(&path) { mk_subvol(run, &path); } else if !subvol(&path) { run.die(&format!("not a subvolume: {path}")); }
	}
}
fn rotate(run: &Run, snaps: &str, vol: &str, live: &str) {
	let a = run.top(&format!("{snaps}/{vol}/{A}"));
	let b = run.top(&format!("{snaps}/{vol}/{B}"));
	let c = run.top(&format!("{snaps}/{vol}/{C}"));
	del_subvol(run, &c);
	if exists(&b) { snap(run, &b, &c, true); del_subvol(run, &b); }
	if exists(&a) { snap(run, &a, &b, true); del_subvol(run, &a); }
	snap(run, live, &a, true);
}
fn publish(run: &Run, vol: &Vol, live: &str, next: &str) {
	if !exists(next) { return; }
	if !run.dry && vol.mount != "/" && mounted(&vol.mount) {
		eprintln!("{} WRN {} mounted; leaving NEXT for boot", run.e(), vol.mount);
		return;
	}
	if !exists(live) { run.rename(Path::new(next), Path::new(live)); return; }
	run.req(&["mv", "-T", "--exchange", "--no-copy", live, next]);
	del_subvol(run, next);
}

fn reset(run: &Run, vol: &Vol, snaps: &str, clean_name: &str, persist: &str, specs: &[Spec]) {
	let live = run.top(&vol.name);
	let clean = run.top(&format!("{snaps}/{}/{}", vol.name, clean_name));
	let next = run.top(&format!("{snaps}/{}/{}", vol.name, NEXT));
	if !exists(&clean) || !subvol(&clean) || !readonly(&clean) { run.die(&format!("bad CLEAN: {clean}")); }
	if !exists(&live) { publish(run, vol, &live, &next); }
	if !exists(&live) { run.die(&format!("live missing: {live}")); }
	if !subvol(&live) { run.die(&format!("live is not a subvolume: {live}")); }
	del_subvol(run, &next);

	let own: Vec<Spec> = specs.iter().filter(|s| s.vol == vol.name).cloned().collect();
	let plans = make_plan(&own, persist, |spec| {
		source(&live, &clean, &spec.rel)
			.and_then(|path| fs::symlink_metadata(path).ok())
			.map(|meta| if meta.is_dir() { Kind::Dir } else { Kind::File })
			.unwrap_or(Kind::File)
	});
	for p in plans.iter().filter(|p| p.kind == Kind::Dir) {
		let store = run.top(&p.store);
		if !exists(&store) {
			mk_subvol(run, &store);
			if let Some(src) = source(&live, &clean, &p.spec.rel) { copy_dir(run, &src, &store); }
		} else if !subvol(&store) { run.die(&format!("not a subvolume: {store}")); }
	}
	for p in plans.iter().filter(|p| p.kind == Kind::File) {
		let dst = PathBuf::from(run.top(&p.store));
		if let Some(src) = source(&live, &clean, &p.spec.rel) {
			let tmp = dst.with_extension(format!("tmp.{}", process::id()));
			run.rm(&tmp); copy(run, &src, &tmp); run.rm(&dst); run.rename(&tmp, &dst);
		} else { run.rm(&dst); }
	}
	rotate(run, snaps, &vol.name, &live);
	snap(run, &clean, &next, false);
	for p in &plans {
		let target = PathBuf::from(format!("{next}/{}", p.spec.rel));
		run.rm(&target);
		if p.kind == Kind::Dir { run.mkdir(&target); }
		else {
			let src = PathBuf::from(run.top(&p.store));
			if exists(&src.display().to_string()) { copy(run, &src, &target); }
		}
	}
	publish(run, vol, &live, &next);
}

fn volumes(run: &Run, args: &[String], specs: &[Spec]) -> Vec<Vol> {
	if args.is_empty() {
		let mut seen = Vec::new();
		return specs.iter().filter(|s| {
			if seen.contains(&s.vol) { false } else { seen.push(s.vol.clone()); true }
		}).map(|s| Vol { name: s.vol.clone(), mount: "/".to_string() }).collect();
	}
	args.iter().map(|arg| {
		let (name, mount) = arg.split_once('=').unwrap_or_else(|| run.die(&format!("bad volume arg: {arg}")));
		Vol {
			name: norm_subvol(name).unwrap_or_else(|e| run.die(&e)),
			mount: norm_mount(mount).unwrap_or_else(|e| run.die(&e)),
		}
	}).collect()
}
fn mount_top<'a>(run: &'a Run, dev: &str, assume: bool) -> Option<Unmount<'a>> {
	if assume {
		if !Path::new(&run.top).is_dir() { run.die(&format!("missing mount path: {}", run.top)); }
		return None;
	}
	fs::create_dir_all(&run.top).unwrap_or_else(|e| run.die(&format!("mkdir {}: {e}", run.top)));
	if run.dry {
		println!("{} DRY mount -t btrfs -o subvolid=5 {dev} {}", run.e(), run.top);
		return None;
	}
	run.req(&["mount", "-t", "btrfs", "-o", "subvolid=5,user_subvol_rm_allowed", dev, &run.top]);
	Some(Unmount(run))
}

fn main() {
	let args: Vec<String> = env::args().collect();
	let mut dry = env::var(ENV_DRY_RUN).map(|v| matches!(v.as_str(), "1" | "true" | "yes" | "on")).unwrap_or(false);
	let mut assume = false;
	let mut i = 1;
	while matches!(args.get(i).map(|s| s.as_str()), Some("--dry-run" | "--assume-mounted")) {
		if args[i] == "--dry-run" { dry = true; } else { assume = true; }
		i += 1;
	}
	if args.len() < i + 6 {
		eprintln!("Usage: {} [--dry-run] [--assume-mounted] <device> <snapshots> <clean> <mode> <persist_root> <spec_file> [name=mount ...]", args[0]);
		process::exit(1);
	}
	let run = Run::new(dry);
	let dev = &args[i];
	let snaps = norm_subvol(&args[i + 1]).unwrap_or_else(|e| run.die(&e));
	let clean = args[i + 2].trim().to_string();
	let mode = args[i + 3].as_str();
	let persist = norm_subvol(&args[i + 4]).unwrap_or_else(|e| run.die(&e));
	let specs = read_specs(&args[i + 5]).unwrap_or_else(|e| run.die(&e));
	let vols = volumes(&run, &args[i + 6..], &specs);
	println!("{} Mode={mode} dry_run={} device={dev} subvolumes={}", run.e(), run.dry, vols.iter().map(|v| v.name.as_str()).collect::<Vec<_>>().join(" "));
	if mode == MODE_DISABLED { println!("{} Semipermeable membrane disabled; skipping", run.e()); return; }
	if run.dry && !assume { println!("{} DRY add --assume-mounted to walk mounted test roots", run.e()); return; }
	let _umount = mount_top(&run, dev, assume);
	namespaces(&run, &persist);
	for vol in &vols {
		match mode {
			MODE_CONVERGE | MODE_RESET | MODE_PREPARE_ONLY => reset(&run, vol, &snaps, &clean, &persist, &specs),
			MODE_SNAPSHOT_ONLY => rotate(&run, &snaps, &vol.name, &run.top(&vol.name)),
			MODE_RESTORE_A => {
				let live = run.top(&vol.name);
				let next = run.top(&format!("{snaps}/{}/{}", vol.name, NEXT));
				snap(&run, &run.top(&format!("{snaps}/{}/{}", vol.name, A)), &next, false);
				publish(&run, vol, &live, &next);
			}
			_ => run.die(&format!("Unknown mode: {mode}")),
		}
	}
	run.req(&["btrfs", "filesystem", "sync", &run.top]);
	println!("{} Semipermeable membrane complete", run.e());
}
