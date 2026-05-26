use std::{
	env, fs,
	path::{Path, PathBuf},
	process::{self, Command, Output},
};

const SOURCE: &str = "scripts/lib/semipermeable_membrane.rs";
const CASES: &str = "scripts/lib/test/semipermeable_membrane";
const SNAPSHOTS: &str = "@snapshots";
const CLEAN: &str = "CLEAN";
const PERSIST: &str = "@persist";

type R<T> = Result<T, String>;

#[derive(Default)]
struct Case {
	name: String,
	image: String,
	spec: Vec<String>,
	volumes: Vec<String>,
	runs: Vec<String>,
	setup_subvolumes: Vec<String>,
	clean: Vec<(String, String)>,
	live: Vec<(String, String)>,
	setup: Vec<(String, String)>,
	expect: Expect,
}

#[derive(Default)]
struct Expect {
	subvolumes: Vec<String>,
	readonly: Vec<String>,
	exists: Vec<String>,
	missing: Vec<String>,
	files: Vec<String>,
	equal: Vec<String>,
	direct_mounts: Vec<String>,
}

fn main() {
	if let Err(e) = real_main() {
		eprintln!("ERR {e}");
		process::exit(1);
	}
}

fn real_main() -> R<()> {
	let mut binary = None;
	let mut cases_dir = PathBuf::from(CASES);
	let mut keep = false;
	let mut selected = Vec::new();
	let mut args = env::args().skip(1);
	while let Some(arg) = args.next() {
		match arg.as_str() {
			"--binary" => binary = args.next().map(PathBuf::from),
			"--cases" => cases_dir = args.next().map(PathBuf::from).ok_or("--cases needs a path")?,
			"--keep" => keep = true,
			_ => selected.push(arg),
		}
	}
	require_root()?;
	need(&["btrfs", "losetup", "mkfs.btrfs", "mount", "umount", "truncate"])?;
	let root = env::current_dir().map_err(|e| e.to_string())?;
	let work = env::temp_dir().join(format!("semipermeable-membrane-test-{}", process::id()));
	rm_dir(&work);
	fs::create_dir_all(&work).map_err(|e| e.to_string())?;
	let binary = match binary {
		Some(path) => path,
		None => compile(&root, &work)?,
	};
	let cases = load_cases(&cases_dir, &selected)?;
	let mut failed = false;
	for case_path in cases {
		let case = parse_case(&case_path)?;
		match run_case(&binary, &work, &case) {
			Ok(()) => println!("PASS {}", case.name),
			Err(e) => {
				failed = true;
				eprintln!("FAIL {}\n{e}", case.name);
			}
		}
	}
	if !keep && !failed { rm_dir(&work); }
	if failed { Err(format!("kept state in {}", work.display())) } else { Ok(()) }
}

fn require_root() -> R<()> {
	if out(&["id", "-u"])?.trim() == "0" { return Ok(()); }
	let exe = env::current_exe().map_err(|e| e.to_string())?;
	let python = if Path::new("/run/current-system/sw/bin/python3").exists() {
		"/run/current-system/sw/bin/python3"
	} else {
		"python3"
	};
	let mut args = vec![
		"-n".to_string(),
		python.to_string(),
		"-c".to_string(),
		"import os,sys; os.execv(sys.argv[1], sys.argv[1:])".to_string(),
		exe.display().to_string(),
	];
	args.extend(env::args().skip(1));
	let status = Command::new("sudo").args(args).status().map_err(|e| e.to_string())?;
	process::exit(status.code().unwrap_or(1));
}

fn need(commands: &[&str]) -> R<()> {
	for command in commands {
		ok(&["which", command]).map_err(|_| format!("missing {command}"))?;
	}
	Ok(())
}

fn compile(root: &Path, work: &Path) -> R<PathBuf> {
	let binary = work.join("semipermeable_membrane");
	ok(&[
		"rustc", "--edition", "2021", "-D", "warnings", "-O", "-o",
		&binary.display().to_string(), &root.join(SOURCE).display().to_string(),
	])?;
	Ok(binary)
}

