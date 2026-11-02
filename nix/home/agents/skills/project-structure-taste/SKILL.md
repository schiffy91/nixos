---
name: project-structure-taste
description: Use when designing, reviewing, reorganizing, or cleaning repository/project layout. Applies when deciding top-level folders, where config/tests/scripts/generated files belong, whether a directory should exist, how to separate source from runtime/build output, or how to simplify a messy project tree across any language or framework.
---

# Project Structure Taste

Prefer ownership-first project layout. A good repository tree makes the source of truth obvious, keeps top-level folders scarce, and prevents implementation details from leaking into project structure.

## Core Preferences

- Keep top-level directories few and durable.
- Top-level folders must represent major ownership domains.
- Do not create top-level folders for one file, one script, one generated artifact, or one temporary workflow.
- Avoid junk-drawer names like `utils`, `helpers`, `misc`, `stuff`, `common`, or `shared` unless the project has a clear established convention and the contents are genuinely cohesive.
- Prefer moving files under the domain that owns them.
- Separate source, config, tests, packaging, docs, generated output, and runtime state.
- Respect existing framework conventions, but do not let framework defaults justify clutter.

## Default Top-Level Shape

For a general project, prefer some subset of:

```text
src/          implementation source
config/       declarative project/product configuration
tests/        tests, fixtures, harnesses, integration/e2e suites
docs/         durable specs, architecture notes, PRDs, decisions
packaging/    packaging, installers, deployment, platform bootstrap
scripts/      only if there are multiple maintained operator/dev scripts
generated/    only as a temporary committed compatibility exception
.github/      repository automation metadata
```

Do not add all of these by default. Use only the ones the project actually needs.

## Source Vs Config

Use `src/` for implementation code.

Use `config/` for source-of-truth policy, product definitions, templates, schemas, static declarations, target definitions, system definitions, user-facing defaults, and structured configuration.

Do not put product config at top level just because it is important. Important config belongs in `config/`.

Examples:

```text
config/
  environments/
  targets/
  features/
  schemas/
  templates/
  integrations/
  assets/
  settings/
```

## Tests

Keep top-level `tests/`, but organize it by execution contract, not random categories.

Prefer:

```text
tests/
  fixtures/
  unit/
  integration/
  e2e/
  smoke/
  linux/
  macos/
  windows/
  ci/
```

Use platform folders only when the behavior or harness is platform-specific. Do not split tests mechanically.

Fixtures belong under `tests/fixtures/` unless they are owned by a specific package/module.

## Scripts And Utils

Avoid top-level `utils/`.

If there is one script, it probably belongs near the domain that owns it.

Rules:

- Install/bootstrap scripts belong under `packaging/`, `deploy/`, or the relevant platform folder.
- Test helper scripts belong under `tests/`.
- Codegen scripts belong near the generator or under `scripts/codegen/` if there are several.
- Maintenance scripts belong under `scripts/` only when there is a real maintained script collection.
- Reusable runtime logic should be source code, not shell script sprawl.

A top-level `scripts/` folder is acceptable. A top-level `utils/` folder is usually a smell.

## Generated And Runtime Output

Generated output should usually be ignored and live under:

```text
build/
dist/
out/
tmp/
.cache/
```

Runtime state should be ignored and clearly local:

```text
.local/
state/
var/
.cache/
```

Only commit generated files when there is a real compatibility reason. Mark that as temporary in docs or comments.

Do not create source folders for generated output.

## Docs

Keep durable docs in `docs/`.

Delete stale docs instead of reorganizing them. A smaller accurate doc set is better than a large misleading one.

Good docs:

- PRD
- architecture notes
- decision records
- setup/install docs
- test strategy

Bad docs:

- obsolete TODO dumps
- old scratch notes
- screenshots-as-documentation without current relevance
- duplicated setup instructions

## Review Checklist

Before changing project layout:

1. Identify each file as source, config, test, fixture, packaging, script, doc, generated output, runtime state, or artifact.
2. Decide the owning domain.
3. Check whether a new top-level folder is truly justified.
4. Prefer moving under an existing owner.
5. Keep generated/runtime output ignored.
6. Preserve intentional existing structure unless there is a clearer ownership model.
7. For broad reorganizations, propose the target tree before editing.

## Hard Stop Rule

If a layout cleanup starts creating more top-level concepts than it removes, stop and discuss the tree before continuing.
