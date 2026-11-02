---
name: taste
description: Use when reviewing, writing, or refactoring code where elegance, idiom, API shape, naming, abstraction boundaries, or verification quality matter. Especially relevant for BTRC, NixOS, stdlib/compiler work, CLI tools, shell/process APIs, editor tooling, simulator tests, and full-stack systems where awkward call sites may indicate a deeper language, stdlib, or architecture problem.
metadata:
  short-description: Improve code taste without overfitting to examples
---

# Taste

Taste is not a checklist. It is the discipline of preserving intent density: the code should expose the domain operation with as little accidental machinery as possible.

When code feels awkward, do not only fix the visible call site. Ask which layer is failing:
- The call site may be using the wrong existing abstraction.
- The abstraction may have the wrong shape.
- The stdlib may be missing a primitive.
- The language may be missing a feature.
- The architecture may still preserve an obsolete concept.
- The tests may verify the wrong layer.

Examples are calibration, not commandments. Do not overfit to them.

## Operating Model

Before changing code, read enough surrounding code to infer the local style. Look for repeated choices in naming, imports, command execution, parser usage, CLI structure, module boundaries, and tests.

Then search for the pattern, not only the instance the user named. Use `rg` across the relevant tree. If one ugly site exists, assume nearby siblings may share the same bad shape.

Prefer the smallest change that fixes the pattern at the right layer.

## What Good Looks Like

Good code makes the real operation obvious.

Names are domain-shaped:
- Prefer names like `help`, `json`, `relativePath`, `hardware`, `recovery`, `volume`.
- Avoid vague or cramped names like `h`, `obj`, `relPath`, `cfg`, `outerConfig` unless the surrounding code clearly uses that style.

APIs are honest:
- If the operation is shell syntax, use a readable command string with explicit quoting.
- If the operation is reusable argv construction, use the command builder.
- Do not use builder ceremony for literals.
- Do not hide secrets in command strings. Use stdin, fd, files with correct permissions, or a purpose-built API.

Parsing is structured:
- Use JSON/TOML/stdlib parsers instead of substring scanning.
- Use path helpers instead of string path hacks.
- Use CLI helpers or declarative command specs instead of scattered `args.count()` checks when the pattern repeats.

Code is current:
- Delete dead policy after refactors.
- Remove references to directories, modes, commands, modules, or boot paths that no longer exist.
- Rename files when their role is unclear.

Imports are intentional:
- Avoid `import std.*` when a file only needs specific stdlib modules.
- Keep imports close to actual dependencies.
- Treat import cleanup as part of architecture, not formatting.

Comments must earn their place:
- Do not narrate obvious code.
- Use comments for external quirks, invariants, security constraints, or non-obvious tradeoffs.
- Prefer clearer code and tests over explanatory clutter.

## BTRC/NixOS Biases

BTRC is a language and stdlib, not just a pile of scripts. If NixOS code is ugly because BTRC lacks a feature or stdlib helper, consider improving BTRC.

Prefer:
- `var` when the type is obvious.
- f-strings over concatenation.
- stdlib collections and helpers over manual loops when available.
- direct `config.settings...` style in Nix modules unless an alias clearly improves readability.
- simulator/e2e coverage for boot, disk, recovery, and update behavior.
- actual LSP/editor protocol tests for editor features.

Do not assume "it compiles" means "it is good."

## Calibration Examples

These examples explain principles, not permanent rules.

`printf | cryptsetup` is bad because secret handling is represented as shell text. The right abstraction is process stdin or a dedicated secret API.

`Command("btrfs").arg("subvolume").arg("show").arg(path)` is bad when it turns an obvious command into ceremony. `Command(f"btrfs subvolume show {UnixShell.quote(path)}")` may better preserve intent. `.arg(...)` is still valid when building reusable argv fragments.

`relPath` is not bad because abbreviations are banned. It is bad when the local style and domain read better as `relativePath`.

`hardware_ui_labels.btrc` is not bad because filenames must be pretty. It is bad when the module's role in the system is unclear.

A helper-function LSP test is not enough to claim cmd-click works. Verify the actual language server protocol response, and verify grammar scopes when the problem is syntax coloring.

A passing unit test is not enough for simulator behavior. Run the VM/e2e path that exercises the user-visible workflow.

## Workflow

1. Read the surrounding code.
2. Infer the style and architecture.
3. Search for the broader pattern with `rg`.
4. Decide which layer should change.
5. Make the smallest coherent fix at that layer.
6. Remove obsolete code made unnecessary by the fix.
7. Add or update focused tests.
8. Verify at the level the user experiences the feature.
9. Report what changed, what was tested, and any remaining risk.

## Review Questions

Use these questions while coding:

- Does this code say what it means?
- Is this ceremony buying safety, reuse, or clarity?
- Is this hand-rolled logic replacing a stdlib feature?
- Is this awkward because the API is wrong?
- Is this preserving a dead architecture?
- Would the same ugliness appear elsewhere?
- Did I verify the real user-facing path?
- Would the author recognize this as their style?
