// BTRFS subvolume reset for NixOS immutable boot
// Usage: immutability <device> <snap_name> <clean_name> <mode> [name=mp:filter ...]
use std::{collections::BTreeSet, env, fs, path::Path, process::{self, Command}, thread, time::Instant};

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

fn die(start: &Instant, message: &str) -> ! {
	eprintln!("{} ERR {message}", elapsed(start));
	run(start, &["umount", "-R", MOUNT_PATH]);
	process::exit(1);
}

fn require(start: &Instant, args: &[&str]) {
	if !run(start, args) { die(start, &format!("Failed: {}", args.join(" "))); }
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

fn parse_persistent_paths(filter_path: &str) -> Vec<String> {
	fs::read_to_string(filter_path).unwrap_or_default().lines()
		.filter(|line| line.starts_with("+ /"))
		.map(|line| line[2..].trim().to_string())
		.filter(|path| !path.ends_with('/') && !path.ends_with("/**"))
		.collect()
}

fn merge_with_clean(current_root: &str, clean_root: &str, directory: &str, persistent_paths: &[String]) {
	let current_directory = format!("{current_root}{directory}");
	let clean_directory = format!("{clean_root}{directory}");
	let mut entries = BTreeSet::new();
	if let Ok(read_dir) = fs::read_dir(&current_directory) {
		for entry in read_dir.flatten() { entries.insert(entry.file_name().to_string_lossy().to_string()); }
	}
	if let Ok(read_dir) = fs::read_dir(&clean_directory) {
		for entry in read_dir.flatten() { entries.insert(entry.file_name().to_string_lossy().to_string()); }
	}

	for name in &entries {
		let relative_path = if directory.is_empty() { format!("/{name}") } else { format!("{directory}/{name}") };
		let current_path = format!("{current_root}{relative_path}");
		let clean_path = format!("{clean_root}{relative_path}");

		let is_persistent = persistent_paths.iter().any(|path| *path == relative_path);
		let is_ancestor = persistent_paths.iter().any(|path| path.starts_with(&format!("{relative_path}/")));

		if is_persistent { continue; }
		if is_ancestor {
			if !Path::new(&current_path).exists() { fs::create_dir_all(&current_path).ok(); }
			merge_with_clean(current_root, clean_root, &relative_path, persistent_paths);
			continue;
		}

		if let Ok(metadata) = fs::symlink_metadata(&current_path) {
			if metadata.is_dir() { fs::remove_dir_all(&current_path).ok(); }
			else { fs::remove_file(&current_path).ok(); }
		}
		if Path::new(&clean_path).exists() {
			Command::new("cp").args(["--reflink=always", "-a", &clean_path, &current_path]).output().ok();
		}
	}
}

fn reset(start: &Instant, name: &str, filter_path: &str, snapshots_name: &str, clean_name: &str) {
	let volume = format!("{MOUNT_PATH}/{name}");
	let snapshots = format!("{MOUNT_PATH}/{snapshots_name}/{name}");
	let clean = format!("{snapshots}/{clean_name}");
	let previous = format!("{snapshots}/PREVIOUS");
	let penultimate = format!("{snapshots}/PENULTIMATE");
	let current = format!("{snapshots}/CURRENT");

	if Path::new(&current).is_dir() && !Path::new(&format!("{current}/.boot-ready")).is_file() {
		eprintln!("{} WRN Incomplete boot (missing .boot-ready)", elapsed(start));
		delete_subvolume(start, &current);
	}
	if !Path::new(&clean).is_dir() { die(start, &format!("CLEAN missing: {clean}")); }

	if !Path::new(&penultimate).is_dir() { create_snapshot(start, &clean, &penultimate); }
	if !Path::new(&previous).is_dir() { create_snapshot(start, &clean, &previous); }
	create_snapshot(start, &previous, &penultimate);
	create_snapshot(start, &volume, &previous);
	create_snapshot(start, &previous, &current);
	require(start, &["btrfs", "property", "set", "-ts", &current, "ro", "false"]);

	let persistent_paths = parse_persistent_paths(filter_path);
	println!("{} Merging {} persistent paths with clean state", elapsed(start), persistent_paths.len());
	merge_with_clean(&current, &clean, "", &persistent_paths);
	let _ = fs::write(format!("{current}/.boot-ready"), "");

	create_snapshot(start, &current, &volume);
}

fn snapshot_only(start: &Instant, name: &str, snapshots_name: &str, clean_name: &str) {
	let volume = format!("{MOUNT_PATH}/{name}");
	let snapshots = format!("{MOUNT_PATH}/{snapshots_name}/{name}");
	let clean = format!("{snapshots}/{clean_name}");
	let previous = format!("{snapshots}/PREVIOUS");
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

	if mode == "disabled" {
		println!("{} Immutability disabled; skipping", elapsed(&start));
	} else {
		let dispatch = |name: &str, filter_path: &str| match mode {
			"reset" => { println!("{} Resetting {name}", elapsed(&start)); reset(&start, name, filter_path, snapshots_name, clean_name); }
			"snapshot-only" => { println!("{} Snapshot-only {name}", elapsed(&start)); snapshot_only(&start, name, snapshots_name, clean_name); }
			"restore-previous" => restore(&start, name, snapshots_name, "PREVIOUS"),
			"restore-penultimate" => restore(&start, name, snapshots_name, "PENULTIMATE"),
			_ => die(&start, &format!("Unknown mode: {mode}")),
		};
		if pairs.len() > 1 {
			let owned: Vec<(String, String)> = pairs.iter().map(|(name, filter)| (name.to_string(), filter.to_string())).collect();
			thread::scope(|scope| { for (name, filter) in &owned { scope.spawn(|| dispatch(name, filter)); } });
		} else {
			for (name, filter_path) in &pairs { dispatch(name, filter_path); }
		}
	}

	if mode != "disabled" { require(&start, &["btrfs", "filesystem", "sync", MOUNT_PATH]); }
	run(&start, &["umount", MOUNT_PATH]);
	println!("{} Immutability complete", elapsed(&start));
}
