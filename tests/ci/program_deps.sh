#!/usr/bin/env bash
# ci/btrc-deps.py over a small tree: a program is made from what its imports reach, across packages,
# with their manifests and headers, and from nothing else.
set -euo pipefail

root=$(cd "$(dirname "$0")/../.." && pwd)
temporary=$(mktemp -d)
trap 'rm -rf "$temporary"' EXIT

mkdir -p "$temporary/app/lib" "$temporary/shared/net" "$temporary/other"
cat > "$temporary/app/btrc.toml" <<'EOF'
manifest-version = 1
[package]
name = "app"
[dependencies]
shared = { path = "../shared" }
EOF
printf 'manifest-version = 1\n[package]\nname = "shared"\n' > "$temporary/shared/btrc.toml"
printf '{}' > "$temporary/app/btrc.lock"
printf 'import Library.Console;\nimport ./lib/Helper.btrc;\nimport shared.net.Wire { Wire };\nimport app.lib.Named;\n' > "$temporary/app/Main.btrc"
printf 'import ./Deeper.btrc;\n' > "$temporary/app/lib/Helper.btrc"
: > "$temporary/app/lib/Deeper.btrc"
: > "$temporary/app/lib/Named.btrc"
: > "$temporary/app/lib/Unused.btrc"
: > "$temporary/app/native.h"
: > "$temporary/shared/net/Wire.btrc"
: > "$temporary/shared/net/Unused.btrc"
: > "$temporary/other/Elsewhere.btrc"

cd "$temporary"
listed=$(python3 "$root/ci/btrc-deps.py" main=app/Main.btrc)
expected='DEPS_main := app/Main.btrc app/btrc.toml app/lib/Deeper.btrc app/lib/Helper.btrc app/lib/Named.btrc app/native.h shared/btrc.toml shared/net/Wire.btrc'
[[ "$listed" == "$expected" ]] || { printf 'expected: %s\nlisted:   %s\n' "$expected" "$listed"; exit 1; }

cd "$root"
power=$(python3 ci/btrc-deps.py power_test=tests/core/PowerTests.btrc)
[[ "$power" == *" tests/core/PowerTests.btrc"* ]] || { echo "a program is not made from its own entry"; exit 1; }
if [[ "$power" == *"config/pkgs/utils/drive/"* || "$power" == *"tests/drive/"* ]]; then echo "the power suite is listed as made from Drive"; exit 1; fi

echo "PASS: a program is made from what its imports reach, their manifests and headers, and nothing else"
