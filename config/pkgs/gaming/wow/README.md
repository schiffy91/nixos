# World of Warcraft

`settings.apps.gaming.wow` owns the client directory and addon selection.
The default directory follows
`settings.apps.gaming.battlenet.prefix`, which follows the administrator's configured
home. Existing defaults still select `_forever_` and every addon in `addons.lock.json`.
Changing the client directory does not select compatible releases automatically.

For example, a host can select a subset without changing the shared release lock:

```nix
{ config, lib, ... }: {
  settings.apps.gaming.battlenet.prefix = "/mnt/games/Battle.net/prefix";
  settings.apps.gaming.wow = {
    # Override directory separately if Battle.net installed WoW elsewhere.
    addons = lib.mkForce (with config.settings.apps.gaming.wow.addonPackages; [
      details questie
    ]);
  };
}
```

`addons.nix` exposes individual packages; `bundle.nix` combines their top-level addon
directories and rejects duplicate names. Extra packages can be appended through
`addons`; use `lib.mkForce []` to remove all managed addons while leaving manual
addons in place. The packages keep their release-archive hashes in one shared lock.
`wow-addons-lock` refreshes that lock during `nixosctl upgrade`. Failed requests and
ambiguous assets abort without publishing a replacement lock. A successfully fetched
release page with no matching asset can still fall back to an earlier release.

`wow-addons-install` is a BTRC program using the stdlib filesystem helpers and
an advisory file lock. It runs as the administrator's user, at user-session startup
and once per minute. This also handles a game installed after a system rebuild.
It does nothing until the configured client directory exists. Run the command
manually to reconcile immediately. WoW loads changed addon code on its next login
or UI reload.

The installer owns only links targeting `../.nixos-wow-addons/<addon>` inside
`Interface/AddOns`. Manual directories, files, and unrelated links are preserved;
a manual addon with a selected name wins and produces a journal message. Updates
atomically switch the hidden bundle link. Removing an addon removes only its owned
link. A manual replacement relinquishes ownership.

The previous module's `/nix/store/*-wow-forever-addons` whole-directory link is
migrated to a writable directory. Its original link is retained under
`Interface/AddOns.backup-*/AddOns`; this backup does not pin the old store path
against garbage collection. Unrelated whole-directory symlinks are left untouched
and reported as conflicts. Disabling WoW stops reconciliation and leaves installed
files alone; use an empty addon selection before disabling to remove managed links.

The installer does not seed profiles or access the `WTF` directory. UI layout and
addon settings travel between machines through `settings/` instead. Run
`nixosctl apps wow settings-capture` with the game closed to copy the account's key bindings,
Edit Mode layout and the SavedVariables of every managed addon into
`settings/`, then commit. Character addon enablement, chat geometry, frame positions,
Edit Mode selection and character bindings travel with those settings.
`settingsExcluded` names addons to leave out; by default it excludes
`Details_DataStorage`. Macros and unmanaged addon SavedVariables are never
captured. `Config.wtf` stays per machine so each display keeps its own graphics. Captured files can contain character names; this repository is public.

`nixosctl apps wow` lists the module's commands with tab completion:
`addons-install`, `addons-refresh`, `settings-apply` and `settings-capture`.
`nixosctl upgrade` runs the addon refresh unless a host sets
`upgradeAddons = false`; it never captures settings.

`wow-settings apply` runs after the installer. When the `settings/` version in the
system differs from the one last applied and the game is closed, it writes the
files into every account, keeps each replaced file under
`WTF/.nixos-wow-settings-backup`, and records the version in
`WTF/.nixos-wow-settings`. In-game changes made after that stay until the next
captured version arrives, so capture before rebuilding to keep them.

