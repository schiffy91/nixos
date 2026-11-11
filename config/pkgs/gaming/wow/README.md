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
      details questie moveAny
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
Edit Mode layout and the account-wide SavedVariables of every managed addon into
`settings/`, then commit. `settingsExcluded` names addons to leave out, by default
the Details combat history. Per-character files, macros and unmanaged addons are
never captured, and `Config.wtf` stays per machine so each display keeps its own
graphics. Captured files can contain character names; this repository is public.

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
remain on Bars 2–4. LB uses Target Scan Enemy, RB cycles enemy targets, and R3
uses Interact With Target on all four modifier layers. The shoulder and trigger
indicators are anchored around the cluster center so narrowing the bar does not
make them overlap.
These client settings live in `Config.wtf`, outside the captured account files;
on another machine, set them in ConsolePort's modifier settings or run:

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
chat input has focus. MoveAny owns the XP bar's position, width and hover alpha.
[LyraelRayne's Gamepad Slots layouts](https://github.com/LyraelRayne/ConsolePort_GamepadSlots#action-bar-presets-presets)
are a related four-modifier example, but that module uses a different slot
arrangement and is not needed for these native bindings.

Run the isolated contract checks without installing or updating real addons:

```sh
nix build --impure --no-link --expr 'let inputs = (builtins.getFlake "git+file:///etc/nixos").inputs; pkgs = import inputs.nixpkgs { system = builtins.currentSystem; }; in import /etc/nixos/tests/gaming/wow { inherit pkgs inputs; }'
```
