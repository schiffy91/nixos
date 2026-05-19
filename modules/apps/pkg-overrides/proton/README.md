# scwhine-proton — handoff to next coding agent

This file exists because the previous agent (me) failed to ship and is being
replaced. Read it end-to-end before touching anything in `./patches/`.

The owner is **Alexander Schiffhauer** (`alexander.schiffhauer@gmail.com`).
He is the architect — your job is to execute on his direction, not invent your
own. Confirm any non-trivial decision with him before doing it.

---

## 1. What this project is

`scwhine-proton` (the project name; do **not** put the word "scwhine" into any
upstream-bound patch content — see §5) is a fork of `GE-Proton10-34` that adds
wine source patches so **Battle.net's launcher renders on native Wayland with
HDR + DPI scaling**, the systray icon appears in KDE Plasma 6 / GNOME / waybar,
and clicking between game icons (D4, WoW, SC, etc.) actually switches games.

It is packaged here as a Nix derivation (`package.nix`) that:

1. Fetches Valve's wine at commit `1729f00` (the exact tree GE-Proton10-34
   builds against).
2. Applies the 504-patch GE wine-wayland hotfix series.
3. Applies the scwhine patch series under `./patches/<NN-bug>/*.patch`.
4. Builds only the touched DLLs (`winewayland.so`, `dcomp.dll`, `dxgi.dll`).
5. Overlays them on top of the upstream GE-Proton binary tarball.

The end goal is to **upstream the patches to wine-devel** (or, failing that,
to proton-ge-custom). They are not personal-use hacks. They must meet upstream
quality bars or be discarded.

## 2. Current state — what works, what doesn't

**Works:**
- Build system applies all 7 patches cleanly against a fresh Valve wine
  1729f00 + GE wayland baseline (verified via `rsync` of the nix store source
  to a scratch dir + `patch -p1` of each). 0 failures.
- Patches 01, 02, 03 (CEF roundtrip, layered windows, SNI systray) are
  architecturally close to landable upstream.
- Patch 07 (`load_desktop_driver` deadlock) is a real bug with a plausible fix.
- The wine maintainer review (see §4) confirmed the bug analysis in 01, 02,
  03, 07 is sound.

**Broken:**
- **Battle.net crashes at startup** with a Qt `bad_array_new_length` exception
  inside `qwindows.dll` (the Qt5 Windows platform plugin BNet bundles at
  `prefix/drive_c/Program Files (x86)/Battle.net/Battle.net.17379/platforms/
  qwindows.dll`). The crash signature is identical across every variation
  I tried today, including with my patches fully reverted. **I never
  root-caused it.**
- The maintainer (see §4) suspects the cause is in our patches 02 or 06:
  one of them likely makes a screen-geometry call (`GetSystemMetrics`,
  `EnumDisplayMonitors`) return a zero-dimensioned or negative-dimensioned
  rect, after which Qt computes `width - 1 = -1`, passes it to
  `operator new[]`, and that throws `bad_array_new_length`. This is the
  single most important lead. **Investigate this before anything else.**
- Patches 04, 05, 06 (the DComp + DXGI + subsurface bridge hardware
  acceleration pipeline) are architecturally wrong for upstream and will be
  rejected on review. See §4 and §6.

**Unverified:**
- I was never able to confirm my deployed `win32u.so` was actually being
  loaded by BNet inside the pressure-vessel container. The trace `fopen()`
  calls I added (to `/home/.../prefix/drive_c/scwhine-init.log` and similar
  locations) produced no output, despite a `__attribute__((constructor))`
  symbol present in the binary. Either pressure-vessel is loading a
  different `win32u.so` than the one I deploy to
  `~/.local/share/Steam/compatibilitytools.d/scwhine-GE-Proton10-34/files/
  lib/wine/x86_64-unix/win32u.so`, or my trace calls are silently failing.
  **The first step in any debugging session must be to verify which binary
  is actually mapped.** Run `WINEDEBUG=+loaddll,+module` and grep the
  proton log for `dcomp`, `winewayland`, `win32u` — confirm the paths point
  at our build output, not at the stock GE binaries.

## 3. The patch series

7 subfolders, one per bug/feature, lexically ordered. Each has a `README.md`
explaining symptom, root cause, fix, dependencies, and affected files.