The captured controller bindings use ConsolePort's native eight-slot gamepad
bars in the Forever client. Slots 1–8 are X, Y, B, A, D-pad left, right, up,
down. No modifier selects Gamepad Bar 1; LT selects Bar 2; RT selects Bar 3;
LT+RT selects Bar 4. The triggers must emulate Shift and Ctrl respectively.
The current layout reserves unmodified A for Jump; its trigger-modified slots
remain on Bars 2–4. LB uses Target Scan Enemy and RB cycles enemy targets.
Unmodified R3 selects the enemy soft target first, then a friendly soft target
when no enemy is available; with neither, it leaves the current target alone.
LT+R3, RT+R3 and LT+RT+R3 use Interact With Target for talking, looting and
objects. Selecting a unit alone does not start an attack or interact with it.
The shoulder and trigger
indicators are anchored around the cluster center so narrowing the bar does not
make them overlap.
Enemy and friendly soft targeting follow the camera while using a gamepad;
explicitly selected targets stay locked. Forced hard targeting remains off.
Friendly soft targets can receive healing spells before being explicitly selected.
The existing FlareUI integration supplies the secure `FlareUISoftTarget` button;
its `displayLayout.softTargetUnit` condition is editable in SavedVariables and
the R3 binding lives in the normal bindings cache. No personal macro is required.
These preferences are saved in ConsolePort and the shared profile CVars,
so switching display or input profiles retains the same targeting behavior.
The FlareUI display profiles carry the trigger modifier and action-camera CVars
in editable SavedVariables. They apply when the profile loads. Without those
profiles, set the modifiers in ConsolePort or run:

```text
/console GamePadEnable 1
/console GamePadEmulateShift PADLTRIGGER
/console GamePadEmulateCtrl PADRTRIGGER
/console GamePadEmulateAlt none
```

On 2026-11-10, applying an older captured `bindings-cache.wtf` removed the
locally configured controller bindings. The pre-apply backup retained partial
bindings; the four-bar mapping above was reconstructed from the requested
layout. Capture again after future in-game binding changes and before rebuilding.
The installed ConsolePort already provides `ConsolePortGamepadBar` buttons.

ForeverFadeChat hides chat controls and decoration while leaving messages on
their normal timed fade. Its packaged patch also reveals the controls while the
chat input has focus. MoveAny has been removed; its XP bar positioning, sizing,
and hover behavior no longer apply.
[LyraelRayne's Gamepad Slots layouts](https://github.com/LyraelRayne/ConsolePort_GamepadSlots#action-bar-presets-presets)
are a related four-modifier example, but that module uses a different slot
arrangement and is not needed for these native bindings.

Run the isolated contract checks without installing or updating real addons:

```sh
nix build --impure --no-link --expr 'let inputs = (builtins.getFlake "git+file:///etc/nixos").inputs; pkgs = import inputs.nixpkgs { system = builtins.currentSystem; }; in import /etc/nixos/tests/gaming/wow { inherit pkgs inputs; }'
```

## Display and input profiles

FlareUI 5.1.2 supplies the unit frames, chat, native damage-meter skin, minimap
skin and quest tracker. Questie retains its map database and pins with its tracker
disabled. BetterBags, AdvancedInterfaceOptions, ForeverFadeChat and Details are
installed but disabled in each character's AddOns.txt; MoveAny is no longer installed.

Open `/fui` and select a profile in its existing profile dropdown:

| Display | Keyboard and mouse | Controller | Logical canvas |
| --- | --- | --- | --- |
| Pro Display XDR, 6016×3384 | Desk / Keyboard | Desk / Controller | 2133×1200 |
| 77-inch TV, 3840×2160, 7–8 feet away | TV / Keyboard | TV / Controller | 1600×900 |
| Steam Deck OLED, 1280×800 | Handheld / Keyboard | Handheld / Controller | 1280×800 |

Accept the reload prompt after switching. The small DisplayLayout integration
inside the packaged FlareUI coordinates its global UI scale and ConsolePort_Bar
with the selected profile. ConsolePort's input and bindings remain installed in
both modes. A physical display-size change picks the display profile while
preserving the current input mode (height ≤1000: Handheld; height ≥2800: Desk;
otherwise TV). It defers changes during combat and Edit Mode. The last observed
resolution is saved, so manual selections survive UI reloads and game restarts
at the same resolution. A display change while the game is closed is detected
on login. Fresh Default profiles select their initial display automatically;
existing profiles retain their choice when no previous resolution was saved.

The profile's `displayLayout` table is normal saved data: scale, input, chat
geometry and the shared gameplay CVars. The integration contains no generated
frame positions. Edit Mode saves FlareUI frame positions inside the selected
FlareUI profile; exiting Edit Mode also records chat geometry for that profile.
Native Edit Mode has a five-account-layout limit, so it is not the six-profile
selector. Its three existing layout names are retained with consistent native
anchors; the FlareUI profiles include positions for each of those names.
The integration never calls C_EditMode.SaveLayouts or selects native layouts.

Chat and the meter share a bottom-left area; use the Meter tab when needed.
Actions stay bottom-center. The controller menu sits at the bottom edge and
uses tightly spaced icons. The XP progress track matches the actual icon-row
width, excluding ConsolePort's container end-cap allowance. The visible
watch-bar strip sits between the menu and LT/RT, splitting the remaining space equally into
two gaps. The integration measures ConsolePort's visible bar textures (including
a second reputation bar) and trigger buttons at their effective scales. The menu
icons target 28 units with a 44-unit height ceiling, alongside an 8-unit XP
strip. These sizes compensate for ConsolePort local scales. The controller
cluster moves upward if another watch bar needs clearance, keeping the menu
readable. The menu backdrop and watch-container gradients are hidden. XP retains a
subtle empty track; its rested overlay follows the main fill height.
ConsolePort fades watch bars and hides main-button input icons through its
normal saved settings. Player and target sit higher;
target-of-target and the pet anchor directly below their parent unit frames.
Dragging either child in Edit Mode replaces its relative anchor with the normal
saved screen position.

