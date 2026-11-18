# The reset at boot and what persists are schiffy91/immutability. This fills that module's options from this
# configuration's settings, which hosts and modules keep setting, and keeps the default persisted paths.
{ config, inputs, lib, ... }:
let
	settings = config.settings.disk;
in {
	imports = [ inputs.immutability.nixosModules.default ./persistence.nix ];
	_module.args.immutabilityPersistKey = inputs.immutability.lib.persistKey;
	immutability = {
		device = if settings.encryption.enable then settings.by.mapper.root else settings.by.partlabel.root;
		inherit (settings.immutability) enable base mode dryRun restoreGeneration;
		enforceOnReboot = settings.immutability.enforce.onReboot;
		generations = settings.immutability.nonPersistedGenerations;
		cleanName = settings.immutability.persist.snapshots.cleanName;
		snapshotsSubvolume = settings.subvolumes.snapshots.name;
		snapshotsMountPoint = settings.subvolumes.snapshots.mountPoint;
		volumes = map (volume: { inherit (volume) name mountPoint resetOnBoot neededForBoot; }) settings.subvolumes.volumes;
		after = [ "systemd-cryptsetup@${settings.partlabel.root}.service" ];  # the unit that opens an encrypted root
		diffIgnore = ./diffignore;
		persist = {
			enable = settings.persistence.enable;
			inherit (settings.immutability.persist) subvolumeRoot paths neededForBoot;
			required = lib.filter (path: lib.hasPrefix "${path}/" config.settings.users.admin.hashedPasswordFile) settings.immutability.persist.neededForBoot;  # boot stops without the password file
		};
	};
}
