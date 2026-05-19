// BTRFS subvolume reset for NixOS immutable boot
// Usage: immutability <device> <snap_name> <clean_name> <mode> [name=mp:filter ...]
//
// Key change from original: reset() now snapshots CLEAN→CURRENT (not PREVIOUS→CURRENT),
// then overlays only the persistent paths from PREVIOUS. This avoids walking the entire
// volume tree and instead touches only O(persistent_paths) entries.
use std::{env, fs, path::Path, process::{self, Command}, thread, time::Instant};

const MOUNT_PATH: &str = "/mnt";

fn elapsed(start: &Instant) -> String { format!("[{:.2}s]", start.elapsed().as_secs_f64()) }

fn run(start: &Instant, args: &[&str]) -> bool {
	println!("{} {}", elapsed(start), args.join(" "));
	let output = Command::new(args[0]).args(&args[1..]).output().expect("exec failed");
	for line in String::from_utf8_lossy(&output.stdout).lines().filter(|l| !l.is_empty()) {
		println!("{}   {line}", elapsed(start));
	}
	for line in String::from_utf8_lossy(&output.stderr).lines().filter(|l| !l.is_empty()) {
		eprintln!("{}   {line}", elapsed(start));
	}
	output.status.success()
}

// Panics rather than process::exit so that thread::scope can wait for sibling threads
// to finish their operations before the panic propagates to main. The Unmounter RAII
// guard in main then runs umount on the way out, regardless of the exit path.
fn die(start: &Instant, message: &str) -> ! {
	eprintln!("{} ERR {message}", elapsed(start));
	panic!("immutability fatal: {message}");
}

fn require(start: &Instant, args: &[&str]) {
	if !run(start, args) { die(start, &format!("Failed: {}", args.join(" "))); }
}

// Umounts MOUNT_PATH on drop, covering both normal exit and die() panic paths.
struct Unmounter<'a>(&'a Instant);
impl Drop for Unmounter<'_> {
	fn drop(&mut self) { run(self.0, &["umount", "-R", MOUNT_PATH]); }
}

fn delete_subvolume(start: &Instant, path: &str) {
	if !Path::new(path).is_dir() { return; }
	let output = Command::new("btrfs").args(["subvolume", "list", "-o", path]).output().unwrap();
	for line in String::from_utf8_lossy(&output.stdout).lines() {
		if let Some(child_path) = line.split(" path ").nth(1) {
			delete_subvolume(start, &format!("{MOUNT_PATH}/{child_path}"));
		}
	}
	require(start, &["btrfs", "subvolume", "delete", path]);
}

fn create_snapshot(start: &Instant, source: &str, destination: &str) {
	if !Path::new(source).is_dir() { die(start, &format!("Source missing: {source}")); }
	delete_subvolume(start, destination);
	require(start, &["btrfs", "subvolume", "snapshot", source, destination]);
}

fn parse_persistent_paths(start: &Instant, filter_path: &str) -> Vec<String> {
	if filter_path.is_empty() { return vec![]; }
	let content = match fs::read_to_string(filter_path) {
		Ok(c)  => c,
		Err(e) => die(start, &format!("Cannot read filter {filter_path}: {e}")),
	};
	content.lines()
		.filter(|line| line.starts_with("+ /"))
		.map(|line| line[2..].trim().to_string())
		.filter(|path| !path.ends_with('/') && !path.ends_with("/**"))
		.collect()
}

// Copies each persistent path from previous_root into current_root, replacing whatever
// CLEAN provided. If a path is absent from previous (user deleted it), leaves CLEAN's
// version in place. Creates parent directories as needed for paths not present in CLEAN.
fn overlay_persistent(start: &Instant, previous_root: &str, current_root: &str, persistent_paths: &[String]) {
	println!("{} Overlaying {} persistent paths from previous state", elapsed(start), persistent_paths.len());
	for path in persistent_paths {
		let src = format!("{previous_root}{path}");
		let dst = format!("{current_root}{path}");

		// Path absent from previous state: leave whatever CLEAN provided (or nothing).
		// Use symlink_metadata so dangling symlinks are treated as present.
		if fs::symlink_metadata(&src).is_err() { continue; }

		// Parent may not exist in CLEAN (e.g. ~/.cert/ never existed in the clean image).
		if let Some(parent) = Path::new(&dst).parent() {
			if !parent.exists() {
				if let Err(e) = fs::create_dir_all(parent) {
					die(start, &format!("Failed to create parent for {dst}: {e}"));
				}
			}
		}

		// Remove CLEAN's version so cp -a can place the PREVIOUS version at the exact path.
		// (cp -a src dst with dst existing as a dir would copy src *into* dst — wrong.)
		match fs::symlink_metadata(&dst) {
			Ok(meta) if meta.is_dir() => { fs::remove_dir_all(&dst).ok(); }
			Ok(_) => { fs::remove_file(&dst).ok(); }
			Err(_) => {}
		}

		require(start, &["cp", "--reflink=always", "-a", &src, &dst]);
	}
}