The square, north-up minimap uses native Edit Mode at 90% (234 units) and sits
bottom-right with the matching-width quest tracker above it. Quest text wraps.
Default tooltips anchor just left of the minimap and grow upward and left;
their gap is editable in `displayLayout.tooltip`. Focus uses the lower center;
boss frames sit to the left of the map/quest column. Party and raid frames share the upper-left area. Raid
frames use columns to leave room above the player frame on the handheld.
Buffs anchor above the player health frame, bottom-left, filling right and
wrapping upward (eight icons per row, two rows). Debuffs retain their top-center
position, with their actual visible group centered as icons arrive or leave.
The packaged FlareUI layout patch adds the center option and relative anchors;
Blizzard's secure aura layout still owns aura contents and resizing. The two-row
debuff block is centered as a whole; a partially filled wrapped row remains
left-aligned inside that block.

Forever's per-character addon APIs take a GUID, not UnitName's first name.
DisplayLayout uses UnitGUID when enabling/disabling ConsolePort_Bar. Prepared
profiles also mark FlareUI's initial visual-style setup complete, because that
setup otherwise clears supplied unit-frame positions on first login.
Temporary warnings and tooltips may use central or side space. Main tanks are
represented in the raid grid rather than a duplicate tank stack.

Before the migration, a full WTF and repository-settings snapshot was saved to
`~/Backups/World of Warcraft/20261111-091209-flareui-layouts/before.tar.gz`.
The geometry envelopes and profile-switch logic have automated checks; final
font/art sizing and secure-frame behavior still require an in-game review.

The display profiles share Friz Quadrata: body text 13, headings 15, and
secondary values 11–12. Minimap fonts compensate for its native 90% scale.
Player and target frames are 180 × 54 with 70-unit portrait rings. The level
badge uses `unitframes.levelScale = 0.78` and `unitframes.levelFont` at 12,
independent of portrait artwork scaling. The packaged ring patch preserves the
original behavior for profiles without these settings. Desk, TV and handheld
keep their existing readability scales; keyboard/controller variants share
component proportions and differ in controls.