fn load_cases(dir: &Path, selected: &[String]) -> R<Vec<PathBuf>> {
	let mut cases = Vec::new();
	if selected.is_empty() {
		for entry in fs::read_dir(dir).map_err(|e| format!("read {}: {e}", dir.display()))? {
			let path = entry.map_err(|e| e.to_string())?.path();
			if path.join("case.toml").is_file() { cases.push(path.join("case.toml")); }
		}
	} else {
		for name in selected {
			cases.push(dir.join(name).join("case.toml"));
		}
	}
	cases.sort();
	Ok(cases)
}

fn parse_case(path: &Path) -> R<Case> {
	let text = fs::read_to_string(path).map_err(|e| format!("read {}: {e}", path.display()))?;
	let mut case = Case { image: "768M".to_string(), ..Default::default() };
	let mut section = String::new();
	let mut array_key = String::new();
	for raw in text.lines() {
		let line = raw.trim();
		if line.is_empty() || line.starts_with('#') { continue; }
		if !array_key.is_empty() {
			if line == "]" { array_key.clear(); continue; }
			push_array(&mut case, &section, &array_key, unquote(line.trim_end_matches(','))?)?;
			continue;
		}
		if line.starts_with('[') && line.ends_with(']') {
			section = line[1..line.len() - 1].to_string();
			continue;
		}
		let (key, value) = line.split_once('=').ok_or_else(|| format!("bad line in {}: {line}", path.display()))?;
		let key = key_name(key.trim())?;
		let value = value.trim();
		if value == "[" {
			array_key = key;
		} else if value.starts_with('[') {
			for item in parse_array(value)? { push_array(&mut case, &section, &key, item)?; }
		} else {
			put_scalar(&mut case, &section, &key, unquote(value)?)?;
		}
	}
	if case.name.is_empty() {
		case.name = path.parent().and_then(Path::file_name).unwrap().to_string_lossy().to_string();
	}
	if case.runs.is_empty() { case.runs.push("reset".to_string()); }
	Ok(case)
}

fn put_scalar(case: &mut Case, section: &str, key: &str, value: String) -> R<()> {
	match section {
		"" if key == "name" => case.name = value,
		"" if key == "image" => case.image = value,
		"clean" => case.clean.push((key.to_string(), value)),
		"live" => case.live.push((key.to_string(), value)),
		"setup" => case.setup.push((key.to_string(), value)),
		_ => return Err(format!("unknown scalar {section}.{key}")),
	}
	Ok(())
}

fn push_array(case: &mut Case, section: &str, key: &str, value: String) -> R<()> {
	match (section, key) {
		("", "spec") => case.spec.push(value),
		("", "volumes") => case.volumes.push(value),
		("", "runs") => case.runs.push(value),
		("", "setup_subvolumes") => case.setup_subvolumes.push(value),
		("expect", "subvolumes") => case.expect.subvolumes.push(value),
		("expect", "readonly") => case.expect.readonly.push(value),
		("expect", "exists") => case.expect.exists.push(value),
		("expect", "missing") => case.expect.missing.push(value),
		("expect", "files") => case.expect.files.push(value),
		("expect", "equal") => case.expect.equal.push(value),
		("expect", "direct_mounts") => case.expect.direct_mounts.push(value),
		_ => return Err(format!("unknown array {section}.{key}")),
	}
	Ok(())
}

fn parse_array(value: &str) -> R<Vec<String>> {
	let mut items = Vec::new();
	let inner = value.trim().strip_prefix('[').and_then(|s| s.strip_suffix(']')).ok_or("bad array")?;
	for item in inner.split(',').map(str::trim).filter(|s| !s.is_empty()) {
		items.push(unquote(item)?);
	}
	Ok(items)
}

fn unquote(value: &str) -> R<String> {
	let value = value.trim();
	if !value.starts_with('"') || !value.ends_with('"') {
		return Err(format!("expected quoted string: {value}"));
	}
	let mut out = String::new();
	let mut chars = value[1..value.len() - 1].chars();
	while let Some(ch) = chars.next() {
		if ch != '\\' { out.push(ch); continue; }
		match chars.next().ok_or("dangling escape")? {
			'n' => out.push('\n'),
			't' => out.push('\t'),
			'\\' => out.push('\\'),
			'"' => out.push('"'),
			ch => return Err(format!("bad escape: {ch}")),
		}
	}
	Ok(out)
}

