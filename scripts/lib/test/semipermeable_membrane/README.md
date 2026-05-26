# Semipermeable Membrane Tests

Each subfolder is one real Btrfs test case:

```text
test_name/
  case.toml
```

The Rust runner:

1. creates a fresh loop-backed Btrfs filesystem
2. loads `[clean]`, snapshots it as readonly `CLEAN`
3. overlays `[live]` and optional `[setup]`
4. runs `semipermeable_membrane` as a CLI
5. serializes final state to `actual.json` in the temp test directory
6. verifies `[expect]`

Supported case keys:

```toml
name = "happy_path"
runs = ["reset", "--dry-run reset", "disabled", "!reset"]
spec = ["@home\t/home\t/home/alex/.bash_history\tfile"]
volumes = ["@home=/tmp/not-mounted"]
setup_subvolumes = ["@snapshots/@home/NEXT"]

[clean]
"@home/file" = "clean\n"

[live]
"@home/file" = "user\n"

[setup]
"@snapshots/@home/NEXT/stale" = "stale\n"

[expect]
subvolumes = ["@persist"]
readonly = ["@snapshots/@home/A"]
files = ["@home/file=user\n"]
exists = ["@home/file"]
missing = ["@snapshots/@home/NEXT"]
equal = ["@persist/blob == @snapshots/@home/A/blob"]
direct_mounts = ["@persist/dirs/home!alex!Downloads:file"]
```

Special values: `"dir"`, `"delete"`, and `"blob:<bytes>"`.

Prefix a run with `!` when the CLI should fail.
