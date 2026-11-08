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

WoW and its addons own all account settings and SavedVariables. The installer
does not seed profiles or access the `WTF` directory. The Details addon remains
available as a package, with its settings managed in-game.

Run the isolated contract checks without installing or updating real addons:

```sh
nix build --impure --no-link --expr 'let inputs = (builtins.getFlake "git+file:///etc/nixos").inputs; pkgs = import inputs.nixpkgs { system = builtins.currentSystem; }; in import /etc/nixos/tests/gaming/wow { inherit pkgs inputs; }'
```