| # | Folder | What it does |
|---|---|---|
| 01 | `01-cef-wayland-roundtrip-deadlock` | Replaces the blocking second `wl_display_roundtrip` in `wayland_process_init` with a queued non-blocking variant. CEF parent/child IPC deadlock fix. |
| 02 | `02-layered-windows-blank-fix` | Wires `pUpdateLayeredWindow` to `ensure_window_surface_contents` and refreshes the surface after `WM_WAYLAND_CONFIGURE`. |
| 03 | `03-sni-systray` | Adds `pSystrayDock*` driver hooks against KDE's StatusNotifierItem D-Bus spec via libdbus. New `wayland_systray.c`. |
| 04 | `04-dcomp-direct-composition` | Real `dcomp.dll` COM implementation that snapshots DXVK back buffers via `IDXGIVkInteropSurface` on a ticker and sends them to the driver via `WM_WAYLAND_COMPOSE_ATTACH`. **108 KB. NACK from maintainer review.** |
| 05 | `05-dxgi-composition-swapchain` | `dxgi.CreateSwapChainForComposition` HWND_MESSAGE synth so DXVK accepts the call. **NACK from maintainer review.** |
| 06 | `06-winewayland-subsurface-bridge` | Vulkan→`wl_shm` copy path consuming `WM_WAYLAND_COMPOSE_ATTACH`, plus four bundled unrelated changes (cursor shape, click-through region, viewporter, dmabuf scaffolding). **NACK from maintainer review.** |
| 07 | `07-load-desktop-driver-deadlock` | Skips `send_message_timeout(WM_NULL)` to the desktop window when caller owns the desktop thread; bounds the wait with `SMTO_ABORTIFHUNG` + 5s otherwise. Cross-process deadlock fix. |

Build system applies them in subfolder lexical order via `package.nix:79-87`.

## 4. Wine maintainer review verdict

A wine-maintainer-persona review was run on the full series. Verbatim
highlights (summarized for brevity):

### Architecture verdict on 04+05+06