fn key_name(value: &str) -> R<String> {
	if value.starts_with('"') { unquote(value) } else { Ok(value.to_string()) }
}

fn run_case(binary: &Path, work: &Path, case: &Case) -> R<()> {
	let dir = work.join(&case.name);
	rm_dir(&dir);
	fs::create_dir_all(&dir).map_err(|e| e.to_string())?;
	let image = dir.join("disk.img");
	let setup = dir.join("setup");
	let run_mount = dir.join("run");
	let verify = dir.join("verify");
	let loopdev;
	ok(&["truncate", "-s", &case.image, &image.display().to_string()])?;
	loopdev = out(&["losetup", "--find", "--show", &image.display().to_string()])?;
	let loopdev = loopdev.trim().to_string();
	let result = (|| -> R<()> {
		ok(&["mkfs.btrfs", "-q", &loopdev])?;
		mount_top(&loopdev, &setup)?;
		setup_case(&setup, case)?;
		umount(&setup)?;
		let spec = dir.join("spec.tsv");
		fs::write(&spec, case.spec.join("\n") + "\n").map_err(|e| e.to_string())?;
		for run in &case.runs { run_membrane(binary, &loopdev, &run_mount, &spec, case, run)?; }
		mount_top(&loopdev, &verify)?;
		fs::write(dir.join("actual.json"), state_json(&verify)?).map_err(|e| e.to_string())?;
		verify_case(&verify, &loopdev, &dir, case)?;
		umount(&verify)?;
		Ok(())
	})();
	let _ = quiet(&["umount", "-R", &verify.display().to_string()]);
	let _ = quiet(&["umount", "-R", &run_mount.display().to_string()]);
	let _ = quiet(&["umount", "-R", &setup.display().to_string()]);
	let _ = quiet(&["losetup", "-d", &loopdev]);
	result
}

fn setup_case(top: &Path, case: &Case) -> R<()> {
	let mut volumes = volumes(case);
	volumes.sort();
	volumes.dedup();
	mk_subvol(top, SNAPSHOTS)?;
	for vol in &volumes {
		mk_subvol(top, &format!("{SNAPSHOTS}/{vol}"))?;
		mk_subvol(top, vol)?;
	}
	for (path, value) in &case.clean { write_item(top, path, value)?; }
	for vol in &volumes {
		ok(&[
			"btrfs", "subvolume", "snapshot", "-r",
			&top.join(vol).display().to_string(),
			&top.join(SNAPSHOTS).join(vol).join(CLEAN).display().to_string(),
		])?;
	}
	for (path, value) in &case.live { write_item(top, path, value)?; }
	for subvol in &case.setup_subvolumes { mk_subvol(top, subvol)?; }
	for (path, value) in &case.setup { write_item(top, path, value)?; }
	Ok(())
}

fn volumes(case: &Case) -> Vec<String> {
	let mut volumes = Vec::new();
	for line in &case.spec {
		if let Some((vol, _)) = line.split_once('\t') { volumes.push(vol.to_string()); }
	}
	for volume in &case.volumes {
		if let Some((vol, _)) = volume.split_once('=') { volumes.push(vol.to_string()); }
	}
	for (path, _) in case.clean.iter().chain(case.live.iter()) {
		if let Some(vol) = path.split('/').next() { volumes.push(vol.to_string()); }
	}
	volumes
}

fn run_membrane(binary: &Path, loopdev: &str, mount: &Path, spec: &Path, case: &Case, run: &str) -> R<()> {
	let mut args = Vec::new();
	let expect_fail = run.trim_start().starts_with('!');
	let run = run.trim_start().trim_start_matches('!').trim();
	let mut parts: Vec<&str> = run.split_whitespace().collect();
	let dry = parts.first() == Some(&"--dry-run");
	if dry {
		args.push("--dry-run".to_string());
		parts.remove(0);
	}
	let mode = parts.first().copied().unwrap_or("reset");
	args.extend([
		loopdev.to_string(),
		SNAPSHOTS.to_string(),
		CLEAN.to_string(),
		mode.to_string(),
		PERSIST.to_string(),
		spec.display().to_string(),
	]);
	args.extend(case.volumes.iter().cloned());
	let output = Command::new(binary)
		.args(args)
		.env("SEMIPERMEABLE_MEMBRANE_MOUNT_PATH", mount)
		.output()
		.map_err(|e| e.to_string())?;
	if output.status.success() && !expect_fail {
		Ok(())
	} else if !output.status.success() && expect_fail {
		Ok(())
	} else if expect_fail {
		Err(format!("run {run:?} succeeded but should have failed\n{}", output_text(&output)))
	} else {
		Err(format!("run {run:?} failed\n{}", output_text(&output)))
	}
}