fn reset(start: &Instant, name: &str, filter_path: &str, snapshots_name: &str, clean_name: &str) {
	let volume      = format!("{MOUNT_PATH}/{name}");
	let snapshots   = format!("{MOUNT_PATH}/{snapshots_name}/{name}");
	let clean       = format!("{snapshots}/{clean_name}");
	let previous    = format!("{snapshots}/PREVIOUS");
	let penultimate = format!("{snapshots}/PENULTIMATE");
	let current     = format!("{snapshots}/CURRENT");

	if Path::new(&current).is_dir() && !Path::new(&format!("{current}/.boot-ready")).is_file() {
		eprintln!("{} WRN Incomplete boot (missing .boot-ready)", elapsed(start));
		delete_subvolume(start, &current);
	}
	if !Path::new(&clean).is_dir() { die(start, &format!("CLEAN missing: {clean}")); }

	// Rotate bookkeeping snapshots: save the live volume as PREVIOUS before reset.
	if !Path::new(&penultimate).is_dir() { create_snapshot(start, &clean, &penultimate); }
	if !Path::new(&previous).is_dir() { create_snapshot(start, &clean, &previous); }
	create_snapshot(start, &previous, &penultimate);
	create_snapshot(start, &volume, &previous);

	// Build CURRENT from CLEAN (instant COW), then overlay persistent paths from PREVIOUS.
	// Original approach snapshotted PREVIOUS→CURRENT then walked the entire tree to reset
	// non-persistent entries to CLEAN. This approach avoids touching non-persistent entries
	// entirely: they come from CLEAN for free via the snapshot.
	create_snapshot(start, &clean, &current);
	require(start, &["btrfs", "property", "set", "-ts", &current, "ro", "false"]);

	let persistent_paths = parse_persistent_paths(start, filter_path);
	overlay_persistent(start, &previous, &current, &persistent_paths);

	if let Err(e) = fs::write(format!("{current}/.boot-ready"), "") {
		eprintln!("{} WRN Failed to write .boot-ready: {e}", elapsed(start));
	}

	create_snapshot(start, &current, &volume);
}

fn snapshot_only(start: &Instant, name: &str, snapshots_name: &str, clean_name: &str) {
	let volume      = format!("{MOUNT_PATH}/{name}");
	let snapshots   = format!("{MOUNT_PATH}/{snapshots_name}/{name}");
	let clean       = format!("{snapshots}/{clean_name}");
	let previous    = format!("{snapshots}/PREVIOUS");
	let penultimate = format!("{snapshots}/PENULTIMATE");
	if !Path::new(&clean).is_dir() { die(start, &format!("CLEAN missing: {clean}")); }
	if !Path::new(&penultimate).is_dir() { create_snapshot(start, &clean, &penultimate); }
	if !Path::new(&previous).is_dir() { create_snapshot(start, &clean, &previous); }
	create_snapshot(start, &previous, &penultimate);
	create_snapshot(start, &volume, &previous);
}

fn restore(start: &Instant, name: &str, snapshots_name: &str, label: &str) {
	let volume = format!("{MOUNT_PATH}/{name}");
	let source = format!("{MOUNT_PATH}/{snapshots_name}/{name}/{label}");
	if !Path::new(&source).is_dir() { die(start, &format!("Cannot restore: {source}")); }
	println!("{} Restoring {name} from {label}", elapsed(start));
	create_snapshot(start, &source, &volume);
}

fn main() {
	let start = Instant::now();
	let args: Vec<String> = env::args().collect();
	if args.len() < 5 {
		eprintln!("Usage: {} <device> <snapshots_name> <clean_name> <mode> [name=mount:filter ...]", args[0]);
		process::exit(1);
	}
	let (device, snapshots_name, clean_name, mode) = (&args[1], &args[2], &args[3], args[4].as_str());
	let pairs: Vec<(&str, &str)> = args[5..].iter().map(|arg| {
		let (name_mount, filter_path) = arg.rsplit_once(':').unwrap_or((arg, ""));
		(name_mount.split('=').next().unwrap(), filter_path)
	}).collect();

	println!("{} Mode={mode} device={device} subvolumes={}",
		elapsed(&start), pairs.iter().map(|pair| pair.0).collect::<Vec<_>>().join(" "));
	require(&start, &["mkdir", "-p", MOUNT_PATH]);
	require(&start, &["mount", "-t", "btrfs", "-o", "subvolid=5,user_subvol_rm_allowed", device, MOUNT_PATH]);

	// Guard umounts MOUNT_PATH on drop — covers both normal return and die() panic paths.
	let _unmount = Unmounter(&start);

	if mode == "disabled" {
		println!("{} Immutability disabled; skipping", elapsed(&start));
	} else {
		let dispatch = |name: &str, filter_path: &str| match mode {
			"reset"               => { println!("{} Resetting {name}", elapsed(&start)); reset(&start, name, filter_path, snapshots_name, clean_name); }
			"snapshot-only"       => { println!("{} Snapshot-only {name}", elapsed(&start)); snapshot_only(&start, name, snapshots_name, clean_name); }
			"restore-previous"    => restore(&start, name, snapshots_name, "PREVIOUS"),
			"restore-penultimate" => restore(&start, name, snapshots_name, "PENULTIMATE"),
			_ => die(&start, &format!("Unknown mode: {mode}")),
		};
		if pairs.len() > 1 {
			let owned: Vec<(String, String)> = pairs.iter().map(|(name, filter)| (name.to_string(), filter.to_string())).collect();
			// thread::scope waits for all threads before propagating any panic, so a die()
			// in one thread gives the sibling time to finish before the process exits.
			thread::scope(|scope| { for (name, filter) in &owned { scope.spawn(|| dispatch(name, filter)); } });
		} else {
			for (name, filter_path) in &pairs { dispatch(name, filter_path); }
		}
		require(&start, &["btrfs", "filesystem", "sync", MOUNT_PATH]);
	}

	println!("{} Immutability complete", elapsed(&start));
	// _unmount drops here → umount
}