> *"NACK. dcomp.dll has no business knowing about wayland. The whole point of
> the user driver split is that dlls/* are platform-agnostic.
> `WM_WAYLAND_COMPOSE_ATTACH` in a generic DLL is an immediate -1. If
> dcomp.dll needs to push surfaces somewhere, the destination is the user
> driver via a new pUser entry point, with an opaque handle, not a
> wayland-specific WM_* in the PE-side public header."*

> *"Passing raw DXVK Vulkan handles across the PE/Unix boundary with
> `(uintptr_t)` casts is undefined. DXVK runs in PE; its VkDevice is a
> wine-thunked dispatchable handle valid only in PE context."*

> *"30 Hz busy-poll re-export regardless of whether the visual tree changed.
> CEF's compositor is supposed to drive presentation; you're driving it from a
> timer. The 5 ms fence cap explicitly accepts wrong pixels on screen, which
> means torn frames any time GPU latency exceeds 5 ms, i.e. constantly under
> load."*

> *"Right approach: extend the dcomp interface to back IDCompositionSurface
> with an ID3D11Texture2D shared via the standard NT-handle DXGI shared
> resource path, surface it to the driver through pCreateLayeredWindow or
> equivalent, and let the driver decide whether to present via dmabuf
> (Wayland), shm (fallback), or pixmap (X11). All the cross-DLL coupling you're
> inventing should be hidden behind a driver-side presenter handle. None of
> this works if dcomp.dll itself is wayland-aware."*

### Per-patch ship verdict

| # | Verdict |
|---|---|
| 01 | **Iterate.** The fix doesn't match the bug description — still has a blocking roundtrip. |
| 02 | **Iterate, split**, then ship the `pUpdateLayeredWindow` wiring half. |
| 03 | **Iterate.** Blockers: copies private `struct explorer_icon` layout (NACK), missing libdbus `configure.ac` plumbing, one dispatch thread per icon should be one shared thread. |
| 04 | **Scrap.** |
| 05 | **Scrap.** The proper fix is in DXVK, not wine. Synthesized HWND is leaked on success. |
| 06 | **Scrap as a unit.** Contains 4 unrelated changes that must be separate patches. Salvage cursor-shape fallback and click-through region change as standalone. Contains dead code (`wayland_dmabuf_buffer_create`, `wayland_surface_attach_dmabuf` implemented but never called). |
| 07 | **Iterate** (justify behavior change), then ship. Near-landable. |

### Likely cause of the BNet Qt crash (per maintainer)

> *"Suspects, in order: (a) Battle.net is using Qt for something (probably the
> secondary login window or the Agent), and you've broken a `GetSystemMetrics`
> or `EnumDisplayMonitors` call so a screen geometry returns negative
> dimensions and Qt allocates `new char[-1]`. Likely culprits in your patches:
> patch 02 changes `ensure_window_surface_contents` semantics on
> `WM_WAYLAND_CONFIGURE`; patch 06 changes the input-region click-through
> condition for `WS_EX_LAYERED`. Either could change a reported window rect to
> (0,0,0,0) and Qt computes width-1 = -1."*

> *"(b) The qwindows.dll being loaded is from a different wine version than
> your patched winewayland.drv — Proton stacks layer DLLs from multiple
> sources. Check `WINEDEBUG=+loaddll` and see where qwindows is loading from."*

> *"Pressure-vessel sandbox question is easy to answer: `WINEDEBUG=+module,+loaddll`
> and grep for `dcomp` and `winewayland`. If you don't see your build paths,
> your patches aren't loaded. Don't speculate, look."*

## 5. Coding-style / hygiene issues to fix everywhere

Quoting the maintainer review, these are non-negotiable upstream:

- **Unicode in source**: em dashes, box-drawing, smart quotes throughout. Wine
  source is **ASCII only**. Run `iconv -f utf-8 -t ascii//TRANSLIT` on every
  modified file before re-generating patches.
- **WARN vs TRACE vs ERR**: We use `WARN` for chatty traces; that is wrong.
  `WARN` is "something unexpected happened but we continue." Normal-path
  tracing under `WINEDEBUG=+chan` is `TRACE`. `ERR` is reserved for "this is
  broken, report it." Audit every `WARN(...)` we added — most should be
  `TRACE`. `ERR("vkCreateBuffer failed\n")` should include the `VkResult`.
- **Forward-declared Vulkan types** in `04-dcomp-direct-composition`: replace
  the 200 lines of hand-rolled typedefs with `#include <vulkan/vulkan.h>`.
- **Duplicated struct definitions**: `struct wayland_compose_attach_request`
  is defined in both `dlls/dcomp/device.c` and `dlls/winewayland.drv/
  waylanddrv.h`. Two copies of a wire struct that must "match" is a hard
  NACK. Move to a shared header under `include/wine/`.
- **In-function block comments**: huge `/* ... */` comment walls explaining
  *why* belong in the commit message, not inline. Trim aggressively.
- **Patch headers**: `From: 0000…` and dates of 2026-05-19 show the patches
  have never been through `git am`. Run `git format-patch -s` from a real
  commit on a real branch.
- **Cover letter**: a 7-patch series needs a `[PATCH 0/N]` cover letter with
  the architecture overview and dependency graph. Not present.
- **The word "scwhine"** must not appear in any patch content — it is the
  project name, not a wine concept. Code symbols are `wayland_compose_*` /
  `WM_WAYLAND_COMPOSE_ATTACH` / `WINE_DCOMP_NO_TICK` / etc. (Already cleaned
  in the current state. Don't reintroduce it.)

## 6. Recommended path forward (maintainer's words)

> *"Drop 04/05/06 entirely. Submit 01 (after actually fixing the deadlock —
> currently it doesn't), 02 (split), 03 (with configure plumbing and without
> the explorer struct copy), 07 in that order. Get those four through
> wine-devel and into the tree."*
>
> *"Then, for the CEF problem, start over: prototype a real
> `IDCompositionSurface` backed by a shareable DXGI resource (NT handle, not
> the wine-PE-Unix DXVK hack), wire it into the user driver via a generic
> presenter interface, and only then add a Wayland backend that uses dmabuf
> properly. Six months of work, not six days."*

This is the path. Confirm with Alexander before committing to it — he may
have constraints (a Battle.net session he wants working tonight) that change
the priority order. Specifically: getting the launcher rendering for *his
machine* may take precedence over upstream-ready patches in the short term,
in which case the existing 04/05/06 stays as a local hack while the proper
restart happens in parallel.

## 7. Operational notes — how to build and test

### Build
```bash
cd /etc/nixos
sudo nixos-rebuild test    # full system; rebuilds proton derivation on patch change
```
The patches under `./patches/<NN-bug>/*.patch` are referenced by `package.nix`
via `${./patches}/*/*.patch` glob. Adding/removing patches requires a Nix
rebuild — there is no in-place install.

### Run BNet
```bash
battlenet         # wayland + HDR (default)
battlenet-x11     # X11 fallback for debugging
```
These wrapper scripts live in `modules/apps/battlenet.nix`. They invoke
`umu-run` → proton → wine in a pressure-vessel container.

### Find the actual binaries loaded
**Do this FIRST before any debugging**:
```bash
PROTON_LOG=+loaddll,+module battlenet 2>&1 | tee /tmp/bnet.log
# Wait for crash, then:
grep -E "(dcomp|winewayland|win32u)\.so" /tmp/bnet.log | head
```
The paths printed are what's actually loaded. If they don't point at our
build's `lib/wine/x86_64-unix/`, **our patches aren't running** and any
"reverted change didn't fix it" observation is meaningless.

### Crash logs
Battle.net writes `BlizzardError-*.txt` to
`~/Games/Battle.net/prefix/drive_c/users/steamuser/AppData/Local/Battle.net/
Errors/`. The Qt `bad_array_new_length` crashes are 17521 bytes and start
with `Exception: Unhandled C++ Exception at 0023:7BBDBADF`. Earlier crashes
(May 10) were a different signature (BREAKPOINT at 6D9BEF76) — those are
not relevant.

### Source tree
- `package.nix` — the Nix derivation. Don't deploy binaries manually; let
  Nix do it.
- `patches/<NN-bug>/` — the patch series. Source of truth.
- `default.nix` — re-exports `package.nix`. Don't touch unless integrating
  a new override.
- `/tmp/ge-wine-build/` — the previous agent's scratch build tree. **Do not
  trust this as authoritative.** It is a stale checkout that has my old
  patches applied + ad-hoc edits + reverts. If you need a clean tree, do
  the rsync recipe used in `package.nix` against
  `/nix/store/9r1i77j68kj1hdz3i9vgx7msxkwa99f6-wine-1729f00/` + GE hotfixes.

## 8. Traps the previous agent fell into

So you don't repeat them:

- **Trap 1: assumed binaries deployed = binaries loaded.** I spent ~2 hours
  rebuilding `win32u.so`, deploying it to the proton tree, relaunching BNet,
  and watching the same crash repeat. I never confirmed pressure-vessel was
  actually mapping my binary. The fact that an `__attribute__((constructor))`
  log file never appeared was the smoking gun — I missed it for hours.
  *Always verify the load path first.*

- **Trap 2: misread a regex effect.** When doing a global symbol rename
  across patches, I included `re.sub(r"  +", " ", text)` to collapse double
  spaces — which silently mangled the indentation of every diff context line
  and broke patch application across the whole series. Recovery was a
  `git checkout --` since the previous good state was staged. *Never run
  whitespace-collapsing regexes on patch files. Diff context lines are
  whitespace-sensitive.*

- **Trap 3: anchored on the wrong hypothesis.** The crash addresses in
  qwindows.dll looked like they pointed at `GetKeyboardLayoutList` (user32
  IAT entry offset 0xa848). I spent hours patching `NtUserGetKeyboardLayoutList`
  with cap-at-256, hard-return-1, `__builtin_trap()`, and file traces. None
  of them changed the crash signature, which should have told me much earlier
  that the function wasn't actually in the failing path. The maintainer's
  review later identified the likely real cause as a geometry call (patches
  02/06) — a hypothesis I never tested. *When evidence repeatedly contradicts
  the hypothesis, abandon the hypothesis. Don't add another layer.*

- **Trap 4: tried to fix a 108 KB monolithic patch in place.** Patch 04 was
  originally one bloated file. I split it into per-subsystem patches (04 =
  dcomp, 05 = dxgi, 06 = winewayland), which the maintainer review confirmed
  was structurally correct — but the architecture is still wrong regardless of
  how you slice it. *Splitting a bad patch into smaller bad patches is not
  progress.*

## 9. What I'd do on day one if I were you

1. **`WINEDEBUG=+loaddll,+module battlenet` → confirm the load path question.**
   Five minutes of work that I never did. Until you know which binaries are
   actually loaded, nothing else matters.

2. **Test the maintainer's geometry hypothesis.** Build with patch 02 removed
   from the series (edit `package.nix` to skip `02-layered-windows-blank-fix/`
   temporarily). If the Qt crash goes away, the WM_WAYLAND_CONFIGURE refresh
   is the bug. Then try with patch 06 removed instead. Bisect.

3. **Talk to Alexander before deciding to drop 04/05/06.** The maintainer's
   recommendation to scrap them assumes upstream-readiness is the priority.
   If his short-term priority is "the launcher renders so I can play tonight,"
   then keeping the hack while planning the rewrite is the right call. Get
   explicit direction.

4. **Do not write new patches today.** The series needs surgery, not addition.

Good luck.

— previous agent