fn verify_case(top: &Path, loopdev: &str, dir: &Path, case: &Case) -> R<()> {
	for path in &case.expect.subvolumes { assert_subvol(top.join(path))?; }
	for path in &case.expect.readonly { assert_readonly(top.join(path))?; }
	for path in &case.expect.exists { assert_exists(top.join(path))?; }
	for path in &case.expect.missing { assert_missing(top.join(path))?; }
	for entry in &case.expect.files {
		let (path, value) = entry.split_once('=').ok_or_else(|| format!("bad expected file: {entry}"))?;
		assert_file(top.join(path), value)?;
	}
	for entry in &case.expect.equal {
		let (left, right) = entry.split_once("==").ok_or_else(|| format!("bad equality: {entry}"))?;
		let left = fs::read(top.join(left.trim())).map_err(|e| e.to_string())?;
		let right = fs::read(top.join(right.trim())).map_err(|e| e.to_string())?;
		if left != right { return Err(format!("files differ: {entry}")); }
	}
	for (index, entry) in case.expect.direct_mounts.iter().enumerate() {
		let (subvol, child) = entry.split_once(':').ok_or_else(|| format!("bad direct mount: {entry}"))?;
		let mount = dir.join(format!("direct-{index}"));
		fs::create_dir_all(&mount).map_err(|e| e.to_string())?;
		ok(&["mount", "-t", "btrfs", "-o", &format!("subvol={subvol}"), loopdev, &mount.display().to_string()])?;
		let result = assert_exists(mount.join(child));
		let _ = quiet(&["umount", &mount.display().to_string()]);
		result?;
	}
	Ok(())
}

fn write_item(top: &Path, rel: &str, value: &str) -> R<()> {
	let path = top.join(rel);
	if value == "delete" {
		rm_any(&path);
		return Ok(());
	}
	if value == "dir" {
		fs::create_dir_all(&path).map_err(|e| e.to_string())?;
		return Ok(());
	}
	if let Some(size) = value.strip_prefix("blob:") {
		write_blob(&path, size.parse::<usize>().map_err(|e| e.to_string())?)
	} else {
		if let Some(parent) = path.parent() { fs::create_dir_all(parent).map_err(|e| e.to_string())?; }
		fs::write(path, value).map_err(|e| e.to_string())
	}
}

fn write_blob(path: &Path, size: usize) -> R<()> {
	if let Some(parent) = path.parent() { fs::create_dir_all(parent).map_err(|e| e.to_string())?; }
	let mut data = Vec::with_capacity(size.min(1024 * 1024));
	for i in 0..size.min(1024 * 1024) { data.push((i % 251) as u8); }
	let mut file = fs::File::create(path).map_err(|e| e.to_string())?;
	let mut left = size;
	while left > 0 {
		let n = left.min(data.len());
		std::io::Write::write_all(&mut file, &data[..n]).map_err(|e| e.to_string())?;
		left -= n;
	}
	Ok(())
}

fn state_json(top: &Path) -> R<String> {
	let mut rows = Vec::new();
	walk_state(top, top, &mut rows)?;
	rows.sort();
	Ok(format!("[\n{}\n]\n", rows.join(",\n")))
}

