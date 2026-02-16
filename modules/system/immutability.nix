{ config, lib, pkgs, ... }:
let
	device = if config.settings.disk.encryption.enable then config.settings.disk.by.mapper.root else config.settings.disk.by.partlabel.root;
	deviceDependency = if config.settings.disk.encryption.enable then "dev-mapper-${config.settings.disk.label.root}.device" else "dev-disk-by\\x2dpartlabel-${config.settings.disk.label.disk}\\x2d${config.settings.disk.label.main}\\x2d${config.settings.disk.label.root}.device";
	snapshotsSubvolumeName = config.settings.disk.subvolumes.snapshots.name;
	cleanName = config.settings.disk.immutability.persist.snapshots.cleanName;
	mode = config.settings.disk.immutability.mode;
	pathsToKeep = config.settings.disk.immutability.persist.paths;
	resetVolumes = lib.filter (v: v.resetOnBoot) config.settings.disk.subvolumes.volumes;

	# Build a precomputed rsync filter file per subvolume mount point
	filterForVolume = volume: let
		mp = volume.mountPoint;
		relevant = builtins.filter (path:
			if mp == "/" then true
			else path == mp || lib.hasPrefix (mp + "/") path
		) pathsToKeep;
		toRelative = path:
			if mp == "/" then lib.removePrefix "/" path
			else let stripped = lib.removePrefix (mp + "/") path;
			in if path == mp then "" else stripped;
		relatives = builtins.filter (r: r != "") (map toRelative relevant);
		lines = [ "+ */" ]
			++ (lib.concatMap (rel: [ "+ /${rel}" "+ /${rel}/" "+ /${rel}/**" ]) relatives)
			++ [ "- *" ];
	in pkgs.writeText "immutability-filter-${volume.name}" (lib.concatStringsSep "\n" lines + "\n");

	# name=mount:filter for each volume
	pairArgs = lib.concatMapStringsSep " " (volume:
		"${volume.name}=${volume.mountPoint}:${filterForVolume volume}"
	) resetVolumes;

	immutabilityScript = ../../scripts/core/immutability.py;

in
lib.mkIf config.settings.disk.immutability.enable {
	fileSystems = lib.mkMerge (lib.lists.forEach (lib.filter (volume: volume.neededForBoot) config.settings.disk.subvolumes.volumes) (volume: { "${volume.mountPoint}".neededForBoot = lib.mkForce true; }));
	boot.nixStoreMountOpts = [ "ro" ];
	boot.tmp.useTmpfs = true;
	boot.initrd = {
		supportedFilesystems = [ "btrfs" ];
		systemd = {
			storePaths = let
				filterFiles = map filterForVolume resetVolumes;
			in [ "${pkgs.python3}" "${immutabilityScript}" ] ++ filterFiles;
			extraBin = {
				btrfs = "${pkgs.btrfs-progs}/bin/btrfs";
				rsync = "${pkgs.rsync}/bin/rsync";
			};
			services.immutability = {
				description = "Factory resets BTRFS subvolumes via compiled Python";
				wantedBy = [ "initrd.target" ];
				requires = [ deviceDependency ];
				after = [ "systemd-cryptsetup@${config.settings.disk.partlabel.root}.service" deviceDependency ];
				before = [ "sysroot.mount" ];
				unitConfig.DefaultDependencies = "no";
				serviceConfig.Type = "oneshot";
				script = ''
					${pkgs.python3}/bin/python3 -S ${immutabilityScript} ${device} ${snapshotsSubvolumeName} ${cleanName} ${mode} ${pairArgs}
				'';
			};
		};
	};
}