fn walk_state(top: &Path, path: &Path, rows: &mut Vec<String>) -> R<()> {
	for entry in fs::read_dir(path).map_err(|e| e.to_string())? {
		let path = entry.map_err(|e| e.to_string())?.path();
		let rel = path.strip_prefix(top).map_err(|e| e.to_string())?.display().to_string();
		let meta = fs::symlink_metadata(&path).map_err(|e| e.to_string())?;
		let kind = if meta.is_dir() && is_subvol(&path) { "subvolume" } else if meta.is_dir() { "dir" } else { "file" };
		rows.push(format!("  {{\"path\":\"{}\",\"kind\":\"{}\"}}", json(&rel), kind));
		if meta.is_dir() { walk_state(top, &path, rows)?; }
	}
	Ok(())
}

fn json(s: &str) -> String {
	s.chars().flat_map(|ch| match ch {
		'\\' => "\\\\".chars().collect::<Vec<_>>(),
		'"' => "\\\"".chars().collect(),
		'\n' => "\\n".chars().collect(),
		'\t' => "\\t".chars().collect(),
		ch => vec![ch],
	}).collect()
}

fn mk_subvol(top: &Path, rel: &str) -> R<()> {
	let path = top.join(rel);
	if path.exists() { return Ok(()); }
	if let Some(parent) = path.parent() { fs::create_dir_all(parent).map_err(|e| e.to_string())?; }
	ok(&["btrfs", "subvolume", "create", &path.display().to_string()])
}

fn mount_top(loopdev: &str, mount: &Path) -> R<()> {
	fs::create_dir_all(mount).map_err(|e| e.to_string())?;
	ok(&["mount", "-t", "btrfs", "-o", "subvolid=5,user_subvol_rm_allowed", loopdev, &mount.display().to_string()])
}

fn umount(mount: &Path) -> R<()> {
	ok(&["umount", &mount.display().to_string()])
}

fn assert_subvol(path: PathBuf) -> R<()> {
	ok(&["btrfs", "subvolume", "show", &path.display().to_string()])
}

fn assert_readonly(path: PathBuf) -> R<()> {
	let stdout = out(&["btrfs", "property", "get", "-ts", &path.display().to_string(), "ro"])?;
	if stdout.contains("ro=true") { Ok(()) } else { Err(format!("not readonly: {}", path.display())) }
}

fn is_subvol(path: &Path) -> bool {
	quiet(&["btrfs", "subvolume", "show", &path.display().to_string()]).is_ok()
}

fn assert_exists(path: PathBuf) -> R<()> {
	if path.exists() { Ok(()) } else { Err(format!("missing: {}", path.display())) }
}

fn assert_missing(path: PathBuf) -> R<()> {
	if path.exists() { Err(format!("should be missing: {}", path.display())) } else { Ok(()) }
}

fn assert_file(path: PathBuf, value: &str) -> R<()> {
	let actual = fs::read_to_string(&path).map_err(|e| format!("read {}: {e}", path.display()))?;
	if actual == value { Ok(()) } else { Err(format!("{} got {actual:?}, want {value:?}", path.display())) }
}

fn ok(args: &[&str]) -> R<()> {
	let output = command(args)?;
	if output.status.success() { Ok(()) } else { Err(format!("{}\n{}", args.join(" "), output_text(&output))) }
}

fn out(args: &[&str]) -> R<String> {
	let output = command(args)?;
	if output.status.success() { Ok(String::from_utf8_lossy(&output.stdout).to_string()) }
	else { Err(format!("{}\n{}", args.join(" "), output_text(&output))) }
}

fn quiet(args: &[&str]) -> R<()> {
	command(args).and_then(|out| if out.status.success() { Ok(()) } else { Err(output_text(&out)) })
}

fn command(args: &[&str]) -> R<Output> {
	Command::new(args[0]).args(&args[1..]).output().map_err(|e| e.to_string())
}

fn output_text(output: &Output) -> String {
	format!(
		"stdout:\n{}\nstderr:\n{}",
		String::from_utf8_lossy(&output.stdout),
		String::from_utf8_lossy(&output.stderr),
	)
}

fn rm_dir(path: &Path) {
	let _ = fs::remove_dir_all(path);
}

fn rm_any(path: &Path) {
	match fs::symlink_metadata(path) {
		Ok(meta) if meta.is_dir() => rm_dir(path),
		Ok(_) => { let _ = fs::remove_file(path); }
		Err(_) => {}
	}
}
